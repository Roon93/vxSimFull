/* mmu30Lib.c - mmu library for 68030 */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01n,10feb99,wsl  Add comment documenting ERRNO value
01m,11jul96,jmb  Make comments consistent with 8K pagesize, not 4K (SPR #5846)
01l,05jul94,rdc  mods to decrease fragmentation and provide alternate memory 
		 source.  Fixed bug that caused pages containing page 
		 descriptors to be write enabled.
01k,12feb93,rdc  changed scheme for sharing data structures with vmLib.
01j,09feb93,rdc  fixed bug in mmuMemPagesWriteDisable.
01i,09oct92,rdc  added prototype for mmuPtest.
01h,01oct92,jcf  changed cache* interface.
01f,22sep92,rdc  doc.
01e,13jul92,rdc  write protected pages used for global trans tbl.
01d,28jul92,jmm  added forward function references
01c,15jul92,jwt  converted cacheClearEntry() to cacheMc68kClearEntry().
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
by all translation tables;  all available  physical memory appears at the same 
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
available physical memory.  All calls made to mmuGlobaPageMap must occur before
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

This module supports the 68030 mmu with a two level translation table:

			    root
			     |
			     |
            -------------------------------------
 top level  |sftd |sftd |sftd |sftd |sftd |sftd | ... 
            -------------------------------------
	       |     |     |     |     |     |    
	       |     |     |     |     |     |    
      ----------     |     v     v     v     v
      |         ------    NULL  NULL  NULL  NULL
      |         |
      v         v
     ----     ----   
l   |sfpd|   |sfpd|
o    ----     ----
w   |sfpd|   |sfpd|     
e    ----     ----
r   |sfpd|   |sfpd|
l    ----     ----
e   |sfpd|   |sfpd|
v    ----     ----
e     .         .
l     .         .
      .         .


where the top level consists of an array of pointers (Short Format Table 
Descriptors) held within a single
8k page.  These point to arrays of Short Format Page Descriptor arrays in 
the lower level.  Each of these lower level arrays is also held within a single
8k page, and describes a virtual space of 16 MB (each short format page
descriptor is 4 bytes, so we get 2048 of these in each array, and each page
descriptor maps a 8KB page - thus 2048 * 8192 = 16MB.)  

To implement global virtual memory, a seperate translation table called 
mmuGlobalTransTbl is created when the module is initialized.  Calls to 
mmuGlobalPageMap will augment and modify this translation table.  When new
translation tables are created, memory for the top level array of sftd's is
allocated and initialized by duplicating the pointers in mmuGlobalTransTbl's
top level sftd array.  Thus, the new translation table will use the global
translation table's state information for portions of virtual memory that are
defined as global.  Here's a picture to illustrate:

	         GLOBAL TRANS TBL		      NEW TRANS TBL

 		       root				   root
		        |				    |
		        |				    |
            -------------------------           -------------------------
 top level  |sftd1|sftd2| NULL| NULL|           |sftd1|sftd2| NULL| NULL|
            -------------------------           -------------------------
	       |     |     |     |                 |     |     |     |   
	       |     |     |     |                 |     |     |     |  
      ----------     |     v     v        ----------     |     v     v
      |         ------    NULL  NULL      |		 |    NULL  NULL
      |         |			  |		 |
      o------------------------------------		 |
      |		|					 |
      |		o-----------------------------------------
      |		|
      v         v
     ----     ----   
l   |sfpd|   |sfpd|
o    ----     ----
w   |sfpd|   |sfpd|     
e    ----     ----
r   |sfpd|   |sfpd|
l    ----     ----
e   |sfpd|   |sfpd|
v    ----     ----
e     .         .
l     .         .
      .         .

Note that with this scheme, the global memory granularity is 16MB.  Each time
you map a section of global virtual memory, you dedicate at least 16MB of 
the virtual space to global virtual memory that will be shared by all virtual
memory contexts.

The physcial memory that holds these data structures is obtained from the
system memory manager via memalign to insure that the memory is page
aligned.  We want to protect this memory from being corrupted,
so we invalidate the descriptors that we set up in the global translation
that correspond to the memory containing the translation table data structures.
This creates a "chicken and the egg" paradox, in that the only way we can
modify these data structures is through virtual memory that is now invalidated,
and we can't validate it because the page descriptors for that memory are
in invalidated memory (confused yet?)
So, you will notice that anywhere that page table descriptors (pte's)
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
   point to another memory partition before mmu30LibInit is called.
*/




#include "vxWorks.h"
#include "string.h"
#include "intLib.h"
#include "stdlib.h"
#include "memLib.h"
#include "private/memPartLibP.h"
#include "private/vmLibP.h"
#include "arch/mc68k/mmu30Lib.h"
#include "mmuLib.h"
#include "errno.h"
#include "cacheLib.h"

/* forward declarations */
 
LOCAL void mmuMemPagesWriteDisable (MMU_TRANS_TBL *transTbl);
LOCAL STATUS mmuPteGet (MMU_TRANS_TBL *pTransTbl, void *virtAddr, PTE **result);
LOCAL MMU_TRANS_TBL *mmuTransTblCreate ();
LOCAL STATUS mmuTransTblInit (MMU_TRANS_TBL *newTransTbl);
LOCAL STATUS mmuTransTblDelete (MMU_TRANS_TBL *transTbl);
LOCAL STATUS mmuVirtualPageCreate (MMU_TRANS_TBL *thisTbl, void *virtPageAddr);
LOCAL STATUS mmuEnable (BOOL enable);
LOCAL void mmuOn ();
LOCAL void mmuOff ();
LOCAL STATUS mmuStateSet (MMU_TRANS_TBL *transTbl, void *pageAddr, UINT stateMask, UINT state);
LOCAL STATUS mmuStateGet (MMU_TRANS_TBL *transTbl, void *pageAddr, UINT *state);
LOCAL STATUS mmuPageMap (MMU_TRANS_TBL *transTbl, void *virtualAddress, void *physPage);
LOCAL STATUS mmuGlobalPageMap (void *virtualAddress, void *physPage);
LOCAL STATUS mmuTranslate (MMU_TRANS_TBL *transTbl, void *virtAddress, void **physAddress);
LOCAL void mmuCurrentSet (MMU_TRANS_TBL *transTbl);
LOCAL void mmuATCFlush (void *addr);
/* XXX for debugging:  LOCAL void mmuPtest (char *addr, int level); */
LOCAL char *mmuPageAlloc (MMU_TRANS_TBL *transTbl);

/* kludgey static data structure for parameters in __asm__ directives. */

LOCAL TC_REG localTc;
LOCAL CRP_REG localCrp;

int mmuPageSize;
int mmuNumPagesInFreeList = 4;
PART_ID mmuPageSource = NULL;

/* a translation table to hold the descriptors for the global transparent
 * translation of physical to virtual memory 
 */

LOCAL MMU_TRANS_TBL mmuGlobalTransTbl;

/* initially, the current trans table is a dummy table with mmu disabled */

LOCAL MMU_TRANS_TBL *mmuCurrentTransTbl = &mmuGlobalTransTbl;

/* array of booleans used to keep track of sections of virtual memory defined
 * as global.
 */

LOCAL BOOL *globalPageBlock;

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
     VM_STATE_CACHEABLE_NOT, MMU_STATE_CACHEABLE_NOT}

    };

