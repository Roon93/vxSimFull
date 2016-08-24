/* semMALib.s - internal VxWorks mutex semaphore assembler library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01l,16may02,pcm  unlock interrupts on error conditions in semMGive ()
01k,29mar02,bwa  Used an unconditional jump to semIntRestrict.
01j,05mar02,bwa  Added check for int context in semMTake() (SPR 73060)
01i,23oct01,bwa  Added VxWorks events support.
01h,06apr98,nps  ported to WV2.0
01g,08jul96,sbs  made windview instrumentation conditionally compiled
01f,10dec93,smb  added instrumentation 
01e,23aug92,jcf  changed bxxx to jxx.  fixed addw a7 to addl a7.
01d,30jul92,rrr  changed _sig_timeout_recalc to _func_sigTimeoutRecalc
01c,27jul92,jcf  took semMTakeKern outline for clarity.
01b,10jul92,rrr  Expanded semMTakeKern inline so a jmp could be put at the end,
                 this is needed for signal restarting.
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

	.globl	_semMGive		/* optimized mutex semaphore give */
	.globl	_semMTake		/* optimized mutex semaphore take */

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
* semMGive - optimized give of a counting semaphore
*

*STATUS semMGive
*    (
*    SEM_ID semId		/@ semaphore id to give @/
*    )

*/

semMIntRestrict:
	jmp	_semIntRestrict			/* let C do the work */

semMRecurse:
	subqw	#1,a0@(SEM_RECURSE)		/* decrement recurse count */
	movew	d1,sr				/* UNLOCK INTERRUPTS */
	rts

_semMGive:					/* a0 = semId! d0 = 0! */
	movel	_taskIdCurrent,a1		/* taskIdCurrent into a1 */
	tstl	_intCnt				/* restrict isr use */
	jne 	semMIntRestrict			/* intCnt > 0 */
	movew	sr,d1				/* old sr into d1 */
	movew	_intLockTaskSR,sr		/* LOCK INTERRUPTS */
        cmpl    #_semClass,a0@          	/* check validity */

#ifdef WV_INSTRUMENTATION

        jeq     objOkMGive

	/* windview - check the validity of instrumented class */
        cmpl    #_semInstClass,a0@              /* check validity */

#endif

	jne 	semIsInvalidUnlock		/* semaphore id error */

objOkMGive:
	cmpl	a0@(SEM_STATE),a1		/* taskIdCurrent is owner? */
	jne 	semIsInvalidUnlock		/* SEM_INVALID_OPERATION */

	tstw	a0@(SEM_RECURSE)		/* if recurse count > 0 */
	jne 	semMRecurse			/* handle recursion */

semMInvCheck:
	btst	#3,a0@(SEM_OPTIONS)		/* SEM_INVERSION_SAFE? */
	jeq 	semMStateSet			/* if not, test semQ */
	subql	#1,a1@(WIND_TCB_MUTEX_CNT)	/* decrement mutex count */
	jne 	semMStateSet			/* if nonzero, test semQ */
	movel	a1@(WIND_TCB_PRIORITY),d0	/* put priority in d0 */
	subl	a1@(WIND_TCB_PRI_NORMAL),d0	/* subtract normal priority */
	jeq 	semMStateSet			/* if same, test semQ */
	moveq	#4,d0				/* or in PRIORITY_RESORT */
semMStateSet:
	movel	a0@(SEM_Q_HEAD),a0@(SEM_STATE)	/* update semaphore state */
	beq 	semMVerifyTaskId		/* no task pending,test taskId*/
	addql	#1,d0				/* set SEM_Q_GET */
semMDelSafe:
	btst	#2,a0@(SEM_OPTIONS)		/* SEM_DELETE_SAFE? */
	jeq 	semMShortCut			/* check for short cut */
	subql	#1,a1@(WIND_TCB_SAFE_CNT)	/* decrement safety count */
	jne 	semMShortCut			/* check for short cut */
	tstl	a1@(WIND_TCB_SAFETY_Q_HEAD)	/* check for pended deleters */
	jeq 	semMShortCut			/* check for short cut */
	addql	#2,d0				/* set SAFETY_Q_FLUSH */
