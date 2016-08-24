/* excArchShow.c - MC680X0 exception show facilities */

/* Copyright 1984-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,30may94,tpr  added MC68060 cpu support.
01c,01oct92,jcf  fixed sysExcMsg printing.
01b,23aug92,jcf  changed filename.
01a,02aug92,jcf  extracted from excMc68kLib.c.
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
#include "taskLib.h"
#include "errno.h"
#include "string.h"
#include "logLib.h"
#include "stdio.h"
#include "stdio.h"
#include "fioLib.h"
#include "intLib.h"
#include "qLib.h"
#include "private/kernelLibP.h"
#include "private/funcBindP.h"


/* locals */

/* 
 * Exception error messages.  These are used by the exception printing routine.
 * Exception numbers are the same as used by the CPU.
 */

LOCAL char *excMsgs [] =
    {
    NULL,				/* reset sp */
    NULL,				/* reset pc */
#if ((CPU == MC68040) || (CPU == MC68060))
    "Access Fault",
#else /* ((CPU == MC68040) || (CPU == MC68060)) */
    "Bus Error",
#endif  /* ((CPU == MC68040) || (CPU == MC68060)) */
    "Address Error",
    "Illegal Instruction",
    "Zero Divide",
    "CHK Trap",
    "TRAPV Trap",
    "Privilege Violation",
    "Trace Exception",
    "Unimplemented Opcode 1010",
    "Unimplemented Opcode 1111",
    "Emulator Interrupt",		/* 68060 only*/
    "Coprocessor Protocol Violation",	/* 68020 only */
    "Format Error",			/* 68010, 68020 only */
    "Uninitialized Interrupt",
    NULL, NULL, NULL, NULL,		/* 16-23 unassigned */
    NULL, NULL, NULL, NULL,
    "Spurious Interrupt",
    NULL, NULL, NULL, NULL,             /* 25-31 autovectored interrupts */
    NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,             /* 32-47 TRAP #0-15 vectors */
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    "FP Branch or Set on Unordered",    /* 68881, 68882, 68040 only */
    "FP Inexact Result",
    "FP Divide by Zero",
    "FP Underflow",
    "FP Operand Error",
    "FP Overflow",
    "FP Signaling NAN",
    "FP Unimplemented Data Type",       /* 68040 only */
    "MMU Configuration Error",
    NULL,NULL,				/* reserved for 68851 only */
    "Unimplemented Effective Address"	/* 68060 */
    "Unimplemented Integer Instruction"	/* 68060 */
    };

LOCAL char *excIntInfoFmt = "\n\
Uninitialized Interrupt!\n\
Vector number %d (0-255). %s\n\
Program Counter: 0x%08x\n\
Status Register: 0x%04x\n";

/* forward declarations */

LOCAL void excInfoShow (EXC_INFO *pExcInfo, BOOL doBell);
LOCAL void excIntInfoShow (int vecNum, ESF0 *pEsf, REG_SET *pRegs,
			   EXC_INFO *pExcInfo);
LOCAL void excPanicShow (int vecNum, ESF0 *pEsf, REG_SET *pRegs,
			 EXC_INFO *pExcInfo);


/*******************************************************************************
*
* excShowInit - initialize exception show facility
*
* NOMANUAL
*/

STATUS excShowInit (void)
    {
    _func_excInfoShow	= (FUNCPTR) excInfoShow;
    _func_excIntHook	= (FUNCPTR) excIntInfoShow;
    _func_excPanicHook	= (FUNCPTR) excPanicShow;

    return (OK);
    }

/*******************************************************************************
*
* excInfoShow - print exception info
*
* NOMANUAL
*/

