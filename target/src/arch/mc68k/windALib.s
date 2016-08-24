/* windALib.s - internal VxWorks kernel assembly library */

/* Copyright 1984-1998 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
03a,17jun98,cjtc fix 68060 crash when logging. (SPR 21321)
02z,26mar98,nps  bug fixes for wv2.0 port.
02y,04mar98,nps  completed triggering instrumentation.
02x,04mar98,nps  upgrade to WV2.0, triggering not complete.
02w,08jul96,sbs  made windview instrumentation conditionally compiled
02r,31oct94,tmk  added MC68LC040 support.
02q,31may94,tpr  added MC68060 cpu support.
02v,04mar94,smb  added some kernel optimisations
		 EVENT_WIND_EXIT_NODISPATCH does not log taskIdCurrent
		 introduced EVENT_INT_EXIT_K
		 removed EVENT_WIND_EXIT_IDLENOT
02u,03feb94,smb  changed interrupt locks to respect intLockLevelSet()
02t,28jan94,smb  corrected intCnt check, optimised idle loop
02s,24jan94,smb  added EVENT_WIND_EXIT_DISPATCH_PI and
		 EVENT_WIND_EXIT_NODISPATCH_PI
02r,19jan94,smb  fixed modhist for 02q
		 event buffer overrun fix SPR #2904
02q,19jan94,smb  added instrumentation
02p,23aug92,jcf  changed bxxx to jxx.  removed AS_WORKS_WELL.
02o,04jul92,jcf  private header files.
02n,26may92,rrr  the tree shuffle
02m,04oct91,rrr  passed through the ansification filter
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
02l,25sep91,yao   added support for CPU32.
02k,28aug91,shl   added support for MC68040, cleaned up #if CPU,
		  updated copyright.
02j,24aug91,jcf	  windExitInt empties work queue. fixed elr modhist.
02i,17may91,elr   made portable for Motorola's SVR4
02h,22jan91,jcf   made portable to the 68000/68010.
02g,01oct90,dab   changed conditional compilation identifier from
		    HOST_SUN to AS_WORKS_WELL.
02f,12sep90,dab   changed tstl a<n> to cmpl #0,a<n>. changed complex
           +lpf     addressing modes instructions to .word's to make
		    non-SUN hosts happy.  changed jra to jmp in 680[01]0
		    section of _intExit.
02e,15jul90,jcf   moved emptyWorkQ to workQDoWork () in workQALib.
		  unlocked interrupts for idle in OPTIMIZED windExit ().
02d,27jun90,jcf   unlocked interrupts for idle in PORTABLE windExit ()
02c,26jun90,jcf   fixed PORTABLE version for wind.
		  unlocked interrupts when saving context.
02b,23apr90,jcf   changed name and moved to src/68k.
02a,20aug89,jcf   version 2.0 enhancements.
01g,29mar89,shl   documentation.
01f,29mar89,shl   changed syntax of TCB_AREGS +4, TCB_AREGS +8, TCB_DREGS +4,
		  andTCB_DREGS +8 to TCB_AREGS4, TCB_AREGS8 TCB_DREGS4,
		  and TCB_DREGS8 respectively so to make force host happy
01e,30oct88,jcf   fixed erroneous conditional compile that broke 68000 version.
01d,31aug88,jcf   changed exception frame handling to preserve format/offset
		    word, so floating point works correctly.
		  changed windIntStackSet to modify variable _ispBot.
01c,23aug88,gae   documentation; moved TCB definitions to wind.h.
01b,12aug88,jcf   reworked stack usage and optimized as per code review.
01a,13mar88,jcf   written.
*/

/*
DESCRIPTION
This module contains internals to the VxWorks kernel.
These routines have been coded in assembler because they are either
specific to this processor, or they have been optimized for performance.
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "private/eventP.h"
#include "private/taskLibP.h"
#include "private/semLibP.h"
#include "private/workQLibP.h"

	.globl	_windExit		/* routine to exit mutual exclusion */
	.globl	_vxTaskEntry		/* task entry wrapper */
	.globl	_intEnt			/* interrupt entrance routine */
	.globl	_intExit		/* interrupt exit routine */
	.globl	_windIntStackSet	/* interrupt stack set routine */

#ifdef PORTABLE
	.globl	_windLoadContext	/* needed by portable reschedule () */
#else
	.globl	_reschedule		/* optimized reschedule () routine */
#endif	/* PORTABLE */


	.text
	.even

/*******************************************************************************
*
* windExitInt - exit kernel routine from interrupt level
*
* windExit branches here if exiting kernel routine from int level
* No rescheduling is necessary because the ISR will exit via intExit, and
* intExit does the necessary rescheduling.  Before leaving kernel state
* the work queue is emptied.
*/

windExitIntWork:
	movew	d0,sr				/* UNLOCK INTERRUPTS */
	jsr	_workQDoWork			/* empty the work queue */

windExitInt:
	movew   sr,d0
	movew	_intLockIntSR,sr		/* LOCK INTERRUPTS */
	tstl	_workQIsEmpty			/* test for work to do */
	jeq 	windExitIntWork			/* workQueue is not empty */

