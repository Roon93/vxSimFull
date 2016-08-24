/* excArchLib.c - mc680x0 exception handling facilities */

/* Copyright 1984-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
03n,10aug98,pr   removed ifdef WV_INSTRUMENTATION comment for header file
03m,16dec96,map  skip vector 59 on MC68000 for EN302 [SPR# 7552]
03l,12jul96,jmb  Installed Thierry's patch for TAS on MC68060 (SPR #4552)
03k,08jul96,sbs  made windview instrumentation conditionally compiled
03j,21jun96,ms   folded in patch for SPR 2650 (excTasRetry wrong for 680[0|1]0)
03i,23may96,ms   fixed SPR 6240 (reseting pTaskLastFpTcb busted nested exc's).
03h,23jun95,ms	 excExcHandle saves fpp in pTaskLastFpTcb, not taskIdCurrent
03g,30may94,tpr  added MC68060 cpu support.
03i,10nov94,dvs  changed fppLastTcb to pTaskLastFpTcb to deal with merge of
		 SPR 3033 with SPR 1060
03h,26oct94,tmk  added MC68LC040 support
03g,14oct94,ism  merged customer support patch for SPR#1060.
03f,23oct93,jcf  eliminated MC68882 coprocessor violations.
03h,28jan94,smb  added instrumentation for excIntHandle
03h,24jan94,smb  added instrumentation macros
03g,10dec93,smb  added instrumentation support for windview
03f,26aug93,jcf  re-added CPU32 support.
03e,19oct92,jcf  swapped 5.0/excInfoShow hook order.
03d,01oct92,jcf  added reboot if exception before kernel.
03c,31aug92,rrr  changed code passed to sigExcKill to be vector number.
03b,23aug92,jcf  changed name.  added format $4 for 68EC040 handler.
03a,02aug92,jcf  overhauled. excJobAdd avoided, reworked hooks.
02y,30jul92,rrr  backed out 1g (now back to 1f)
02x,30jul92,kdl  backed out 01f changes pending rest of exc handling.
02w,29jul92,rrr  pass one of exc of split
02v,09jul92,rrr  changed xsignal.h to private/sigLibP.h
02u,26may92,rrr  the tree shuffle
02t,30apr92,rrr  some preparation for posix signals.
02s,21apr92,rrr  fixed number of args to printExc for 68000 version
02r,27jan92,yao  removed architecture independent routines excInit(), excTask(),
		 excHookAdd(), excJobAdd(), printExc(), excDeliverSignal().
		 added excFaultTab [].
02q,20jan92,jcf  use vxMemProbe() to handle writebacks in case of berr.
		 rewrote handling of ESF 7 of 68040.
02p,24oct91,rfs  fixed spelling error in exception message
02o,17oct91,yao  minor documentation cleanup.
02n,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed READ, WRITE and UPDATE to O_RDONLY O_WRONLY O_RDWR
		  -changed VOID to void
		  -changed copyright notice
02m,25sep91,yao  added support for CPU32.
02l,28aup91,shl  added handling of ESF3 and ESF7 of MC68040.
02k,30apr91,jdi	 documentation tweaks.
02j,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
02i,04feb91,jaa	 documentation cleanup.

Deleted pre 91 modification history -- See RCS
*/

/*
This module contains MC680X0 architecture dependent portions of the
exception handling facilities.  See excLib for the portions that are
architecture independent.

SEE ALSO: dbgLib, sigLib, intLib, "Debugging"
*/

#include "vxWorks.h"
#include "esf.h"
#include "iv.h"
#include "sysLib.h"
#include "intLib.h"
#include "taskLib.h"
#include "qLib.h"
#include "errno.h"
#include "string.h"
#include "fppLib.h"
#include "vxLib.h"
#include "logLib.h"
#include "rebootLib.h"
#include "private/funcBindP.h"
#include "private/kernelLibP.h"
#include "private/taskLibP.h"
#include "private/sigLibP.h"
#include "private/eventP.h"

