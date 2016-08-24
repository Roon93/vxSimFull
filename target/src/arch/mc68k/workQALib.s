/* workQALib.s - internal VxWorks kernel work queue assembler library */

/* Copyright 1984-1994 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01s,06apr98,nps  ported to wv2.0, WindView variant no longer necessary!
01r,08jul96,sbs  made windview instrumentation conditionally compiled
01q,01sep94,rdc  spr 3442 (scratch pad mismanagement).
01p,03feb94,smb  changed interrupt lock to respect intLockLevelSet.
01o,10dec93,smb  added instrumentation
01n,23aug92,jcf  changed bxxx to jxx.  removed HOST_MOTOROLA.
01m,04jul92,jcf  private header files.
01l,26may92,rrr  the tree shuffle
01k,04oct91,rrr  passed through the ansification filter
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01j,17may91,elr   made portable code portable for Motorola's SVR4
01h,22jan91,jcf   made portable to the 68000/68010.
01g,01oct90,dab   changed conditional compilation identifier from
		    HOST_SUN to AS_WORKS_WELL.
01f,12sep90,dab   changed complex addressing mode instructions to .word's
           +lpf     to make non-SUN hosts happy.
01e,01aug90,jcf   added work queue overflow panic.
01d,15jul90,jcf   optimized workQDoWork.  Made callable from work routine.
01c,26jun90,jcf   change interrupt lockout to employ lock out level.
01b,23apr90,jcf   changed name and moved to src/68k.
01a,12sep89,jcf   written.
*/

/*
DESCRIPTION
This module contains internals to the VxWorks kernel.
These routines have been coded in assembler because they are optimized for
performance.
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "private/workQLibP.h"
#include "private/eventP.h"

#ifndef PORTABLE

	/* globals */

	.globl	_workQAdd0		/* add function to workQ */
	.globl	_workQAdd1		/* add function and 1 arg to workQ */
	.globl	_workQAdd2		/* add function and 2 args to workQ */
	.globl	_workQDoWork		/* do all queued work in a workQ */

	.text
	.even

/*******************************************************************************
*
* workQOverflow - work queue has overflowed so call workQPanic ()
*
* NOMANUAL
*/

workQOverflow:					/* leave interrupts locked */
	jsr	_workQPanic			/* panic and never return */

/*******************************************************************************
*
* workQAdd0 - add a function with no argument to the work queue
*
* NOMANUAL

* void workQAdd0
*     (
*     FUNCPTR func,	/@ function to invoke @/
*     )

*/

_workQAdd0:
	clrl	d0
	movew   sr,d1                           /* save old sr */
	movew	_intLockIntSR,sr		/* LOCK INTERRUPTS */
	moveb	_workQWriteIx,d0		/* get write index into d0 */
	addqb	#4,d0				/* advance write index */
	cmpb	_workQReadIx,d0			/* overflow? */
	jeq 	workQOverflow			/* panic if overflowed */
	moveb	d0,_workQWriteIx		/* update write index */
	subqb	#4,d0				/* d0 indexes job */
	movew   d1,sr				/* UNLOCK INTERRUPTS */
	clrl	_workQIsEmpty			/* work queue isn't empty */
	lea	_pJobPool,a0			/* get the start of job pool */
	lsll	#2,d0				/* scale d0 by 4 */
	movel	a7@(0x4),a0@(JOB_FUNCPTR,d0:l)	/* move the function to pool */
	rts					/* we're done */

/*******************************************************************************
*
* workQAdd1 - add a function with one argument to the work queue
*
* NOMANUAL

* void workQAdd1
*     (
*     FUNCPTR func,	/@ function to invoke @/
*     int arg1		/@ parameter one to function @/
*     )

*/

