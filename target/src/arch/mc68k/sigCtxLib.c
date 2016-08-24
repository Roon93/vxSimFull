/* sigCtxLib.c - software signal architecture support library */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01e,31aug92,rrr  cleanup of code passed to signal handler
01d,30jul92,rrr  backed out 01c (now back to 01b)
01c,30jul92,kdl  backed out 01b changes pending rest of exc handling.
01b,29jul92,rrr  added fault table
01a,08jul92,rrr  written.
*/

/*
This library provides the architecture specific support needed by
software signals.
*/

#include "vxWorks.h"
#include "private/sigLibP.h"
#include "string.h"

struct sigfaulttable _sigfaulttable [] =
    {
    {2,  SIGSEGV},
    /* XXX {2,  SIGBUS}, */
    {3,  SIGBUS},
    {4,  SIGILL},
    {5,  SIGFPE},
    {6,  SIGFPE},
    {7,  SIGFPE},
    {8,  SIGILL},
    {9,  SIGTRAP},
    {10, SIGEMT},
    {11, SIGEMT},
    {13, SIGILL},
    {14, SIGFMT},
    {48, SIGFPE},
    {49, SIGFPE},
    {50, SIGFPE},
    {51, SIGFPE},
    {52, SIGFPE},
    {53, SIGFPE},
    {54, SIGFPE},
    {0,  0 },
    };

/*******************************************************************************
*
* _sigCtxRtnValSet - set the return value of a context
*
* Set the return value of a context.
* This routine should be almost the same as taskRtnValueSet in taskArchLib.c
*/

void _sigCtxRtnValSet
    (
    REG_SET		*pRegs,
    int			val
    )
    {

    pRegs->dataReg[0] = val;
    }


/*******************************************************************************
*
* _sigCtxStackEnd - get the end of the stack for a context
*
* Get the end of the stack for a context, the context will not be running.
* If during a context switch, stuff is pushed onto the stack, room must
* be left for that (on the 68k the fmt, pc, and sr are pushed just before
* a ctx switch)
*/

void *_sigCtxStackEnd
    (
    const REG_SET	*pRegs
    )
    {

    /*
     * The 16 is pad for the fmt, pc, and sr which are pushed onto the stack.
     */
    return (void *)(pRegs->addrReg[7] - 16);
    }

/*******************************************************************************
*
* _sigCtxSetup - Setup of a context
*
* This routine will set up a context that can be context switched in.
* <pStackBase> points beyond the end of the stack. The first element of
* <pArgs> is the number of args to call <taskEntry> with.
* When the task gets swapped in, it should start as if called like
*
* (*taskEntry) (pArgs[1], pArgs[2], ..., pArgs[pArgs[0]])
*
* This routine is a blend of taskRegsInit and taskArgsSet.
*
* Currently (for signals) pArgs[0] always equals 1, thus the task should
* start as if called like
* (*taskEntry) (pArgs[1]);
*
* Furthermore (for signals), the function taskEntry will never return.
* For the 68k case, we push vxTaskEntry() onto the stack so a stacktrace
* looks good.
*/

void _sigCtxSetup
    (
    REG_SET		*pRegs,
    void		*pStackBase,
    void		(*taskEntry)(),
    int			*pArgs
    )
    {
    extern void vxTaskEntry();
    int i;
    union
	{
	void		*pv;
	int		*pi;
	void		(**ppfv)();
	int		i;
	} pu;

    bzero((void *)pRegs, sizeof(*pRegs) + 4);
    pu.pv = (void *)((int)pStackBase & ~3);

    for (i = pArgs[0]; i > 0; --i)
        *--pu.pi = pArgs[i];

    *--pu.ppfv = vxTaskEntry;
    pRegs->addrReg[7] = pu.i;
    pRegs->pc = (INSTR *)taskEntry;
    pRegs->sr = 0x3000;
    }