LOCAL MMU_LIB_FUNCS mmuLibFuncsLocal =
    {
    mmu30LibInit,
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

LOCAL BOOL mmuEnabled = FALSE;

/* MMU_UNLOCK and MMU_LOCK are used to access page table entries that are in
 * virtual memory that has been invalidated to protect it from being corrupted
 */

#define MMU_UNLOCK(wasEnabled, oldLevel) \
(((wasEnabled) = mmuEnabled)  ? (((oldLevel) = intLock ()), mmuOff (), 0) : 0)

#define MMU_LOCK(wasEnabled, oldLevel) ((wasEnabled) ? (mmuOn (), intUnlock (oldLevel)) : 0)


/******************************************************************************
*
* mmu30LibInit - initialize module
*
* Build a dummy translation table that will hold the page table entries
* for the global translation table.  The mmu remains disabled upon
* completion.
*
* RETURNS: OK if no error, ERROR otherwise.
*
* ERRNO: S_mmuLib_INVALID_PAGE_SIZE
*/

STATUS mmu30LibInit 
    (
    int pageSize	/* system pageSize (must be 8192 for 68k) */
    )
    {

    PTE *pUpperLevelTable;
    int i;

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

    /* initialize the static data structure that mmuEnable uses to do the
     * pmove instruction
     */

    localTc.zero = 0;
    localTc.sre = 0;
    localTc.fcl = 0;

    localTc.pageSize = NUM_PAGE_OFFSET_BITS;

    localTc.initialShift = 0;

    localTc.tia = NUM_TIA_BITS;
    localTc.tib = NUM_TIB_BITS;
    localTc.tic = 0;
    localTc.tid = 0;

    /* initialize the static data structure that mmuCurrentSet uses to load 
     * the crp. 
     */

    localCrp.lu = 1;
    localCrp.limit = 0;
    localCrp.zero = 0; 
    localCrp.dt = DT_VALID_4_BYTE; 

    /* we assume a 8192 byte page size */

    if (pageSize != 8192)
	{
	errno = S_mmuLib_INVALID_PAGE_SIZE;
	return (ERROR);
	}

    mmuEnabled = FALSE;

    mmuPageSize = pageSize;

    /* allocate the global page block array to keep track of which parts
     * of virtual memory are handled by the global translation tables.
     * Allocate on page boundry so we can write protect it.
     */

    globalPageBlock = 
	(BOOL *) memPartAlignedAlloc (mmuPageSource, pageSize, pageSize);

    bzero ((char *) globalPageBlock, pageSize);

    /* build a dummy translation table which will hold the pte's for
     * global memory.  All real translation tables will point to this
     * one for controling the state of the global virtual memory  
     */

    lstInit (&mmuGlobalTransTbl.memFreePageList);

    mmuGlobalTransTbl.memBlocksUsedArray = 
	calloc (MMU_BLOCKS_USED_SIZE, sizeof (void *));

    if (mmuGlobalTransTbl.memBlocksUsedArray == NULL)
	return (ERROR);

    mmuGlobalTransTbl.memBlocksUsedIndex = 0;
    mmuGlobalTransTbl.memBlocksUsedSize  = MMU_BLOCKS_USED_SIZE;

    /* allocate a page to hold the upper level descriptor array */

    mmuGlobalTransTbl.pUpperLevelTable = pUpperLevelTable = 
	(PTE *) mmuPageAlloc (&mmuGlobalTransTbl);

    if (pUpperLevelTable == NULL)
	return (ERROR);

    /* invalidate all the upper level descriptors */

    for (i = 0; i < (1 << NUM_TIA_BITS) ;i++)
	{
	pUpperLevelTable[i].pte.sftd.addr = -1;
	pUpperLevelTable[i].pte.sftd.u = 0; 
	pUpperLevelTable[i].pte.sftd.wp = 0; 
	pUpperLevelTable[i].pte.sftd.dt = DT_INVALID; 
	}

    return (OK);
    }

/******************************************************************************
*
* mmuPteGet - get the pte for a given page
*
* mmuPteGet traverses a translation table and returns the (physical) address of
* the pte for the given virtual address.
*
* RETURNS: OK or ERROR if there is no virtual space for the given address 
*
*/

LOCAL STATUS mmuPteGet 
    (
    MMU_TRANS_TBL *pTransTbl, 	/* translation table */
    void *virtAddr,		/* virtual address */ 
    PTE **result		/* result is returned here */
    )
    {

    PTE *upperLevelPte;
    PTE *lowerLevelPteTable;
    int lowerLevelPteTableIndex;

    upperLevelPte = 
	   &pTransTbl->pUpperLevelTable [(UINT) virtAddr / PAGE_BLOCK_SIZE];    
    lowerLevelPteTable = (PTE *) upperLevelPte->pte.sftd.addr; 

    if ((UINT) lowerLevelPteTable == 0xfffffff) /* -1 in 28 bits */
	return (ERROR);

    lowerLevelPteTable = (PTE *)((UINT)lowerLevelPteTable << 4);

    lowerLevelPteTableIndex = 
       ((UINT) virtAddr >> NUM_PAGE_OFFSET_BITS) & ((1 << NUM_TIB_BITS) - 1);

    *result = &lowerLevelPteTable[lowerLevelPteTableIndex];

    return (OK);
    }

/******************************************************************************
*
* mmuTransTblCreate - create a new translation table.
*
* create a 68k translation table.  Allocates space for the MMU_TRANS_TBL
* data structure and calls mmuTransTblInit on that object.  
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
* Initialize a new translation table.  The upper level is copyed from the
* global translation mmuGlobalTransTbl, so that we
* will share the global virtual memory with all
* other translation tables.
* 
* RETURNS: OK or ERROR if unable to allocate memory for upper level.
*/

LOCAL STATUS mmuTransTblInit 
    (
    MMU_TRANS_TBL *newTransTbl		/* translation table to be inited */
    )
    {

    FAST PTE *pUpperLevelTable;

    lstInit (&newTransTbl->memFreePageList);

    newTransTbl->memBlocksUsedArray = 
	calloc (MMU_BLOCKS_USED_SIZE, sizeof (void *));

    if (newTransTbl->memBlocksUsedArray == NULL)
	return (ERROR);

    newTransTbl->memBlocksUsedIndex = 0;
    newTransTbl->memBlocksUsedSize  = MMU_BLOCKS_USED_SIZE;

    /* allocate a page to hold the upper level descriptor array */

    newTransTbl->pUpperLevelTable = pUpperLevelTable = 
	(PTE *) mmuPageAlloc (newTransTbl);

    if (pUpperLevelTable == NULL)
	return (ERROR);

    /* copy the upperlevel table from mmuGlobalTransTbl,
     * so we get the global virtual memory 
     */

    bcopy ((char *) mmuGlobalTransTbl.pUpperLevelTable, 
	   (char *) pUpperLevelTable, mmuPageSize);

    /* write protect virtual memory pointing to the the upper level table in 
     * the global translation table to insure that it can't be corrupted 
     */

    mmuStateSet (&mmuGlobalTransTbl, (void *) pUpperLevelTable, 
		 MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE_NOT);

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
    FAST int i;

    /* write enable the physical page containing the upper level pte */

    mmuStateSet (&mmuGlobalTransTbl, transTbl->pUpperLevelTable, 
		 MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE);

    /* free all the blocks of pages in memBlocksUsedArray */

    for (i = 0; i < transTbl->memBlocksUsedIndex; i++)
	free (transTbl->memBlocksUsedArray[i]);

    free (transTbl->memBlocksUsedArray);

    /* free the translation table data structure */

    free (transTbl);
    
    return (OK);
    }

/******************************************************************************
*
* mmuVirtualPageCreate - set up translation tables for a virtual page
*
* simply check if there's already a lower level sfpd array that has a
* pte for the given virtual page.  If there isn't, create one.
*
* RETURNS OK or ERROR if couldn't allocate space for lower level sfpd array.
*/

LOCAL STATUS mmuVirtualPageCreate 
    (
    MMU_TRANS_TBL *thisTbl, 		/* translation table */
    void *virtPageAddr			/* virtual addr to create */
    )
    {
    PTE *upperLevelPte;
    FAST PTE *lowerLevelPteTable;
    FAST UINT i;
    PTE *dummy;

    if (mmuPteGet (thisTbl, virtPageAddr, &dummy) == OK)
	return (OK);

    lowerLevelPteTable = (PTE *) mmuPageAlloc (thisTbl);

    if (lowerLevelPteTable == NULL)
	return (ERROR);

    /* invalidate every page in the new page block */

    for (i = 0; i < (1 << NUM_TIB_BITS); i++)
	{
	lowerLevelPteTable[i].pte.sfpd.fields.addr = -1;
	lowerLevelPteTable[i].pte.sfpd.fields.ci = 0;
	lowerLevelPteTable[i].pte.sfpd.fields.m = 0; 
	lowerLevelPteTable[i].pte.sfpd.fields.u = 0; 
	lowerLevelPteTable[i].pte.sfpd.fields.wp = 0; 
	lowerLevelPteTable[i].pte.sfpd.fields.dt = DT_INVALID; 
	}

    /* write protect the new physical page containing the pte's
       for this new page block */

    mmuStateSet (&mmuGlobalTransTbl, lowerLevelPteTable, 
   		 MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE_NOT); 

    /* unlock the physical page containing the upper level pte,
       so we can modify it */

    mmuStateSet (&mmuGlobalTransTbl, thisTbl->pUpperLevelTable, 
		 MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE);

    upperLevelPte = 
	    &thisTbl->pUpperLevelTable [(UINT) virtPageAddr / PAGE_BLOCK_SIZE];    

    /* modify the upperLevel pte to point to the new lowerLevel pte */

    upperLevelPte->pte.sftd.addr = (UINT) lowerLevelPteTable >> 4;
    upperLevelPte->pte.sftd.dt = DT_VALID_4_BYTE;

    /* write protect the upper level pte table */

    mmuStateSet (&mmuGlobalTransTbl, thisTbl->pUpperLevelTable, 
		     MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE_NOT);

    mmuATCFlush (virtPageAddr);

    return (OK);
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

    /* lock out interrupts to protect kludgey static data structure */

    oldIntLev = intLock ();  

    localTc.enable = enable;

    __asm__ ("pmove _localTc, tc");

    mmuEnabled = enable;

    intUnlock (oldIntLev);

    return (OK);
    }

/******************************************************************************
*
* mmuOn - turn mmu on 
*
* This routine assumes that interrupts are locked out.  It is called internally
* to enable the mmu after it has been disabled for a short period of time
* to access internal data structs.
*
* RETURNS: OK
*/

LOCAL void mmuOn 
    (
    )
    {
    localTc.enable = TRUE;
    __asm__ ("pmove _localTc, tc"); 
    }

/******************************************************************************
*
* mmuOff - turn mmu off 
*
* This routine assumes that interrupts are locked out.  It is called internally
* to disable the mmu for a short period of time
* to access internal data structs.
*
* RETURNS: OK
*/

LOCAL void mmuOff 
    (
    )
    {
    localTc.enable = FALSE;
    __asm__ ("pmove _localTc, tc"); 
    }

/******************************************************************************
*
* mmuStateSet - set state of virtual memory page
*
* mmuStateSet is used to modify the state bits of the pte for the given
* virtual page.  The following states are provided:
*
* MMU_STATE_VALID 	MMU_STATE_VALID_NOT	 vailid/invalid
* MMU_STATE_WRITABLE 	MMU_STATE_WRITABLE_NOT	 writable/writeprotected
* MMU_STATE_CACHEABLE 	MMU_STATE_CACHEABLE_NOT	 notcachable/cachable
*
* these may be or'ed together in the state parameter.  Additionally, masks
* are provided so that only specific states may be set:
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
    PTE *pagePte;
    volatile int oldIntLev;  /* volatile hushes warnings */
    BOOL wasEnabled;

    if (mmuPteGet (transTbl, pageAddr, &pagePte) != OK)
	return (ERROR);

    /* modify the pte with mmu turned off and interrupts locked out */

    /* XXX can't dynamically turn mmu on and off if virtual stack */
    /* only way is if you make mmuEnable inline and guarantee that this
       code is in physical memory  */

    MMU_UNLOCK (wasEnabled, oldIntLev);
    pagePte->pte.sfpd.bits = 
	(pagePte->pte.sfpd.bits & ~stateMask) | (state & stateMask);
    MMU_LOCK (wasEnabled, oldIntLev);

    mmuATCFlush (pageAddr);
    cacheArchClearEntry (DATA_CACHE, pagePte);

    return (OK);
    }