#ifdef WV_INSTRUMENTATION

	/* windview instrumentation - BEGIN
	 * enter an interrupt handler. 
	 *
	 * ALL registers must be saved.  
	 */

	tstl	_evtAction		/* intEnt instrumentation */
        jeq     noWindE
	moveml  d0-d3/a0-a1,a7@-           /* save regs */

        movel   #EVENT_WIND_EXIT_NODISPATCH,d1 /* event ID is now in d1 */
        movel   _taskIdCurrent, a0         /* current task */
        movel   a0@(WIND_TCB_PRIORITY),d3  /* ...and priority  */

        /* Check if we need to log this event */

        movel   _wvEvtClass,d2          /* Load event class */

        andl    #WV_CLASS_1_ON,d2            /* Examine these bits */
        cmpl    #WV_CLASS_1_ON,d2
        jne     windECheckTrg          /* Jump if not set */

 
        /* Here we try to determine if the task is running at an
         * inherited priority, if so a different event is generated.
         */

        cmpl    a0@(WIND_TCB_PRI_NORMAL), d3
        jge     windE1Inheritance                 /* no inheritance */
        movel   #EVENT_WIND_EXIT_NODISPATCH_PI, d1 /* replace event ID */

windE1Inheritance:
        /* Log event */

        movel   d3,a7@-                 /* push priority */
        movel   #0,a7@-                 /* push unused param */
        movel   d1,a7@-                 /* push event ID onto int stack */
        movel   __func_evtLogTSched,a1  /* Call log fn */
        jsr     a1@
        addl    #12,a7                  /* Pop params */

windECheckTrg:
        /* Check if we need to evaluate triggers for this event */

        movel   _trgEvtClass,d2          /* Load event class */

        andl    #TRG_CLASS_1_ON,d2            /* Examine these bits */
        cmpl    #TRG_CLASS_1_ON,d2
        jne     windEInstDone           /* Jump if not set */

        movel   _taskIdCurrent, a0         /* current task */
                                           /* priority intact in d3 */

        cmpl    a0@(WIND_TCB_PRI_NORMAL),d3
        jge     windE1InheritanceTrg    /* no inheritance */
        movel   #EVENT_WIND_EXIT_NODISPATCH_PI, d1 /* replace event ID */
windE1InheritanceTrg:

        /* Evaluate triggers */

        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        movel   d3,a7@-                  /* push priority */
        movel   a0,a7@-                  /* push task ID  */
        clrl    a7@-                     /* push 0 / NULL */
        movel   #TRG_CLASS1_INDEX,a7@-
        movel   d1,a7@-                  /* push event ID onto int stack */
        movel   __func_trgCheck,a0       /* Call log fn */
        jsr     a0@
        addl    #32,a7                   /* Pop params */

windEInstDone:
	moveml  a7@+,d0-d3/a0-a1           /* restore regs */
noWindE:
	/* windview instrumentation - END */

#endif

	clrl	_kernelState			/* else release exclusion */
	movew	d0,sr				/* UNLOCK INTERRUPTS */
	clrl	d0				/* return OK */
	rts					/* back to calling task */

/*******************************************************************************
*
* checkTaskReady - check that taskIdCurrent is ready to run
*
* This code branched to by windExit when it finds preemption is disabled.
* It is possible that even though preemption is disabled, a context switch
* must occur.  This situation arrises when a task block during a preemption
* lock.  So this routine checks if taskIdCurrent is ready to run, if not it
* branches to save the context of taskIdCurrent, otherwise it falls thru to
* check the work queue for any pending work.
*/

checkTaskReady:
	tstl	a0@(WIND_TCB_STATUS)	/* is task ready to run */
	jne 	saveTaskContext		/* if no, we blocked with preempt off */

	/* FALL THRU TO CHECK WORK QUEUE */

/*******************************************************************************
*
* checkWorkQ -	check the work queue for any work to do
*
* This code is branched to by windExit.  Currently taskIdCurrent is highest
* priority ready task, but before we can return to it we must check the work
* queue.  If there is work we empty it via doWorkPreSave, otherwise we unlock
* interrupts, clear d0, and return to taskIdCurrent.
*/

checkWorkQ:
	movew	_intLockTaskSR,sr		/* LOCK INTERRUPTS */
	tstl	_workQIsEmpty			/* test for work to do */
	jeq 	doWorkPreSave			/* workQueue is not empty */

#ifdef WV_INSTRUMENTATION

	/* windview instrumentation - BEGIN
	 * enter an interrupt handler. 
	 *
	 * ALL registers must be saved.  
	 */
	tstl	_evtAction		/* intEnt instrumentation */
        jeq     noChkW
	moveml  d0-d3/a0-a1,a7@-           /* save regs */

        movel   #EVENT_WIND_EXIT_NODISPATCH,d1 /* event ID is now in d1 */
        movel   _taskIdCurrent, a0         /* current task */
        movel   a0@(WIND_TCB_PRIORITY),d3  /* ...and priority  */

        /* Check if we need to log this event */

        movel   _wvEvtClass,d2          /* Load event class */

        andl    #WV_CLASS_1_ON,d2            /* Examine these bits */
        cmpl    #WV_CLASS_1_ON,d2
        jne     chkWCheckTrg          /* Jump if not set */

 
        /* Here we try to determine if the task is running at an
         * inherited priority, if so a different event is generated.
         */

        cmpl    a0@(WIND_TCB_PRI_NORMAL), d3
        jge     chkW1Inheritance                 /* no inheritance */
        movel   #EVENT_WIND_EXIT_NODISPATCH_PI, d1 /* replace event ID */

chkW1Inheritance:

        /* Log event */

        movel   d3,a7@-                 /* push priority */
        movel   a0,a7@-                 /* push task ID onto int stack */
        movel   d1,a7@-                 /* push event ID onto int stack */
        movel   __func_evtLogTSched,a1  /* Call log fn */
        jsr     a1@
        addl    #12,a7                   /* Pop params */

