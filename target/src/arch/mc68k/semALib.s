/* semALib.s - internal VxWorks binary semaphore assembler library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
02b,29mar02,bwa  Used an unconditional jump to semIntRestrict.
02a,05mar02,bwa  Added check for int context in semBTake() (SPR 73060)
01z,17dec01,bwa  Changed the locking of interrupts to a read-modify-write in
                 semBGive (SPR #72240).
01y,10oct01,bwa  Added VxWorks events support.
01x,16feb99,cpd  Fix SPR25112: Added test before jeq in semQPut and semQGet
01w,24apr98,nps  bug fixes
01v,30mar98,nps  port to WV2.0.
01u,08jul96,sbs  made windview instrumentation conditionally compiled
01t,11apr94,smb  corrected semaphore recurse logging
01s,24mar94,smb  added instrumentation modifications and optimisations.
01r,10dec93,smb  added instrumentation 
01q,23aug92,jcf  changed bxxx to jxx. changed addw a7 to addl a7.
01p,30jul92,rrr  changed _sig_timeout_recalc to _func_sigTimeoutRecalc
01o,19jul92,pme  added shared memory semaphores support.
01n,04jul92,jcf  split into sem[CM]ALib.s to increase modularity.
01m,26may92,rrr  the tree shuffle
01l,30apr92,rrr  added signal restart
01k,04oct91,rrr  passed through the ansification filter
		  -fixed #else and #endif
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01j,17may91,elr   made portable code portable for Motorola SVR4
01i,22jan91,jcf   made portable to the 68000/68010.
01h,16oct90,jcf   fixed race of priority inversion in semMGive.
01g,01oct90,dab   changed conditional compilation identifier from
		    HOST_SUN to AS_WORKS_WELL.
01f,12sep90,dab   changed complex addressing mode instructions to .word's
           +lpf     to make non-SUN hosts happy.
01e,27jun90,jcf   optimized version once again.
01d,26jun90,jcf   made PORTABLE for the nonce.
01c,10may90,jcf   added semClear optimization.
01b,23apr90,jcf   changed name and moved to src/68k.
01a,02jan90,jcf   written.
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
#include "eventLib.h"
#include "private/eventLibP.h"
#include "private/taskLibP.h"
#include "private/semLibP.h"
#include "private/eventP.h"
#include "private/evtLogLibP.h"

#ifndef PORTABLE

	/* globals */

	.globl	_semGive		/* optimized semGive demultiplexer */
	.globl	_semTake		/* optimized semTake demultiplexer */
	.globl	_semBGive		/* optimized binary semaphore give */
	.globl	_semBTake		/* optimized binary semaphore take */
	.globl	_semQGet		/* semaphore queue get routine */
	.globl	_semQPut		/* semaphore queue put routine */
	.globl	_semOTake		/* optimized old semaphore take */
	.globl	_semClear		/* optimized old semaphore semClear */

	.text
	.even

/*******************************************************************************
*
* semGiveKern - add give routine to work queue
*
*/

semGiveKern:
	jmp	_semGiveDefer		/* let C rtn defer work and rts */

/*******************************************************************************
*
* semGive - give a semaphore
*

*STATUS semGive
*    (
*    SEM_ID semId		/@ semaphore id to give @/
*    )

*/

_semGive:
	movel	a7@(0x4),a0		/* semId goes into a0 */
	btst    #0,a7@(0x7)		/* is it a global semId */
	jne 	semGiveGlobal		/* if semId lsb = 1 its a global sem */

#ifdef WV_INSTRUMENTATION

	/* windview instrumentation - BEGIN
	 * semGive level 1 (object status event )
	 */

        tstl    _evtAction
        jeq     noSemGiveEvt

        cmpl    #_semClass,a0@          /* check validity */
        jeq     objOkGive
        cmpl    #_semInstClass,a0@      /* check validity */
        jne     noSemGiveEvt		/* invalid semaphore */