semMShortCut:
	tstl	d0				/* any work for kernel level? */
	jne 	semMKernWork			/* enter kernel if any work */
	movew	d1,sr				/* UNLOCK INTERRUPTS */
	rts					/* d0 is still 0 for OK */
semMKernWork:
	movel	#0x1,_kernelState		/* KERNEL ENTER */
	movew	d1,sr				/* UNLOCK INTERRUPTS */
	movel	d0,_semMGiveKernWork		/* setup work for semMGiveKern*/
	jmp	_semMGiveKern			/* finish semMGive in C */

semMVerifyTaskId:
	cmpil	#0,a0@(SEM_EVENTS_TASKID)	/* task waiting for events ? */
	beq	semMDelSafe			/* no, finish semMGive work */
	addql	#0x8,d0				/* yes, set SEM_M_SEND_EVENTS */
	bra	semMDelSafe			/* finish semMGive work */

/*******************************************************************************
*
* semMTake - optimized take of a mutex semaphore
*

*STATUS semMTake
*    (
*    SEM_ID semId		/@ semaphore id to give @/
*    )

*/

_semMTake:					/* a0 = semId! */
	cmpil	#0,_intCnt			/* in ISR ? */
	jne	semMIntRestrict			/* if so, return */
	movel	_taskIdCurrent,a1		/* taskIdCurrent into a1 */
	movew	sr,d1				/* old sr into d1 */
	movew	_intLockTaskSR,sr		/* LOCK INTERRUPTS */
        cmpl    #_semClass,a0@          	/* check validity */

#ifdef WV_INSTRUMENTATION

        jeq     objOkMTake

	/* windview - check the validity of instrumented class */
        cmpl    #_semInstClass,a0@              /* check validity */
#endif

        jne     semIsInvalidUnlock      	/* invalid semaphore */

objOkMTake:
	movel	a0@(SEM_STATE),d0		/* test for owner */
	jne 	semMEmpty			/* sem is owned, is it ours? */
	movel	a1,a0@(SEM_STATE)		/* we now own semaphore */
	btst	#2,a0@(SEM_OPTIONS)		/* SEM_DELETE_SAFE? */
	jeq 	semMPriCheck			/* semMPriCheck */
	addql	#1,a1@(WIND_TCB_SAFE_CNT)	/* bump safety count */
semMPriCheck:
	btst	#3,a0@(SEM_OPTIONS)		/* SEM_INVERSION_SAFE? */
	jeq 	semMDone			/* if not, skip increment */
	addql	#1,a1@(WIND_TCB_MUTEX_CNT)	/* bump priority mutex count */
semMDone:
	movew	d1,sr				/* UNLOCK INTERRUPTS */
	rts					/* d0 is still 0 for OK */

semMEmpty:
	cmpl	a1,d0				/* recursive take */
	jne 	semMQUnlockPut			/* if not, block */
	addqw	#1,a0@(SEM_RECURSE)		/* increment recurse count */
	movew	d1,sr				/* UNLOCK INTERRUPTS */
	clrl	d0				/* return OK */
	rts

semMQUnlockPut:
	movel	#0x1,_kernelState		/* KERNEL ENTER */
	movew	d1,sr				/* UNLOCK INTERRUPTS */
	movel	a7@(0x8),a7@-			/* push the timeout */
	movel	a0,a7@-				/* push the semId */
        jsr	_semMPendQPut			/* do as much in C as possible*/
        addql	#0x8,a7				/* cleanup */
	tstl	d0				/* test return */
	jne 	semMFail			/* if !OK, exit kernel, ERROR */

semMQPendQOk:
        jsr	_windExit			/* KERNEL EXIT */
	tstl	d0				/* test the return value */
	jgt 	semMRestart			/* is it a RESTART? */
        rts					/* finished OK or TIMEOUT */

semMRestart:
	movel   a7@(0x8),a7@-			/* push the timeout */
        movel   __func_sigTimeoutRecalc,a0	/* address of recalc routine */
        jsr     a0@				/* recalc the timeout */
        addql	#0x4,a7				/* clean up */
        movel   d0,a7@(0x8)			/* and restore timeout */
        jmp     _semTake			/* start the whole thing over */

semMFail:
        jsr	_windExit			/* KERNEL EXIT */
        moveq	#-1,d0				/* return ERROR */
	rts					/* failed */

#endif	/* !PORTABLE */
