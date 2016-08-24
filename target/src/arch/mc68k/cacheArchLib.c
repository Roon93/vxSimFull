/* cacheArchLib.c - 68K cache management library */

/* Copyright 1984-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
03j,29nov95,jdi  doc: fixed typos.
03i,02aug95,tpr  added cacheArch060DmaMalloc ().
03g,26oct94,tmk  added MC68LC040 support
03h,02jun94,tpr  added cacheStoreBufEnable() and cacheStoreBufDisable().
		 clean up following code review.
		 added MC68060 cpu support.
03g,23feb93,jdi  doc: changed cacheArchInvalidate() to refer to 68040 only.
03f,15feb93,jdi  made everything NOMANUAL except for cacheArchLibInit() and
		 cacheArchClearEntry().
03e,10feb93,jwt  added missing address alignment for !ENTIRE_CACHE cases.
03d,24jan93,jdi  documentation cleanup for 5.1.
03c,22dec92,jwt  added cacheArchInvalidate() for '040 invalidate pointer.
03b,19oct92,jcf  fixed writethrough mode cacheFlushRtn.
03a,01oct92,jcf  reduced interface to attach to new cacheLib.
02l,23sep92,jdi  documentation cleanup, some ansification.
02k,23aug92,jcf  changed call to valloc to be indirect.  changed filename.
02j,02aug92,jwt  corrected and added "fail" return value for cacheIsOn().
02i,31jul92,jwt  converted per buffer function pointers to global; new names.
02h,19jul92,jcf  got CPU32 to compile.
02g,18jul92,smb  Changed errno.h to errnoLib.h.
02f,15jul91,jwt  further refinement of 68K cache library for 5.1:
                 freeze -> lock; added address and bytes parameters; 
                 added cacheMc68kModeSet(); deleted burst routines; cleanup.
02e,14jul92,jwt  added missing cacheMc68kFree; cleaned up compiler warnings.
02d,13jul92,rdc  added cacheMalloc.
02c,03jul92,jwt  first cut at 5.1 cache support; changed name from cacheLib.c.
02b,26may92,rrr  the tree shuffle
02a,07jan92,jcf  added special 68040 data cache disable routine.
		 cleaned up so 68000/68010 don't carry unnecessary luggage.
		 obsoleted cacheReset().
		 cacheEnable()/cacheDisable() take into account cache state.
01f,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed copyright notice
01e,25sep91,yao  added support for CPU32.
01d,28aug91,shl  added support for MC68040, added cacheClearPage(),
		 updated copyright.
01c,28sep90,jcf  documentation.
01b,02aug90,jcf  lint.
01a,15jul90,jcf  written.
*/

/*
DESCRIPTION
This library contains architecture-specific cache library functions for
the Motorola 68K family instruction and data caches.  The various members
of the 68K family of processors support different cache mechanisms; thus,
some operations cannot be performed by certain processors because they
lack particular functionalities.  In such cases, the routines in this
library return ERROR.  Processor-specific constraints are addressed in the
manual entries for routines in this library.  If the caches are
unavailable or uncontrollable, the routines return ERROR.  There is no
data cache on the 68020; however, data cache operations return OK.

INTERNAL
The cache enable and disable processes consist of the following actions,
executed by cacheArchEnable() and cacheArchDisable().  To enable
a disabled cache, first the cache is fully invalidated.  Then the cache mode 
(write-through, copy-back, etc.) is configured.  Finally, the cache is turned
on.  Enabling an already enabled cache results in no operation.

INTERNAL
To disable an enabled cache, first the cache is invalidated.  However, a cache
configured in copy-back mode must first have been pushed out to memory.  Once
invalidated, the cache is turned off.  Disabling an already disabled cache
results in no operation.

For general information about caching, see the manual entry for cacheLib.

INCLUDE FILES: cacheLib.h

SEE ALSO: cacheLib, vmLib

INTERNAL
The 68040 has a totally different way of handling its larger caches.
"burst fill", "freezing", and "clearing" are managed in hardware, or in
the ITT/DTT/PTE entry definitions.  They are NOT direct/simple entries in
the CAAR (which does not exist), or int the CACR (which is now a simple
enable/disable).

In addition, the 040 caches are not the simple mechanism used by the
020/030: caches may be "non serialized off", "serialized off",
"write-through", or "copy-back".  And these types may be set individually
based on decoding an address range through one of four transparant
translation registers, or through the MMU page table entries (a three-level
table structure).

The 68060 caches are slightly different from the 68040 caches. The inhibited
serialized and non serialized mode was removed and replaced by precise and 
imprecise mode. But except the names these modes seem to be identical. 
A four-deep store buffer is added to increase writethrough or imprecise mode
performance. For these modes the store buffer can be enabled or not.
A branch cache is also available, linked to the instruction cache to smooth
the instruction stream flow thus achieving integer performance level.
The frozen cache capability is provided for instruction and data cache.
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "errnoLib.h"
#include "cacheLib.h"
#include "stdlib.h"
#include "private/memPartLibP.h"
#include "private/vmLibP.h"
#include "private/funcBindP.h"

/* forward declarations */