objOkGive:

	moveml	d0-d3/a0-a2,a7@-	/* save regs */
	movel	a0, a2			/* save semId in a2 for later */

	/* is this semaphore object instrumented? */
	movel	a0@, a1			/* a1 - semId objCore */
	tstl	a1@(SEM_INST_RTN)	/* event routine attached? */
	jeq	semGiveCheckTrg

        /* Check if we need to log this event */

        movel   _wvEvtClass,d2          /* Load event class */

        andl    #WV_CLASS_3_ON,d2       /* Examine these bits */
        cmpl    #WV_CLASS_3_ON,d2
        jne     semGiveCheckTrg           /* Jump if not set */

        /* Log event */

	/* log event for this object */
	movel   #0,a7@-	
	movel   #0,a7@-	
	movel   #0,d0
	movew	a0@(SEM_RECURSE),d0	/* recursively called */
	movel	d0,a7@-	
	movel	a0@(SEM_STATE),a7@-	/* state/count/owner */
	movel   a0,a7@-			/* semId */
	movel	#3,a7@-			/* nParam */
	movel   #EVENT_SEMGIVE,a7@-     /* EVENT_SEMGIVE, event id */
	movel	a1@(SEM_INST_RTN),a0	/* get logging routine */
	jsr	a0@			/* call routine */
	addl    #28,a7			/* restore stack pointer */
        movel   a2,a0			/* restore semId from a2 */

semGiveCheckTrg:
        /* Check if we need to evaluate triggers for this event */

        movel   _trgEvtClass,d2          /* Load event class */

        andl    #TRG_CLASS_3_ON,d2            /* Examine these bits */
        cmpl    #TRG_CLASS_3_ON,d2
        jne     semGiveInstDone             /* Jump if not set */

        /* Evaluate triggers */

        clrl    a7@-                     /* arg5 = NULL */
        clrl    a7@-                     /* arg4 = NULL */
        movew	a0@(SEM_RECURSE),d0
	movel	d0,a7@-			 /* arg3 = recurse */
        movel	a0@(SEM_STATE),a7@-      /* arg2 = state */
	movel	a0,a7@-			 /* arg1 = semId  */
        movel   a0,a7@-                  /* obj = semId */
        movel   #TRG_CLASS3_INDEX,a7@-
        movel   #EVENT_SEMGIVE,a7@-      /* push event ID onto int stack */
        movel   __func_trgCheck,a0       /* Call log fn */
        jsr     a0@
        addl    #32,a7                   /* Pop params */

semGiveInstDone:

	moveml  a7@+,d0-d3/a0-a2         /* restore regs */

noSemGiveEvt:

	/* windview instrumentation - END */
#endif

	movel	_kernelState,d0		/* are we in kernel state? */
	jne 	semGiveKern		/* d0 = 0 if we are not */
	moveb	a0@(SEM_TYPE),d1	/* put the sem class into d1 */
	jne 	semGiveNotBinary	/* optimization for BINARY if d1 == 0 */

		/* BINARY SEMAPHORE OPTIMIZATION */

_semBGive:					/* a0 = semId! d0 = 0! */
		movew	sr,d1			/* old sr into d1 */
		movew	d1,d0
		orw	#0x0700,d0		/* set interrupt mask in SR */
		movew	d0,sr			/* LOCK INTERRUPTS */
		moveq	#0,d0

		cmpl    #_semClass,a0@          /* check validity */

#ifdef WV_INSTRUMENTATION
        	jeq     objOkBGive

		/* windview - check the validity of instrumented class */
        	cmpl    #_semInstClass,a0@      /* instrumented class check */

#endif
        	jne     semIsInvalidUnlock      /* semaphore id error */

objOkBGive:
		movel	d2,-(a7)		/* save d2 */
		movel	a0@(SEM_STATE),d2	/* save old semOwner */
		movel	a0@(SEM_Q_HEAD),a0@(SEM_STATE)
		beq	semBNoPendingTask	/* nothing on pend Q */
		movel	(a7)+,d2		/* restore d2 */
		jmp	_semQGet		/* if not empty, get from q */
