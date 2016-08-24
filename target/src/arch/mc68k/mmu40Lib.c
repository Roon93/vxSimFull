/* mmu40Lib.c - mmu library for MC68040 and MC68060 */

/* Copyright 1984-1996 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01o,10feb99,wsl  add comment documenting ERRNO code
01n,29oct96,dat  fixed in-line assembly for 2.7 compiler. (SPR 7403) 
01m,07mar95,ism  fixed problem with restoration of d) from stack (SPR#4065).
01l,22nov94,dzb  added cacheClear() in mmuEnable() as per Heurikon request.
01k,31oct94,tmk  added MC68LC040 support
01j,16jun94,tpr  added MC68060 cpu support.
		 clean up following code review.
01j,05jul94,rdc  mods to decrease fragmentation and provide alternate memory 
		 source.  Fixed bug that caused pages containing page 
		 descriptors to be write enabled.
01i,12feb93,rdc  changed scheme for sharing data structures with vmLib.
01h,09feb93,rdc  fixed bug preventing mapping of large vm spaces.
01g,09oct92,rdc  moved reset of TT regs to mmuEnable.
01f,01oct92,jcf  warnings.
01e,22sep92,rdc  doc.
01d,28jul92,jmm  added forward declarations for all functions
01c,21jul92,rdc  fixed bug in mmuMemPagesWriteEnable/Disable, 
		 added init code to mmuCurrentSet to write protect pte memory
		 for global trans tbl.
01b,13jul92,rdc  changed reference to vmLib.h to vmLibP.h.
01a,08jul92,rdc  written.
*/