chkWCheckTrg:

        /* Check if we need to evaluate triggers for this event */

        movel   _trgEvtClass,d2          /* Load event class */

        andl    #TRG_CLASS_1_ON,d2            /* Examine these bits */
        cmpl    #TRG_CLASS_1_ON,d2
        jne     chkWInstDone           /* Jump if not set */

        movel   _taskIdCurrent, a0         /* current task */
                                           /* priority intact in d3 */

        cmpl    a0@(WIND_TCB_PRI_NORMAL),d3
        jge     chkW1InheritanceTrg    /* no inheritance */
        movel   #EVENT_WIND_EXIT_NODISPATCH_PI, d1 /* replace event ID */
chkW1InheritanceTrg:

        /* Evaluate triggers */

        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        movel   d3,a7@-                  /* push priority */
        movel   a0,a7@-                  /* push task ID  */
        movel   #TRG_CLASS1_INDEX,a7@-
        movel   d1,a7@-                  /* push event ID onto int stack */
        movel   __func_trgCheck,a0       /* Call log fn */
        jsr     a0@
        addl    #32,a7                   /* Pop params */

chkWInstDone:

	moveml  a7@+,d0-d3/a0-a1           /* restore regs */
noChkW:
	/* windview instrumentation - END */

#endif

	clrl	_kernelState			/* else release exclusion */
	movew	#0x3000,sr			/* UNLOCK INTERRUPTS */
	clrl	d0				/* return OK */
	rts					/* back to calling task */

/*******************************************************************************
*
* doWorkPreSave - empty the work queue with current context not saved
*
* We try to empty the work queue here, rather than let reschedule
* perform the work because there is a strong chance that the
* work we do will not preempt the calling task.  If this is the case, then
* saving the entire context just to restore it in reschedule is a waste of
* time.  Once the work has been emptied, the ready queue must be checked to
* see if reschedule must be called, the check of the ready queue is done by
* branching back up to checkTaskCode.
*/

doWorkPreSave:
	movew	#0x3000,sr			/* UNLOCK INTERRUPTS */
	jsr	_workQDoWork			/* empty the work queue */
	jra 	checkTaskSwitch			/* back up to test if tasks */


/******************************************************************************
*
* windExit - task level exit from kernel
*
* Release kernel mutual exclusion (kernelState) and dispatch any new task if
* necessary.  If a higher priority task than the current task has been made
* ready, then we invoke the rescheduler.  Before releasing mutual exclusion,
* the work queue is checked and emptied if necessary.
*
* If rescheduling is necessary, the context of the calling task is saved in its
* associated TCB with the PC pointing at the next instruction after the jsr to
* this routine.  The SSP in the tcb is modified to ignore the return address
* on the stack.  Thus the context saved is as if this routine was never called.
*
* Only the volatile registers d0,d1,a0,a1 are safe to use until the context
* is saved in saveTaskContext.
*
* At the call to reschedule the value of taskIdCurrent must be in a0.
*
* RETURNS: OK or
*	   ERROR if semaphore timeout occurs.
*
* NOMANUAL

* STATUS windExit ()

*/
	
_windExit:
	tstl	_intCnt			/* if intCnt == 0 we're from task */
	jne 	windExitInt		/* else we're exiting interrupt code */

	/* FALL THRU TO CHECK THAT CURRENT TASK IS STILL HIGHEST */

/*******************************************************************************
*
* checkTaskSwitch - check to see if taskIdCurrent is still highest task
*
* We arrive at this code either as the result of falling thru from windExit,
* or if we have finished emptying the work queue.  We compare taskIdCurrent
* with the highest ready task on the ready queue.  If they are same we
* go to a routine to check the work queue.  If they are different and preemption
* is allowed we branch to a routine to make sure that taskIdCurrent is really
* ready (it may have blocked with preemption disabled).  If they are different
* we save the context of taskIdCurrent and fall thru to reschedule.
*/

checkTaskSwitch:
	movel	_taskIdCurrent,a0		/* move taskIdCurrent into a0 */
	cmpl	_readyQHead,a0			/* compare highest ready task */
	jeq 	checkWorkQ			/* if same then time to leave */

	tstl	a0@(WIND_TCB_LOCK_CNT)		/* is task preemption allowed */
	jne 	checkTaskReady			/* if no, check task is ready */

saveTaskContext:
	movel	a7@,a0@(WIND_TCB_PC)		/* save return address as PC */
	movew	#0x3000,a0@(WIND_TCB_SR)	/* save a brand new SR */
	clrw	a0@(WIND_TCB_FOROFF)		/* clear the format/offset */

	moveml	d2-d7,a0@(WIND_TCB_DREGS8)	/* d2-d7; d0,d1 are volatile */
	moveml	a2-a7,a0@(WIND_TCB_AREGS8)	/* a2-a7; a0,a1 are volatile */
	clrl	a0@(WIND_TCB_DREGS)		/* clear saved d0 for return */

	addql	#4,a0@(WIND_TCB_SSP)		/* fix up SP for no ret adrs */

	movel	_errno,a0@(WIND_TCB_ERRNO)	/* save errno */

#ifdef PORTABLE
	jsr	_reschedule			/* goto rescheduler */
#else
	/* FALL THRU TO RESCHEDULE */

/*******************************************************************************
*
* reschedule - rescheduler for VxWorks kernel
*
* This routine is called when either intExit, or windExit, thinks the
* context might change.  All of the contexts of all of the tasks are
* accurately stored in the task control blocks when entering this function.
* The status register is 0x3000. (Supervisor, Master Stack, Interrupts UNLOCKED)
*
* The register a0 must contain the value of _taskIdCurrent at the entrance to
* this routine.
*
* At the conclusion of this routine, taskIdCurrent will equal the highest
* priority task eligible to run, and the kernel work queue will be empty.
* If a context switch to a different task is to occur, then the installed
* switch hooks are called.
*
* NOMANUAL

* void reschedule ()

*/