semBNoPendingTask:
		cmpil	#0,a0@(SEM_EVENTS_TASKID)/* does a task want events? */
		bne	semBCompareStatus	/* if (taskId != NULL), yes */
semBDontSendEvents:
		movew	d1,sr			/* UNLOCK INTERRUPTS */
		movel	(a7)+,d2		/* restore d2 */
		rts				/* d0 = retStatus */
semBCompareStatus:
		cmpil	#0,d2			/* check for change of state */
		beq	semBDontSendEvents	/* no change,don't send events*/

		/* we want to call eventSend() */
semBSendEvents:
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
		bne	semBEventSendFailed	/* if so, set errno ? */
		btst	#0,a0@(SEM_EVENTS_OPTIONS) /* if not,send events once?*/
		beq	semBGiveWindExit	/* if not, kernel exit */
semBGiveTaskIdClear:				/* if so, clear taskId */
		clrl	a0@(SEM_EVENTS_TASKID)	/* semId->events.taskId = NULL*/
semBGiveWindExit:
		jsr	_windExit		/* KERNEL EXIT */
		movel	(a7)+,_errno		/* put wanted value in errno */
		movel	(a7)+,d0		/* and wanted error code in d0*/
		movel	(a7)+,d2		/* restore d2 */
		rts				/* d0 = retStatus */

semBEventSendFailed:
		btst	#4,a0@(SEM_OPTIONS)	/* want to return error ? */
		beq	semBGiveTaskIdClear	/* no, clear taskId */
		movel	#-1,a7@(4)		/* yes, save ERROR on stack */
		movel	#((134<<16)+0x4),a7@	/* and save errno on stack */
		bra	semBGiveTaskIdClear	/* then clear taskId */

/* end of semBGive */

semGiveNotBinary:

        /* call semGive indirectly via semGiveTbl.  Note that the index could
	 * equal zero after it is masked.  semBGive is the zeroeth element
	 * of the table, but for it to function correctly in the optimized
	 * version above, we must be certain not to clobber a0.  Note, also
	 * that old semaphores will also call semBGive above.
	 */

	andl	#7,d1			/* mask d1 to MAX_SEM_TYPE value */
	lea	_semGiveTbl,a1		/* get table address into a1 */
	lsll	#2,d1			/* scale d1 by sizeof (FUNCPTR) */
	movel	a1@(0,d1:l),a1		/* get right give rtn for this class */
	jmp	a1@			/* invoke give rtn, it will do rts */

semGiveGlobal:
	addl    _smObjPoolMinusOne,a0	/* convert id to local adress */
	movel   a0@(4),d1		/* get semaphore type in d1 */
	andl    #7,d1			/* mask d1 to MAX_SEM_TYPE value */
	lea     _semGiveTbl,a1		/* a1 = semaphore give table */
	lsll	#2,d1			/* scale d1 by sizeof (FUNCPTR) */
	movel   a1@(0,d1:l),a1		/* a1 = appropriate give function */
	movel   a0,a7@-			/* push converted semId */
	jsr     a1@			/* call appropriate give function */
	addql   #4,a7			/* clean up */
	rts

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
* semTake - take a semaphore
*

*STATUS semTake
*    (
*    SEM_ID semId,		/@ semaphore id to take @/
*    ULONG  timeout		/@ timeout in ticks @/
*    )

*/

semBIntRestrict:
	jmp	_semIntRestrict

_semTake:
	movel	a7@(0x4),a0		/* semId goes into a0 */
	btst    #0,a7@(0x7)		/* is it a global semId */
	jne 	semTakeGlobal		/* if semId lsb = 1 its a global sem */

#ifdef WV_INSTRUMENTATION

        /* windview instrumentation - BEGIN
         * semTake level 1 (object status event )
         */
        tstl    _evtAction		/* is level 1 event collection on? */
        jeq     noSemTakeEvt

        cmpl    #_semClass,a0@          /* check validity */
        jeq     objOkTake
        cmpl    #_semInstClass,a0@      /* check validity */
        jne     noSemTakeEvt		/* invalid semaphore */