/*
DESCRIPTION:

mmuLib.c provides the architecture dependent routines that directly control
the memory management unit.  It provides 10 routines that are called by the
higher level architecture independent routines in vmLib.c: 

mmuLibInit - initialize module
mmuTransTblCreate - create a new translation table
mmuTransTblDelete - delete a translation table.
mmuEnable - turn mmu on or off
mmuStateSet - set state of virtual memory page
mmuStateGet - get state of virtual memory page
mmuPageMap - map physical memory page to virtual memory page
mmuGlobalPageMap - map physical memory page to global virtual memory page
mmuTranslate - translate a virtual address to a physical address
mmuCurrentSet - change active translation table

Applications using the mmu will never call these routines directly; 
the visable interface is supported in vmLib.c.

mmuLib supports the creation and maintenance of multiple translation tables,
one of which is the active translation table when the mmu is enabled.  
Note that VxWorks does not include a translation table as part of the task
context;  individual tasks do not reside in private virtual memory.  However,
we include the facilities to create multiple translation tables so that
the user may create "private" virtual memory contexts and switch them in an
application specific manner.  New
translation tables are created with a call to mmuTransTblCreate, and installed
as the active translation table with mmuCurrentSet.  Translation tables
are modified and potentially augmented with calls to mmuPageMap and mmuStateSet.
The state of portions of the translation table can be read with calls to 
mmuStateGet and mmuTranslate.

The traditional VxWorks architecture and design philosophy requires that all
objects and operating systems resources be visable and accessable to all agents
(tasks, isrs, watchdog timers, etc) in the system.  This has traditionally been
insured by the fact that all objects and data structures reside in physical 
memory; thus, a data structure created by one agent may be accessed by any
other agent using the same pointer (object identifiers in VxWorks are often
pointers to data structures.) This creates a potential 
problem if you have multiple virtual memory contexts.  For example, if a
semaphore is created in one virtual memory context, you must gurantee that
that semaphore will be visable in all virtual memory contexts if the semaphore
is to be accessed at interrupt level, when a virtual memory context other than
the one in which it was created may be active. Another example is that
code loaded using the incremental loader from the shell must be accessable
in all virtual memory contexts, since code is shared by all agents in the
system.

This problem is resolved by maintaining a global "transparent" mapping
of virtual to physical memory for all the contiguous segments of physical 
memory (on board memory, i/o space, sections of vme space, etc) that is shared
by all translation tables;   all available  physical memory appears at the same 
address in virtual memory in all virtual memory contexts. This technique 
provides an environment that allows
resources that rely on a globally accessable physical address to run without
modification in a system with multiple virtual memory contexts.

An additional requirement is that modifications made to the state of global 
virtual memory in one translation table appear in all translation tables.  For
example, memory containing the text segment is made read only (to avoid
accidental corruption) by setting the appropriate writeable bits in the 
translation table entries corresponding to the virtual memory containing the 
text segment.  This state information must be shared by all virtual memory 
contexts, so that no matter what translation table is active, the text segment
is protected from corruption.  The mechanism that implements this feature is
architecture dependent, but usually entails building a section of a 
translation table that corresponds to the global memory, that is shared by
all other translation tables.  Thus, when changes to the state of the global
memory are made in one translation table, the changes are reflected in all
other translation tables.

mmuLib provides a seperate call for constructing global virtual memory -
mmuGlobalPageMap - which creates translation table entries that are shared
by all translation tables.  Initialization code in usrConfig makes calls
to vmGlobalMap (which in turn calls mmuGlobalPageMap) to set up global 
transparent virtual memory for all
available physical memory.  All calls made to mmuGlobalPageMap must occur before
any virtual memory contexts are created;  changes made to global virtual
memory after virtual memory contexts are created are not guaranteed to be 
reflected in all virtual memory contexts.

Most mmu architectures will dedicate some fixed amount of virtual memory to 
a minimal section of the translation table (a "segment", or "block").  This 
creates a problem in that the user may map a small section of virtual memory
into the global translation tables, and then attempt to use the virtual memory
after this section as private virtual memory.  The problem is that the 
translation table entries for this virtual memory are contained in the global 
translation tables, and are thus shared by all translation tables.  This 
condition is detected by vmMap, and an error is returned, thus, the lower
level routines in mmuLib.c (mmuPageMap, mmuGlobalPageMap) need not perform
any error checking.

A global variable called mmuPageBlockSize should be defined which is equal to 
the minimum virtual segment size.  mmuLib must provide a routine 
mmuGlobalInfoGet, which returns a pointer to the globalPageBlock array.
This provides the user with enough information to be able to allocate virtual 
memory space that does not conflict with the global memory space.

This module supports the 68040 and 68060 mmu with a three level translation
table:

                                            root
                                             |
                                             |
    LEVEL 1                 -------------------------------------
    TABLE DESCRIPTORS (128) | td  | td  | td  | td  | td  | td  | ...     
                            -------------------------------------
                               |     |     |     |     |     |  
                               |     |     |     |     |     |  
                      ----------     |     v     v     v     v  
                      |              |    NULL  NULL  NULL  NULL
                      |              |  
                      |              -------------------------
                      |                                      |
                      v                                      v
LEVEL2   -------------------------------      -------------------------------
TABLE    | td  | td  | td  | td  | td  | ...  | td  | td  | td  | td  | td  | ..
DESCRIP. -------------------------------      -------------------------------  
(128/table) |     |                              |     |
            |     ----  ...                      |     ----  ... 
            |        |                           |        |
            v        v                           v        v
          ----     ----                         ----     ---- 
         | pd |   | pd |                       | pd |   | pd |
PAGE      ----     ----                         ----     ---- 
DESCRIP. | pd |   | pd |   ...                 | pd |   | pd |   ...
(32-64    ----     ----                         ----     ---- 
 per     | pd |   | pd |                       | pd |   | pd |
 table)   ----     ----                         ----     ----
         | pd |   | pd |                       | pd |   | pd |
          ----     ----                         ----     ----
           .         .
           .         .
           .         .


The top level consists of an array of 128 level 1 table descriptors 
(LEVEL_1_TABLE_DESC).  These point to arrays of 128 level 2 table descriptors 
(LEVEL_2_TABLE_DESC), which in turn point to arrays of page descriptors
(PAGE_DESC).  The page descriptor arrays contain either 32 entries when used
with an 8k page size, or 64 entries when used with a 4k page size. Memory for
these data structures is managed internal to the module by the routines
mmuTableDescBlockAlloc and mmuPageDescBlockAlloc, which are fixed block 
memory allocators.  They obtain memory from the system memory partition aligned
to page boundries, and break this memory up into fixed length buffers.  The
memory for all descriptors is free'd automatically when a translation table is 
deleted.

To implement global virtual memory, a seperate translation table called 
mmuGlobalTransTbl is created when the module is initialized.  Calls to 
mmuGlobalPageMap will augment and modify this translation table.  When new
translation tables are created, memory for the top level array of level 1 table
descriptors is allocated and initialized by duplicating the pointers in 
mmuGlobalTransTbl's top level level 1 table descriptor array.  Thus, the 
new translation table will use the global
translation table's state information for portions of virtual memory that are
defined as global.  Here's a picture to illustrate:

	   GLOBAL TRANS TBL		        NEW TRANS TBL

		root				     root
                 |                                    |
                 |                                    |
                 v                                    v
LEVEL 1  -------------------                  -------------------
TABLE    | td  | td  | td  |...               | td  | td  | td  |... 
DESC.    -------------------                  ------------------- 
            |     |                              |     |
            o-------------------------------------     |
            |     |                                    |
            |     ------------                         |
            |                |                         |
            |                o--------------------------
            |                |
            v                v
LEVEL2   -------------      ------------
TABLE    | td  | td  |...  | td  | td  |...
DESCRIP. -------------      ------------ 
(128/table) |     |           |     |
            |     ----  ...   |     ----  ... 
            |        |        |        |
            v        v        v        v
          ----     ----       ----     ---- 
         | pd |   | pd |     | pd |   | pd |
PAGE      ----     ----       ----     ---- 
DESCRIP. | pd |   | pd | ..  | pd |   | pd | ...
(32-64    ----     ----       ----     ---- 
 per     | pd |   | pd |     | pd |   | pd |
 table)   ----     ----       ----     ----
         | pd |   | pd |     | pd |   | pd |
          ----     ----       ----     ----
           .         .
           .         .
           .         .


Note that with this scheme, the global memory granularity is 32MB.  Each time
you map a section of global virtual memory, you dedicate at least 32MB of 
the virtual space to global virtual memory that will be shared by all virtual
memory contexts.

The physcial memory that holds these data structures is obtained from the
system memory manager via memalign to insure that the memory is page
aligned.  We want to protect this memory from being corrupted,
so we write protect the descriptors that we set up in the global translation
that correspond to the memory containing the translation table data structures.
This creates a "chicken and the egg" paradox, in that the only way we can
modify these data structures is through virtual memory that is now write 
protected, and we can't write enable it because the page descriptors for that 
memory are in write protected memory (confused yet?)
So, you will notice that anywhere that page descriptors 
are modified, we do so by locking out interrupts, momentarily disabling the 
mmu, accessing the memory with its physical address, enabling the mmu, and
then re-enabling interrupts (see mmuStateSet, for example.)

USER MODIFIABLE OPTIONS:

1) Memory fragmentation - mmuLib obtains memory from the system memory
   manager via memalign to contain the mmu's translation tables.  This memory
   was allocated a page at a time on page boundries.  Unfortunately, in the
   current memory management scheme, the memory manager is not able to allocate
   these pages contiguously.  Building large translation tables (ie, when
   mapping large portions of virtual memory) causes excessive fragmentation
   of the system memory pool.  An attempt to alleviate this has been installed
   by providing a local buffer of page aligned memory;  the user may control
   the buffer size by manipulating the global variable mmuNumPagesInFreeList.
   By default, mmuPagesInFreeList is set to 4.

2) Alternate memory source - A customer has special purpose hardware that
   includes seperate static RAM for the mmu's translation tables.  Thus, they
   require the ability to specify an alternate source of memory other than
   memalign.  A global variable has been created that points to the memory
   partition to be used as the source for translation table memory; by default,
   it points to the system memory partition.  The user may modify this to 
   point to another memory partition before mmu40LibInit is called.


*/




#include "vxWorks.h"
#include "string.h"
#include "intLib.h"
#include "stdlib.h"
#include "memLib.h"
#include "private/memPartLibP.h"
#include "private/vmLibP.h"
#include "arch/mc68k/mmu40Lib.h"
#include "mmuLib.h"
#include "errno.h"
#include "cacheLib.h"

/* forward declarations */

LOCAL void mmuMemPagesWriteEnable (MMU_TRANS_TBL_ID transTbl);
LOCAL void mmuMemPagesWriteDisable (MMU_TRANS_TBL *transTbl);
LOCAL STATUS mmuPageDescGet (MMU_TRANS_TBL *pTransTbl, void *virtAddr,
                             PAGE_DESC **result);