LOCAL void excInfoShow
    (
    EXC_INFO *	pExcInfo,	/* exception information to summarize */
    BOOL	doBell		/* print task id and ring warning bell */
    )
    {
    FAST int valid  = pExcInfo->valid;
    FAST int vecNum = pExcInfo->vecNum;

    /* print each piece of info if valid */

    if (valid & EXC_VEC_NUM)
	{
	if ((vecNum < NELEMENTS (excMsgs)) && (excMsgs [vecNum] != NULL))
	    printExc ("\n%s\n", (int) excMsgs [vecNum], 0, 0, 0, 0);
	else
	    printExc ("\nTrap to uninitialized vector number %d (0-255).\n",
		      vecNum, 0, 0, 0, 0);
	}

    if (valid & EXC_PC)
	printExc ("Program Counter: 0x%08x\n", (int) pExcInfo->pc, 0, 0, 0, 0);

    if (valid & EXC_STATUS_REG)
	printExc ("Status Register: 0x%04x\n", (int) pExcInfo->statusReg, 0, 0,
		  0, 0);

    if (valid & EXC_ACCESS_ADDR)
	printExc ("Access Address : 0x%08x\n", (int) pExcInfo->accessAddr, 0, 0,
		  0, 0);
#if (CPU != MC68060)
    if (valid & EXC_INSTR_REG)
	printExc ("Instruction    : 0x%04x\n", (int) pExcInfo->instrReg, 0, 0,
		  0, 0);
#endif /* (CPU != MC68060) */

    if (valid & EXC_FUNC_CODE)
#if (CPU==MC68000)
	printExc ("Access Type    : %s %c FC%x\n",
		  (pExcInfo->funcCode & 0x10) ? "Read": "Write",
		  (pExcInfo->funcCode & 0x08) ? 'I' : 'N',
		  pExcInfo->funcCode & 0x07, 0, 0);
#else
	printExc ("Special Status : 0x%04x\n", pExcInfo->funcCode, 0, 0, 0, 0);
#endif

    if (valid & EXC_FSLW)
	printExc ("Fault Status   : 0x%08x\n", pExcInfo->funcCode, 0, 0,
	0, 0);

    if (valid & EXC_INVALID_TYPE)
	printExc ("Invalid ESF type 0x%x\n", pExcInfo->funcCode, 0, 0, 0, 0);

    if (doBell)
	printExc ("Task: %#x \"%s\"\007\n", (int)taskIdCurrent, 
		  (int)taskName ((int)taskIdCurrent), 0, 0, 0);
    }
/*******************************************************************************
*
* excIntInfoShow - print out uninitialized interrupt info
*/

LOCAL void excIntInfoShow
    (
    int		vecNum,		/* exception vector number */
    ESF0 *	pEsf,		/* pointer to exception stack frame */
    REG_SET *	pRegs,		/* pointer to register info on stack */
    EXC_INFO *	pExcInfo	/* parsed exception information */
    )
    {
    char *vecName = "";

    if ((vecNum < NELEMENTS (excMsgs)) && (excMsgs [vecNum] != NULL))
	vecName = excMsgs [vecNum];

    if (Q_FIRST (&activeQHead) == NULL)			/* pre kernel */
	printExc (excIntInfoFmt, vecNum, (int)vecName, (int)pEsf->pc, 
		  (int)pEsf->sr, 0);
    else
	logMsg (excIntInfoFmt, vecNum, (int)vecName, (int)pEsf->pc,
		(int)pEsf->sr, 0, 0);
    }
/*******************************************************************************
*
* excPanicShow - exception at interrupt level
*
* This routine is called if an exception is caused at interrupt
* level.  We can't handle it in the usual way.  Instead, we save info in
* sysExcMsg and trap to rom monitor.
*/

LOCAL void excPanicShow
    (
    int		vecNum,		/* exception vector number */
    ESF0 *	pEsf,		/* pointer to exception stack frame */
    REG_SET *	pRegs,		/* pointer to register info on stack */
    EXC_INFO *	pExcInfo	/* parsed exception information */
    )
    {
    if (INT_CONTEXT ())
	printExc (" \nException at interrupt level:\n", 0, 0, 0, 0, 0);

    if (Q_FIRST (&activeQHead) == NULL)
	printExc ("Exception before kernel initialized:\n", 0, 0, 0, 0, 0);

    excInfoShow (pExcInfo, FALSE);	/* print the message into sysExcMsg */

    printExc ("Regs at 0x%x\n", (int) pRegs, 0, 0, 0, 0);
    }