objOkTake:
	moveml  d0-d3/a0-a2,a7@-	/* save regs */
	movel	a0, a2			/* save semId in a2 for later */

        /* is this semaphore object instrumented? */
        movel   a0@, a1                 /* a1 - semId */
        tstl    a1@(SEM_INST_RTN)      /* event routine attached? */
        jeq     semTakeCheckTrg

	movel   _wvEvtClass,d2		/* load event class */
        andl    #WV_CLASS_3_ON,d2       /* Examine these bits */
        cmpl    #WV_CLASS_3_ON,d2
        jne     semTakeCheckTrg           /* Jump if not set */

        /* log event for this object */
        movel   #0,a7@-
        movel   #0,a7@-
        movel   #0,d0
        movew   a0@(SEM_RECURSE),d0     /* recursively called */
        movel   d0,a7@-
        movel   a0@(SEM_STATE),a7@-     /* state/count/owner */
        movel   a0,a7@-                 /* semId */
	movel	#3,a7@-			/* nParam */
	movel   #EVENT_SEMTAKE,a7@-    	/* EVENT_SEMTAKE, event id */
        movel   a1@(SEM_INST_RTN),a0    /* get logging routine */
        jsr     a0@                     /* call routine */
        addl    #28,a7                  /* restore stack pointer */
        movel   a2,a0               	/* restore semId from a2 */

semTakeCheckTrg:
        /* Check if we need to evaluate triggers for this event */

        movel   _trgEvtClass,d2          /* Load event class */

        andl    #TRG_CLASS_3_ON,d2            /* Examine these bits */
        cmpl    #TRG_CLASS_3_ON,d2
        jne     semTakeInstDone             /* Jump if not set */

        /* Evaluate triggers */

        clrl    a7@-                     /* arg5 = NULL */
        clrl    a7@-                     /* arg4 = NULL */
        movew   a0@(SEM_RECURSE),d0
        movel   d0,a7@-                  /* arg3 = recurse */
        movel   a0@(SEM_STATE),a7@-      /* arg2 = state */
        movel   a0,a7@-                  /* arg1 = semId  */
        movel   a0,a7@-                  /* obj = semId */
        movel   #TRG_CLASS3_INDEX,a7@-
        movel   #EVENT_SEMTAKE,a7@-      /* push event ID onto int stack */
        movel   __func_trgCheck,a0       /* Call log fn */
        jsr     a0@
        addl    #32,a7                   /* Pop params */

semTakeInstDone:

	moveml  a7@+,d0-d3/a0-a2         /* restore regs */

noSemTakeEvt:
        /* windview instrumentation - END */

#endif 

	moveb	a0@(SEM_TYPE),d1	/* get semaphore class into d1 */
	jne 	semTakeNotBinary	/* optimize binary semaphore d1 == 0 */

		/* BINARY SEMAPHORE OPTIMIZATION */
_semBTake:					/* a0 = semId! */
		cmpil	#0,_intCnt		/* in ISR? */
		jne	semBIntRestrict		/* if so, return */
		movew	sr,d1			/* old sr into d1 */
		movew	_intLockTaskSR,sr	/* LOCK INTERRUPTS */
		cmpl    #_semClass,a0@          /* check validity */

#ifdef WV_INSTRUMENTATION

        	jeq     objOkBTake

		/* windview - check the validity of instrumented class */
        	cmpl    #_semInstClass,a0@              /* check validity */

#endif

        	jne     semIsInvalidUnlock              /* semaphore id error */

objOkBTake:
		movel	a0@(SEM_STATE),d0	/* test for owner */
		jne 	_semQPut		/* if sem is owned we block */
		movel	_taskIdCurrent,a0@(SEM_STATE)
		movew	d1,sr			/* UNLOCK INTERRUPTS */
		rts				/* d0 is still 0 for OK */