LOCAL MMU_TRANS_TBL *mmuTransTblCreate ();
LOCAL STATUS mmuTransTblInit (MMU_TRANS_TBL *newTransTbl);
LOCAL STATUS mmuTransTblDelete (MMU_TRANS_TBL *transTbl);
LOCAL STATUS mmuVirtualPageCreate (MMU_TRANS_TBL *pTransTbl, void *virtAddr);
LOCAL PAGE_DESC *mmuPageDescBlockAlloc (MMU_TRANS_TBL *transTbl);
LOCAL LEVEL_2_TABLE_DESC *mmuTableDescBlockAlloc (MMU_TRANS_TBL *transTbl);
LOCAL void *mmuBufBlockAlloc(LIST *pList,UINT bufSize,MMU_TRANS_TBL *pTransTbl);
LOCAL STATUS mmuEnable (BOOL enable);
LOCAL STATUS mmuStateSet (MMU_TRANS_TBL *transTbl, void *pageAddr,
                          UINT stateMask, UINT state);
LOCAL STATUS mmuStateGet (MMU_TRANS_TBL *transTbl, void *pageAddr, UINT *state);
LOCAL STATUS mmuPageMap (MMU_TRANS_TBL *transTbl, void *virtualAddress,
                         void *physPage);
LOCAL STATUS mmuGlobalPageMap (void *virtualAddress, void *physPage);
LOCAL STATUS mmuTranslate (MMU_TRANS_TBL *transTbl, void *virtAddress,
                           void **physAddress);
LOCAL void mmuCurrentSet (MMU_TRANS_TBL *transTbl);
LOCAL void mmuATCFlush (void *addr);

int mmuPageSize;

/* a translation table to hold the descriptors for the global transparent
 * translation of physical to virtual memory 
 */

LOCAL MMU_TRANS_TBL mmuGlobalTransTbl;

/* initially, the current trans table is a dummy table with mmu disabled */

LOCAL MMU_TRANS_TBL *mmuCurrentTransTbl = &mmuGlobalTransTbl;

LOCAL BOOL mmuEnabled = FALSE;

LOCAL STATE_TRANS_TUPLE mmuStateTransArrayLocal [] =
    {

    {VM_STATE_MASK_VALID, MMU_STATE_MASK_VALID,
     VM_STATE_VALID, MMU_STATE_VALID},

    {VM_STATE_MASK_VALID, MMU_STATE_MASK_VALID,
     VM_STATE_VALID_NOT, MMU_STATE_VALID_NOT},

    {VM_STATE_MASK_WRITABLE, MMU_STATE_MASK_WRITABLE,
     VM_STATE_WRITABLE, MMU_STATE_WRITABLE},

    {VM_STATE_MASK_WRITABLE, MMU_STATE_MASK_WRITABLE,
     VM_STATE_WRITABLE_NOT, MMU_STATE_WRITABLE_NOT},

    {VM_STATE_MASK_CACHEABLE, MMU_STATE_MASK_CACHEABLE,
     VM_STATE_CACHEABLE, MMU_STATE_CACHEABLE},

    {VM_STATE_MASK_CACHEABLE, MMU_STATE_MASK_CACHEABLE,
     VM_STATE_CACHEABLE_NOT, MMU_STATE_CACHEABLE_NOT},

    {VM_STATE_MASK_CACHEABLE, MMU_STATE_MASK_CACHEABLE,
     VM_STATE_CACHEABLE_WRITETHROUGH, MMU_STATE_CACHEABLE_WRITETHROUGH},

#if (CPU == MC68040 || CPU == MC68LC040)
    {VM_STATE_MASK_CACHEABLE, MMU_STATE_MASK_CACHEABLE,
     VM_STATE_CACHEABLE_NOT_NON_SERIAL, MMU_STATE_CACHEABLE_NOT_NON_SERIAL}
#elif (CPU == MC68060)
    {VM_STATE_MASK_CACHEABLE, MMU_STATE_MASK_CACHEABLE,
     VM_STATE_CACHEABLE_NOT_IMPRECISE, MMU_STATE_CACHEABLE_NOT_IMPRECISE}
#endif /* (CPU == MC68060) */

    };

LOCAL MMU_LIB_FUNCS mmuLibFuncsLocal =
    {
    mmu40LibInit,
    mmuTransTblCreate,
    mmuTransTblDelete,
    mmuEnable,   
    mmuStateSet,
    mmuStateGet,
    mmuPageMap,
    mmuGlobalPageMap,
    mmuTranslate,
    mmuCurrentSet
    };

IMPORT STATE_TRANS_TUPLE *mmuStateTransArray;
IMPORT int mmuStateTransArraySize;
IMPORT MMU_LIB_FUNCS mmuLibFuncs;
IMPORT int mmuPageBlockSize;
int mmuNumPagesInFreeList = 4;
PART_ID mmuPageSource = NULL;

LOCAL TC_REG localTc;
LOCAL CRP_REG localCrp;

/*
 * mmuEnableInline: move the new tc value into d0,
 * do a movec d0 -> tc.
 */

#define mmuEnableInline(value) \
    { \
    localTc.enable = value; \
    __asm__ ("movew %0, d0; .word 0x4e7b, 0x0003": : "r" (localTc): "d0" ); \
    } 

#define PAGE_SIZE_8K 8192
#define PAGE_SIZE_4K 4096

/******************************************************************************
*
* mmu40LibInit - initialize module
*
* Build a dummy translation table that will hold the page table entries
* for the global translation table.  The mmu remains disabled upon
* completion.  Note that this routine is global so that it may be referenced
* in usrConfig.c to pull in the correct mmuLib for the specific architecture.
*
* RETURNS: OK if no error, ERROR otherwise.
*
* ERRNO: S_mmuLib_INVALID_PAGE_SIZE
*/