LOCAL STATUS	cacheProbe (CACHE_TYPE cache);
LOCAL void	cacheSet (CACHE_TYPE cache, int value, int mask);
LOCAL BOOL	cacheIsOn (CACHE_TYPE cache);
#if	(CPU==MC68060)
LOCAL VOID	cacheBranchInv (void);
LOCAL void *	cacheArch060DmaMalloc (size_t bytes);
#endif	/* (CPU==MC68060) */


/*******************************************************************************
*
* cacheArchLibInit - initialize the 68K cache library
* 
* This routine initializes the cache library for Motorola MC680x0
* processors.  It initializes the function pointers and configures the
* caches to the specified cache modes.  Modes should be set before caching
* is enabled.  If two complementary flags are set (enable/disable), no
* action is taken for any of the input flags.
*
* The caching modes vary for members of the 68K processor family:
*
* .TS
* tab(|);
* l l l.
* 68020: | CACHE_WRITETHROUGH     | (instruction cache only)
*
* 68030: | CACHE_WRITETHROUGH     | 
*        | CACHE_BURST_ENABLE     |
*        | CACHE_BURST_DISABLE    |
*        | CACHE_WRITEALLOCATE    | (data cache only)
*        | CACHE_NO_WRITEALLOCATE | (data cache only)
*
* 68040: | CACHE_WRITETHROUGH     |
*        | CACHE_COPYBACK         | (data cache only)
*        | CACHE_INH_SERIAL       | (data cache only)
*        | CACHE_INH_NONSERIAL    | (data cache only)
*        | CACHE_BURST_ENABLE     | (data cache only)
*        | CACHE_NO_WRITEALLOCATE | (data cache only)
*
* 68060: | CACHE_WRITETHROUGH     |
*        | CACHE_COPYBACK         | (data cache only)
*        | CACHE_INH_PRECISE      | (data cache only)
*        | CACHE_INH_IMPRECISE    | (data cache only)
*        | CACHE_BURST_ENABLE     | (data cache only)
* .TE
* 
* The write-through, copy-back, serial, non-serial, precise and non precise
* modes change the state of the data transparent translation register (DTTR0)
* CM bits. Only DTTR0 is modified, since it typically maps DRAM space. 
*
* RETURNS: OK.
*/

STATUS cacheArchLibInit
    (
    CACHE_MODE	instMode,	/* instruction cache mode */
    CACHE_MODE	dataMode	/* data cache mode */
    )
    {
#if ((CPU == MC68020) || (CPU == MC68030) || (CPU == MC68040) || \
     (CPU==MC68060) || (CPU == MC68LC040))

    cacheLib.enableRtn		= cacheArchEnable;	/* cacheEnable() */
    cacheLib.disableRtn		= cacheArchDisable;	/* cacheDisable() */
    cacheLib.lockRtn		= cacheArchLock;	/* cacheLock() */
    cacheLib.unlockRtn		= cacheArchUnlock;	/* cacheUnlock() */
    cacheLib.clearRtn		= cacheArchClear;	/* cacheClear() */
    cacheLib.dmaMallocRtn	= (FUNCPTR)cacheArchDmaMalloc;
#if	(CPU == MC68060)
    if (dataMode & CACHE_SNOOP_ENABLE)
	cacheLib.dmaMallocRtn	= (FUNCPTR) cacheArch060DmaMalloc;
#endif
    cacheLib.dmaFreeRtn		= (FUNCPTR) cacheArchDmaFree;
    cacheLib.dmaVirtToPhysRtn	= NULL;
    cacheLib.dmaPhysToVirtRtn	= NULL;
    cacheLib.textUpdateRtn	= cacheArchTextUpdate;

#if ((CPU == MC68020) || (CPU == MC68030))
    cacheLib.flushRtn		= NULL;			/* writethrough */
    cacheLib.invalidateRtn	= cacheArchClear;	/* cacheFlush() */
    cacheLib.pipeFlushRtn	= NULL;