/******************************************************************************
*
* mmuStateGet - get state of virtual memory page
*
* mmuStateGet is used to retrieve the state bits of the pte for the given
* virtual page.  The following states are provided:
*
* MMU_STATE_VALID 	MMU_STATE_VALID_NOT	 vailid/invalid
* MMU_STATE_WRITABLE 	MMU_STATE_WRITABLE_NOT	 writable/writeprotected
* MMU_STATE_CACHEABLE 	MMU_STATE_CACHEABLE_NOT	 notcachable/cachable
*
* these are or'ed together in the returned state.  Additionally, masks
* are provided so that specific states may be extracted from the returned state:
*
* MMU_STATE_MASK_VALID 
* MMU_STATE_MASK_WRITABLE
* MMU_STATE_MASK_CACHEABLE
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
    PTE *pagePte;

    if (mmuPteGet (transTbl, pageAddr, &pagePte) != OK)
	return (ERROR);

    *state = pagePte->pte.sfpd.bits; 

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
    PTE *pagePte;
    volatile int oldIntLev;  /* volatile hushes warnings */
    FAST UINT addr = (UINT)physPage >> 8; 
    BOOL wasEnabled;

    if (mmuPteGet (transTbl, virtualAddress, &pagePte) != OK)
	{
	/* build the translation table for the virtual address */

	if (mmuVirtualPageCreate (transTbl, virtualAddress) != OK)
	    return (ERROR);

	if (mmuPteGet (transTbl, virtualAddress, &pagePte) != OK)
	    return (ERROR);
	}

    MMU_UNLOCK (wasEnabled, oldIntLev);
    pagePte->pte.sfpd.fields.addr = addr; 
    MMU_LOCK (wasEnabled, oldIntLev);

    mmuATCFlush (virtualAddress);
    cacheArchClearEntry (DATA_CACHE, pagePte);

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
    PTE *pagePte;
    volatile int oldIntLev;  /* volatile hushes warnings */
    FAST UINT addr = (UINT)physPage >> 8; 
    BOOL wasEnabled;

    if (mmuPteGet (&mmuGlobalTransTbl, virtualAddress, &pagePte) != OK)
	{
	/* build the translation table for the virtual address */

	if (mmuVirtualPageCreate (&mmuGlobalTransTbl, virtualAddress) != OK)
	    return (ERROR);

	if (mmuPteGet (&mmuGlobalTransTbl, virtualAddress, &pagePte) != OK)
	    return (ERROR);


	/* the globalPageBlock array is write protected */

	mmuStateSet (&mmuGlobalTransTbl, globalPageBlock, 
		     MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE);
	globalPageBlock [(unsigned) virtualAddress / PAGE_BLOCK_SIZE] = TRUE;	
	mmuStateSet (&mmuGlobalTransTbl, globalPageBlock, 
		     MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE_NOT);
	}

    MMU_UNLOCK (wasEnabled, oldIntLev);
    pagePte->pte.sfpd.fields.addr = addr; 
    MMU_LOCK (wasEnabled, oldIntLev);

    mmuATCFlush (virtualAddress);
    cacheArchClearEntry (DATA_CACHE, pagePte);

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
    PTE *pagePte;
    UINT dt;

    if (mmuPteGet (transTbl, virtAddress, &pagePte) != OK)
	{
	errno = S_mmuLib_NO_DESCRIPTOR; 
	return (ERROR);
	}

    *physAddress = (PTE *) pagePte->pte.sfpd.fields.addr; 
    dt = pagePte->pte.sfpd.fields.dt;

    if (dt == DT_INVALID)
	{
	errno = S_mmuLib_INVALID_DESCRIPTOR; 
	return (ERROR);
	}

    *physAddress = (void *) ((UINT) *physAddress << 8);

    /* add offset into page */

    *physAddress += (unsigned) virtAddress & ((1 << NUM_PAGE_OFFSET_BITS) - 1);

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
	mmuMemPagesWriteDisable (&mmuGlobalTransTbl);
	mmuMemPagesWriteDisable (transTbl);
	firstTime = FALSE;
	}

    oldLev = intLock ();
    localCrp.ta = ((unsigned int) transTbl->pUpperLevelTable) >> 4 ;
    __asm__("pmove _localCrp, crp");
    mmuCurrentTransTbl = transTbl;

    /* the address translation cache is automatically flushed when the
     * crp register is loaded by the pmove instruction ONLY if the fd bit
     * of the opcode is zero.  Thus, this is dependent on the assembler
     * implementation, so we flush the atc explicitly.
     */

    /* flush the address translation cache cause we're in a new context */

    __asm__("pflusha");
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
    __asm__ ("movel a6@(8)  ,a0");

    /* pflush 0,#0, a0 */
    __asm__ (".word 0xf010");
    __asm__ (".word 0x3810");

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
    int i;

    for (i = 0; i < (1 << NUM_TIA_BITS) ;i++)
	{
	thisPage = (void *) (transTbl->pUpperLevelTable[i].pte.sftd.addr << 4);
	if (thisPage != (void *) 0xfffffff0)
	    mmuStateSet (transTbl, thisPage, MMU_STATE_MASK_WRITABLE,
			 MMU_STATE_WRITABLE_NOT);
	}

    mmuStateSet (transTbl, transTbl->pUpperLevelTable, MMU_STATE_MASK_WRITABLE, 
		 MMU_STATE_MASK_WRITABLE);
    }