STATUS mmu40LibInit 
    (
    int pageSize	/* system pageSize (4096 or 8192) */
    )
    {
    LEVEL_1_TABLE_DESC *pLevel1Table;
    int i;
    void *pMem;

    /* if the user has not specified a memory partition to obtain pages 
       from (by initializing mmuPageSource), then initialize mmuPageSource
       to the system memory partition.
    */

    if (mmuPageSource == NULL)
	mmuPageSource = memSysPartId;

    /* initialize the data objects that are shared with vmLib.c */

    mmuStateTransArray = &mmuStateTransArrayLocal [0];

    mmuStateTransArraySize =
          sizeof (mmuStateTransArrayLocal) / sizeof (STATE_TRANS_TUPLE);

    mmuLibFuncs = mmuLibFuncsLocal;

    mmuPageBlockSize = PAGE_BLOCK_SIZE;

    if ((pageSize != PAGE_SIZE_4K) && (pageSize != PAGE_SIZE_8K))
	{
	errno = S_mmuLib_INVALID_PAGE_SIZE;
	return (ERROR);
	}

    /* initialize kludgey static data struct for loading tc reg */

    localTc.pageSize = (pageSize == PAGE_SIZE_4K) ? 0 : 1;
    localTc.enable = 0;
    localTc.pad = 0;

    /* initialize kludgey static data struct for loading crp reg */

    localCrp.addr = 0;
    localCrp.pad = 0;

    mmuEnabled = FALSE;

    mmuPageSize = pageSize;

    /* build a dummy translation table which will hold the pte's for
     * global memory.  All real translation tables will point to this
     * one for controling the state of the global virtual memory  
     */

    /* initialize the lists that hold free memory for table descriptors 
     * and page descriptors.
     */

    lstInit (&mmuGlobalTransTbl.freePageDescBlockList);
    lstInit (&mmuGlobalTransTbl.freeTableDescBlockList);
    lstInit (&mmuGlobalTransTbl.memPageList);
    lstInit (&mmuGlobalTransTbl.memBlockList);
    lstInit (&mmuGlobalTransTbl.memFreePageList);

    /* if pageSize == 4k, then page descriptor array has 64 elements,
     * if 8k, then array has 32 elements.
     */

    pMem = mmuBufBlockAlloc (&mmuGlobalTransTbl.freePageDescBlockList, 
		 sizeof (PAGE_DESC) * ((mmuPageSize == PAGE_SIZE_4K) ? 64 : 32),
		 &mmuGlobalTransTbl);

    if (pMem == NULL)
	return (NULL);

    /* pMem points to a physical page that we allocate page descriptor blocks
     * out of - add it to the translation table's list of pages that it owns.
     */

    lstAdd (&mmuGlobalTransTbl.memPageList, (NODE *) pMem);

    /* both level 1 and level 2 table descriptor arrays have 128 elements */

    pMem = mmuBufBlockAlloc (&mmuGlobalTransTbl.freeTableDescBlockList, 
		  sizeof (LEVEL_1_TABLE_DESC) * NUM_LEVEL_1_TABLE_DESCRIPTORS,
		  &mmuGlobalTransTbl);

    if (pMem == NULL)
	return (NULL);

    /* pMem points to a physical page that we allocate table descriptor blocks
     * out of - add it to the translation table's list of pages that it owns.
     */

    lstAdd (&mmuGlobalTransTbl.memPageList, (NODE *) pMem);

    /* allocate memory to hold the level 1 descriptor array  - we can get it
     * from the freeTableDescBlockList since the level 1 tables are the same
     * size as the level 2 tables.
     */

    mmuGlobalTransTbl.pLevel1 = pLevel1Table = 
      (LEVEL_1_TABLE_DESC *) lstGet (&mmuGlobalTransTbl.freeTableDescBlockList);

    if (pLevel1Table == NULL)
	return (ERROR);	

    /* invalidate all the level 1 descriptors */

    for (i = 0; i < NUM_LEVEL_1_TABLE_DESCRIPTORS; i++)
	{
	pLevel1Table[i].addr = -1;
	pLevel1Table[i].used = 0; 
	pLevel1Table[i].writeProtect = 0; 
	pLevel1Table[i].udt = UDT_INVALID; 
	}

    return (OK);
    }

/******************************************************************************
*
* mmuMemPagesWriteEnable - write enable the memory holding a table's descriptors
*
* Each translation table has a linked list of physical pages that contain its
* table and page descriptors.  Before you can write into any descriptor, you
* must write enable the page it is contained in.  This routine enables all
* the pages used by a given translation table.
*
*/

LOCAL void mmuMemPagesWriteEnable
    (
    MMU_TRANS_TBL_ID transTbl
    )
    {
    void *thisPage; 

    thisPage = (void *) lstFirst (&transTbl->memPageList);

    while (thisPage != NULL)
	{
	mmuStateSet (transTbl, thisPage, MMU_STATE_MASK_WRITABLE, 
		     MMU_STATE_WRITABLE);
	thisPage = (void *) lstNext ((NODE *) thisPage);
	}
    }

/******************************************************************************
*
* mmuMemPagesWriteDisable - write disable memory holding a table's descriptors 
*
* Memory containing translation table descriptors is marked as read only
* to protect the descriptors from being corrupted.  This routine write protects
* all the memory used to contain a given translation table's descriptors.
*
*/

LOCAL void mmuMemPagesWriteDisable
    (
    MMU_TRANS_TBL *transTbl
    )
    {
    void *thisPage; 

    thisPage = (void *) lstFirst (&transTbl->memPageList);

    while (thisPage != NULL)
	{
	mmuStateSet (transTbl, thisPage, MMU_STATE_MASK_WRITABLE, 
		     MMU_STATE_WRITABLE_NOT);
#if	(CPU == MC68060)
	/*
	 * The MC68060 does not use the data cache when performing a table
	 * search because of the tablewalker unit is interfaced directly to
	 * the Bus controller. Therefore translation tables must always be
	 * placed in writethrough space.
	 */
	mmuStateSet (transTbl, thisPage, MMU_STATE_MASK_CACHEABLE,
		     MMU_STATE_CACHEABLE_WRITETHROUGH);
#endif	/* (CPU == MC68060) */
	thisPage = (void *) lstNext ((NODE *) thisPage);
	}
    }

/******************************************************************************
*
* mmuPageDescGet - get the page descriptor for a given page
*
* mmuPageDescGet traverses a translation table and returns the (physical) 
* address of the page descriptor for the given virtual address.
*
* RETURNS: OK or ERROR if there is no virtual space for the given address 
*
*/

LOCAL STATUS mmuPageDescGet 
    (
    MMU_TRANS_TBL *pTransTbl, 	/* translation table */
    void *virtAddr,		/* virtual address */ 
    PAGE_DESC **result		/* result is returned here */
    )
    {

    LEVEL_1_TABLE_DESC *pLevel1;
    LEVEL_2_TABLE_DESC *pLevel2;
    PAGE_DESC *pPageDescTable;
    UINT level1Index = LEVEL_1_TABLE_INDEX (virtAddr);
    UINT level2Index = LEVEL_2_TABLE_INDEX (virtAddr);
    UINT pageDescTableIndex = PAGE_DESC_TABLE_INDEX (virtAddr);

    pLevel1 = &pTransTbl->pLevel1[level1Index];

    if (pLevel1->udt == UDT_INVALID)
	return (ERROR);

    pLevel2 = &(((LEVEL_2_TABLE_DESC *)(pLevel1->addr << 9))[level2Index]); 

    if (pLevel2->generic.udt == UDT_INVALID)
	return (ERROR);

    pPageDescTable = (PAGE_DESC *) ((mmuPageSize == PAGE_SIZE_4K) ? 
		     (pLevel2->pageSize4k.addr << 8) :
		     (pLevel2->pageSize8k.addr << 7));

    *result = &pPageDescTable[pageDescTableIndex];

    return (OK);
    }