#elif ((CPU == MC68040) || (CPU == MC68060) || (CPU == MC68LC040))
    cacheLib.flushRtn		= cacheArchClear;	/* cacheFlush() */
    cacheLib.invalidateRtn	= cacheArchInvalidate;	/* cacheInvalidate() */
    cacheLib.pipeFlushRtn	= (FUNCPTR) cache040WriteBufferFlush;
#endif

#endif	/* ((CPU==MC68020) || (CPU==MC68030) || (CPU==MC68040) || \
	 *  (CPU==MC68060) || (CPU == MC68LC040)) */

#if ((CPU == MC68020) || (CPU == MC68030))

    /* check for parameter errors */

    if (((instMode & CACHE_COPYBACK)) || ((dataMode & CACHE_COPYBACK)) ||
	((instMode & (CACHE_SNOOP_ENABLE | CACHE_SNOOP_DISABLE)) != 0) ||
	((dataMode & (CACHE_SNOOP_ENABLE | CACHE_SNOOP_DISABLE)) != 0) ||
	((instMode & (CACHE_WRITEALLOCATE | CACHE_NO_WRITEALLOCATE)) != 0) ||
	((dataMode & CACHE_WRITEALLOCATE)&&(dataMode &CACHE_NO_WRITEALLOCATE))||
	((instMode & CACHE_BURST_ENABLE) && (instMode & CACHE_BURST_DISABLE)) ||
	((dataMode & CACHE_BURST_ENABLE) && (dataMode & CACHE_BURST_DISABLE)))
	return (ERROR);

    /* set instruction cache mode attributes */

    if (instMode & CACHE_BURST_ENABLE)
	cacheSet (INSTRUCTION_CACHE, C_BURST, C_BURST);

    if (instMode & CACHE_BURST_DISABLE)
	cacheSet (INSTRUCTION_CACHE, 0, C_BURST);

    /* set data cache mode attributes */

    if (dataMode & CACHE_WRITEALLOCATE)
	cacheSet (DATA_CACHE, C_ALLOCATE, C_ALLOCATE);

    if (dataMode & CACHE_NO_WRITEALLOCATE) 
	cacheSet (DATA_CACHE, 0, C_ALLOCATE);

    if (dataMode & CACHE_BURST_ENABLE)
	cacheSet (DATA_CACHE, C_BURST, C_BURST);

    if (dataMode & CACHE_BURST_DISABLE)
	cacheSet (DATA_CACHE, 0, C_BURST);

    cacheSet (INSTRUCTION_CACHE, 0, C_ENABLE);	/* turn off instruction cache */
    cacheSet (DATA_CACHE, 0, C_ENABLE);		/* turn off data cache */

    cacheSet (INSTRUCTION_CACHE, C_CLR, C_CLR);	/* invalidate I-cache entries */
    cacheSet (DATA_CACHE, C_CLR, C_CLR);	/* invalidate D-cache entries */

#elif ((CPU == MC68040) || (CPU == MC68060) || (CPU == MC68LC040))

    /* check for parameter errors */
#if 	(CPU == MC68040 || CPU == MC68LC040)
    if (((instMode & ~CACHE_WRITETHROUGH) != 0) ||
        ((dataMode & CACHE_COPYBACK) && (dataMode & CACHE_WRITETHROUGH)) ||
	((dataMode & CACHE_INH_SERIAL) && (dataMode & CACHE_INH_NONSERIAL)) ||
	((dataMode & CACHE_SNOOP_ENABLE) && (dataMode & CACHE_SNOOP_DISABLE)) ||
	((dataMode & CACHE_WRITEALLOCATE)) ||
	((dataMode & CACHE_BURST_DISABLE)))
#elif	(CPU == MC68060)
    if (((instMode & ~(CACHE_WRITETHROUGH)) !=0) ||
	((dataMode & CACHE_COPYBACK) && (dataMode & CACHE_WRITETHROUGH)) ||
	((dataMode & CACHE_INH_PRECISE) && (dataMode & CACHE_INH_IMPRECISE)) ||
	((dataMode & CACHE_SNOOP_ENABLE) && (dataMode & CACHE_SNOOP_DISABLE)) ||
	((dataMode & CACHE_WRITEALLOCATE)) ||
	((dataMode & CACHE_NO_WRITEALLOCATE)) ||
	((dataMode & CACHE_BURST_DISABLE)))
#endif 
	return (ERROR);

    if (dataMode & CACHE_WRITETHROUGH) 
	{
	cacheDTTR0ModeSet (C_TTR_WRITETHROUGH);
	cacheLib.flushRtn = cacheArchClear;	/* mmu may turn on copy back */
	}
    else if (dataMode & CACHE_COPYBACK)
	{
	cacheDTTR0ModeSet (C_TTR_COPYBACK);
	cacheLib.flushRtn = cacheArchClear;
	}
    else
