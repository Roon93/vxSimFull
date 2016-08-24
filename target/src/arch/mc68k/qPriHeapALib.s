/* qPriHeapLib.s - priority heap queue optimizations */

/* Copyright 1984-1992 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01i,23aug92,jcf  changed bxxx to jxx.
01h,26may92,rrr  the tree shuffle
01g,04oct91,rrr  passed through the ansification filter
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01f,22jan91,jcf   made portable to the 68000/68010.
01e,01oct90,dab   changed conditional compilation identifier from
		    HOST_SUN to AS_WORKS_WELL.
01d,12sep90,dab   changed complex addressing mode instructions to .word's
           +lpf     to make non-SUN hosts happy.
01c,06jul90,jcf   made comparisons unsigned.
01b,10may90,jcf   fixed PORTABLE definition.
		  fixed clobbering of d2 by saving on stack.
01a,15jun89,jcf   written.
*/

/*
DESCRIPTION
This module contains internals to the VxWorks kernel.
These routines have been coded in assembler because they have been optimized
for performance.

INTERNAL
The C code versions of these routines can be found in qPriHeapLib.c.
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"

/* optimized version available for 680X0 */

#if (defined(PORTABLE) || (CPU_FAMILY != MC680X0))
#define qPriHeapALib_PORTABLE
#endif

#ifndef qPriHeapALib_PORTABLE

	/* globals */

	.globl	_qPriHeapUp
	.globl	_qPriHeapDown

	.text
	.even

/*******************************************************************************
*
* qPriHeapUp - heap up
*

*void qPriHeapUp (pQPriHeapHead, index)
*    Q_PRI_BMAP_HEAD	*pQPriHeapHead;
*    int		index;


*/

_qPriHeapUp:
	movel	a7@(0x4),a0		/* ARG1 (pQPriHeapHead) goes into a0 */
	movel	a7@(0x8),d0		/* ARG2 (index) goes into d0 */
	movel	a0@(0x4),a0 		/* a0 is pHeapArray XXX !define! XXX */
	lsll	#2,d0			/* scale by 4 */
	movel	a0@(0,d0:l),a1		/* a1 gets initial work node */
	lsrl	#2,d0			/* unscale by 4 */
	movel	a1,a7@-			/* push initial work node on stack */
	movel	d2,a7@-			/* save d2 on stack */
	movel	a1@,d2			/* d2 gets key */
	movel	d0,d1			/* d0 has work index */
	jra 	heapUpWhile
heapUpSwap:
	lsll	#2,d1			/* scale by 4 */
	movel	a0@(0,d1:l),a1		/* move parent node to a1 */
	lsrl	#2,d1			/* unscale by 4 */
	movel	d0,a1@(4)		/* update parent node index w/ work ix*/
	lsll	#2,d0			/* scale by 4 */
	movel	a1,a0@(0,d0:l)		/* move parent node to work index */
	lsrl	#2,d0			/* unscale by 4 */
	movel	d1,d0			/* d0 gets parent ix */
heapUpWhile:
	jeq 	heapUpDone
	subql	#1,d1
	asrl	#1,d1			/* d1 gets parent */
	lsll	#2,d1			/* scale by 4 */
	movel	a0@(0,d1:l),a1
	lsrl	#2,d1			/* unscale by 4 */
	cmpl	a1@,d2
	jcs 	heapUpSwap
heapUpDone:
	movel	a7@+,d2			/* restore d2 from stack */
	movel	a7@+,a1			/* move work node to a1 */
	movel	d0,a1@(4)		/* update work node index w/ work ix*/
	lsll	#2,d0			/* scale by 4 */
	movel	a1,a0@(0,d0:l)		/* move work node to work index */
	lsrl	#2,d0			/* unscale by 4 */
	movel	a7@(0x4),a1		/* ARG1 (pQPriHeapHead) goes into a1 */
	movel	a0@,a1@			/* move highest node to highNode */
	rts				/* done */