/******************************************************************************
*
* mmuTransTblCreate - create a new translation table.
*
* create a 68040 or 68060 translation table.  Allocates space for the
* MMU_TRANS_TBL data structure and calls mmuTransTblInit on that object.  
*
* RETURNS: address of new object or NULL if allocation failed,
*          or NULL if initialization failed.
*/

LOCAL MMU_TRANS_TBL *mmuTransTblCreate 
    (
    )
    {

    MMU_TRANS_TBL *newTransTbl;

    newTransTbl = (MMU_TRANS_TBL *) malloc (sizeof (MMU_TRANS_TBL));

    if (newTransTbl == NULL)
	return (NULL);

    if (mmuTransTblInit (newTransTbl) == ERROR)
	{
	free ((char *) newTransTbl);
	return (NULL);
	}

    return (newTransTbl);
    }


/******************************************************************************
*
* mmuTransTblInit - initialize a new translation table 
*
* Initialize a new translation table.  The level 1 table is copyed from the
* global translation mmuGlobalTransTbl, so that we
* will share the global virtual memory with all
* other translation tables.
* 
* RETURNS: OK or ERROR if unable to allocate memory. 
*/

LOCAL STATUS mmuTransTblInit 
    (
    MMU_TRANS_TBL *newTransTbl		/* translation table to be inited */
    )
    {
    LEVEL_1_TABLE_DESC *pLevel1Table;
    void *pMem;

    lstInit (&newTransTbl->memPageList);
    lstInit (&newTransTbl->memBlockList);
    lstInit (&newTransTbl->memFreePageList);

    /* initialize the lists that hold free memory for table descriptors 
     * and page descriptors.
     */

    lstInit (&newTransTbl->freePageDescBlockList);
    lstInit (&newTransTbl->freeTableDescBlockList);

    /* add buffers to the free page descriptor list */

    /* if pageSize == 4k, then page descriptor array has 64 elements,
     * if 8k, then array has 32 elements.
     */

    pMem = mmuBufBlockAlloc (&newTransTbl->freePageDescBlockList, 
	      sizeof (PAGE_DESC) * ((mmuPageSize == PAGE_SIZE_4K) ? 64 : 32),
	      newTransTbl);

    if (pMem == NULL)
	return (ERROR);

    /* pMem points to a physical page that we allocate page descriptor blocks
     * out of - add it to the translation table's list of pages that it owns.
     */

    lstAdd (&newTransTbl->memPageList, (NODE *) pMem);

    /* add buffers to the free table  descriptor list */

    pMem = mmuBufBlockAlloc (&newTransTbl->freeTableDescBlockList, 
		  sizeof (LEVEL_1_TABLE_DESC) * NUM_LEVEL_1_TABLE_DESCRIPTORS,
		  newTransTbl);

    if (pMem == NULL)
	return (ERROR);

    /* pMem points to a physical page that we allocate table descriptor blocks
     * out of - add it to the translation table's list of pages that it owns.
     */

    lstAdd (&newTransTbl->memPageList, (NODE *) pMem);

    /* allocate memory to hold the level 1 descriptor array  - we can get it
     * from the freeTableDescBlockList since the level 1 tables are the same
     * size as the level 2 tables.
     */

    newTransTbl->pLevel1 = pLevel1Table = 
      (LEVEL_1_TABLE_DESC *) lstGet (&newTransTbl->freeTableDescBlockList);

    if (pLevel1Table == NULL)
	return (ERROR);

    /* copy the level 1 table from mmuGlobalTransTbl,
     * so we get the global virtual memory 
     */

    bcopy ((char *) mmuGlobalTransTbl.pLevel1, 
	   (char *) pLevel1Table, 
	   sizeof (LEVEL_1_TABLE_DESC) * NUM_LEVEL_1_TABLE_DESCRIPTORS);

    mmuMemPagesWriteDisable (newTransTbl);

    return (OK);
    }

/******************************************************************************
*
* mmuTransTblDelete - delete a translation table.
* 
* mmuTransTblDelete deallocates all the memory used to store the translation
* table entries.  It does not deallocate physical pages mapped into the
* virtual memory space.
*
* RETURNS: OK
*
*/

LOCAL STATUS mmuTransTblDelete 
    (
    MMU_TRANS_TBL *transTbl		/* translation table to be deleted */
    )
    {
    char *thisBlock = (char *) lstFirst (&transTbl->memBlockList);
    char *nextBlock;

    /* free all the pages in the translation table's memory partition */

    mmuMemPagesWriteEnable (transTbl);

    while (thisBlock != NULL)
	{
	nextBlock = (void *) lstNext ((NODE *) thisBlock);
	thisBlock = (char *)((NODE *) thisBlock - 1); 
	free ((void *) thisBlock);
	thisBlock = nextBlock;
	}

    /* free the translation table data structure */

    free (transTbl);
    
    return (OK);
    }

/******************************************************************************
*
* mmuVirtualPageCreate - set up translation tables for a virtual page
*
* simply check if there's already a page descriptor array that has a
* page descriptor for the given virtual page.  If there isn't, create one.
*
* RETURNS OK or ERROR if couldn't allocate space. 
*/