#if	(CPU == MC68040 || CPU == MC68LC040)
	if (dataMode & CACHE_INH_SERIAL)
	{
	cacheDTTR0ModeSet (C_TTR_SERIALIZED);
	cacheLib.flushRtn = NULL;
	}
    else if (dataMode & CACHE_INH_NONSERIAL)
	{
	cacheDTTR0ModeSet (C_TTR_NOT_SERIAL);
	cacheLib.flushRtn = (FUNCPTR) cache040WriteBufferFlush;
	}
#elif	(CPU == MC68060)
	if (dataMode & CACHE_INH_PRECISE)
	{
	cacheDTTR0ModeSet (C_TTR_PRECISE);
	cacheLib.flushRtn = NULL;
	}
    else if (dataMode & CACHE_INH_IMPRECISE)
	{
	cacheDTTR0ModeSet (C_TTR_NOT_PRECISE);
	cacheLib.flushRtn = (FUNCPTR) cache040WriteBufferFlush;
	}
#endif

    cacheSet (INSTRUCTION_CACHE, 0, C_ENABLE);	/* turn off instruction cache */
    cacheSet (DATA_CACHE, 0, C_ENABLE);		/* turn off data cache */

    cacheCINV (INSTRUCTION_CACHE,CACHE_ALL,0x0);/* invalidate I-cache entries */
    cacheCINV (DATA_CACHE, CACHE_ALL, 0x0);	/* invalidate D-cache entries */

#endif	/* (CPU == MC68040 || CPU == MC68060 || CPU == MC68LC040) */

    cacheDataMode	= dataMode;		/* save dataMode for enable */
    cacheDataEnabled	= FALSE;		/* d-cache is currently off */
    cacheMmuAvailable	= FALSE;		/* no mmu yet */

    return (OK);
    }

/* No further cache support required MC68000, MC68010, or CPU32 */

#if ((CPU==MC68020) || (CPU==MC68030) || (CPU==MC68040) || \
     (CPU==MC68060) || (CPU==MC68LC040))

/*******************************************************************************
*
* cacheArchEnable - enable a 68K cache
*
* This routine enables the specified 68K instruction or data cache.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*
* NOMANUAL
*/

STATUS cacheArchEnable
    (
    CACHE_TYPE	cache		/* cache to enable */
    )
    {
    if (cacheProbe (cache) != OK)
	return (ERROR);				/* invalid cache */

    if (!cacheIsOn (cache))			/* cache already on? */
	cacheSet (cache, C_ENABLE, C_ENABLE);	/* turn the cache on */

    if (cache == DATA_CACHE)
	{
	cacheDataEnabled = TRUE;
	cacheFuncsSet ();
	}

    return (OK);
    }

/*******************************************************************************
*
* cacheArchDisable - disable a 68K cache
*
* This routine disables the specified 68K instruction or data cache.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*
* NOMANUAL
*/

STATUS cacheArchDisable
    (
    CACHE_TYPE	cache		/* cache to disable */
    )
    {
    if (cacheProbe (cache) != OK)
	return (ERROR);				/* invalid cache */

    if (cacheIsOn (cache))			/* already off? */
	{
#if ((CPU == MC68020) || (CPU == MC68030))
	cacheSet (cache, 0, C_ENABLE);		/* disable the cache */
	cacheSet (cache, C_CLR, C_CLR); 	/* invalidate cache entries */
#elif ((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040))

	switch (cache)
	    {
	    case DATA_CACHE:
		cache040DataDisable ();		/* disable/push/invalidate */
		break;

#if (CPU==MC68060)
	    case BRANCH_CACHE:
		cacheSet (cache, 0, C_ENABLE);	/* disable branch cache */
		cacheBranchInv ();		/* invalidate B-cache entries */
		break;
#endif /* (CPU==MC68060) */

	    case INSTRUCTION_CACHE:
		cacheSet (cache, 0, C_ENABLE);	/* disable instruction cache */
		cacheCINV (INSTRUCTION_CACHE, CACHE_ALL, 0x0);	/* inv cache */	
		break;

	    default :
		/* cacheProbe returns ERROR */
	    }
#endif
	}

    if (cache == DATA_CACHE)
	{
	cacheDataEnabled = FALSE;		/* data cache is off */
	cacheFuncsSet ();			/* update data function ptrs */
	}

    return (OK);
    }