#if (CPU==MC68060)
#include "math/isp060Lib.h"
#endif /* (CPU==MC68060 */
/* local variables */

LOCAL int excTasErrors;		/* count of TAS bus errors - just curiosity */


/* forward static functions */

LOCAL BOOL	excTasRetry (int vecNum, ESF0 *pEsf, REG_SET *pRegs);
LOCAL void	excGetInfoFromESF (int vecNum, ESF0 *pEsf, REG_SET *pRegs,
				   EXC_INFO *pExcInfo);


/*******************************************************************************
*
* excVecInit - initialize the exception/interrupt vectors
*
* This routine sets all exception vectors to point to the appropriate
* default exception handlers.  These handlers will safely trap and report
* exceptions caused by program errors or unexpected hardware interrupts.
* All vectors from vector 2 (address 0x0008) to 255 (address 0x03fc) are
* initialized.  Vectors 0 and 1 contain the reset stack pointer and program
* counter.
*
* WHEN TO CALL
* This routine is usually called from the system start-up routine
* usrInit() in usrConfig, before interrupts are enabled.
*
* RETURNS: OK (always).
*/

STATUS excVecInit (void)
    {
    FAST int vecNum;

    /* make exception vectors point to proper place in bsr table */

#if (CPU==MC68000)

    for (vecNum = LOW_VEC; vecNum < 59; ++vecNum)
	intVecSet (INUM_TO_IVEC (vecNum), (FUNCPTR) &excBsrTbl[vecNum]);

    /* skip vectors 59 - 63 so we don't clobber MC68[EN]302 internal regs */

    for (vecNum = 64; vecNum <= HIGH_VEC; ++vecNum)
	intVecSet (INUM_TO_IVEC (vecNum), (FUNCPTR) &excBsrTbl[vecNum]);

#else	/* (CPU==MC680[12346]0 || CPU==CPU32) */

    for (vecNum = LOW_VEC; vecNum <= HIGH_VEC; ++vecNum)
	{
	if (((BUS_ERROR_VEC <= vecNum) && (vecNum <= FORMAT_ERROR)) ||
	    ((TRAP_0_VEC <= vecNum ) && (vecNum < USER_VEC_START)))
	    intVecSet ((FUNCPTR *)INUM_TO_IVEC (vecNum), (FUNCPTR)excStub);
	else
	    intVecSet ((FUNCPTR *)INUM_TO_IVEC (vecNum), (FUNCPTR)excIntStub);
	}

#if (CPU==MC68060)
    /*
     * initialize the call out table used by the unimplemented integer
     * instruction library
     */
    isp060COTblInit();

    /* connect the umimplemented integer instruction exception handler */
    intVecSet ((FUNCPTR *) IV_UNIMP_INTEGER_INSTRUCTION,
		(FUNCPTR) (ISP_060_UNIMP));
#endif /* (CPU==MC68060) */

#endif	/* (CPU==MC680[12346]0 || CPU==CPU32) */

    return (OK);
    }
/*******************************************************************************
*
* excExcHandle - interrupt level handling of exceptions
*
* This routine handles exception traps.  It is never to be called except
* from the special assembly language interrupt stub routine.
*
* It prints out a bunch of pertinent information about the trap that
* occurred via excTask.
*
* Note that this routine runs in the context of the task that got the exception.
*
* NOMANUAL
*/