_reschedule:
	movel	_readyQHead,d0			/* get highest task to d0 */
	jeq 	idle				/* idle if nobody ready */

switchTasks:
	movel	d0,_taskIdCurrent		/* update taskIdCurrent */
	exg	d0,a0				/* a0 gets highest task*/
	movel	d0,a1				/* a1 gets previous task */

	movew	a0@(WIND_TCB_SWAP_IN),d3	/* swap hook mask into d3 */
	orw	a1@(WIND_TCB_SWAP_OUT),d3	/* or in swap out hook mask */
	jne 	doSwapHooks			/* any swap hooks to do */
	tstl	_taskSwitchTable		/* any global switch hooks? */
	jne 	doSwitchHooks			/* any switch hooks to do */

dispatch:
	movel	a0@(WIND_TCB_ERRNO),_errno	/* retore errno */
	movel	a0@(WIND_TCB_SSP),a7		/* push dummy except */

#if (CPU == MC68000)				/* BUILD 68000 EXC FRAME */
	movel	a0@(WIND_TCB_PC),a7@-		/* push new pc and sr */
	movew	a0@(WIND_TCB_SR),a7@-		/* onto stack */
#endif	/* (CPU == MC68000) */

#if (CPU==MC68010 || CPU==MC68020 || CPU==MC68040 || CPU==MC68060 || \
     CPU==MC68LC040 || CPU==CPU32)
	movel	a0@(WIND_TCB_FRAME2),a7@- 	/* push second half of frame */
	movel	a0@(WIND_TCB_FRAME1),a7@- 	/* push first half of frame */
#endif	/* (CPU == MC680[1246]0 || CPU==CPU32) */

	moveml	a0@(WIND_TCB_REGS),d0-d7/a0-a6	/* load register set */

	movew	_intLockTaskSR,sr		/* LOCK INTERRUPTS */
	tstl	_workQIsEmpty			/* if work q is not empty */
	jeq 	doWorkUnlock			/* then unlock and do work */

#ifdef WV_INSTRUMENTATION

	/* windview instrumentation - BEGIN
	 * enter an interrupt handler. 
	 *
	 * ALL registers must be saved.  
	 */

	tstl	_evtAction		/* intEnt instrumentation */
        jeq     noResched

	moveml  d0-d3/a0-a1,a7@-           /* save regs */

        movel   #EVENT_WIND_EXIT_DISPATCH,d1 /* event ID is now in d1 */
        movel   _taskIdCurrent, a0         /* current task */
        movel   a0@(WIND_TCB_PRIORITY),d3  /* ...and priority  */

        /* Check if we need to log this event */

        movel   _wvEvtClass,d2          /* Load event class */

        andl    #WV_CLASS_1_ON,d2            /* Examine these bits */
        cmpl    #WV_CLASS_1_ON,d2
        jne     reschedCheckTrg          /* Jump if not set */

 
        /* Here we try to determine if the task is running at an
         * inherited priority, if so a different event is generated.
         */
        movel   _taskIdCurrent, a0         /* current task */
                                           /* priority intact in d3 */

        cmpl    a0@(WIND_TCB_PRI_NORMAL), d3
        jge     resched1Inheritance                 /* no inheritance */
        movel   #EVENT_WIND_EXIT_DISPATCH_PI, d1 /* replace event ID */
resched1Inheritance:

        /* Log event */

        movel   d3,a7@-                 /* push priority */
        movel   a0,a7@-                 /* push task ID onto int stack */
        movel   d1,a7@-                 /* push event ID onto int stack */
        movel   __func_evtLogTSched,a1      /* Call log fn */
        jsr     a1@
        addl    #12,a7                   /* Pop params */

reschedCheckTrg:

        /* Check if we need to evaluate triggers for this event */

        movel   _trgEvtClass,d2          /* Load event class */

        andl    #TRG_CLASS_1_ON,d2            /* Examine these bits */
        cmpl    #TRG_CLASS_1_ON,d2
        jne     reschedInstDone           /* Jump if not set */

        cmpl    a0@(WIND_TCB_PRI_NORMAL),d3
        jge     resched1InheritanceTrg    /* no inheritance */
        movel   #EVENT_WIND_EXIT_DISPATCH_PI, d1 /* replace event ID */
resched1InheritanceTrg:

        /* Evaluate triggers */

        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        movel   d3,a7@-                  /* push priority */
        movel   a0,a7@-                  /* push task ID  */
        movel   #TRG_CLASS1_INDEX,a7@-
        movel   d1,a7@-                  /* push event ID onto int stack */
        movel   __func_trgCheck,a0       /* Call log fn */
        jsr     a0@
        addl    #32,a7                   /* Pop params */

reschedInstDone:

	moveml  a7@+,d0-d3/a0-a1           /* restore regs */
noResched:
	/* windview instrumentation - END */

#endif

	clrl	_kernelState			/* release kernel mutex */
	rte					/* UNLOCK INTERRUPTS */

/*******************************************************************************
*
* idle - spin here until there is more work to do
*
* When the kernel is idle, we spin here continually checking for work to do.
*/

idle:

#ifdef WV_INSTRUMENTATION

	/* windview instrumentation - BEGIN
	 * enter an interrupt handler. 
	 *
	 * ALL registers must be saved.  
	 */

	tstl	_evtAction		/* intEnt instrumentation */
        jeq     noIdle

	moveml  d0-d3/a0-a1,a7@-           /* save regs */
	movew	sr,a7@-
	movew   _intLockTaskSR,sr	/* LOCK INTERRUPTS */

        /* Check if we need to log this event */

        movel   _wvEvtClass,d2          /* Load event class */

        andl    #WV_CLASS_1_ON,d2            /* Examine these bits */
        cmpl    #WV_CLASS_1_ON,d2
        jne     idleCheckTrg          /* Jump if not set */

        /* Log event */

        movel   #EVENT_WIND_EXIT_IDLE,a7@- /* push event ID onto int stack */
        movel   __func_evtLogT0,a0   /* Call log fn */
        jsr     a0@
        addl    #0x4,a7                   /* Pop params */