/******************************************************************************
*
* mmuPageAlloc - allocate a page of physical memory
*
*/

LOCAL char *mmuPageAlloc 
    (
    MMU_TRANS_TBL *thisTbl
    )
    {
    char *pPage;
    int i;

    if ((pPage = (char *) lstGet (&thisTbl->memFreePageList)) == NULL)
	{
	pPage = memPartAlignedAlloc (mmuPageSource,
				     mmuPageSize * mmuNumPagesInFreeList,
				     mmuPageSize); 

	if (pPage == NULL)
	    return (NULL);

	if (thisTbl->memBlocksUsedIndex >= thisTbl->memBlocksUsedSize)
	    {
	    void *newArray;

	    /* realloc the array */

	    thisTbl->memBlocksUsedSize *= 2;

	    newArray = realloc (thisTbl->memBlocksUsedArray, 
			        sizeof (void *) * thisTbl->memBlocksUsedSize);

	    if (newArray == NULL)
		{
		thisTbl->memBlocksUsedSize /= 2;
		return (NULL);	
		}

	    thisTbl->memBlocksUsedArray = (void **) newArray;
	    }

	thisTbl->memBlocksUsedArray [thisTbl->memBlocksUsedIndex++] = 
	      (void *) pPage; 

	for (i = 0; i < mmuNumPagesInFreeList; i++, pPage += mmuPageSize)
	    lstAdd (&thisTbl->memFreePageList, (NODE *) pPage);

	pPage = (char *) lstGet (&thisTbl->memFreePageList);
	}

    return (pPage);
    }