void excExcHandle
    (
    int		vecNum,	/* exception vector number */
    ESF0 *	pEsf,	/* pointer to exception stack frame */
    REG_SET *	pRegs	/* pointer to register info on stack */
    )
    {
    EXC_INFO excInfo;

    /*
     * save the floating point state here to avoid doing it in
     * the swap hook. The MC68882 requires an FSAVE to be executed
     * early in an exception handler.
     */

    if (pTaskLastFpTcb != NULL)
	fppSave (pTaskLastFpTcb->pFpContext);

    excGetInfoFromESF (vecNum, pEsf, pRegs, &excInfo);	/* fill excInfo/pRegs */

    if ((_func_excBaseHook != NULL) && 			/* user hook around? */
	((* _func_excBaseHook) (vecNum, pEsf, pRegs, &excInfo)))
	return;						/* user hook fixed it */

    if (excTasRetry (vecNum, pEsf, pRegs))
	return;						/* retry the TAS */
#ifdef WV_INSTRUMENTATION

    /* windview - level 3 event logging */
    EVT_CTX_1(EVENT_EXCEPTION, vecNum);

#endif

    /* if exception occured in an isr or before multi tasking then reboot */

    if ((INT_CONTEXT ()) || (Q_FIRST (&activeQHead) == NULL))
	{
	if (_func_excPanicHook != NULL)			/* panic hook? */
	    (*_func_excPanicHook) (vecNum, pEsf, pRegs, &excInfo);

	reboot (BOOT_WARM_AUTOBOOT);
	return;						/* reboot returns?! */
	}

    /* task caused exception */

    taskIdCurrent->pExcRegSet = pRegs;			/* for taskRegs[GS]et */

    taskIdDefault ((int)taskIdCurrent);			/* update default tid */

    bcopy ((char *) &excInfo, (char *) &(taskIdCurrent->excInfo),
	   sizeof (EXC_INFO));				/* copy in exc info */

    if (_func_sigExcKill != NULL)			/* signals installed? */
	(*_func_sigExcKill) (vecNum, INUM_TO_IVEC(vecNum), pRegs);

    if (_func_excInfoShow != NULL)			/* default show rtn? */
	(*_func_excInfoShow) (&excInfo, TRUE);

    if (excExcepHook != NULL)				/* 5.0.2 hook? */
        (* excExcepHook) (taskIdCurrent, vecNum, pEsf);

    taskSuspend (0);					/* whoa partner... */

    taskIdCurrent->pExcRegSet = (REG_SET *) NULL;	/* invalid after rts */
    }
/*******************************************************************************
*
* excIntHandle - interrupt level handling of interrupts
*
* This routine handles interrupts.  It is never to be called except
* from the special assembly language interrupt stub routine.
*
* It prints out a bunch of pertinent information about the trap that
* occurred via excTask().
*
* NOMANUAL
*/

void excIntHandle
    (
    int		vecNum,	/* exception vector number */
    ESF0 *	pEsf,	/* pointer to exception stack frame */
    REG_SET *	pRegs	/* pointer to register info on stack */
    )
    {
    EXC_INFO excInfo;

    excGetInfoFromESF (vecNum, pEsf, pRegs, &excInfo);	/* fill excInfo/pRegs */
   
#ifdef WV_INSTRUMENTATION

    /* windview - level 3 event logging */
	EVT_CTX_1(EVENT_EXCEPTION, vecNum);

#endif

    if (_func_excIntHook != NULL)
	(*_func_excIntHook) (vecNum, pEsf, pRegs, &excInfo);

    if (Q_FIRST (&activeQHead) == NULL)			/* pre kernel */
	reboot (BOOT_WARM_AUTOBOOT);			/* better reboot */
    }
/*****************************************************************************
*
* excGetInfoFromESF - get relevent info from exception stack frame
*
*/