semTakeNotBinary:
	andl	#7,d1			/* mask d1 to sane value */
	lea	_semTakeTbl,a1		/* get address of take routine table */
	lsll	#2,d1			/* scale d1 by sizeof (FUNCPTR) */
	movel	a1@(0,d1:l),a1		/* get right give rtn for this class */
	jmp	a1@			/* invoke the routine; it will do rts */

semTakeGlobal:
	addl    _smObjPoolMinusOne,a0   /* convert id to local adress */
	movel   a0@(4),d1               /* get semaphore type in d1 */
	andl    #7,d1                   /* mask d1 to MAX_SEM_TYPE value */
	lea     _semTakeTbl,a1          /* a1 = semaphore take table */
	lsll	#2,d1			/* scale d1 by sizeof (FUNCPTR) */
	movel   a1@(0,d1:l),a1          /* a0 = appropriate take function */
	movel   a7@(0x8),a7@-           /* push timeout */
	movel   a0,a7@-                 /* push semId */
	jsr     a1@                     /* call appropriate take function */
	addql   #8,a7                   /* clean up */
	rts

/*******************************************************************************
*
* semQGet - unblock a task from the semaphore queue head
*/

_semQGet:
	movel	#0x1,_kernelState	/* KERNEL ENTER */

#ifdef WV_INSTRUMENTATION

        /* windview instrumentation - BEGIN
         * semGive level 2 (task transition state event )
         */

        tstl    _evtAction
        jeq     noSemQGetEvt

	moveml  d0-d3/a0-a1,a7@-           /* save regs */

        /* Check if we need to log this event */

        movel   _wvEvtClass,d2          /* Load event class */

        andl    #WV_CLASS_2_ON,d2       /* Examine these bits */
        cmpl    #WV_CLASS_2_ON,d2
        jne     semQGCheckTrg           /* Jump if not set */

        /* Log event */


        movel   __func_evtLogM1,a1      /* call event log routine */
        tstl	__func_evtLogM1         /* need to test because we're */
                                        /* moving to an address  */
        jeq     semQGCheckTrg
        movel   a0,a7@-                 /* push semId */
        movel   #EVENT_OBJ_SEMGIVE,a7@- /* EVENT_OBJ_SEMGIVE */
        jsr     a1@
        addql   #8,a7                   /* restore stack pointer */

semQGCheckTrg:

        /* Check if we need to evaluate triggers for this event */

        movel   _trgEvtClass,d2          /* Load event class */

        andl    #TRG_CLASS_2_ON,d2            /* Examine these bits */
        cmpl    #TRG_CLASS_2_ON,d2
        jne     semQGInstDone             /* Jump if not set */

        /* Evaluate triggers */

        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        movel   a0,a7@-                  /* push semId */
        movel   #TRG_CLASS2_INDEX,a7@-
        movel   #EVENT_OBJ_SEMGIVE,a7@-  /* push event ID onto int stack */
        movel   __func_trgCheck,a0       /* Call log fn */
        jsr     a0@
        addl    #32,a7                   /* Pop params */

semQGInstDone:
	moveml  a7@+,d0-d3/a0-a1         /* restore regs */

noSemQGetEvt:
        /* windview instrumentation - END */

#endif

	movew	d1,sr			/* UNLOCK INTERRUPTS */
	pea	a0@(SEM_Q_HEAD)		/* push the pointer to qHead */
	jsr	_windPendQGet		/* unblock someone */
	addql	#4,a7			/* clean up */
	jsr	_windExit		/* KERNEL EXIT */
	rts         			/* windExit sets d0 */

/*******************************************************************************
*
* semQPut - block current task on the semaphore queue head
*/

_semQPut:
	movel	#0x1,_kernelState	/* KERNEL ENTER */