idleCheckTrg:

        /* Check if we need to evaluate triggers for this event */

        movel   _trgEvtClass,d2          /* Load event class */

        andl    #TRG_CLASS_1_ON,d2            /* Examine these bits */
        cmpl    #TRG_CLASS_1_ON,d2
        jne     idleInstDone             /* Jump if not set */

        /* Evaluate triggers */

	moveml  a1/a6,a7@-           /* save regs */

        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        movel   #TRG_CLASS1_INDEX,a7@-
        movel   #EVENT_WIND_EXIT_IDLE,a7@- /* push event ID onto int stack */
        movel   __func_trgCheck,a0       /* Call log fn */
        jsr     a0@
        addl    #32,a7                   /* Pop params */

	moveml  a7@+,a1/a6           /* restore regs */

idleInstDone:

	movew   a7@+,sr
	moveml  a7@+,d0-d3/a0-a1            /* restore regs */
noIdle:
	/* windview instrumentation - END */

#endif

	movew	#0x3000,sr			/* UNLOCK INTERRUPTS (in case)*/
	movel	#1,_kernelIsIdle		/* set idle flag for spyLib */
idleLoop:
	tstl	_workQIsEmpty			/* if work queue is empty */
	jne 	idleLoop			/* keep hanging around */
	clrl	_kernelIsIdle			/* unset idle flag for spyLib */
	jra 	doWork				/* go do the work */

/*******************************************************************************
*
* doSwapHooks - execute the tasks' swap hooks
*/

doSwapHooks:
	pea	a0@			/* push pointer to new tcb */
	pea	a1@			/* push pointer to old tcb */
	lea	_taskSwapTable,a5	/* get adrs of task switch rtn list */
	moveq	#-4,d2			/* start index at -1, heh heh */
	jra 	doSwapShift		/* jump into the loop */

doSwapHook:
	movel	a5@(0,d2:l),a1		/* get task switch rtn into a1 */
	jsr	a1@			/* call routine */

doSwapShift:
	addql	#4,d2			/* bump swap table index */
	lslw	#1,d3			/* shift swapMask bit pattern left */
	jcs 	doSwapHook		/* if carry bit set then do ix hook */
	jne 	doSwapShift		/* any bits still set */
					/* no need to clean stack */
	movel	_taskIdCurrent,a0	/* restore a0 with taskIdCurrent */
	tstl	_taskSwitchTable	/* any global switch hooks? */
	jeq 	dispatch		/* if no then dispatch taskIdCurrent */
	jra 	doSwitchFromSwap	/* do switch routines from swap */

/*******************************************************************************
*
* doSwitchHooks - execute the global switch hooks
*/

doSwitchHooks:
	pea	a0@			/* push pointer to new tcb */
	pea	a1@			/* push pointer to old tcb */

doSwitchFromSwap:
	lea	_taskSwitchTable,a5	/* get adrs of task switch rtn list */
	movel	a5@,a1			/* get task switch rtn into a1 */

doSwitchHook:
	jsr	a1@			/* call routine */
	addql	#4,a5			/* bump to next task switch routine */
	movel	a5@,a1			/* get next task switch rtn */
	cmpl	#0,a1			/* check for end of table (NULL) */
	jne 	doSwitchHook		/* loop */
					/* no need to clean stack */
	movel	_taskIdCurrent,a0	/* restore a0 with taskIdCurrent */
	jra	dispatch		/* dispatch task */

/*******************************************************************************
*
* doWork - empty the work queue
* doWorkUnlock - unlock interrupts and empty the work queue
*/

doWorkUnlock:
	movew	#0x3000,sr		/* UNLOCK INTERRUPTS */
doWork:
	jsr	_workQDoWork		/* empty the work queue */
	movel	_taskIdCurrent,a0	/* put taskIdCurrent into a0 */
	movel	_readyQHead,d0		/* get highest task to d0 */
	jeq	idle			/* nobody is ready so spin */
	cmpl	a0,d0			/* compare to last task */
	jeq	dispatch		/* if the same dispatch */
	jra	switchTasks		/* not same, do switch */

#endif	/* !PORTABLE */

#ifdef PORTABLE

/*******************************************************************************
*
* windLoadContext - load the register context from the control block
*
* The registers of the current executing task, (the one reschedule chose),
* are restored from the control block.  Then the appropriate exception frame
* for the architecture being used is constructed.  To unlock interrupts and
* enter the new context we simply use the instruction rte.
*
* NOMANUAL

* void windLoadContext ()

*/

_windLoadContext:
	movel	_taskIdCurrent,a0		/* current tid */
	movel	a0@(WIND_TCB_ERRNO),_errno	/* save errno */
	movel	a0@(WIND_TCB_SSP),a7		/* push dummy except. */

#if (CPU == MC68000)				/* 68000 EXC FRAME */
	movel	a0@(WIND_TCB_PC),a7@-		/* push new pc and sr */
	movew	a0@(WIND_TCB_SR),a7@-		/* onto stack */

#endif	/* (CPU == MC68000) */

#if (CPU==MC68010 || CPU==MC68020 || CPU==MC68040 || CPU==MC68060 || \
     CPU==MC68LC040 || CPU==CPU32)

	movew	a0@(WIND_TCB_FOROFF),a7@- 	/* push format/offset */
	movel	a0@(WIND_TCB_PC),a7@-		/* push new pc and sr */
	movew	a0@(WIND_TCB_SR),a7@-		/* onto stack */