LOCAL STATUS mmuVirtualPageCreate 
    (
    MMU_TRANS_TBL *pTransTbl, 		/* translation table */
    void *virtAddr			/* virtual addr to create */
    )
    {
    FAST UINT i;
    LEVEL_1_TABLE_DESC *pLevel1;
    LEVEL_2_TABLE_DESC *pLevel2;
    PAGE_DESC *pPageDescTable;
    UINT level1Index = LEVEL_1_TABLE_INDEX (virtAddr);
    UINT level2Index = LEVEL_2_TABLE_INDEX (virtAddr);

    pLevel1 = &pTransTbl->pLevel1[level1Index];

    if (pLevel1->udt == UDT_INVALID)
	{
	/* create the level 2 table */

	pLevel2 = mmuTableDescBlockAlloc (pTransTbl); 

	if (pLevel2 == NULL)
	    return (ERROR);

	/* invalidate all the level 2 descriptors */

	mmuMemPagesWriteEnable (pTransTbl);

	for (i = 0; i < NUM_LEVEL_2_TABLE_DESCRIPTORS; i++)
	    {
	    pLevel2 [i].pageSize8k.addr = -1;
	    pLevel2 [i].generic.used = 0; 
	    pLevel2 [i].generic.writeProtect = 0; 
	    pLevel2 [i].generic.udt = UDT_INVALID; 
	    }

	pLevel1->addr = (UINT) pLevel2 >> 9;
	pLevel1->udt = UDT_VALID;

	mmuMemPagesWriteDisable (pTransTbl);
	}

    pLevel2 = &(((LEVEL_2_TABLE_DESC *)(pLevel1->addr << 9))[level2Index]); 

    if (pLevel2->generic.udt == UDT_INVALID)
	{
	UINT numDesc = ((mmuPageSize == PAGE_SIZE_4K) ? 64 : 32);

	/* create page descriptor table */

	pPageDescTable = mmuPageDescBlockAlloc (pTransTbl);

	if (pPageDescTable == NULL)
	    return (ERROR);

	/* invalidate all the page descriptors */

	mmuMemPagesWriteEnable (pTransTbl);

	for (i = 0; i < numDesc; i++)
	    {
	    pPageDescTable[i].pageSize4k.addr = -1;
	    pPageDescTable[i].generic.global = 0; 
	    pPageDescTable[i].generic.u1 = 0; 
	    pPageDescTable[i].generic.u0 = 0; 
	    pPageDescTable[i].generic.supervisor = 0; 
	    pPageDescTable[i].generic.cacheMode = 1;  /* cachable, copyback */
	    pPageDescTable[i].generic.modified = 0; 
	    pPageDescTable[i].generic.used = 0; 
	    pPageDescTable[i].generic.writeProtect = 0; 
	    pPageDescTable[i].generic.pdt = PDT_INVALID; 
	    }

	if (mmuPageSize == PAGE_SIZE_4K)
	    pLevel2->pageSize4k.addr = (UINT) pPageDescTable >> 8;
	else
	    pLevel2->pageSize8k.addr = (UINT) pPageDescTable >> 7;

	pLevel2->generic.udt = UDT_VALID;

	mmuMemPagesWriteDisable (pTransTbl);
	}

    mmuATCFlush (virtAddr);

    return (OK);
    }

/******************************************************************************
*
* mmuPageDescBlockAlloc - allocate a block of page descriptors
*
* This routine is used to allocate a block of page descriptors.
*
* RETURNS: pointer to new page descriptor block
*/

LOCAL PAGE_DESC *mmuPageDescBlockAlloc
    (
    MMU_TRANS_TBL *transTbl
    )
    {
    PAGE_DESC *newPageDescBlock;

    mmuMemPagesWriteEnable (transTbl);

    newPageDescBlock = (PAGE_DESC *) lstGet (&transTbl->freePageDescBlockList);

    if (newPageDescBlock == NULL)
	{
	UINT numDesc = ((mmuPageSize == PAGE_SIZE_4K) ? 64 : 32);
	PAGE_DESC *pMem;

	/* get some more memory for page descriptor blocks */

	pMem = (PAGE_DESC *) mmuBufBlockAlloc (&transTbl->freePageDescBlockList,
						numDesc * sizeof (PAGE_DESC),
						transTbl);

	if (pMem == NULL)
	    return (NULL);

	lstAdd (&transTbl->memPageList, (NODE *) pMem);

	newPageDescBlock = 
	    (PAGE_DESC *) lstGet (&transTbl->freePageDescBlockList);

	if (newPageDescBlock == NULL)
	    return (NULL);
	}

    mmuMemPagesWriteDisable (transTbl);
    
    return (newPageDescBlock);
    }

/******************************************************************************
*
* mmuTableDescBlockAlloc - allocate a block of table descriptors
*
* This routine is used to allocate a block of table descriptors.
*
* RETURNS: pointer to new table descriptor block
*/

LOCAL LEVEL_2_TABLE_DESC *mmuTableDescBlockAlloc
    (
    MMU_TRANS_TBL *transTbl
    )
    {
    LEVEL_2_TABLE_DESC *newTableDescBlock;

    mmuMemPagesWriteEnable (transTbl);

    newTableDescBlock = 
	(LEVEL_2_TABLE_DESC *) lstGet (&transTbl->freeTableDescBlockList);

    if (newTableDescBlock == NULL)
	{
	LEVEL_2_TABLE_DESC *pMem;

	/* get some more memory for table descriptor blocks */

	pMem = (LEVEL_2_TABLE_DESC *) 
	           mmuBufBlockAlloc (&transTbl->freeTableDescBlockList,
		   NUM_LEVEL_2_TABLE_DESCRIPTORS * sizeof (LEVEL_2_TABLE_DESC),
		   transTbl);

	if (pMem == NULL)
	    return (NULL);

	lstAdd (&transTbl->memPageList, (NODE *) pMem);

	newTableDescBlock = 
	    (LEVEL_2_TABLE_DESC *) lstGet (&transTbl->freeTableDescBlockList);

	if (newTableDescBlock == NULL)
	    return (NULL);
	}

    mmuMemPagesWriteDisable (transTbl);
    
    return (newTableDescBlock);
    }

/******************************************************************************
*
* mmuBufBlockAlloc - get memory and break it up into fixed length blocks
*
* Allocates a page of memory, breaks it up into as many buffers as it can,
* and hangs all the buffers on the specified list.  The first buffer is
* not hung on the list, so that the beginning of the page can be used to
* keep track of the page (on a seperate linked list).
*
* RETURNS: pointer to allocated page or NULL if allocation fails
*/

LOCAL void *mmuBufBlockAlloc 
    (
    LIST *pList,
    UINT bufSize,
    MMU_TRANS_TBL *pTransTbl
    )
    {
    char *pPage; 
    char *nextBuf;
    UINT numBufs = mmuPageSize / bufSize;
    int i;


    if ((pPage = (char *) lstGet (&pTransTbl->memFreePageList)) == NULL)
	{
	pPage = memPartAlignedAlloc (mmuPageSource, 
				     mmuPageSize * mmuNumPagesInFreeList, 
				     mmuPageSize); 

	if (pPage == NULL)
	    return (NULL);

	/* the beginning of each page is devoted to a NODE that strings
	   pages together on memPageList;  a second NODE immediately
	   following will string together blocks of memory that we malloc 
	   on memBlockList.
	*/

	lstAdd (&pTransTbl->memBlockList, (NODE *) (pPage + sizeof (NODE)));

	/* break the buffer up into individual pages and stash them in the
	   free page list.
	 */

	for (i = 0; i < mmuNumPagesInFreeList; i++, pPage += mmuPageSize)
	    lstAdd (&pTransTbl->memFreePageList, (NODE *) pPage);

	pPage = (char *) lstGet (&pTransTbl->memFreePageList);
	}

    if (pPage == NULL)
	return (NULL);
	    
    nextBuf = pPage + bufSize;

    for (i = 1; i < numBufs; i++)
	{
	lstAdd (pList, (NODE *) nextBuf);
	nextBuf += bufSize;
        } 
    
    return ((void *) pPage);
    }

