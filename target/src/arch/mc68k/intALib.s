/* intALib.s - interrupt library assembly language routines */

/* Copyright 1984-1992 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
05l,12mar99,elg  delete the SEE ALSO comment in intLevelSet() (SPR 22809).
05k,15sep92,jdi  ansified declarations for mangenable routines.
05j,23aug92,jcf  cleanup.
05i,26may92,rrr  the tree shuffle
05h,04oct91,rrr  passed through the ansification filter
		  -changed VOID to void
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
05g,23jul91,gae  changed 68k/asm.h to asm.h.
05f,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
05e,07feb91,jaa	 documentation cleanup.
05d,27oct90,dnw  changed to include "68k/asm.h" explicitly.
05c,28sep90,jcf  documentation.
05b,26jun90,jcf  documentation.
05a,17apr90,jcf  optimized intLock and intUnlock.
04f,19aug88,gae  documentation.
04e,30may88,dnw  changed to v4 names.
04d,28may88,jcf  fixed over optimized v4c which didn't work.
04c,28apr88,jcf  optimized intSetLevel().
		  removed unused external _intCnt.
04b,22apr88,gae  documentation of intSetVBR().
04a,25jan88,jcf  remove intEnt and intExit, as they are kernel dependent.
03r,13feb88,dnw  added .data before .asciz above, for Intermetrics assembler.
03q,16nov87,ecs  documentation.
03p,24oct87,dnw  removed unnecessary declaration of sysVwTrap.
03o,18sep87,dnw  removed intDrop() (obsolete).
		 changed to call sysKernelTrap() and sysVwTrap()
		   instead of doing traps directly.
03n,24mar87,jlf  documentation
03m,20dec86,dnw  fixed to be compatible with motorola assembler.
03l,04dec86,dnw  changed to code for vector base register to be compatible
		   with all assemblers.
03k,29nov86,dnw  removed conditional assembly for 68020/68000.
03j,20nov86,dnw  Changed "mov[bwl]" to "move[bwl]" for compatiblity w/Sun as.
03i,28oct86,llk  conditionally compile out intSetVBR for non 68020 systems.
03h,27oct86,llk  added intSetVBR.  Used for setting the vector base register.
03g,04sep86,jlf  minor documentation.
03f,01jul86,jlf  documentation.
03e,18jan86,dnw  removed intEnd; code now directly built by intConnect.
03d,18jul85,jlf  added some stuff for mangen.
03c,19jun85,rdc  changed .globls for 4.2 as.
03b,12jun85,rdc  added C style comments.
03a,22may85,jlf+ translated from asm to as.  Removed intContext (now in C).
                 added intEnt and intExit.  Fixed intSet to do almost
                 everything at trap level, since move sr is priveleged on
                 68010.
02a,05apr85,rdc  installed modifications for vrtx version 3.
01c,06sep84,jlf  added copyright, some comments
01b,04sep84,dnw  added intContext.
01a,03aug84,jlf  written, by modifying gathering routines from hither
		   and yon.
*/

/*
DESCRIPTION
This library supports various functions associated with interrupts from C
routines.  The routine intLevelSet() changes the current interrupt level
of the processor.

SEE ALSO: intLib, intArchLib

INTERNAL
Some routines in this module "link" and "unlk" the "c" frame pointer
(a6) although they don't use it in any way!  This is only for the benefit of
the stacktrace facility to allow it to properly trace tasks executing within
these routines.
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"

	.text
	.even

	/* globals */

	.globl	_intLevelSet
	.globl	_intLock
	.globl	_intUnlock
	.globl	_intVBRSet


/*******************************************************************************
*
* intLevelSet - set the interrupt level (for 68K processors)
*
* This routine changes the interrupt mask in the status register to take
* on the value specified by <level>.  Interrupts are locked out at or below
* that level.  The level must be in the range 0 - 7.
*
* WARNING
* Do not call VxWorks system routines with interrupts locked.
* Violating this rule may re-enable interrupts unpredictably.
*
* RETURNS: The previous interrupt level (0 - 7).
*
* int intLevelSet
*    (
*    int level	/* new interrupt level mask, 0 - 7 *
*    )
*/

_intLevelSet:
	link	a6,#0
	movew	sr,d0		/* get old sr into d0 */
	andw	#0xf8ff,d0	/* clear interrupt mask in saved sr */
	movel	a6@(ARG1),d1	/* get new level into d1 */
	andl	#0x7,d1		/* clear all but interrupt mask */
	lslw	#8,d1		/* get level into high order byte */
	orw	d0,d1		/* combine with saved status reg contents */
	movew	sr,d0		/* remember status register */
	movew	d1,sr		/* set status register */

	lsrw	#8,d0		/* shift interrupt level to low bits */
	andl	#0x7,d0		/* clear all but interrupt mask */
	unlk	a6
	rts

/*******************************************************************************
*
* intLock - lock out interrupts
*
* This routine disables interrupts.  The interrupt level is set to the
* lock-out level set by intLockLevelSet().  The default lock-out level is
* the highest value.  The routine returns an architecture-dependent lock-out
* key for the interrupt level prior to the call, and this should be passed back
* to the routine intUnlock() to enable interrupts.
*
* WARNINGS
* Do not call VxWorks system routines with interrupts locked.
* Violating this rule may re-enable interrupts unpredictably.
*
* The routine intLock() can be called from either interrupt or task level.
* When called from a task context, the interrupt lock level is part of the
* task context.  Locking out interrupts does not prevent rescheduling.
* Thus, if a task locks out interrupts and invokes kernel services that
* cause the task to block (e.g., taskSuspend() or taskDelay()) or that cause a
* higher priority task to be ready (e.g., semGive() or taskResume()), then
* rescheduling will occur and interrupts will be unlocked while other tasks
* run.  Rescheduling may be explicitly disabled with taskLock().
*
* EXAMPLE
* .CS
*     lockKey = intLock ();
*
*      ...
*
*     intUnlock (lockKey);
* .CE
*
* RETURNS
* An architecture-dependent lock-out key for the interrupt level
* prior to the call; the interrupt field mask, on the MC680x0 family.
*
* SEE ALSO: intUnlock(), taskLock()

* int intLock ()

*/

_intLock:
	movew	sr,d0
	movew	d0,d1
	andw	#0xf8ff,d1
	orw	_intLockMask,d1
	movew	d1,sr
	rts

/*******************************************************************************
*
* intUnlock - cancel interrupt locks
*
* This routine re-enables interrupts that have been disabled by the routine
* intLock().  Use the architecture-dependent lock-out key obtained from the
* preceding intLock() call.
*
* RETURNS: N/A
*
* SEE ALSO: intLock()

* void intUnlock
*	(
*	int lockKey
*	)

*/

_intUnlock:
	movew	a7@(0x6),sr
	rts

/*******************************************************************************
*
* intVBRSet - set the vector base register
*
* This routine should only be called in supervisor mode.
* It is not used on the M68000.
*
* NOMANUAL

* void intVBRSet (baseAddr)
*      FUNCPTR *baseAddr;	/* vector base address *

*/

_intVBRSet:

	link	a6,#0
	movel	a6@(ARG1),d0	/* put base address in d0 */

	/* Since the assemblers differ on the syntax for setting the
	 * vector base register (if they even have it), its done with
	 * two .word's.  Yucko. */

	.word	0x4e7b,0x0801	/* movec d0,vbr */

	unlk	a6
	rts