/*******************************************************************************
*
* qPriHeapDown - heap down
*

*void qPriHeapDown (pQPriHeapHead, index)
*    Q_PRI_BMAP_HEAD	*pQPriHeapHead;
*    int		index;

*/

_qPriHeapDown:
	movel	a7@(0x4),a0		/* ARG1 (pQPriHeapHead) goes into a0 */
	movel	a7@(0x8),d0		/* ARG2 (index) goes into d0 */
	movel	d2,a7@-			/* save d2 so we can clobber it */
	movel	d3,a7@-			/* save d3 so we can clobber it */
	movel	d4,a7@-			/* save d4 so we can clobber it */
	movel	a0@(0x8),d3 		/* d3 gets heapHead->heapIndex */
	movel	a0@(0x4),a0 		/* a0 is pHeapArray XXX !define! XXX */
	lsll	#2,d0			/* scale by 4 */
	movel	a0@(0,d0:l),a1		/* a1 gets initial work node */
	lsrl	#2,d0			/* unscale by 4 */
	movel	a1@,d2			/* d2 gets key */
	movel	a1,a7@-			/* push initial work node on stack */
	movel	d0,d1			/* d0 has work index */
	asll	#1,d1
	addql	#2,d1			/* d1 has right child */
	jra 	heapDownWhile
heapDownSwap:
	lsll	#2,d1			/* scale by 4 */
	movel	a0@(0,d1:l),a1		/* move child node to a1 */
	lsrl	#2,d1			/* unscale by 4 */
	movel	d0,a1@(4)		/* update child node index w/ work ix */
	lsll	#2,d0			/* scale by 4 */
	movel	a1,a0@(0,d0:l)		/* move child node to work index */
	lsrl	#2,d0			/* unscale by 4 */
	movel	d1,d0			/* d0 has work index */
	asll	#1,d1
	addql	#2,d1			/* d1 has right child */
heapDownWhile:
	cmpl	d3,d1			/* is right child > index */
	jhi 	heapDownDone		/*  then both kids are out of bounds */
	jeq 	heapDownLeft		/* if == then right kid out of bds. */
	lsll	#2,d1			/* scale by 4 */
	movel	a0@(0,d1:l),a1		/* right child into a1 */
	lsrl	#2,d1			/* unscale by 4 */
	movel	a1@,d4			/* right child key into d4 */
	subql	#1,d1			/* left child is one less than right */
	lsll	#2,d1			/* scale by 4 */
	movel	a0@(0,d1:l),a1		/* left child into a1 */
	lsrl	#2,d1			/* unscale by 4 */
	addql	#1,d1			/* d1 should point to right child */
	cmpl	a1@,d4			/* compare left key to right key */
	jcs 	heapDownRight		/* heap down right if right key less */
heapDownLeft:
	subql	#1,d1			/* lesser child is left node */
	lsll	#2,d1			/* scale by 4 */
	movel	a0@(0,d1:l),a1		/* left child node into a1 */
	lsrl	#2,d1			/* unscale by 4 */
	movel	a1@,d4			/* move left child's key to d4 */
heapDownRight:
	cmpl	d4,d2			/* compare child key with key */
	jhi 	heapDownSwap		/* if child is smaller we swap */
heapDownDone:
	movel	a7@+,a1			/* move the work node to a1 */
	movel	d0,a1@(4)		/* update work node index w/ work ix */
	lsll	#2,d0			/* scale by 4 */
	movel	a1,a0@(0,d0:l)		/* move work node to work index */
	lsrl	#2,d0			/* unscale by 4 */
	movel	a7@(0x10),a1		/* ARG1 (pQPriHeapHead) goes into a1 */
	movel	a0@,a1@			/* move highest node to highNode */
	movel	a7@+,d4			/* restore d4 */
	movel	a7@+,d3			/* restore d3 */
	movel	a7@+,d2			/* restore d2 */
	rts				/* done */

#endif	/* !qPriHeapALib_PORTABLE */
