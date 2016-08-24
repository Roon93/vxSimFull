/* wdbDbgALib.s - debugging aids assembly language interface */

/* Copyright 1984-1998 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01d,08jan98,dbt  modified for new breakpoint scheme.
01c,28aug95,ms   changed 68000 to MC68000
01b,23aug95,tpr  different exception frame size for 68000 
01a,07dec94,rrr  written from dbgALib.s
*/

/*
DESCRIPTION
This module contains assembly language routines needed for the debug
package and the 680x0 exception vectors.
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"

	/* internal */

	.globl	_wdbDbgBpStub		/* breakpoint exceptions handler */
	.globl	_wdbDbgTraceStub	/* trace exceptions handler */

	/* external */

	.globl	_wdbDbgBreakpoint	/* breakpoint processing routine */
	.globl	_wdbDbgTrace		/* trace processing routine */

	.text
	.even

/****************************************************************************
*
* wdbDbgBpStub - breakpoint handling
*
* This routine is attached to the breakpoint trap (default trap #2).  It
* saves the entire task context on the stack and calls wdbDbgBreakpoint () to
* handle the event.
*
* NOMANUAL
*/

_wdbDbgBpStub:		/* breakpoint interrupt driver */
	subl	#76,a7			/* make room for REG_SET */
	moveml	d0-d7/a0-a7,(a7)	/* save regs */
	movew	a7@(76),a7@(66)		/* get sr from exc frame */
	movel	a7@(78),a7@(68)		/* get pc from exc frame */
	subql	#0x2,a7@(68)		/* adjust program counter */
#if     (CPU == MC68000)
	addl	#82,a7@(60)		/* adjust sp */
#else
	addl	#84,a7@(60)		/* adjust sp */
#endif
	clrl	a7@(72)
	clrw	a7@(64)

	movel	a7,a5			/* a5 points to saved regs */
	link	a6,#0

	movel   #0,a7@-			/* push FALSE (not a hardware bp) */
	movel   #0,a7@-			/* push NULL (no debug registers) */
	movel	a5,a7@-			/* push pointer to saved regs */
	pea	a5@(76)			/* push pointer to saved info */
	jsr	_wdbDbgBreakpoint	/* do breakpoint handling */

/**************************************************************************
*
* wdbDbgTraceStub - trace exception processing
*
* This routine is attached to the 68k trace exception vector.  It saves the
* entire task context on the stack and calls wdbDbgTrace () to handle the event.
*
* NOMANUAL
*/

_wdbDbgTraceStub:			/* trace interrupt driver */
	subl	#76,a7			/* make room for REG_SET */
	moveml	d0-d7/a0-a7,(a7)	/* save regs */
	movew	a7@(76),a7@(66)		/* get sr from exc frame */
	andw	#0x7fff,a7@(66)		/* clear trace bit in saved status */
	movel	a7@(78),a7@(68)		/* get pc from exc frame */
#if     (CPU == MC68000)
	addl	#82,a7@(60)		/* adjust sp */
#else
	addl	#88,a7@(60)		/* adjust sp */
#endif
	clrl	a7@(72)
	clrw	a7@(64)

	movel	a7,a5			/* a5 points to saved regs */
	link	a6,#0

	movel	a5,a7@-			/* push pointer to saved regs */
	pea	a5@(76)			/* push pointer to saved info */
	jsr	_wdbDbgTrace		/* do breakpoint handling */
