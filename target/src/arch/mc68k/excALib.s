/* excALib.s - exception handling 68K assembly language routines */

/* Copyright 1984-1994 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
02f,21jun96,ms   merged kdl's patch for SPR 2245 (68000 exception handling)
02e,26oct94,tmk  added MC68LC040 support
02d,30may94,tpr  added MC68060 cpu support.
02c,01oct92,jcf  fixed excIntStub to accomodate for handling errno.. sigh.
		 fixed E29 for MC68040.
02b,23aug92,jcf  fixed excIntStub to handle errno on stack correctly.
02a,02aug92,jcf  overhauled.  exception handling now avoids excJobAdd().
01p,26may92,rrr  the tree shuffle
01o,04oct91,rrr  passed through the ansification filter
		  -fixed #else and #endif
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01n,25sep91,yao  added support for CPU32.
01m,28aug91,shl  added support for MC68040, cleaned up #if CPU,
		 updated copyright.
01l,24aug91,jcf  fixed sp offset calculation to account for errno.
01k,14mar90,jdi  documentation cleanup; fixed jcf's last mod letter to j.
01j,17jan89,jcf  fixed bug in excIntStub; intExit no longer takes d0 on stack.
01i,13feb88,dnw  added .data before .asciz above, for Intermetrics assembler.
01h,01nov87,jcf	 added code in excStub to retry an instruction
01g,24mar87,dnw  added .globl for excExcHandle.
		 documentation.
01f,26feb87,rdc  modifications for VRTX 3.2.
01e,21dec86,dnw  changed to not get include files from default directories.
01d,31oct86,dnw  Eliminated magic f/b numeric labels which mitToMot can't
		   handle.
		 Changed "moveml" instructions to use Motorola style register
		   lists, which are now handled by "aspp".
		 Changed "mov[bwl]" to "move[bwl]" for compatiblity w/Sun as.
01c,26jul86,dnw  changed 68000 version to use BSR table w/ single handler rtn.
01b,03jul86,dnw  documentation.
01a,03apr86,dnw  extracted from dbgALib.s
*/

/*
DESCRIPTION
This module contains the assembly language exception handling stub.
It is connected directly to the 680x0 exception vectors.
It sets up an appropriate environment and then calls a routine
in excLib(1).

SEE ALSO: excLib(1)
*/

#define _ASMLANGUAGE
#include "vxWorks.h"

	/* globals */

#if (CPU==MC68000)
	.globl	_excBsrTbl	/* BSR table */
#endif	/* (CPU==MC68000) */

	.globl	_excStub	/* generic stub routine */
	.globl	_excIntStub	/* uninitialized interrupt handler */

#if (CPU==MC68040 || CPU==MC68LC040)
	.comm	safearea,24	/* ERRATA: E29 */
#endif /* (CPU==MC68040 || CPU==MC68LC040) */

	.text
	.even

#if (CPU==MC68000)
/**************************************************************************
*
* excBsrTbl - table of BSRs
*
* NOMANUAL
*/

_excBsrTbl:
	bsr	_excIntStub	/* 0 */ /* reset sp */
	bsr	_excIntStub	/* reset pc */
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excIntStub	/* uninitialized interrupt */
	bsr	_excIntStub	/* 10 */ /* unassigned reserved */
	bsr	_excIntStub 	/* unassigned reserved */
	bsr	_excIntStub 	/* unassigned reserved */
	bsr	_excIntStub 	/* unassigned reserved */
	bsr	_excIntStub 	/* unassigned reserved */
	bsr	_excIntStub 	/* unassigned reserved */
	bsr	_excIntStub 	/* unassigned reserved */
	bsr	_excIntStub 	/* unassigned reserved */
	bsr	_excIntStub	/* spurious interrupt */
	bsr	_excIntStub	/* level 1 Auto Vec */
	bsr	_excIntStub	/* level 2 Auto Vec */
	bsr	_excIntStub	/* level 3 Auto Vec */
	bsr	_excIntStub	/* level 4 Auto Vec */
	bsr	_excIntStub	/* level 5 Auto Vec */
	bsr	_excIntStub	/* level 6 Auto Vec */
	bsr	_excIntStub	/* level 7 Auto Vec */
	bsr	_excStub	/* 20 */
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub	/* 30 */
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excStub
	bsr	_excIntStub	/* 40 */ /* User Interrupts */
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub	/* 50 */
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub	/* 60 */
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub	/* 70 */
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub	/* 80 */
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub	/* 90 */
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub	/* a0 */
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub	/* b0 */
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub	/* c0 */
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub	/* d0 */
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub	/* e0 */
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub	/* f0 */
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
	bsr	_excIntStub
#endif	/* (CPU==MC68000) */

/*********************************************************************
*
* excStub - exception handler
*
* NOMANUAL
*/

_excStub:

