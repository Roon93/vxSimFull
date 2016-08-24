/* semCALib.s - internal VxWorks counting semaphore assembler library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01h,29mar02,bwa  Used an unconditional jump to semIntRestrict.
01g,05mar02,bwa  Added check for int context in semCTake() (SPR 73060)
01f,21dec01,bwa  Changed the locking of interrupts to a read-modify-write in
                 semCGive() (SPR #72240).
01e,06apr98,nps  ported to wv2.0
01d,08jul96,sbs  made windview instrumentation conditionally compiled
01c,10dec93,smb  added instrumentation 
01b,23aug92,jcf  changed bxxx to jxx.
01a,15jun92,jcf  extracted from semALib.s v1m.
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
#include "private/taskLibP.h"
#include "private/semLibP.h"

#ifndef PORTABLE

	/* globals */

	.globl	_semCGive		/* optimized counting semaphore give */
	.globl	_semCTake		/* optimized counting semaphore take */

	.text
	.even

/*******************************************************************************
*
* semIsInvalid - unlock interupts and call semInvalid ().
*/

semIsInvalidUnlock:
	movew	d1,sr			/* UNLOCK INTERRUPTS */
semIsInvalid:
	jmp	_semInvalid		/* let C rtn do work and rts */

/*******************************************************************************
*
* semCGive - optimized give of a counting semaphore
*

*STATUS semCGive
*    (
*    SEM_ID semId		/@ semaphore id to give @/
*    )

*/
_semCGive:				/* a0 = semId! d0 = 0! */
	movew	sr,d1			/* old sr into d1 */
	movew	d1,d0
	orw	#0x0700,d0		/* set interrupt mask in SR */
	movew	d0,sr			/* LOCK INTERRUPTS */
	moveq	#0,d0

        cmpl    #_semClass,a0@          /* check validity */

#ifdef WV_INSTRUMENTATION

        jeq     objOkCGive

	/* windview - check the validity of instrumented class */
        cmpl    #_semInstClass,a0@              /* check validity */
#endif

        jne     semIsInvalidUnlock      /* invalid semaphore */

objOkCGive:
	movel	a0@(SEM_Q_HEAD),d0	/* test semaphore queue head */
	jne 	_semQGet		/* if not empty, get from q */
	addql	#1,a0@(SEM_STATE)	/* increment count */

	cmpil	#0,a0@(SEM_EVENTS_TASKID)/* does a task want events? */
	bne	semCSendEvents		/* if (taskId != NULL), yes */

	movew	d1,sr			/* UNLOCK INTERRUPTS */
	rts				/* d0 = retStatus */

	/* we want to call eventSend() */
semCSendEvents:
	movel	#OK,-(a7)		/* retStatus = OK */
	movel	_errno,-(a7)		/* save old errno */
	movel	#TRUE,_kernelState	/* kernelState = TRUE */
	movew	d1,sr			/* UNLOCK INTERRUPTS */
	movel	a0,-(a7)		/* save a0 */
	movel	a0@(SEM_EVENTS_REGISTERED),-(a7) /* args on stack */
	movel	a0@(SEM_EVENTS_TASKID),-(a7)
	jsr	_eventRsrcSend		/* call fcn,return value in d0*/
	addal	#8,a7			/* cleanup eventSend args */
	moveal	(a7)+,a0		/* restore a0 */
	cmpil	#0,d0			/* eventSend failed ? */
	bne	semCEventSendFailed	/* if so, set errno ? */
	btst	#0,a0@(SEM_EVENTS_OPTIONS) /* if not,send events once?*/
	beq	semCGiveWindExit	/* if not, kernel exit */
semCGiveTaskIdClear:				/* if so, clear taskId */
	clrl	a0@(SEM_EVENTS_TASKID)	/* semId->events.taskId = NULL*/
semCGiveWindExit:
	jsr	_windExit		/* KERNEL EXIT */
	movel	(a7)+,_errno		/* and wanted value in errno */
	movel	(a7)+,d0		/* put wanted error code in d0*/
	rts				/* d0 = retStatus */

semCEventSendFailed:
	btst	#4,a0@(SEM_OPTIONS)	/* want to return error ? */
	beq	semCGiveTaskIdClear	/* no, clear taskId */
	movel	#-1,a7@(4)		/* yes, save ERROR on stack */
	movel	#((134<<16)+0x4),a7@	/* and save errno on stack */
	bra	semCGiveTaskIdClear	/* then clear taskId */

/* end of semCGive */

/*******************************************************************************
*
* semCTake - optimized take of a counting semaphore
*

*STATUS semCTake
*    (
*    SEM_ID semId		/@ semaphore id to take @/
*    )

*/
semCIntRestrict:
	jmp	_semIntRestrict

_semCTake:				/* a0 = semId! */
	cmpil	#0,_intCnt		/* in ISR ? */
	jne	semCIntRestrict		/* if so, return */
	movew	sr,d1			/* old sr into d1 */
	movew	_intLockTaskSR,sr	/* LOCK INTERRUPTS */
        cmpl    #_semClass,a0@          /* check validity */

#ifdef WV_INSTRUMENTATION

        jeq     objOkCTake

	/* windview - check the validity of instrumented class */
        cmpl    #_semInstClass,a0@              /* check validity */

#endif

        jne     semIsInvalidUnlock      /* invalid semaphore */

objOkCTake:
	tstl	a0@(SEM_STATE)		/* test count */
	jeq 	_semQPut		/* if sem is owned we block */
	subql	#1,a0@(SEM_STATE)	/* decrement count */
	movew	d1,sr			/* UNLOCK INTERRUPTS */
	clrl	d0			/* return OK */
	rts

#endif	/* !PORTABLE */