/******************************************************************************
*
* mmuEnable - turn mmu on or off
*
* RETURNS: OK
*/

LOCAL STATUS mmuEnable 
    (
    BOOL enable			/* TRUE to enable, FALSE to disable MMU */
    )
    {
    FAST int oldIntLev;
    LOCAL BOOL firstTime = TRUE;

    /* lock out interrupts to protect kludgey static data structure */

    oldIntLev = intLock ();  
    mmuEnableInline(enable);
    mmuEnabled = enable;
    intUnlock (oldIntLev);

    /* reset the transparent translation registers */

    if (enable && firstTime)
	{
	cacheClear (DATA_CACHE, NULL, ENTIRE_CACHE);	/* from Heurikon */

	__asm__ ("movel #0, d0; "
		 ".word 0x4e7b, 0x0004; "   /* movec d0, ITT0 */
		 ".word 0x4e7b, 0x0005; "   /* movec d0, ITT1 */
		 ".word 0x4e7b, 0x0006; "   /* movec d0, DTT0 */
		 ".word 0x4e7b, 0x0007; "   /* movec d0, DTT1 */
		 :		/* outputs */
		 :		/* inputs */
		 : "d0"		/* temps */
		 );

	firstTime = FALSE;
	}


    return (OK);
    }

/******************************************************************************
*
* mmuStateSet - set state of virtual memory page
*
* mmuStateSet is used to modify the state bits of the pte for the given
* virtual page.  The following states are provided:
*
* MMU_STATE_VALID       MMU_STATE_VALID_NOT      vailid/invalid
* MMU_STATE_WRITABLE    MMU_STATE_WRITABLE_NOT   writable/writeprotected
* MMU_STATE_CACHEABLE   MMU_STATE_CACHEABLE_NOT  notcachable/cachable
*
* these may be or'ed together in the state parameter.  The 68040 and 68060
* provides several caching modes, so the following cache states are provided
* which may be substituted for MMU_STATE_CACHEABLE/MMU_STATE_CACHEABLE_NOT:
*
* MMU_STATE_CACHEABLE_WRITETHROUGH          
* MMU_STATE_CACHEABLE_COPYBACK             
* MMU_STATE_CACHEABLE_NOT_SERIAL		(MC68040 only)
* MMU_STATE_CACHEABLE_NOT_NON_SERIAL		(MC68040 only)
* MMU_STATE_CACHEABLE_NOT_PRECISE		(MC68060 only)
* MMU_STATE_CACHEABLE_NOT_IMPRECISE		(MC68060 only)
*
* MMU_STATE_CACHEABLE is equivalent to MMU_STATE_CACHEABLE_COPYBACK, and
* MMU_STATE_CACHEABLE_NOT is equivalent to MMU_STATE_CACHEABLE_NOT_SERIAL or
* MMU_STATE_CACHEABLE_NOT_PRECISE.
* See the MC68040 or MC68060 32-bit Microprocessor User's Manual for
* additional information.
* 
* Additionally, masks are provided so that only specific states may be set:
*
* MMU_STATE_MASK_VALID
* MMU_STATE_MASK_WRITABLE
* MMU_STATE_MASK_CACHEABLE
*
* These may be or'ed together in the stateMask parameter.  
*
* Accesses to a virtual page marked as invalid will result in a bus error.
*
* RETURNS: OK or ERROR if virtual page does not exist.
*/

LOCAL STATUS mmuStateSet 
    (
    MMU_TRANS_TBL *transTbl, 	/* translation table */
    void *pageAddr,		/* page whose state to modify */ 
    UINT stateMask,		/* mask of which state bits to modify */
    UINT state			/* new state bit values */
    )
    {
    PAGE_DESC *pageDesc;
    FAST UINT *pageDescBits;
    FAST int oldIntLev;

    if (mmuPageDescGet (transTbl, pageAddr, &pageDesc) != OK)
	return (ERROR);

    /* modify the pte with mmu turned off and interrupts locked out */

    pageDescBits = (UINT *) pageDesc;

    /* XXX can't dynamically turn mmu on and off if virtual stack */
    /* only way is if you make mmuEnable inline and guarantee that this
       code is in physical memory  */

    if (mmuEnabled)
	{
	oldIntLev = intLock ();  
	mmuEnableInline(FALSE);
	*pageDescBits = (*pageDescBits & ~stateMask) | (state & stateMask);
	mmuEnableInline(TRUE);
	intUnlock (oldIntLev);
	}
    else
	*pageDescBits = (*pageDescBits & ~stateMask) | (state & stateMask);

    mmuATCFlush (pageAddr);
    cacheClear (DATA_CACHE, (void *) pageDescBits, sizeof (*pageDescBits));

    return (OK);
    }

/******************************************************************************
*
* mmuStateGet - get state of virtual memory page
*
* mmuStateGet is used to retrieve the state bits of the pte for the given
* virtual page.  The following states are provided:
*
* VM_STATE_VALID 	VM_STATE_VALID_NOT	 vailid/invalid
* VM_STATE_WRITABLE 	VM_STATE_WRITABLE_NOT	 writable/writeprotected
* VM_STATE_CACHEABLE 	VM_STATE_CACHEABLE_NOT	 notcachable/cachable
*
* these are or'ed together in the returned state.  Additionally, masks
* are provided so that specific states may be extracted from the returned state:
*
* VM_STATE_MASK_VALID 
* VM_STATE_MASK_WRITABLE
* VM_STATE_MASK_CACHEABLE
*
* RETURNS: OK or ERROR if virtual page does not exist.
*/

LOCAL STATUS mmuStateGet 
    (
    MMU_TRANS_TBL *transTbl, 	/* tranlation table */
    void *pageAddr, 		/* page whose state we're querying */
    UINT *state			/* place to return state value */
    )
    {
    PAGE_DESC *pageDesc;
    FAST UINT *pageDescBits;

    if (mmuPageDescGet (transTbl, pageAddr, &pageDesc) != OK)
	return (ERROR);

    pageDescBits = (UINT *) pageDesc;

    *state = *pageDescBits; 

    return (OK);
    }