#endif	/* (CPU == MC680[1246]0 || CPU==CPU32) */

	moveml	a0@(WIND_TCB_REGS),d0-d7/a0-a6	/* load register set */
	rte					/* enter task's context. */

#endif	/* PORTABLE */

/*******************************************************************************
*
* intEnt - enter an interrupt service routine
*
* intEnt must be called at the entrance of an interrupt service routine.
* This normally happens automatically, from the stub built by intConnect (2).
* This routine should NEVER be called from C.
*
* SEE ALSO: intConnect(2)

* void intEnt ()

*/

_intEnt:
	movel	a7@,a7@-		/* bump return address up a notch */
	movel	_errno,a7@(0x4)		/* save errno where return adress was */
	addql	#1,_intCnt		/* Bump the counter */

#ifdef WV_INSTRUMENTATION

	/* windview instrumentation - BEGIN
	 * enter an interrupt handler. 
	 *
	 * ALL registers must be saved.  
	 */

	tstl	_evtAction		/* intEnt instrumentation */
        jeq     noIntEnt

	moveml  d0-d2/a0-a6,a7@-        /* save regs */
	movew	sr,a7@-			/* save sr */
	movew   _intLockIntSR,sr	/* LOCK INTERRUPTS */

        /*
         * We're going to need the event ID,
         * so get it now.
         */

        movew   a7@,d1                  /* SR */
        andw    #0x0700,d1              /* mask interrupt level */
        asrw    #8,d1
        addw    #MIN_INT_ID,d1          /* event ID is now in d1 */

        /* Check if we need to log this event */

        movel   _wvEvtClass,d2          /* Load event class */

        andl    #WV_CLASS_1_ON,d2            /* Examine these bits */
        cmpl    #WV_CLASS_1_ON,d2
        jne     intEntCheckTrg          /* Jump if not set */

        /* Log event */

        movel   d1,a7@-                 /* push event ID onto int stack */
        movel   __func_evtLogT0,a0      /* Call log fn */

	jsr     a0@

	addl    #0x4,a7                   /* Pop params */

intEntCheckTrg:

        /* Check if we need to evaluate triggers for this event */

        movel   _trgEvtClass,d2          /* Load event class */

        andl    #TRG_CLASS_1_ON,d2            /* Examine these bits */
        cmpl    #TRG_CLASS_1_ON,d2
        jne     intEntInstDone           /* Jump if not set */

        /* Evaluate triggers */

	/* moveml  a1/a6,a7@-            save regs */

        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        movel   #TRG_CLASS1_INDEX,a7@-
        movel   d1,a7@-                  /* push event ID onto int stack */
        movel   __func_trgCheck,a0       /* Call log fn */
        jsr     a0@
        addl    #32,a7                   /* Pop params */

	/* moveml  a7@+,a1/a6            restore regs */

intEntInstDone:

	movew   a7@+,sr
	moveml  a7@+,d0-d2/a0-a6           /* restore regs */
noIntEnt:
	/* windview instrumentation - END */

#endif

	rts

/*******************************************************************************
*
* intExit - exit an interrupt service routine
*
* Check the kernel ready queue to determine if resheduling is necessary.  If
* no higher priority task has been readied, and no kernel work has been queued,
* then we return to the interrupted task.
*
* If rescheduling is necessary, the context of the interrupted task is saved
* in its associated TCB with the PC, SR and SP retrieved from the exception
* frame on the master stack.
*
* This routine must be branched to when exiting an interrupt service routine.
* This normally happens automatically, from the stub built by intConnect (2).
*
* This routine can NEVER be called from C.
*
* It can only be jumped to because a jsr will push a return address on the
* stack.
*
* SEE ALSO: intConnect(2)

* void intExit ()

* INTERNAL
* This routine must preserve all registers up until the context is saved,
* so any registers that are used to check the queues must first be saved on
* the stack.
*
* At the call to reschedule the value of taskIdCurrent must be in a0.
*/

#if (CPU == MC68020 || CPU == MC68040 || CPU == MC68060 || CPU == MC68LC040)

_intExit:
	movel	a7@+,_errno		/* restore errno */
	movel	a0,a7@-			/* push a0 onto interrupt stack */

#ifdef WV_INSTRUMENTATION
	/* windview instrumentation - BEGIN
	 * enter an interrupt handler. 
	 *
	 * ALL registers must be saved.  
	 */

	tstl	_evtAction		/* intEnt instrumentation */
        jeq     noIntExit

	moveml  d0-d2/a0-a6,a7@-        /* save regs */
	movew	sr,a7@-			/* save sr */
	movew   _intLockIntSR,sr	/* LOCK INTERRUPTS */

        /*
         * We're going to need the event ID and timestamp,
         * so get them now.
         */

        tstl    _workQIsEmpty           /* work in work queue? */
        jne     intExitEvent
        movel   #EVENT_INT_EXIT_K,d1    /* event ID is now in d1 */
        jmp     intExitCont
intExitEvent:
        movel   #EVENT_INT_EXIT,d1      /* event ID is now in d1 */
intExitCont:

        /* Check if we need to log this event */

        movel   _wvEvtClass,d2          /* Load event class */

        andl    #WV_CLASS_1_ON,d2            /* Examine these bits */
        cmpl    #WV_CLASS_1_ON,d2
        jne     intExitCheckTrg         /* Jump if not set */

        /* Log event */

        movel   d1,a7@-                 /* push event ID onto int stack */
        movel   __func_evtLogT0,a0      /* Call log fn */
       
	jsr     a0@

	addl    #0x4,a7                 /* Pop params */

