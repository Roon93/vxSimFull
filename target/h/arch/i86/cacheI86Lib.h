/* cacheI86Lib.h - I80486 cache library header file */

/* Copyright 1984-2002 Wind River Systems, Inc. */
/*
modification history
--------------------
01e,07mar02,hdn  added CLFLUSH related macros (spr 73360)
01d,04dec01,hdn  added CACHE_TYPE parameter to cachePen4Flush/Clear
01c,23aug01,hdn  renamed cache486XXX to cacheI86XXX
		 added PENTIUM4 support
01b,29may94,hdn  deleted I80486 macro.
01a,15jun93,hdn  created.
*/

#ifndef __INCcacheI86Libh
#define __INCcacheI86Libh

#ifdef __cplusplus
extern "C" {
#endif

#include "vxWorks.h"


#define	CLFLUSH_DEF_BYTES	64	/* def bytes flushed by CLFLUSH */
#define	CLFLUSH_MAX_BYTES	255	/* max bytes flushed with CLFLUSH */


#ifndef	_ASMLANGUAGE

#if defined(__STDC__) || defined(__cplusplus)

extern STATUS cacheArchEnable (CACHE_TYPE cache);
extern STATUS cacheArchDisable (CACHE_TYPE cache);
extern STATUS cacheArchLock (CACHE_TYPE cache, void * address, size_t bytes);
extern STATUS cacheArchUnlock (CACHE_TYPE cache,
				void * address, size_t bytes);
extern STATUS cacheArchClear (CACHE_TYPE cache, void * address, size_t bytes);
extern STATUS cacheArchFlush (CACHE_TYPE cache, void * address, size_t bytes);
extern void * cacheArchDmaMalloc (size_t bytes);
extern STATUS cacheArchDmaFree (void * pBuf);
extern STATUS cacheArchTextUpdate (void * address, size_t bytes);
extern STATUS cacheArchClearEntry (CACHE_TYPE cache, void * address);

extern void cacheI86Reset (void);
extern void cacheI86Enable (void);
extern void cacheI86Disable (void);
extern void cacheI86Lock (void);
extern void cacheI86Unlock (void);
extern void cacheI86Clear (void);
extern void cacheI86Flush (void);
extern void cachePen4Clear (CACHE_TYPE cache, void * address, size_t bytes);
extern void cachePen4Flush (CACHE_TYPE cache, void * address, size_t bytes);

#else

extern STATUS cacheArchEnable ();
extern STATUS cacheArchDisable ();
extern STATUS cacheArchLock ();
extern STATUS cacheArchUnlock ();
extern STATUS cacheArchClear ();
extern STATUS cacheArchFlush ();
extern void * cacheArchDmaMalloc ();
extern STATUS cacheArchDmaFree ();
extern STATUS cacheArchTextUpdate ();
extern STATUS cacheArchClearEntry ();

extern void cacheI86Reset ();
extern void cacheI86Enable ();
extern void cacheI86Disable ();
extern void cacheI86Lock ();
extern void cacheI86Unlock ();
extern void cacheI86Clear ();
extern void cacheI86Flush ();
extern void cachePen4Clear ();
extern void cachePen4Flush ();

#endif /* __STDC__ */

#endif	/* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __INCcacheI86Libh */