/*******************************************************************************
*
* cacheArchLock - lock entries in a 68K cache
*
* This routine locks all entries in the specified 68K cache.  Only the 68020, 
* 68030 and 68060 allow cache locking.  The <bytes> argument must be set 
* to ENTIRE_CACHE.
*
* The 68060 banch cache can not be locked so if <cache> is BRANCH_CACHE 
* the function will return ERROR.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*
* NOMANUAL
*/

STATUS cacheArchLock
    (
    CACHE_TYPE	cache, 		/* cache to lock */
    void *	address,	/* address to lock */
    size_t	bytes		/* bytes to lock (ENTIRE_CACHE) */
    )
    {
    if (cacheProbe (cache) != OK)
	return (ERROR);				/* invalid cache */

#if ((CPU == MC68020) || (CPU == MC68030) || (CPU == MC68060))

    if (bytes != ENTIRE_CACHE)
	return (ERROR);				/* invalid operation */

#if (CPU == MC68060)
    if (cache == BRANCH_CACHE)
	return (ERROR);				/* invalid operation */
#endif /* (CPU == MC68060) */

    cacheSet (cache, C_FREEZE, C_FREEZE);	/* diddle the CACR bit */
    return (OK);

#elif (CPU == MC68040 || CPU == MC68LC040)
    errno = S_cacheLib_INVALID_CACHE;		/* set errno */
    return (ERROR);				/* no cache */
#endif
    }

/*******************************************************************************
*
* cacheArchUnlock - unlock a 68K cache
*
* This routine unlocks all entries in the specified 68K cache.  Only the 
* 68020, 68030 and 68060 allow cache locking.  The <bytes> argument must be 
* set to ENTIRE_CACHE.
*
* The 68060 banch cache can not be unlocked so if <cache> is BRANCH_CACHE 
* the function will return ERROR.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*
* NOMANUAL
*/

STATUS cacheArchUnlock
    (
    CACHE_TYPE	cache, 		/* cache to unlock */
    void *	address,	/* address to unlock */
    size_t	bytes		/* bytes to unlock (ENTIRE_CACHE) */
    )
    {
    if (cacheProbe (cache) != OK)
	return (ERROR);				/* invalid cache */

#if ((CPU == MC68020) || (CPU == MC68030) || (CPU == MC68060))

    if (bytes != ENTIRE_CACHE)
	return (ERROR);				/* invalid operation */

#if (CPU == MC68060)
    if (cache == BRANCH_CACHE)
	return (ERROR);				/* invalid operation */
#endif /* (CPU == MC68060) */

    cacheSet (cache, 0, C_FREEZE);		/* diddle the CACR bit */
    return (OK);

#elif (CPU == MC68040 || CPU == MC68LC040)
    errno = S_cacheLib_INVALID_CACHE;		/* set errno */
    return (ERROR);				/* no cache */
#endif
    }

/*******************************************************************************
*
* cacheArchClear - clear all entries from a 68K cache
*
* This routine clears some or all entries from the specified 68K cache.  
* 
* For the MC68060 processor, when the instruction cache is cleared (invalidated)* the branch cache is also invalidated by the hardware. One line in the branch
* cache cannot be invalidated so each time the branch cache is entirely
* invalidated.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*
* NOMANUAL
*/

STATUS cacheArchClear
    (
    CACHE_TYPE	cache, 		/* cache to clear */
    void *	address,	/* address to clear */
    size_t	bytes		/* bytes to clear */
    )
    {
#if (CPU == MC68060)
    int cacr;			/* cache control register value */
#endif /* (CPU == MC68060) */

    if (cacheProbe (cache) != OK)
	return (ERROR);				/* invalid cache */

#if ((CPU == MC68020) || (CPU == MC68030))

    if (bytes == ENTIRE_CACHE)
        {
	cacheSet (cache, C_CLR, C_CLR);		/* invalidate all cache */
	}
    else
	{
	bytes += (size_t) address;
	address = (void *) ((UINT) address & ~(4 - 1));

	do  {
	    cacheCAARSet ((void *) ((UINT) address & C_INDEX_MASK));
	    cacheSet (cache, C_CLR_ENTRY, C_CLR_ENTRY);
	    address = (void *) ((UINT) address + 4);
	    } while ((size_t) address < bytes);
	}

    return (OK);

#elif ((CPU == MC68040) || (CPU == MC68060) || (CPU == MC68LC040))

#if (CPU == MC68060)
    if (cache == BRANCH_CACHE)
	{
	cacheBranchInv ();			/* invalidate branch cache */
	return (OK);
	}

    cacr = cacheCACRGet ();			/* get CACR value */
    cacheCACRSet (cacr & ~C_CACR_DPI); 		/* enable CPUSH invalidation */
#endif /* (CPU == MC68060) */

    if (bytes == ENTIRE_CACHE)
        {
	cacheCPUSH (cache, CACHE_ALL, 0x0);	/* push/invalidate all cache */
	}
    else
	{
	bytes += (size_t) address;
	address = (void *) ((UINT) address & ~(_CACHE_ALIGN_SIZE - 1));

	do  {
	    cacheCPUSH (cache, CACHE_LINE, (void *) address);
	    address = (void *) ((UINT) address + _CACHE_ALIGN_SIZE);
	    } while ((size_t) address < bytes);
	}

    cache040WriteBufferFlush ();		/* flush write buffer */

#if (CPU == MC68060)
	cacheCACRSet (cacr);			/* restore CACR value */
#endif /* (CPU == MC68060) */

    return (OK);
#endif
    }