#if FALSE

// left here for debugging only
/* EXPERIMENTAL STUFF */

LOCAL char *addrBuf;
LOCAL PTE *ptestResult;
LOCAL MMU_SR copyOfSr;

/******************************************************************************
*
* mmuPtest - use the ptest instruction to traverse the translation table
*
*/

LOCAL void mmuPtest 
    (
    char *addr, 
    int level
    )
    {

    addrBuf = addr;

    if (level == 0)
	{
	__asm__ ("movel _addrBuf, a0");
	__asm__ ("ptestr #1,a0@,#7,a1");
	__asm__ ("movel a1, _ptestResult");
	__asm__ ("pmove psr, _copyOfSr");
	}
    else
	{
	__asm__ ("movel _addrBuf, a0");
	__asm__ ("ptestr #1,a0@,#1,a1");
	__asm__ ("movel a1, _ptestResult");
	__asm__ ("pmove psr, _copyOfSr");
	}


    printf ("mmu status register:\n");
    printf ("  bus error = %d\n", copyOfSr.b);
    printf ("  limit violation = %d\n", copyOfSr.l);
    printf ("  supervisor only = %d\n", copyOfSr.s);
    printf ("  write protected = %d\n", copyOfSr.w);
    printf ("  invalid         = %d\n", copyOfSr.i);
    printf ("  modified        = %d\n", copyOfSr.m);
    printf ("  transparent     = %d\n", copyOfSr.t);
    printf ("  num levels      = %d\n", copyOfSr.n);

    printf ("descriptor address = %x\n", ptestResult);
    printf ("  addr          = %x\n", ptestResult->pte.sfpd.fields.addr);
    printf ("  cache inhibit = %x\n", ptestResult->pte.sfpd.fields.ci);
    printf ("  modified      = %x\n", ptestResult->pte.sfpd.fields.m);
    printf ("  u/l           = %x\n", ptestResult->pte.sfpd.fields.u);
    printf ("  write protect = %x\n", ptestResult->pte.sfpd.fields.wp);
    printf ("  desc type     = %x\n", ptestResult->pte.sfpd.fields.dt);
    }
#endif


