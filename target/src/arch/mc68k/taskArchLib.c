/* taskArchLib.c - MC680X0-specific task management routines */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01o,23aug92,jcf  cleanup.
01n,04jul92,jcf  clean up. changed taskRegIndex to taskRegName.
01m,26may92,rrr  the tree shuffle
01l,18mar92,yao  removed routine taskStackAllot(), macro MEM_ROUND_UP.
01k,12mar92,yao  removed taskRegsShow().  changed to clear pad byte in
		 taskRegsInit().  changed copyright notice.
01j,09mar92,yao  added regIndex[].
01i,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed VOID to void
		  -changed copyright notice
01h,05apr91,jdi	  documentation -- removed header parens and x-ref numbers;
		  doc review by dnw.
01g,24mar91,jdi   documentation cleanup.
01f,28sep90,jcf   documentation.
01e,02aug90,jcf   documentation.
01d,10jul90,jcf   moved taskStackAllot () from taskLib.c.
01c,26jun90,jcf   added taskRtnValueSet ().
01b,23apr90,jcf   changed name and moved to src/68k.
01a,18dec89,jcf   written by extracting from taskLib (2).
*/

/*
DESCRIPTION
This library provides an interface to 680X0-specific task management routines.

SEE ALSO: taskLib
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "taskArchLib.h"
#include "regs.h"
#include "private/taskLibP.h"
#include "private/windLibP.h"


/* globals */

REG_INDEX taskRegName[] =
    {
    {"d0", D_REG_OFFSET(0)},
    {"d1", D_REG_OFFSET(1)},
    {"d2", D_REG_OFFSET(2)},
    {"d3", D_REG_OFFSET(3)},
    {"d4", D_REG_OFFSET(4)},
    {"d5", D_REG_OFFSET(5)},
    {"d6", D_REG_OFFSET(6)},
    {"d7", D_REG_OFFSET(7)},
    {"a0", A_REG_OFFSET(0)},
    {"a1", A_REG_OFFSET(1)},
    {"a2", A_REG_OFFSET(2)},
    {"a3", A_REG_OFFSET(3)},
    {"a4", A_REG_OFFSET(4)},
    {"a5", A_REG_OFFSET(5)},
    {"a6/fp", A_REG_OFFSET(6)},
    {"a7/sp", A_REG_OFFSET(7)},
    {"sr", SR_OFFSET},
    {"pc", PC_OFFSET},
    {NULL, 0},
    };

/*******************************************************************************
*
* taskRegsInit - initialize a task's registers
*
* During task initialization this routine is called to initialize the specified
* task's registers to the default values.
*
* NOMANUAL
* ARGSUSED
*/

void taskRegsInit
    (
    WIND_TCB    *pTcb,          /* pointer TCB to initialize */
    char        *pStackBase     /* bottom of task's stack */
    )
    {
    FAST int ix;

    pTcb->regs.sr = 0x3000;			/* set status register */
    pTcb->regs.pad = 0;				/* clear pad byte */
    pTcb->regs.pc = (INSTR *)vxTaskEntry;	/* set entry point */
    pTcb->foroff = 0;				/* format/offset (68020 only) */

    for (ix = 0; ix < 8; ++ix)
	pTcb->regs.dataReg[ix] = 0;		/* initialize d0 - d7 */

    for (ix = 0; ix < 7; ++ix)
	pTcb->regs.addrReg[ix] = 0;		/* initialize a0 - a6 */

    /* initial stack pointer is just after MAX_TASK_ARGS task arguments */

    pTcb->regs.spReg = (int) (pStackBase - (MAX_TASK_ARGS * sizeof (int)));
    }

/*******************************************************************************
*
* taskArgsSet - set a task's arguments
*
* During task initialization this routine is called to push the specified
* arguments onto the task's stack.
*
* NOMANUAL
* ARGSUSED
*/

void taskArgsSet
    (
    WIND_TCB    *pTcb,          /* pointer TCB to initialize */
    char        *pStackBase,    /* bottom of task's stack */
    int         pArgs[]         /* array of startup arguments */
    )
    {
    FAST int ix;
    FAST int *sp;

    /* push args on the stack */

    sp = (int *) pStackBase;			/* start at bottom of stack */

    for (ix = MAX_TASK_ARGS - 1; ix >= 0; --ix)
	*--sp = pArgs[ix];			/* put arguments onto stack */
    }

/*******************************************************************************
*
* taskRtnValueSet - set a task's subroutine return value
*
* This routine sets register d0, the return code, to the specified value.  It
* may only be called for tasks other than the executing task.
*
* NOMANUAL
* ARGSUSED
*/

void taskRtnValueSet
    (
    WIND_TCB    *pTcb,          /* pointer TCB for return value */
    int         returnValue     /* return value to fill into WIND_TCB */
    )
    {
    pTcb->regs.dataReg [0] = returnValue;
    }

/*******************************************************************************
*
* taskArgsGet - get a task's arguments
*
* This routine is utilized during task restart to recover the original task
* arguments.
*
* NOMANUAL
* ARGSUSED
*/

void taskArgsGet
    (
    WIND_TCB *pTcb,             /* pointer TCB to initialize */
    char *pStackBase,           /* bottom of task's stack */
    int  pArgs[]                /* array of arguments to fill */
    )
    {
    FAST int ix;
    FAST int *sp;

    /* push args on the stack */

    sp = (int *) pStackBase;			/* start at bottom of stack */

    for (ix = MAX_TASK_ARGS - 1; ix >= 0; --ix)
	pArgs[ix] = *--sp;			/* fill arguments from stack */
    }
/*******************************************************************************
*
* taskSRSet - set task status register
*
* This routine sets the status register of a specified task that is not
* running (i.e., the TCB must not be that of the calling task).  Debugging
* facilities use this routine to set the trace bit in the status register of
* a task that is being single-stepped.
*
* RETURNS: OK, or ERROR if the task ID is invalid.
*/

STATUS taskSRSet
    (
    int    tid,         /* task ID */
    UINT16 sr           /* new SR */
    )
    {
    FAST WIND_TCB *pTcb = taskTcb (tid);

    if (pTcb == NULL)		/* task non-existent */
	return (ERROR);

    pTcb->regs.sr = sr;

    return (OK);
    }