/*******************************************************************************
*
* cacheArchClearEntry - clear an entry from a 68K cache
*
* This routine clears a specified entry from the specified 68K cache.
*
* For 68040 processors, this routine clears the cache line from the cache
* in which the cache entry resides.
*
* For the MC68060 processor, when the instruction cache is cleared (invalidated)* the branch cache is also invalidated by the hardware. One line in the branch
* cache cannot be invalidated so each time the branch cache is entirely
* invalidated.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*/

STATUS cacheArchClearEntry
    (
    CACHE_TYPE	cache,		/* cache to clear entry for */
    void *	address		/* entry to clear */
    )
    {
#if (CPU == MC68060)
    int cacr;			/* cache control register value */
#endif /* (CPU == MC68060) */

    if (cacheProbe (cache) != OK)
	return (ERROR);				/* invalid cache */

#if (CPU == MC68060)
    if (cache == BRANCH_CACHE)
	{
	cacheBranchInv ();			/* invalidate branch cache */
	return (OK);
	}

    cacr = cacheCACRGet ();			/* get CACR value */
    cacheCACRSet (cacr & ~C_CACR_DPI);		/* enable CPUSH invalidation */
#endif /* (CPU == MC68060) */

#if ((CPU == MC68020) || (CPU == MC68030))
    cacheCAARSet (address);			/* address to invalidate */
    cacheSet (cache, C_CLR_ENTRY, C_CLR_ENTRY); /* invalidate cache entries */
#elif ((CPU == MC68040) || (CPU == MC68060) || (CPU == MC68LC040))
    cacheCPUSH (cache, CACHE_LINE, address);	/* push/invalidate entry */
    cache040WriteBufferFlush ();		/* flush write buffer */
#endif

#if (CPU == MC68060)
	cacheCACRSet (cacr);			/* restore CACR value */
#endif /* (CPU == MC68060) */

    return (OK);
    }

/*******************************************************************************
*
* cacheArchDmaMalloc - allocate a cache-safe buffer
*
* This routine attempts to return a pointer to a section of memory
* that will not experience cache coherency problems.  This routine
* is only called when MMU support is available 
* for cache control.
*
* INTERNAL
* We check if the cache is actually on before allocating the memory.  It
* is possible that the user wants Memory Management Unit (MMU)
* support but does not need caching.
*
* RETURNS: A pointer to a cache-safe buffer, or NULL.
*
* SEE ALSO: cacheArchDmaFree(), cacheDmaMalloc()
*
* NOMANUAL
*/

void *cacheArchDmaMalloc 
    (
    size_t      bytes			/* size of cache-safe buffer */
    )
    {
    void *pBuf;
    int	  pageSize;

    if (!cacheIsOn (DATA_CACHE))	/* cache is off just allocate buffer */
	{
	pBuf = malloc (bytes);
	return (pBuf);
	}
    
    if ((pageSize = VM_PAGE_SIZE_GET ()) == ERROR)
	return (NULL);

    /* make sure bytes is a multiple of pageSize */

    bytes = bytes / pageSize * pageSize + pageSize;

    if ((_func_valloc == NULL) || 
	((pBuf = (void *)(* _func_valloc) (bytes)) == NULL))
	return (NULL);

    VM_STATE_SET (NULL, pBuf, bytes,
		  VM_STATE_MASK_CACHEABLE, VM_STATE_CACHEABLE_NOT);

    return (pBuf);
    }

#if 	(CPU == MC68060)
/*******************************************************************************
*
* cacheArch060DmaMalloc - allocate a buffer with the cache in writethrough
*
* The MC68060 cache must be in writethrough mode when the snoop is used.
*/