intExitCheckTrg:

        /* Check if we need to evaluate triggers for this event */

        movel   _trgEvtClass,d2          /* Load event class */

        andl    #TRG_CLASS_1_ON,d2            /* Examine these bits */
        cmpl    #TRG_CLASS_1_ON,d2
        jne     intExitInstDone          /* Jump if not set */

        /* Evaluate triggers */

	/* moveml  a1/a6,a7@-           save regs */

        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        movel   #TRG_CLASS1_INDEX,a7@-
        movel   d1,a7@-                  /* push event ID onto int stack */
        movel   __func_trgCheck,a0
        jsr     a0@
        addl    #32,a7                   /* Pop params */

	/* moveml  a7@+,a1/a6            restore regs */

intExitInstDone:

	movew   a7@+,sr
	moveml  a7@+,d0-d2/a0-a6         /* restore regs */
noIntExit:
	/* windview instrumentation - END */

#endif

	subl	#1,_intCnt		/* decrement intCnt */
	tstl	_kernelState		/* if kernelState == TRUE then */
	jne	intRte			/*  just clean up and rte */
	btst	#4,a7@(4)		/* if M bit is clear, then we were in */
	jeq	intRte			/*  in an ISR so just clean up/rte */

	movew	_intLockIntSR,sr	/* LOCK INTERRUPTS */
	movel	_taskIdCurrent,a0	/* put current task in a0 */
	cmpl	_readyQHead,a0	 	/* compare to highest ready task */
	jeq	intRte			/* if same then don't reschedule */

	tstl	a0@(WIND_TCB_LOCK_CNT)	/* is task preemption allowed */
	jeq	saveIntContext		/* if yes, then save context */
	tstl	a0@(WIND_TCB_STATUS)	/* is task ready to run */
	jne	saveIntContext		/* if no, then save context */

intRte:
	movel	a7@+,a0			/* restore a0 */
	rte				/* UNLOCK INTERRUPTS */

/* We are here if we have decided that rescheduling is a distinct possibility.
 * The context must be gathered and stored in the current task's tcb.
 * The stored stack pointers must be modified to clean up the stacks (ISP, MSP).
 */

saveIntContext:
	/* interrupts are still locked out */
	movel	#1,_kernelState			/* kernelState = TRUE; */
	movel	_taskIdCurrent,a0		/* tcb to be fixed up */
	movel	a7@+,a0@(WIND_TCB_AREGS)	/* store a0 in tcb */

#if	(CPU != MC68060)
	movel	_vxIntStackBase,a7		/* fix up isp */
#endif	/* (CPU != MC68060) */

	movew	#0x3000,sr			/* UNLOCK INTS, switch to msp */

	/* interrupts unlocked and using master stack*/
	movew	a7@,a0@(WIND_TCB_SR)		/* save sr in tcb */
	movel	a7@(2),a0@(WIND_TCB_PC)		/* save pc in tcb */
	movew	a7@(6),a0@(WIND_TCB_FOROFF)	/* save format/offset */
	moveml	d0-d7,a0@(WIND_TCB_DREGS)	/* d0-d7 */
	moveml	a1-a7,a0@(WIND_TCB_AREGS4)	/* a1-a7 */
	addl	#8,a0@(WIND_TCB_SSP)		/* adj master stack ptr */
	movel	_errno,a0@(WIND_TCB_ERRNO)	/* save errno */
	jra	_reschedule			/* goto rescheduler */

#endif	/* (CPU==MC68020 || CPU==MC68040 || CPU==MC68060 || CPU==MC68LC040) */

#if (CPU == MC68000 || CPU==MC68010 || CPU==CPU32)

_intExit:
	movel	a7@+,_errno		/* restore errno */
	movel	d0,a7@-			/* push d0 onto interrupt stack */
	movel	a0,a7@-			/* push a0 onto interrupt stack */

#ifdef WV_INSTRUMENTATION

	/* windview instrumentation - BEGIN
	 * enter an interrupt handler. 
	 *
	 * ALL registers must be saved.  
	 */

	tstl	_evtAction		/* intEnt instrumentation */
        jeq     noIntExit

	moveml  d0-d2/a0,a7@-           /* save regs */
	movew	sr,a7@-			/* save sr */
	movew   _intLockIntSR,sr	/* LOCK INTERRUPTS */

        /*
         * We're going to need the event ID and timestamp,
         * so get them now.
         */

        tstl    _workQIsEmpty           /* work in work queue? */
        jne     intExitEvent
        movel   #EVENT_INT_EXIT_K,d1    /* event ID is now in d1 */
        jmp     intExitCont
intExitEvent:
        movel   #EVENT_INT_EXIT,d1      /* event ID is now in d1 */
intExitCont:

        /* Check if we need to log this event */

        movel   _wvEvtClass,d2          /* Load event class */

        andl    #WV_CLASS_1_ON,d2            /* Examine these bits */
        cmpl    #WV_CLASS_1_ON,d2
        jne     intExitCheckTrg         /* Jump if not set */

        /* Log event */

        movel   d1,a7@-                 /* push event ID onto int stack */
        movel   __func_evtLogT0,a0      /* Call log fn */
        jsr     a0@
        addl    #4,a7                   /* Pop params */

intExitCheckTrg:

        /* Check if we need to evaluate triggers for this event */

        movel   _trgEvtClass,d2          /* Load event class */

        andl    #TRG_CLASS_1_ON,d2            /* Examine these bits */
        cmpl    #TRG_CLASS_1_ON,d2
        jne     intExitInstDone          /* Jump if not set */

        /* Evaluate triggers */

	moveml  a1/a6,a7@-           /* save regs */

        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        clrl    a7@-                     /* push 0 / NULL */
        movel   #TRG_CLASS1_INDEX,a7@-
        movel   d1,a7@-                  /* push event ID onto int stack */
        movel   __func_trgCheck,a0
        jsr     a0@
        addl    #32,a7                   /* Pop params */

	moveml  a7@+,a1/a6           /* restore regs */

