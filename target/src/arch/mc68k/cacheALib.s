/* cacheALib.s - 68K cache management assembly routines */

/* Copyright 1984-1994 Wind River Systems, Inc. */
	.data
	.globl  _copyright_wind_river
	.long   _copyright_wind_river

/*
modification history
--------------------
01p,31oct94,kdl  merge cleanup.
01o,26oct94,tmk  added MC68LC040 support
01n.03jun94,tpr  clean up following code review.
		 added MC68060 cpu support.
01m,03feb93,jdi  mangen fixes.
01l,24jan93,jdi  documentation cleanup for 5.1.
01k,19oct92,jcf  back to version 1j. I_CACHE == 0, D_CACHE == 1
01j,09oct92,rdc  DATA_CACHE = 0, INSTRUCTION_CACHE = 1 !!
01i,01oct92,jcf  changed names. added cacheDTTR0ModeSet.
01h,24sep92,jdi  documentation cleanup.
01g,23aug92,jcf  changed bxxx to jxx.
01f,16jul92,jwt  added mc68kWriteBufferFlush(), cacheDtr0Get(), and 
                 cacheDtr0Set() for 68040 5.1 cache library.
01e,26may92,rrr  the tree shuffle
01d,07jan92,jcf  added special 68040 data cache disable routine.
		 cleaned up so 68000/68010 don't carry unnecessary luggage.
		 cleaned up cacheCPUSHSet() and cacheCINVSet().
01c,04oct91,rrr  passed through the ansification filter
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01b,28aug91,shl  added cacheCPUSHSet() and cacheCINVSet() for MC68040
		 cache manipulation, updated copyright.
01a,17jul90,jcf  written; based on sysCacheEnable ().
*/

/*
DESCRIPTION
This library contains routines to modify the Motorola 68K family 
CACR/CAAR control registers.

INCLUDE FILES: cacheLib.h

SEE ALSO: cacheLib

INTERNAL
We avoid assembler differences by use of .word's to hand code the tricky
assembly syntax.

*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"


#if (CPU==MC68020)				/* 68020 ONLY routines */
	.globl	_cacheCAARSet			/* set CAAR register */
	.globl	_cacheCAARGet			/* get CAAR register */
#endif	/* CPU==MC68020 */

/* 68040/060 ONLY routines */
#if (CPU==MC68040 || CPU==MC68060 || CPU==MC68LC040)
	.globl  _cacheCPUSH			/* push and invalidate cache */
	.globl  _cacheCINV			/* invalidate cache */
	.globl	_cache040DataDisable		/* data cache disable routine */
	.globl	_cache040WriteBufferFlush	/* flush the push buffer */
	.globl	_cacheDTTR0ModeSet		/* set with mode DTTR0 reg */
	.globl	_cacheDTTR0Set			/* set DTTR0 register */
	.globl	_cacheDTTR0Get			/* get DTTR0 register value */
#endif	/* CPU==MC68040 || CPU==MC68060 CPU==MC68LC040 */

/* 680[246]0 routines */
#if (CPU==MC68020 || CPU==MC68040 || CPU==MC68060 || CPU==MC68LC040)	
	.globl	_cacheCACRSet		/* set CACR register */
	.globl	_cacheCACRGet		/* get CACR register */
#endif	/* CPU==MC68020 || CPU==MC68040 || CPU==MC68LC040 */

	.text
	.even
/* 680[246]0 routines */
#if (CPU==MC68020 || CPU==MC68040 || CPU==MC68060 || CPU==MC68LC040)

/*******************************************************************************
*
* cacheCACRSet - set the CACR register (68K)
*
* This routine sets the CACR control register.
*
* RETURNS: N/A
*
* SEE ALSO: 
* .I "Motorola MC68020 32-Bit Microprocessor User's Manual"

* void cacheCACRSet
*     (
*     int newValue		/@ new value for the CACR @/
*     )

*/

_cacheCACRSet:

	link	a6,#0
	movel	a6@(ARG1),d0		/* put new CACR value */
	.word	0x4e7b, 0x0002		/* movec d0,cacr */
	unlk	a6
	rts

/*******************************************************************************
*
* cacheCACRGet - get the CACR register (68K)
*
* This routine gets the CACR control register.
*
* RETURNS: The value of the CACR register.
*
* SEE ALSO: 
* .I "Motorola MC68020 32-Bit Microprocessor User's Manual"

* int cacheCACRGet (void)

*/

_cacheCACRGet:

	link	a6,#0
	.word	0x4e7a, 0x0002		/* movec cacr,d0 */
	unlk	a6
	rts
#endif /* (CPU==MC68020 || CPU==MC68040 || CPU==MC68060 || CPU==MC68LC040) */

#if (CPU==MC68020)			/* CAAR only on MC68020 */

/*******************************************************************************
*
* cacheCAARSet - set the CAAR register (68K)
*
* This routine sets the CAAR control register.
*
* RETURNS: N/A
*
* SEE ALSO: 
* .I "Motorola MC68020 32-Bit Microprocessor User's Manual"

* void cacheCAARSet
*     (
*     int newValue		/@ new value for the CAAR @/
*     )

*/