LOCAL void * cacheArch060DmaMalloc
    (
    size_t      bytes                   /* size of cache-safe buffer */
    )
    {
    void *pBuf;
    int   pageSize;

    if ((pageSize = VM_PAGE_SIZE_GET ()) == ERROR)
	return (NULL);

    /* make sure bytes is a multiple of pageSize */

    bytes = bytes / pageSize * pageSize + pageSize;

    if ((_func_valloc == NULL) || 
	((pBuf = (void *)(* _func_valloc) (bytes)) == NULL))
	return (NULL);

    VM_STATE_SET (NULL, pBuf, bytes,
		  VM_STATE_MASK_CACHEABLE, VM_STATE_CACHEABLE_WRITETHROUGH);

    return (pBuf);
    }
#endif 	/* (CPU == MC68060) */
	
/*******************************************************************************
*
* cacheArchDmaFree - free the buffer acquired by cacheArchDmaMalloc()
*
* This routine returns to the free memory pool a block of memory previously
* allocated with cacheArchDmaMalloc().  The buffer is marked cacheable.
*
* RETURNS: OK, or ERROR if cacheArchDmaMalloc() cannot be undone.
*
* SEE ALSO: cacheArchDmaMalloc(), cacheDmaFree()
*
* NOMANUAL
*/

STATUS cacheArchDmaFree
    (
    void *pBuf		/* ptr returned by cacheArchDmaMalloc() */
    )
    {
    BLOCK_HDR *	pHdr;			/* pointer to block header */
    STATUS	status = OK;		/* return value */

    if (cacheIsOn (DATA_CACHE) && vmLibInfo.vmLibInstalled)
	{
	pHdr = BLOCK_TO_HDR (pBuf);

	status = VM_STATE_SET (NULL,pBuf,(pHdr->nWords * 2) - sizeof(BLOCK_HDR),
			       VM_STATE_MASK_CACHEABLE, VM_STATE_CACHEABLE);
	}

    free (pBuf);			/* free buffer after modified */

    return (status);
    }

/*******************************************************************************
*
* cacheProbe - test for the prescence of a type of cache
*
* This routine returns status with regard to the prescence of a particular
* type of cache.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*
* CAVEATS
* We do not distinguish between MC68020/MC68030, therefore a probe of a data
* cache on a MC68020 will return OK.
*
*/

LOCAL STATUS cacheProbe
    (
    CACHE_TYPE	cache 		/* cache to test */
    )
    {
    if ((cache==INSTRUCTION_CACHE) || (cache==DATA_CACHE))
        return (OK);

#if (CPU == MC68060)
    if (cache == BRANCH_CACHE) 
        return (OK);
#endif /* (CPU == MC68060) */

    errno = S_cacheLib_INVALID_CACHE;			/* set errno */
    return (ERROR);
    }

/*******************************************************************************
*
* cacheSet - set the cache CACR register accordingly
*
* This routine underlies all cache control routines.  It does the actual
* 68k cache CACR register manipulation based on the specified request.
*
* RETURN : N/A
*
*/

LOCAL void cacheSet
    (
    CACHE_TYPE	cache,		/* cache to change */
    int		value,		/* value to set */
    int		mask 		/* bits in value to pay attention to */
    )

    {
    int cacr = cacheCACRGet ();		/* get current CACR value */

    if (cache == DATA_CACHE)		/* shift params for data cache*/
	{
	mask  <<= C_CACR_SHIFT;
	value <<= C_CACR_SHIFT;
	}

#if (CPU == MC68060)
    if (cache == BRANCH_CACHE)		/* shift params for branch cache*/
	{
	mask  <<= C_CACR_BRANCH_SHIFT;
	value <<= C_CACR_BRANCH_SHIFT;
	}
#endif /* (CPU == MC68060) */


    cacr = (cacr & ~mask) | value;	/* or in the new value */
    cacheCACRSet (cacr);		/* set CACR control bits */
    }

/*****************************************************************************
*
* cacheIsOn - boolean function to return state of cache
*
* This routine returns the state of the specified cache.  The cache is
* assumed to exist.
*
* RETURNS: TRUE, if specified cache is enabled, FALSE otherwise.
*/

LOCAL BOOL cacheIsOn
    (
    CACHE_TYPE	cache 		/* cache to examine state */
    )
    {
    int cacr = cacheCACRGet ();		/* get current CACR value */
    int mask = C_ENABLE;		/* cache enable mask */

    if (cache == DATA_CACHE)		/* shift mask for data cache */
	mask <<= C_CACR_SHIFT;
    else
#if (CPU == MC68060)
	if (cache == BRANCH_CACHE)	/* shift mask for branch cache */
	    mask <<= C_CACR_BRANCH_SHIFT;
	else
#endif /* (CPU == MC68060) */
	if (cache != INSTRUCTION_CACHE)/* if not i-cache then unknown! */
	    return (FALSE);

    return ((cacr & mask) != 0);	/* return state of enable bit */
    }