LOCAL void excGetInfoFromESF
    (
    FAST int 	vecNum,		/* vector number */
    FAST ESF0 *	pEsf,		/* pointer to exception stack frame */
    REG_SET *	pRegs,		/* pointer to register info on stack */
    EXC_INFO *	pExcInfo	/* where to fill in exception info */
    )
    {
    int size;

    pExcInfo->vecNum  = vecNum;
    pExcInfo->valid   = EXC_VEC_NUM;

#if (CPU==MC68000)
    if ((vecNum == BUS_ERROR_VEC) || (vecNum == ADRS_ERROR_VEC))
	{
	/* Its an address or bus error */

	pExcInfo->valid     |= EXC_PC | EXC_STATUS_REG | EXC_ACCESS_ADDR |
			       EXC_FUNC_CODE | EXC_INSTR_REG;
	pExcInfo->pc         = ((ESF68K_BA *)pEsf)->pc;
	pExcInfo->statusReg  = ((ESF68K_BA *)pEsf)->sr;
	pExcInfo->accessAddr = ((ESF68K_BA *)pEsf)->aa;
	pExcInfo->funcCode   = ((ESF68K_BA *)pEsf)->fn;
	pExcInfo->instrReg   = ((ESF68K_BA *)pEsf)->ir;

	size = sizeof (ESF68K_BA);
	}
    else
	{
	pExcInfo->valid    |= EXC_PC | EXC_STATUS_REG;
	pExcInfo->pc        = ((ESF68K *)pEsf)->pc;
	pExcInfo->statusReg = ((ESF68K *)pEsf)->sr;

	size = sizeof (ESF68K);
	}
#else	/* (CPU==MC680[12346]0 || CPU==CPU32) */

    /* switch on ESF type */

    switch ((pEsf->vectorOffset & 0xf000) >> 12)
	{
	case 0:
	case 1:
	    pExcInfo->valid    |= EXC_PC | EXC_STATUS_REG;
	    pExcInfo->pc        = pEsf->pc;
	    pExcInfo->statusReg = pEsf->sr;

	    size = (sizeof (ESF0));
	    break;

	case 2:
	    pExcInfo->valid     |= EXC_PC | EXC_STATUS_REG | EXC_ACCESS_ADDR;
	    pExcInfo->pc         = ((ESF2 *)pEsf)->pc;
	    pExcInfo->statusReg  = ((ESF2 *)pEsf)->sr;
	    pExcInfo->accessAddr = ((ESF2 *)pEsf)->aa;

	    size = (sizeof (ESF2));
	    break;

#if ((CPU == MC68040) || (CPU == MC68LC040) || (CPU == MC68060))
	case 3:
	    pExcInfo->valid	|= EXC_PC | EXC_STATUS_REG | EXC_ACCESS_ADDR;
	    pExcInfo->pc	 = ((ESF3 *)pEsf)->pc;
	    pExcInfo->statusReg	 = ((ESF3 *)pEsf)->sr;
	    pExcInfo->accessAddr = ((ESF3 *)pEsf)->effectiveAddr;

	    size =  sizeof (ESF3);
	    break;
#endif /* (CPU == MC68040) || (CPU == MC68LC040) || (CPU == MC68060)) */

#if (CPU == MC68040 || CPU == MC68LC040)
	case 4:
	    pExcInfo->valid	|= EXC_PC | EXC_STATUS_REG | EXC_ACCESS_ADDR;
	    pExcInfo->pc	 = ((ESF3 *)pEsf)->pc;
	    pExcInfo->statusReg	 = ((ESF3 *)pEsf)->sr;
	    pExcInfo->accessAddr = ((ESF3 *)pEsf)->effectiveAddr;

	    size =  sizeof (ESF3) + 2;
	    break;
#endif /* (CPU == MC68040) || (CPU == MC68LC040) */

#if (CPU == MC68060)
	case 4:
	    pExcInfo->valid	|= EXC_PC | EXC_STATUS_REG | EXC_ACCESS_ADDR |
				   EXC_FSLW;
	    pExcInfo->pc	 = ((ESF4 *)pEsf)->pc;
	    pExcInfo->statusReg	 = ((ESF4 *)pEsf)->sr;
	    pExcInfo->accessAddr = ((ESF4 *)pEsf)->effectiveAddr;
	    pExcInfo->funcCode	 = ((ESF4 *)pEsf)->fslw;

	    size =  sizeof (ESF4);
	    break;
#endif  /* (CPU == MC68060) */

#if (CPU == MC68040 || CPU == MC68LC040)
	case 7:
	    {
	    FAST ESF7 *pEsf7 = (ESF7 *)pEsf;
	    static int sizeTbl[4] = {4, 1, 2, 0}; /* bytes per ssw:siz field */

            pExcInfo->valid     |= EXC_PC | EXC_STATUS_REG | EXC_ACCESS_ADDR |
                                   EXC_FUNC_CODE;
            pExcInfo->pc         = pEsf7->pc;
            pExcInfo->statusReg  = pEsf7->sr;
            pExcInfo->funcCode   = pEsf7->fn;

	    if (pEsf7->fn & 0xf000)		/* if any continuation flags */
		pExcInfo->accessAddr = pEsf7->effectiveAddr;
	    else				/* else, get original fault */
		pExcInfo->accessAddr = pEsf7->aa;

	    /* In traditional operating systems, the code here would correct
	     * the fault by paging in the appropriate page and completing the
	     * cycle manually.  VxWorks currently runs in a linear address
	     * space fully consistent with physical memory.  Legitimate bus
	     * errors take place when accesses to empty regions of the address
	     * space occur.  Even when an MMU is employed to protect memory
	     * regions, unwarranted accesses should not be corrected and
	     * completed but rather undergo the standard VxWorks exception
	     * processing.
	     *
	     * So we make no attempt to complete or repair the offending cycle.
	     * With regard to an instruction ATC fault, performing an rte will
	     * retry the instruction.  A data access fault, however, must be
	     * completed here manually if the access is desired because an rte
	     * will not retry the cycle.  Future revisions of VxWorks may
	     * require the completion of faulted cycles, but for now these data
	     * accesses are lost.  Note that completion of the data access
	     * must occur before the writebacks are handled.
	     *
	     * With copyback caching mode, the processor may have pending
	     * writebacks to complete.  These writebacks could have nothing to
	     * do with the offending cycle, but must be completed to
	     * maintain consistancy of memory.  An instruction access error
	     * will always allow pending accesses to complete before reporting
	     * the instruction fault.  Therefore, no pending writebacks will
	     * be contained in the stack frame.  Data accesses, on the other
	     * hand, require completion of the writebacks.
	     *
	     * Many possible types data accesses exist; a normal user or
	     * supervisor access, a move16 operation, a cache push operation,
	     * or an MMU tablewalk access.  Each of these faults set the
	     * SSW-TT and TM bits for identification.  Completion of the
	     * cycles differ slightly for each type, but as stated we do not
	     * wish to complete these cycles.  The writebacks are completed
	     * for writeback2 and writeback3.  Writeback1 is either associated
	     * with the faulted cycle or invalid as is the case for move16 and
	     * cache push faults.
	     *
	     * With regard to a cache push fault, be aware that the push buffer
	     * is invalidated after the fault and memory coherency is lost
	     * because the only valid copy of the cache line now resides
	     * in the exception frame which cannot be snooped.  Pages should
	     * pushed to memory before invalidating.
	     */

	    /* check if access is inst. fault and writebacks are necessary */

	    if (!(((pEsf7->fn & 0x0007) == 6) || ((pEsf7->fn & 0x0007) == 2)))
		{
		/* perform writeback 2 if necessary */

		if (pEsf7->wb2stat & 0x80)
		    vxMemProbe ((char *) pEsf7->wb2addr, VX_WRITE,
				sizeTbl[(pEsf7->wb2stat & 0x0060) >> 5],
				(char *) &pEsf7->wb2data);

		/* perform writeback 3 if necessary */

		if (pEsf7->wb3stat & 0x80)
		    vxMemProbe ((char *) pEsf7->wb3addr, VX_WRITE,
				sizeTbl[(pEsf7->wb3stat & 0x0060) >> 5],
				(char *) &pEsf7->wb3data);
		}

	    size = sizeof (ESF7);
	    break;
	    }
#endif /* (CPU == MC68040 || CPU == MC68LC040) */

#if (CPU != MC68060)
	case 8:
	    pExcInfo->valid     |= EXC_PC | EXC_STATUS_REG | EXC_ACCESS_ADDR |
				   EXC_FUNC_CODE | EXC_INSTR_REG;
	    pExcInfo->pc         = ((ESF8 *)pEsf)->pc;
	    pExcInfo->statusReg  = ((ESF8 *)pEsf)->sr;
	    pExcInfo->accessAddr = ((ESF8 *)pEsf)->aa;
	    pExcInfo->funcCode   = ((ESF8 *)pEsf)->fn;
	    pExcInfo->instrReg   = ((ESF8 *)pEsf)->ir;

	    size =  sizeof (ESF8);
	    break;

	case 9:
	    pExcInfo->valid     |= EXC_PC | EXC_STATUS_REG | EXC_ACCESS_ADDR;
	    pExcInfo->pc         = ((ESF9 *)pEsf)->pc;
	    pExcInfo->statusReg  = ((ESF9 *)pEsf)->sr;
	    pExcInfo->accessAddr = ((ESF9 *)pEsf)->aa;

	    size =  sizeof (ESF9);
	    break;

	case 10:
	    pExcInfo->valid     |= EXC_PC | EXC_STATUS_REG | EXC_ACCESS_ADDR |
				   EXC_FUNC_CODE | EXC_INSTR_REG;
	    pExcInfo->pc         = ((ESFA *)pEsf)->pc;
	    pExcInfo->statusReg  = ((ESFA *)pEsf)->sr;
	    pExcInfo->accessAddr = ((ESFA *)pEsf)->aa;
	    pExcInfo->funcCode   = ((ESFA *)pEsf)->fn;
	    pExcInfo->instrReg   = ((ESFA *)pEsf)->instPipeC;

	    size = sizeof (ESFA);
	    break;

	case 11:
	    pExcInfo->valid     |= EXC_PC | EXC_STATUS_REG | EXC_ACCESS_ADDR |
				   EXC_FUNC_CODE | EXC_INSTR_REG;
	    pExcInfo->pc         = ((ESFA *)pEsf)->pc;
	    pExcInfo->statusReg  = ((ESFA *)pEsf)->sr;
	    pExcInfo->accessAddr = ((ESFA *)pEsf)->aa;
	    pExcInfo->funcCode   = ((ESFA *)pEsf)->fn;
	    pExcInfo->instrReg   = ((ESFA *)pEsf)->instPipeC;

	    size = sizeof (ESFB);
	    break;
#endif /* (CPU != MC68060) */

#if CPU == CPU32
	case 12:
	    pExcInfo->valid     |= EXC_PC | EXC_STATUS_REG | EXC_ACCESS_ADDR |
				   EXC_FUNC_CODE;
	    pExcInfo->pc         = ((ESFC *)pEsf)->pc;
	    pExcInfo->statusReg  = ((ESFC *)pEsf)->sr;
	    pExcInfo->accessAddr = ((ESFC *)pEsf)->aa;
	    pExcInfo->funcCode   = ((ESFC *)pEsf)->fn;

	    size = sizeof (ESFC);
	    break;
#endif	/* CPU == CPU32 */

	default:
	    pExcInfo->valid   |= EXC_INVALID_TYPE;
	    pExcInfo->funcCode = ((pEsf->vectorOffset & 0xf000) >> 12);

	    size = 0;
	    break;
	}
#endif	/* (CPU==MC680[12346]0 || CPU==CPU32) */

    pRegs->spReg = (ULONG)((char *) pEsf + size);	/* bump up stack ptr */
    }