#if	(CPU==MC68040 || CPU==MC68LC040)
	moveml	d0-d2/a0-a2,safearea	/* MC68040 ERRATA: E29 */
#endif	/* (CPU==MC68040 || CPU==MC68LC040) */

#if 	(CPU==MC68000)
	subl	#0x44,a7		/* save room for REG_SET - sizeof (PC)*/
	moveml	d0-d7/a0-a7,a7@		/* save registers */
	addl	#0x48,a7@(0x3c)		/* point saved stack pointer at esf */
	movel	a7@(0x44),d0		/* get BSR return adrs */
	subql	#4,d0			/* adjust return adrs to be BSR adrs */
	subl	#_excBsrTbl,d0		/* get offset from start of BSR table */
	lsrw	#2,d0			/* turn vector offset into excep num */
        clrw    a7@(0x40)               /* clear padding */
        movew   a7@(0x50),a7@(0x42)     /* fill in status register */
        movel   a7@(0x52),a7@(0x44)     /* fill in program counter */
#else	/* (CPU==MC680[12346]0 || CPU==CPU32) */
	subl	#0x48,a7		/* save room for REG_SET */
	moveml	d0-d7/a0-a7,a7@		/* save registers */
	addl	#0x48,a7@(0x3c)		/* point saved stack pointer at esf */
	clrl	d0			/* clear d0 */
	movew	a7@(0x4e),d0		/* get the vector offset from the esf */
	andw	#0x0fff,d0		/* clear the format */
	lsrw	#2,d0			/* turn vector offset into excep num */
	clrw	a7@(0x40)		/* clear padding */
	movew	a7@(0x48),a7@(0x42)	/* fill in status register */
	movel	a7@(0x4a),a7@(0x44)	/* fill in program counter */
#endif	/* (CPU==MC680[12346]0 || CPU==CPU32) */
	movel	a7,a7@-			/* push pointer to REG_SET */
	pea	a7@(0x4c)		/* push pointer to exception frame */
	movel	d0,a7@-			/* push exception number */
	jsr	_excExcHandle		/* do exception processing */
	addl	#0xc,a7			/* clean up pushed arguments */
	moveml	a7@,d0-d7/a0-a6		/* restore registers except adj. a7 */
	addl	#0x48,a7		/* pop REG_SET off stack */
	rte				/* return to task that got exception */

/*********************************************************************
*
* excIntStub - uninitialized interrupt handler
*
* NOMANUAL
*/

_excIntStub:
	addql	#1,_intCnt		/* from intEnt(); errno saved below */
#if 	(CPU==MC68000)
	subl	#0x44,a7		/* save room for REG_SET - sizeof (PC)*/
	moveml	d0-d7/a0-a7,a7@		/* save registers */
	addl	#0x48,a7@(0x3c)		/* point saved stack pointer at esf */
	movel	a7@(0x44),d0		/* get BSR return adrs */
	subql	#4,d0			/* adjust return adrs to be BSR adrs */
	subl	#_excBsrTbl,d0		/* get offset from start of BSR table */
	lsrw	#2,d0			/* turn vector offset into excep num */
        clrw    a7@(0x40)               /* clear padding */
        movew   a7@(0x50),a7@(0x42)     /* fill in status register */
        movel   a7@(0x52),a7@(0x44)     /* fill in program counter */
#else	/* (CPU==MC680[12346]0 || CPU==CPU32) */
	subl	#0x48,a7		/* save room for REG_SET */
	moveml	d0-d7/a0-a7,a7@		/* save registers */
	addl	#0x48,a7@(0x3c)		/* point saved stack pointer at esf */
	clrl	d0			/* clear d0 */
	movew	a7@(0x4e),d0		/* get the vector offset from the esf */
	andw	#0x0fff,d0		/* clear the format */
	lsrw	#2,d0			/* turn vector offset into excep num */
	clrw	a7@(0x40)		/* clear padding */
	movew	a7@(0x48),a7@(0x42)	/* fill in status register */
	movel	a7@(0x4a),a7@(0x44)	/* fill in program counter */
#endif	/* (CPU==MC680[12346]0 || CPU==CPU32) */
	movel	_errno,a7@-		/* save errno on the stack (intEnt()) */
	pea	a7@(0x4)		/* push pointer to REG_SET */
	pea	a7@(0x50)		/* push pointer to exception frame */
	movel	d0,a7@-			/* push exception number */
	jsr	_excIntHandle		/* do exception processing */
	addl	#0xc,a7			/* clean up pushed arguments */
	movel	a7@+,_errno		/* restore errno */
	moveml	a7@,d0-d7/a0-a6		/* restore registers from REG_SET */
	addl	#0x48,a7		/* point stack pointer at esf */
	movel	_errno,a7@-		/* save errno on the stack (intEnt()) */
	jmp	_intExit		/* exit the ISR thru the kernel */