/*******************************************************************************
*
* cacheArchTextUpdate - synchronize the 68K instruction and data caches
*
* This routine flushes the 68K data cache, and then invalidates the 
* instruction cache.  The instruction cache is forced to fetch code that
* may have been created via the data path.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*
* NOMANUAL
*/

STATUS cacheArchTextUpdate 
    (
    void * address,	/* virtual address */
    size_t bytes	/* number of bytes to update */
    )
    {
    if (cacheFlush (DATA_CACHE, address, bytes) == OK)
	return (cacheInvalidate (INSTRUCTION_CACHE, address, bytes));
    else
	{
#if (CPU == MC68060)
	cacheBranchInv ();
#endif /* (CPU == MC68060) */
	return (ERROR);
	}
    }

#endif	/* ((CPU==MC68020) || (CPU==MC68030) || (CPU==MC68040) || \
	 *  (CPU==MC68060) || (CPU == MC68LC040)) */

#if	((CPU == MC68040) || (CPU == MC68060) || (CPU == MC68LC040))
/*******************************************************************************
*
* cacheArchInvalidate - invalidate entries in a 68040/68060 cache
*
* This routine invalidates some or all entries in a specified
* 68040/68060 cache.  
*
* In the MC68060 case, when the instruction cache is invalidated the 
* branch cache is also invalidated by the hardware.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*
* NOMANUAL
*/

STATUS cacheArchInvalidate
    (
    CACHE_TYPE	cache, 		/* cache to invalidate */
    void *	address,	/* virtual address */
    size_t	bytes		/* number of bytes to invalidate */
    )
    {
    if (cacheProbe (cache) != OK)
	return (ERROR);				/* invalid cache */

#if (CPU == MC68060)
    if (cache == BRANCH_CACHE)
	{
	cacheBranchInv();			/* invalid the branch cache */
	return (OK);
	}
#endif /* (CPU == MC68060) */

    if (bytes == ENTIRE_CACHE)
        {
	cacheCINV (cache, CACHE_ALL, 0x0);	/* invalidate all cache */
	}
    else
	{
	bytes += (size_t) address;
	address = (void *) ((UINT) address & ~(_CACHE_ALIGN_SIZE - 1));

	do  {
	    cacheCINV (cache, CACHE_LINE, (int) address);
	    address = (void *) ((UINT) address + _CACHE_ALIGN_SIZE);
	    } while ((size_t) address < bytes);
	}

    return (OK);
    }
#endif	/* CPU == MC68040 || CPU == MC68060 || CPU == MC68LC040 */

#if (CPU == MC68060)
/*******************************************************************************
*
* cacheBranchInv - invalidate all branch cache entries (MC68060 only)
*
* This routine invalidates all branch cache entries by setting the
* Cache Control Register CABC bit.
*
* RETURN: N/A
*
* NOMANUAL
*/

LOCAL void cacheBranchInv (void)
    {
    int cacr;

    cacr = cacheCACRGet();			/* get CACR register value */
    cacheCACRSet (cacr | C_CACR_CABC);		/* set CABC bit of CACR */
    }

/*******************************************************************************
*
* cacheStoreBufEnable - enable the store buffer (MC68060 only)
*
* This routine sets the ESB bit of the Cache Control Register (CACR) to
* enable the store buffer.  To maximize performance, the four-entry
* first-in-first-out (FIFO) store buffer is used to defer pending writes
* to writethrough or cache-inhibited imprecise pages.
*
* RETURNS: N/A
*
*/

void cacheStoreBufEnable (void)
    {
    int cacr;

    cacr = cacheCACRGet();			/* get CACR register value */
    cacheCACRSet (cacr | C_CACR_ESB);		/* set ESB bit of CACR */
    }

/*******************************************************************************
*
* cacheStoreBufDisable - disable the store buffer (MC68060 only)
*
* This routine resets the ESB bit of the Cache Control Register (CACR) to  
* disable the store buffer.
*
* RETURNS: N/A
*
*/

void cacheStoreBufDisable (void)
    {
    int cacr;

    cacr = cacheCACRGet();			/* get CACR register value */
    cacheCACRSet (cacr & ~C_CACR_ESB);		/* reset ESB bit of CACR */
    }

#endif /* (CPU == MC68060) */