/*******************************************************************************
*
* excTasRetry - retry a TAS instruction
*
* If this was a bus error involving a RMW cycle (TAS instruction) we
* return to the handler to retry it.  Such is the case in a vme
* bus deadlock cycle, where the local CPU initiates a TAS instuction
* (or RMW cycle) at the same time it's dual port arbiter grants the local bus
* to an external access.  The cpu backs off by signaling a bus error and
* setting the RM bit in the special status word of the bus error exception
* frame.  The solution is simply to retry the instruction hoping that the
* external access has been resolved.  Even if a card such as a disk controller
* has grabed the bus for DMA accesses for a long time, the worst that will
* happen is we'll end up back here again, and we can keep trying until we get
* through.
*
* RETURNS: TRUE if retry desired, FALSE if not TAS cycle.
* NOMANUAL
*/

LOCAL BOOL excTasRetry
    (
    int		vecNum,		/* exception vector number */
    ESF0 *	pEsf,		/* pointer to exception stack frame */
    REG_SET *	pRegs		/* pointer to register info on stack */
    )
    {
#if (CPU==MC68000)
    if (FALSE) 
        /* no way to tell if this was a RMW - just return FALSE */
#endif  /* (CPU==MC68000) */

#if (CPU==MC68010)
    if ((((pEsf->vectorOffset & 0xf000) >> 12) == 8) &&         /* BERR! */
        (((ESF8 *)pEsf)->fn & 0x800))                           /* RMW cycle */
#endif  /* (CPU==MC68010) */

#if (CPU==MC68020)
    if (((((pEsf->vectorOffset & 0xf000) >> 12) == 10) ||
	 (((pEsf->vectorOffset & 0xf000) >> 12) == 11)) &&	/* BERR! */
	(((ESFA *)pEsf)->fn & 0x80))				/* RMW cycle */
#endif	/* (CPU==MC68020) */

#if (CPU==CPU32)
    if (((((pEsf->vectorOffset & 0xf000) >> 12) == 10) ||
	 (((pEsf->vectorOffset & 0xf000) >> 12) == 11)) &&	/* BERR! */
	(((ESFA *)pEsf)->fn & 0x100))				/* RMW cycle */
#endif	/* (CPU==CPU32) */

#if ((CPU==MC68040) || (CPU==MC68LC040) || (CPU==MC68060))
#if (CPU==MC68040) || (CPU==MC68LC040) 
if ((((pEsf->vectorOffset & 0xf000) >> 12) == 7) &&             /* BERR! */
    (((ESF7 *)pEsf)->fn & 0x200))                               /* ATC Fault */
#else
if ((((pEsf->vectorOffset & 0xf000) >> 12) == 4) &&            /* BERR! */
   ((((ESF4 *)pEsf)->fslw & 0x03800020) == 0x03800020))        /* LCK+RMW+RE */
#endif

/* The 68040/68060 has NO data retry capability.  Just returning from here would
 * cause the access to "seem" OK, but throw away any writeback data that
 * was contained in the stack frame, corrupting memory.  On the other hand,
 * the address that were to be written to could ALSO be invalid.
 *
 * It is a problem.
 *
 * the vxTas call prevents this by flushing the write pipeline with a "nop"
 * before doing the TAS.  In fact, we can simplify everything for the vxTas
 * case by placing the retry logic at the application (vxTas) level.
 *
 * We tell vxTas that an error occured by placing a -1 in "d0"
 */
#endif	/* ((CPU==MC68040) || (CPU==MC68LC040) || (CPU==MC68060)) */

	{
	++excTasErrors;				/* keep count of TAS errors */
#if (CPU==MC68040 || CPU==MC68LC040 || CPU==MC68060)
	pRegs->dataReg[0] = -1;			/* and place a -1 in "d0" */
#endif	/* (CPU==MC68040 || CPU==MC68LC040 || CPU==MC68060) */
	return (TRUE);				/* retry the instruction */
	}
    
    return (FALSE);
    }