_cacheCAARSet:

	link	a6,#0
	movel	a6@(ARG1),d0		/* put new CAAR value */
	.word	0x4e7b, 0x0802		/* movec d0,caar */
	unlk	a6
	rts

/*******************************************************************************
*
* cacheCAARGet - get the CAAR register (68K)
*
* This routine gets the CAAR control register.
*
* RETURNS: The value of the CAAR register.
*
* SEE ALSO: 
* .I "Motorola MC68020 32-Bit Microprocessor User's Manual"

* int cacheCAARGet (void)

*/

_cacheCAARGet:

	link	a6,#0
	.word	0x4e7a, 0x0802		/* movec caar,d0 */
	unlk	a6
	rts

#endif /* CPU==MC68040 || CPU==MC68060 CPU==MC68LC040 */

#if (CPU==MC68040 || CPU==MC68060 || CPU==MC68LC040)

/*******************************************************************************
*
* cache040DataDisable - push, invalidate, and disable MC68040/MC68060 data cache
*
* This is the support routine for turning off the MC68040/MC68060 data cache.
* This routine pushes the entire cache to the main memory,
* invalidates the data cache, and finally disables the CACR bit for the
* data cache.
*
* This special purpose routine is due to the constraint that the push and
* the disable of the MC68040/MC68060 data cache must be done as adjacent
* operations when using copyback mode.  The issue only arises when disabling
* the data cache.
*
* The MC68060 provides a push buffer, like the MC68040, but also a four-entry
* store buffer. This store buffer is used to defer pending writes in order to
* increase the processor performance. When enabled the store buffer is used
* only in writethrough mode and cache inhibited non serial mode. The store
* buffer and push buffer are fluched when synchronize instructions (i.e. nop)
* are excecuted.
*
* Interrupts are locked out during the execution of this routine.
*
 
* void cache040DataDisable ()
 
* NOMANUAL
*/

_cache040DataDisable:
	link	a6,#0
	movew	sr,d1			/* preserve status register */
	orw	#0x0700,sr		/* LOCK INTERRUPTS */
	.word	0xf478			/* cpusha dc */
	.word	0x4e7a,0x0002		/* movec cacr,d0 */
	andl	#0x7fffffff,d0		/* turn off data cache */
	.word	0x4e7b,0x0002		/* movec d0,cacr */
	nop				/* flush the push and store buffer */
	movew	d1,sr			/* UNLOCK INTERRUPTS */
	unlk	a6
	rts

/*******************************************************************************
*
* cacheCPUSH - push and invalidate cache lines (68K)
*
* This routine pushes the specified cache lines to the main memory and
* then invalidates the specified cache lines from a specified
* cache.
*
* RETURNS: N/A

* void cacheCPUSH
*     (
*     int cache,			/@ cache type  @/
*     int scope,			/@ scope field @/
*     int line				/@ cache line  @/
*     )

*/

_cacheCPUSH:
	link	a6,#0

	movel	a6@(ARG3),a0		/* line */
	movel	a6@(ARG2),d1		/* scope */
	movel	a6@(ARG1),d0		/* cache */
	jne     CPUSHData		/* not i-cache manipulation */

CPUSHInst:
	cmpl	#1,d1			/* scope == CACHE_LINE? */
	jne 	CPUSHInstPage		/* try CACHE_PAGE */
	nop				/* as per 68040 errata sheet */
	.word	0xf4a8			/* cpushl ic,a0@ */
	jra 	CPUSHFinish		/* done */
CPUSHInstPage:
	cmpl	#2,d1			/* scope == CACHE_PAGE? */
	jne 	CPUSHInstAll		/* try CACHE_ALL */
	nop				/* as per 68040 errata sheet */
	.word	0xf4b0			/* cpushp ic,a0@ */
	jra 	CPUSHFinish		/* done */
CPUSHInstAll:
	cmpl	#3,d1			/* scope == CACHE_ALL? */
	jne 	CPUSHFinish		/* invalid scope type */
	nop				/* as per 68040 errata sheet */
	.word	0xf4b8			/* cpusha ic */
	jra 	CPUSHFinish		/* done */

CPUSHData:
	cmpl	#1,d0			/* cache == DATA_CACHE? */
	jne 	CPUSHFinish		/* invalid cache type */
	cmpl	#1,d1			/* scope == CACHE_LINE? */
	jne 	CPUSHDataPage		/* try CACHE_PAGE */
	nop				/* as per 68040 errata sheet */
	.word	0xf468			/* cpushl dc,a0@ */
	jra 	CPUSHFinish		/* done */
CPUSHDataPage:
	cmpl	#2,d1			/* scope == CACHE_PAGE? */
	jne 	CPUSHDataAll		/* try CACHE_ALL */
	nop				/* as per 68040 errata sheet */
	.word	0xf470			/* cpushp dc,a0@ */
	jra 	CPUSHFinish		/* done */