#ifdef WV_INSTRUMENTATION

        /* windview instrumentation - BEGIN
         * semTake level 2 (task transition state event )
         */

        tstl    _evtAction
        jeq     noSemQPutEvt

	moveml  d0-d3/a0-a1,a7@-           /* save regs */

        /* Check if we need to log this event */

        movel   _wvEvtClass,d2          /* Load event class */

        andl    #WV_CLASS_2_ON,d2       /* Examine these bits */
        cmpl    #WV_CLASS_2_ON,d2
        jne     semQPCheckTrg           /* Jump if not set */

        /* Log event */


        movel   __func_evtLogM1,a1      /* call event log routine */
        tstl	__func_evtLogM1         /* need to test because we're */
                                        /* moving to an address  */
        jeq     semQPCheckTrg
        movel   a0,a7@-                 /* push semId */
        movel   #EVENT_OBJ_SEMTAKE,a7@- /* EVENT_OBJ_SEMTAKE */
        jsr     a1@
        addql   #8,a7                   /* restore stack pointer */

semQPCheckTrg:

        /* Check if we need to evaluate triggers for this event */

        movel   _trgEvtClass,d2          /* Load event class */

        andl    #TRG_CLASS_2_ON,d2            /* Examine these bits */
        cmpl    #TRG_CLASS_2_ON,d2
        jne     semQPInstDone             /* Jump if not set */

        /* Evaluate triggers */

        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        movel   a0,a7@-                  /* push semId */
        movel   #TRG_CLASS2_INDEX,a7@-
        movel   #EVENT_OBJ_SEMTAKE,a7@-  /* push event ID onto int stack */
        movel   __func_trgCheck,a0       /* Call log fn */
        jsr     a0@
        addl    #32,a7                   /* Pop params */

semQPInstDone:
	moveml  a7@+,d0-d3/a0-a1         /* restore regs */

noSemQPutEvt:
        /* windview instrumentation - END */

#endif

	movew	d1,sr			/* UNLOCK INTERRUPTS */
	movel	a7@(0x8),a7@-		/* push the timeout */
	pea	a0@(SEM_Q_HEAD)		/* push the &semId->qHead */
	jsr	_windPendQPut		/* block on the semaphore */
	addql	#0x8,a7			/* tidy up */
	tstl	d0			/* if (windPendQPut != OK) */
	jne 	semQPutFail		/* put failed */
	jsr	_windExit		/* else KERNEL EXIT */
	tstl	d0			/* test windExit */
	jgt 	semRestart		/* RESTART */
	rts				/* done */

semQPutFail:
	jsr	_windExit		/* KERNEL EXIT */
	movel	#-1,d0			/* return ERROR */
	rts				/* return to sender */

semRestart:
	movel	a7@(0x8),a7@-		/* push the timeout */
	movel	__func_sigTimeoutRecalc,a0
	jsr	a0@			/* recalc the timeout */
	addql	#0x4,a7			/* tidy up */
	movel	d0,a7@(0x8)		/* and store it */
	jra	_semTake		/* start the whole thing over */

/*******************************************************************************
*
* semOTake - VxWorks 4.x semTake
*
* Optimized version of semOTake.  This inserts the necessary argument of
* WAIT_FOREVER for semBTake.
*/

_semOTake:
	pea	-1			/* push WAIT_FOREVER on stack */
	movel	a7@(0x8),a0		/* put semId in a0 */
	movel	a0,a7@-			/* put semId on stack */
	jsr	_semBTake		/* do semBTake */
	addql	#8,a7			/* cleanup */
	rts

/*******************************************************************************
*
* semClear - VxWorks 4.x semClear
*
* Optimized version of semClear.  This inserts the necessary argument of
* NO_WAIT for semBTake.
*/

_semClear:
	clrl	a7@-			/* push NO_WAIT on stack */
	movel	a7@(0x8),a0		/* put semId in a0 */
	movel	a0,a7@-			/* put semId on stack */
	jsr	_semBTake		/* do semBTake */
	addql	#8,a7			/* cleanup */
	rts

#endif	/* !PORTABLE */