/******************************************************************************
*
* mmuPageMap - map physical memory page to virtual memory page
*
* The physical page address is entered into the pte corresponding to the
* given virtual page.  The state of a newly mapped page is undefined. 
*
* RETURNS: OK or ERROR if translation table creation failed. 
*/

LOCAL STATUS mmuPageMap 
    (
    MMU_TRANS_TBL *transTbl, 	/* translation table */
    void *virtualAddress, 	/* virtual address */
    void *physPage		/* physical address */
    )
    {
    PAGE_DESC *pageDesc;

    if (mmuPageDescGet (transTbl, virtualAddress, &pageDesc) != OK)
	{
	/* build the translation table for the virtual address */

	if (mmuVirtualPageCreate (transTbl, virtualAddress) != OK)
	    return (ERROR);

	if (mmuPageDescGet (transTbl, virtualAddress, &pageDesc) != OK)
	    return (ERROR);
	}


    mmuMemPagesWriteEnable (transTbl);

    if (mmuPageSize == PAGE_SIZE_4K)
	pageDesc->pageSize4k.addr = (UINT) physPage >> 12;
    else
	pageDesc->pageSize8k.addr = (UINT) physPage >> 13;

    mmuMemPagesWriteDisable (transTbl);

    mmuATCFlush (virtualAddress);
    cacheClear (DATA_CACHE, pageDesc, sizeof (*pageDesc));

    return (OK);
    }

/******************************************************************************
*
* mmuGlobalPageMap - map physical memory page to global virtual memory page
*
* mmuGlobalPageMap is used to map physical pages into global virtual memory
* that is shared by all virtual memory contexts.  The translation tables
* for this section of the virtual space are shared by all virtual memory
* contexts.
*
* RETURNS: OK or ERROR if no pte for given virtual page.
*/

LOCAL STATUS mmuGlobalPageMap 
    (
    void *virtualAddress, 	/* virtual address */
    void *physPage		/* physical address */
    )
    {
    PAGE_DESC *pageDesc;

    if (mmuPageDescGet (&mmuGlobalTransTbl, virtualAddress, &pageDesc) != OK)
	{
	/* build the translation table for the virtual address */

	if (mmuVirtualPageCreate (&mmuGlobalTransTbl, virtualAddress) != OK)
	    return (ERROR);

	if (mmuPageDescGet(&mmuGlobalTransTbl, virtualAddress, &pageDesc) != OK)
	    return (ERROR);
	}

    mmuMemPagesWriteEnable (&mmuGlobalTransTbl);

    if (mmuPageSize == PAGE_SIZE_4K)
	pageDesc->pageSize4k.addr = (UINT) physPage >> 12;
    else
	pageDesc->pageSize8k.addr = (UINT) physPage >> 13;

    mmuMemPagesWriteDisable (&mmuGlobalTransTbl);

    mmuATCFlush (virtualAddress);
    cacheClear (DATA_CACHE, pageDesc, sizeof (*pageDesc));

    return (OK);
    }

/******************************************************************************
*
* mmuTranslate - translate a virtual address to a physical address
*
* Traverse the translation table and extract the physical address for the
* given virtual address from the pte corresponding to the virtual address.
*
* RETURNS: OK or ERROR if no pte for given virtual address.
*/

LOCAL STATUS mmuTranslate 
    (
    MMU_TRANS_TBL *transTbl, 		/* translation table */
    void *virtAddress, 			/* virtual address */
    void **physAddress			/* place to return result */
    )
    {
    PAGE_DESC *pageDesc;
    UINT dt;

    if (mmuPageDescGet (transTbl, virtAddress, &pageDesc) != OK)
	{
	errno = S_mmuLib_NO_DESCRIPTOR; 
	return (ERROR);
	}

    dt = pageDesc->generic.pdt;

    if (dt == PDT_INVALID)
	{
	errno = S_mmuLib_INVALID_DESCRIPTOR; 
	return (ERROR);
	}

    if (mmuPageSize == PAGE_SIZE_4K)
	*physAddress = 
	     (void *) ((UINT) pageDesc->pageSize4k.addr << 12);
    else
	*physAddress = 
	     (void *) ((UINT) pageDesc->pageSize8k.addr << 13);

    /* add offset into page */

    *physAddress = (void *) (((UINT) *physAddress) + 
		   ((mmuPageSize == PAGE_SIZE_4K) ? 
		   ((unsigned) virtAddress & 0xfff) :
		   ((unsigned) virtAddress & 0x1fff))); 

    return (OK);
    }

/******************************************************************************
*
* mmuCurrentSet - change active translation table
*
* mmuCurrent set is used to change the virtual memory context.
* Load the CRP (root pointer) register with the given translation table.
*
*/

LOCAL void mmuCurrentSet 
    (
    MMU_TRANS_TBL *transTbl	/* new active tranlation table */
    ) 
    {
    FAST int oldLev;
    static BOOL firstTime = TRUE;

    if (firstTime)
	{
	/* write protect all the pages containing the ptes allocated for
	 * the global translation table.  Need to do this cause when this
	 * memory is allocated, the global trans tbl doesn't exist yet,
	 * so the state sets fail.
	 */

	mmuMemPagesWriteDisable (&mmuGlobalTransTbl);
	mmuMemPagesWriteDisable (transTbl);

	firstTime = FALSE;
	}

    oldLev = intLock ();
    localCrp.addr = ((unsigned int) transTbl->pLevel1) >> 9 ;

    /* movel localCrp, d0; movec d0, srp */

    __asm__ ("movel %0, d0; .word 0x4e7b, 0x0807" : : "r" (localCrp): "d0" ); 

    mmuCurrentTransTbl = transTbl;


    /* flush the address translation cache cause we're in a new context */

    __asm__ (".word 0xf518");	/* pflusha */

    intUnlock (oldLev);
    }

/******************************************************************************
*
* mmuATCFlush - flush an entry from the address translation cache
*
*/

LOCAL void mmuATCFlush 
    (
    void *addr
    )
    {
    __asm__ (
	    "movel %0, a0; "
	    "movel #1, d0; "
	    ".word 0x4e7b, 0x0001; "	/* movec d0, srp */
	    ".word 0xf508; "		/* pflush a0 */
	    "movel #5, d0; "
	    ".word 0x4e7b, 0x0001; "	/* movec d0, srp */
	    ".word 0xf508; "		/* pflush a0 */
	    :			/* outputs */
	    : "r" (addr)	/* inputs */
	    : "d0", "a0"	/* temps */
	    );
    }