CPUSHDataAll:
	cmpl	#3,d1			/* scope == CACHE_ALL? */
	jne 	CPUSHFinish		/* invalid scope type */
	nop				/* as per 68040 errata sheet */
	.word	0xf478			/* cpusha dc */

CPUSHFinish:
	unlk	a6			/* unlink */
	rts				/* we're outta here */

/*******************************************************************************
*
* cacheCINV - invalidate cache lines (68K)
*
* This routine invalidates the specified cache lines from a specified
* cache.
*
* RETURNS: N/A

* void cacheCINV
*     (
*     int cache,		/@ cache type  @/
*     int scope,		/@ scope field @/
*     int line			/@ cache line  @/
*     )

*/

_cacheCINV:
	link	a6,#0

	movel	a6@(ARG3),a0		/* line */
	movel	a6@(ARG2),d1		/* scope */
	movel	a6@(ARG1),d0		/* cache */
	jne     CINVData		/* not i-cache manipulation */

CINVInst:
	cmpl	#1,d1			/* scope == CACHE_LINE? */
	jne 	CINVInstPage		/* try CACHE_PAGE */
	nop				/* as per 68040 errata sheet */
	.word	0xf488			/* cinvl ic,a0@ */
	jra 	CINVFinish		/* done */
CINVInstPage:
	cmpl	#2,d1			/* scope == CACHE_PAGE? */
	jne 	CINVInstAll		/* try CACHE_ALL */
	nop				/* as per 68040 errata sheet */
	.word	0xf490			/* cinvp ic,a0@ */
	jra 	CINVFinish		/* done */
CINVInstAll:
	cmpl	#3,d1			/* scope == CACHE_ALL? */
	jne 	CINVFinish		/* invalid scope type */
	nop				/* as per 68040 errata sheet */
	.word	0xf498			/* cinva ic */
	jra 	CINVFinish		/* done */

CINVData:
	cmpl	#1,d0			/* cache == DATA_CACHE? */
	jne 	CINVFinish		/* invalid cache type */
	cmpl	#1,d1			/* scope == CACHE_LINE? */
	jne 	CINVDataPage		/* try CACHE_PAGE */
	nop				/* as per 68040 errata sheet */
	.word	0xf448			/* cinvl dc,a0@ */
	jra 	CINVFinish		/* done */
CINVDataPage:
	cmpl	#2,d1			/* scope == CACHE_PAGE? */
	jne 	CINVDataAll		/* try CACHE_ALL */
	nop				/* as per 68040 errata sheet */
	.word	0xf450			/* cinvp dc,a0@ */
	jra 	CINVFinish		/* done */
CINVDataAll:
	cmpl	#3,d1			/* scope == CACHE_ALL? */
	jne 	CINVFinish		/* invalid scope type */
	nop				/* as per 68040 errata sheet */
	.word	0xf458			/* cinva dc */

CINVFinish:
	unlk	a6			/* unlink */
	rts				/* we're outta here */

/*******************************************************************************
*
* cache040WriteBufferFlush - flush the MC68040/MC68060 push buffer
*
* This routine forces the MC68040/MC68060 push buffer to be flushed (via
* execution of a NOP instruction) and the store buffer(MC68060 only) .
*
* 
*
* RETURNS: N/A

* STATUS cache040WriteBufferFlush (void)

*/

_cache040WriteBufferFlush:
	nop				/* Flush the push and store buffer */
	clrl	d0			/* return OK */
	rts

/*******************************************************************************
*
* cacheDTTR0ModeSet - set the data transparent translation register to mode
*
* void cacheDTTR0Set (UINT ttr)
*
* NOMANUAL
*/

_cacheDTTR0ModeSet:
	link	a6,#0
	movel	a7@(ARG1),d1
	.word   0x4e7a,0x0006		/* movec DTT0,d0 */
	andl	#0xffffff9f,d0		/* mask off mode specifier */
	orl	d1,d0			/* or in the new mode specifier */
	.word   0x4e7b,0x0006		/* movec d0,DTT0 */
	unlk	a6
	rts

/*******************************************************************************
*
* cacheDTTROGet - read data transparent translation register 0
*
* UINT cacheDTTR0Get (void)
*
* NOMANUAL
*/

_cacheDTTR0Get:
	.word   0x4e7a,0x0006		/* movec DTT0,d0 */
	rts

/*******************************************************************************
*
* cacheDTTR0Set - write data transparent translation register 0
*
* void cacheDTTR0Set (UINT ttr)
*
* NOMANUAL
*/

_cacheDTTR0Set:
	link	a6,#0
	movel	a7@(ARG1),d0
	.word   0x4e7b,0x0006		/* movec d0,DTT0 */
	.word	0xf518			/* pflusha */
	unlk	a6
	rts

#endif	/* (CPU==MC68040 || CPU==MC68060 || CPU==MC68LC040) */