intExitInstDone:

	movew   a7@+,sr
	moveml  a7@+,d0-d2/a0           /* restore regs */
noIntExit:
	/* windview instrumentation - END */
#endif

	subl	#1,_intCnt		/* decrement intCnt */
	tstl	_kernelState		/* if kernelState == TRUE */
	jne	intRte			/*  then just clean up and rte */

	movew	a7@(8),d0		/* get the sr off the stack into d0 */
	andw	#0x0700,d0		/* if the interrupt level > 0 */
	jne	intRte			/*  then came from ISR...clean up/rte */

	movew	_intLockIntSR,sr	/* LOCK INTERRUPTS */
	movel	_taskIdCurrent,a0	/* put current task into a0 */
	cmpl	_readyQHead,a0		/* compare to highest ready task */
	jeq	intRte			/* if same then don't reschedule */

	tstl	a0@(WIND_TCB_LOCK_CNT)	/* is task preemption allowed */
	jeq	saveIntContext		/* if yes, then save context */
	tstl	a0@(WIND_TCB_STATUS)	/* is task ready to run */
	jne	saveIntContext		/* if no, then save context */

intRte:
	movel	a7@+,a0		/* restore a0 */
	movel	a7@+,d0		/* restore d0 */
	rte			/* UNLOCKS INTERRUPT and return to interupted */

/* We come here if we have decided that rescheduling is a distinct possibility.
 * The context must be gathered and stored in the current task's tcb.
 * The stored stack pointer must be modified to clean up the SSP.
 */

saveIntContext:
	/* interrupts are still locked out */
	movel	#1,_kernelState			/* kernelState = TRUE; */
	movel	_taskIdCurrent,a0		/* tcb to be fixed up */
	movel	a7@+,a0@(WIND_TCB_AREGS)	/* store a0 in tcb */
	movel	a7@+,a0@(WIND_TCB_DREGS)	/* store d0 in tcb */
	movew	#0x3000,sr			/* unlock int., switch to msp */

	/* interrupts unlocked and using master stack*/
	movew	a7@,a0@(WIND_TCB_SR)		/* save sr in tcb */
	movel	a7@(2),a0@(WIND_TCB_PC)		/* save pc in tcb */
	moveml	d1-d7,a0@(WIND_TCB_DREGS4)	/* d1-d7 */
	moveml	a1-a7,a0@(WIND_TCB_AREGS4)	/* a1-a7 */

#if (CPU == MC68000)
	addl    #6,a0@(WIND_TCB_SSP)            /* adj master stack ptr */
#endif	/* (CPU == MC68000) */

#if (CPU == MC68010 || CPU==CPU32)
	 movew   a7@(6),a0@(WIND_TCB_FOROFF)     /* save format/offset */
	 addl    #8,a0@(WIND_TCB_SSP)            /* adj master stack ptr */
#endif	/* (CPU==MC68010 || CPU==CPU32) */

	movel	_errno,a0@(WIND_TCB_ERRNO)	/* save errno */

	jmp	_reschedule			/* goto rescheduler */

#endif	/* (CPU==MC68000 || CPU==MC68010 || CPU==CPU32) */

/*******************************************************************************
*
* vxTaskEntry - task startup code following spawn
*
* This hunk of code is the initial entry point to every task created via
* the "spawn" routines.  taskCreate(2) has put the true entry point of the
* task into the tcb extension before creating the task,
* and then pushed exactly ten arguments (although the task may use
* fewer) onto the stack.  This code picks up the real entry point and calls it.
* Upon return, the 10 task args are popped, and the result of the main
* routine is passed to "exit" which terminates the task.
* This way of doing things has several purposes.  First a task is easily
* "restartable" via the routine taskRestart(2) since the real
* entry point is available in the tcb extension.  Second, the call to the main
* routine is a normal call including the usual stack clean-up afterwards,
* which means that debugging stack trace facilities will handle the call of
* the main routine properly.
*
* NOMANUAL

* void vxTaskEntry ()

*/

_vxTaskEntry:
	movel	#0,a6			/* make sure frame pointer is 0 */
	movel	_taskIdCurrent,a0 	/* get current task id */
	movel	a0@(WIND_TCB_ENTRY),a0	/* entry point for task is in tcb */
	jsr	a0@			/* call main routine */
	addl	#40,a7			/* pop args to main routine */
	movel	d0,a7@-			/* pass result to exit */
	jsr	_exit			/* gone for good */

/*******************************************************************************
*
* windIntStackSet - set the interrupt stack pointer
*
* This routine sets the interrupt stack pointer to the specified address.
* It is only valid on architectures with an interrupt stack pointer.
*
* NOMANUAL

* void windIntStackSet
*     (
*     char *pBotStack	/@ pointer to bottom of interrupt stack @/
*     )

*/

_windIntStackSet:
	link	a6,#0
#if (CPU == MC68020 || CPU==MC68040 || CPU==MC68LC040)
	movel	a6@(ARG1),d1		/* get new botOfIsp */
	movew	sr,d0			/* save sr */
	movew	_intLockIntSR,sr	/* LOCK INTERRUPTS */
	movel	d1,a7			/* set stack */
	movew	d0,sr			/* UNLOCK INTERRUPTS */
	movel	d1,_vxIntStackBase	/* update the global variable */
#endif	/* (CPU == MC68020  || CPU == MC68040 || CPU==MC68LC040) */
	unlk	a6
	rts


