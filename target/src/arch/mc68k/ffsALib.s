/* ffsALib.s - find first set function */

/* Copyright 1984-1994 Wind River Systems, Inc. */
	.data
	.globl  _copyright_wind_river
	.long   _copyright_wind_river


/*
modification history
--------------------
01n,26oct94,tmk  added MC68LC040 support
01m,30may94,tpr	 added MC68060 cpu support.
01l,23aug92,jcf  changed bxxx to jxx. removed HOST_MOTOROLA.
01k,26may92,rrr  the tree shuffle
01j,17oct91,yao  added support for CPU32.
01i,04oct91,rrr  passed through the ansification filter
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01h,28aug91,shl   added support for MC68040, cleaned up #if CPU,
		  updated copyright.
01g,17may91,elr   made portable code portable for Motorola SVR4
01f,22jan91,jcf   made portable to the 68000/68010.
01e,01oct90,dab   changed conditional compilation identifier from
		    HOST_SUN to AS_WORKS_WELL.
01d,12sep90,dab   conditionally compiled bfffo instruction as a .word
           +lpf     to make non-SUN hosts happy.
01c,09jul90,jcf   wrote ffsMsb () for 68000/68010.
01b,26jun90,jcf   changed ffs() to ffsMsb() to maintain UNIX compatibility.
01a,17jun89,jcf   written.
*/

/*
DESCRIPTION
This library implements ffsMsb() which returns the most significant bit set. By
taking advantage of the BFFFO instruction of 68020 processors and later, the
implementation determines the first bit set in constant time.  For 68000/68010
ffsMsb() utilizes a lookup table to perform the operation.
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"


#if (defined(PORTABLE))
#define ffsALib_PORTABLE
#endif

#ifndef ffsALib_PORTABLE

	/* exports */

	.globl	_ffsMsb

	.text
	.even


/*******************************************************************************
*
* ffsMsb - find first set bit (searching from the most significant bit)
*
* This routine finds the first bit set in the argument passed it and
* returns the index of that bit.  Bits are numbered starting
* at 1 from the least signifficant bit.  A return value of zero indicates that
* the value passed is zero.
*

* void ffsMsb (i)
*     int i;       /* argument to find first set bit in *

*/

_ffsMsb:
	link	a6,#0

#if (CPU==MC68020) || (CPU==MC68030) || (CPU==MC68040) || \
    (CPU==MC68LC040) || (CPU==MC68060)

	movel	a6@(ARG1),d0		/* d0 gets i */
	jeq 	ffsDone			/* no bit is set */
    /*  bfffo	a6@(ARG1){#0:#0},d0 */   .word 0xedee,0x0000,0x0008
	negb	d0			/* negate offset */
	addb	#32,d0			/* add bits in a int */
#endif	/* (CPU==MC68020) || (CPU==MC68030) || (CPU==MC68040) || \
	 * (CPU==MC68LC040) || (CPU==MC68060) */

#if (CPU==MC68000) || (CPU==MC68010 || CPU==CPU32)
	lea	_ffsMsbTbl,a1		/* lookup table address in a1 */
	clrl	d0			/* clear out d0 */
	movew	a6@(ARG1),d0		/* put msw into d0 */
	jeq 	ffsLsw			/* if zero, try the lsw */
	cmpw	#0x100,d0		/* compare msw to 0x100 */
	jcs 	ffsMswLsb		/* if unsigned less than, then MswLsb */
	lsrw	#8,d0			/* shift Msb to Lsb of d0 */
	moveb	a1@(0,d0:l),d0		/* find first bit set of d0[0:1] */
	addw	#25,d0			/* add bit offset to MswMsb (25) */
	jra 	ffsDone			/* d0 contains bit number */
ffsMswLsb:

	moveb	a1@(0,d0:l),d0		/* find first bit set of d0[0:1] */
	addw	#17,d0			/* add bit offset to MswLsb (17) */
	jra 	ffsDone			/* d0 contains bit number */
ffsLsw:
	moveb	a6@(ARG1+2),d0		/* put LswMsb in d0 */
	jeq 	ffsLswLsb		/* if zero, try LswLsb */
	moveb	a1@(0,d0:l),d0		/* find first bit set of d0[0:1] */
	addw	#9,d0			/* add bit offset to LswMsb (9) */
	jra 	ffsDone			/* d0 contains bit number */
ffsLswLsb:
	movel	a6@(ARG1),d0		/* put LswLsb in d0 */
	jeq 	ffsDone			/* no bit set so return 0 */
	moveb	a1@(0,d0:l),d0		/* find first bit set of d0[0:1] */
	addw	#1,d0			/* add bit offset to LswLsb (1) */

#endif	/* (CPU==MC68000) || (CPU==MC68010 || CPU==CPU32) */

ffsDone:
	unlk	a6
	rts

#endif	/* ! ffsALib_PORTABLE */