_workQAdd1:
	clrl	d0				/* clear d0 */
	movew   sr,d1                           /* save old sr */
	movew   _intLockIntSR,sr                /* LOCK INTERRUPTS */
	moveb	_workQWriteIx,d0		/* get write index into d0 */
	addqb	#4,d0				/* advance write index */
	cmpb	_workQReadIx,d0			/* overflow? */
	jeq 	workQOverflow			/* panic if overflowed */
	moveb	d0,_workQWriteIx		/* update write index */
	subqb	#4,d0				/* d0 indexes job */
	movew   d1,sr                           /* UNLOCK INTERRUPTS */
	clrl	_workQIsEmpty			/* work queue isn't empty */
	lea	_pJobPool,a0			/* get the start of job pool */
	lsll	#2,d0				/* scale d0 by 4 */
	movel	a7@(0x4),a0@(JOB_FUNCPTR,d0:l)	/* move the function to pool */
	movel	a7@(0x8),a0@(JOB_ARG1,d0:l)	/* move the argument to pool */
	rts					/* we're done */

/*******************************************************************************
*
* workQAdd2 - add a function with two arguments to the work queue
*
* NOMANUAL

* void workQAdd2
*     (
*     FUNCPTR func,	/@ function to invoke @/
*     int arg1,		/@ parameter one to function @/
*     int arg2		/@ parameter two to function @/
*     )

*/

_workQAdd2:
	clrl	d0				/* clear d0 */
	movew   sr,d1                           /* save old sr */
	movew	_intLockIntSR,sr		/* LOCK INTERRUPTS */
	moveb	_workQWriteIx,d0		/* get write index into d0 */
	addqb	#4,d0				/* advance write index */
	cmpb	_workQReadIx,d0			/* overflow? */
	jeq	workQOverflow			/* panic if overflowed */
	moveb	d0,_workQWriteIx		/* update write index */
	subqb	#4,d0				/* d0 indexes job */
	movew   d1,sr                           /* UNLOCK INTERRUPTS */
	clrl	_workQIsEmpty			/* work queue isn't empty */
	lea	_pJobPool,a0			/* get the start of job pool */
	lsll	#2,d0				/* scale d0 by 4 */
	movel	a7@(0x4),a0@(JOB_FUNCPTR,d0:l)	/* move the function to pool */
	movel	a7@(0x8),a0@(JOB_ARG1,d0:l)	/* move the argument to pool */
	movel	a7@(0xc),a0@(JOB_ARG2,d0:l)	/* move the argument to pool */
	rts					/* we're done */

/*******************************************************************************
*
* workQDoWork - perform all the work queued in the kernel work queue
*
* This routine empties all the deferred work in the work queue.  The global
* variable errno is saved restored, so the work will not clobber it.
* The work routines may be C code, and thus clobber the volatile registers
* d0,d1,a0, or a1.  This routine avoids using these registers.
*
* NOMANUAL

* void workQDoWork ()

*/

_workQDoWork:
	movel	_errno,a7@-			/* push _errno */

	moveb	_workQReadIx,d0			/* load read index */
	cmpb	_workQWriteIx,d0		/* if readIndex != writeIndex */
	jeq 	workQNoMoreWork			/* more work to be done */

workQMoreWork:
	addb	#4,_workQReadIx			/* increment readIndex */
	lea	_pJobPool,a0		 	/* base of job pool into a0 */
	andl	#0xff,d0			/* mask noise in upper bits */
	lsll	#2,d0				/* scale d0 by 4 */
	movel	a0@(JOB_ARG2,d0:l),a7@-		/* push arg2 */
	movel	a0@(JOB_ARG1,d0:l),a7@-		/* push arg1 */
	movel	a0@(JOB_FUNCPTR,d0:l),a1	/* load pointer to function */
	jsr	a1@				/* do the work routine */
	addql	#0x8,a7				/* clean up stack */

	movel	#1,_workQIsEmpty		/* set boolean before test! */
	moveb	_workQReadIx,d0			/* load the new read index */
	cmpb	_workQWriteIx,d0		/* if readIndex !=writeIndex */
	jne 	workQMoreWork			/* more work to be done */

workQNoMoreWork:
	movel	a7@+,_errno			/* pop _errno */
	rts					/* return to caller */

#endif	/* !PORTABLE */
