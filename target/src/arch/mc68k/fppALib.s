/* fppALib.s - floating-point coprocessor support assembly language routines */

/* Copyright 1984-1998 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01y,25feb98,ms   checked in nortel fix for SPR 8392 - add nops for fpp probe
01x,05mar97,tam  reworked conditional compilation for fpcr[G|S]et. 
01w,23may96,ms   added fpcr[G|S]et so users can enable fpp exceptions
01v,06jan95,jcf  fixed MC68060 null frame support.
           +kdl
01u,09nov94,tmk  fixed comment
01t,26oct94,tmk  added MC68LC040 support
01s,23jun94,tpr	 added MC68060 cpu support.
01r,15sep92,jdi  ansified declarations for mangenable routines.
01q,23aug92,jcf  added support for MC68EC040.  changed bxxx to jxx.
01p,16jul92,rdc  moved fppNullStateFrame + fppCPVersion from text to data.
01o,26may92,rrr  the tree shuffle
01n,07jan92,jcf  reworked fppProbeSup() to store version number of CP.
		 cleaned up.
	    kdl  modified fppSave() to mark NULL 68040 state frames as IDLE
		 (per Motorola 040 errata F33); do null frestore to reset
		 fpu if large farme detected (per errata F23).
01m,04oct91,rrr  passed through the ansification filter
		  -changed VOID to void
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01l,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
01k,06mar91,jaa	 documentation cleanup.
01j,14mar90,jdi  documentation cleanup; fixed gae's last mod letter to h.
01i,28may88,dnw  fixed bug in fppProbe of only setting byte instead of long
		   flag.
		 made FPP_ASSEM conditionals.
		 cleaned-up.
		 removed extra spaces in .word declarations that may have
		   been screwing up the VMS port.
01h,31mar88,gae  took out save & restore in fppProbeSup() so that
		   privilege violation exceptions wouldn't be generated.
01g,28mar88,gae  oops! forgot hand-assembly for fp codes in 01f.
01f,18mar88,gae  now supports both MC68881 & MC68882.
01e,09mar88,gae  moved host specific _errno definition to hostALib.s.
01d,13feb88,dnw  added .data before .asciz above, for Intermetrics assembler.
01c,04dec87,gae  removed most HOST_TYPE dependencies - fp codes hand assembled.
01b,26aug87,gae  added include of asm.h and special HOST_IRIS version
		   of floating-point instructions.
01a,06aug87,gae  written/extracted from vxALib.s
*/

/*
DESCRIPTION
This library contains routines to support the MC68881/MC68882 floating-point
coprocessor.  The routines fppSave() and fppRestore() save and restore all the
task floating-point context information, which consists of the eight extended
double precision registers and three control registers.  Higher-level access
mechanisms are found in fppLib.

SEE ALSO: fppLib, MC68881/MC68882 Floating-Point Coprocessor User's Manual
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "fppLib.h"
#include "asm.h"

/* The following flag controls the assembly of the fpp instructions.
 *   If TRUE, the fpp instructions are assembled (many assemblers can't handle).
 *   If FALSE, hand-coded machine code equivalents are assembled.
 */

#define FPP_ASSEM	FALSE


	.text
	.even

	/* internals */

	.globl _fppSave
	.globl _fppRestore
	.globl _fppDtoDx
	.globl _fppDxtoD
	.globl _fppProbeSup
	.globl _fppProbeTrap

	.data
fppNullStateFrame:
	.long  0x00000000
fppCPVersion:
	.byte  0xff

	.text
	.even

/*******************************************************************************
*
* fppSave - save the floating-pointing coprocessor context
*
* This routine saves the floating-point coprocessor context.
* The context saved is:
*
*	- registers fpcr, fpsr, and fpiar
*	- registers f0 - f7
*	- internal state frame
*
* If the internal state frame is null, the other registers are not saved.
*
* RETURNS: N/A
*
* SEE ALSO: fppRestore(), MC68881/MC68882 Floating-Point Coprocessor
* User's Manual

* void fppSave
*    (
*    FP_CONTEXT *  pFpContext  /* where to save context *
*    )

*/

_fppSave:
	link	a6,#0
	movel	a6@(ARG1),a0			/* where to save registers */


#if (CPU==MC68040 || CPU==MC68LC040)		/* START MC68040 VERSION */

#if FPP_ASSEM			/* FP ASSEMBLY */
	fsave	a0@(STATEFRAME)			/* get MC68881 state frame */
	tstb	a0@(STATEFRAME)			/* check for null frame */
	jne	so_not_null			/* br if not NULL */
	moveb	fppCPVersion,a0@(STATEFRAME)	/* stick in correct version # */
	clrb	a0@(STATEFRAME+1)		/* make idle, Mot 040 bug F33 */
so_not_null:
	fmovemx	f0-f7,a0@(FPX)			/* save data registers */
	fmoveml	#<fpcr,fpsr,fpiar>,a0@(FPCR)	/* save control registers */
	tstb	a0@(STATEFRAME+1)		/* is it an IDLE frame? */
	jeq	so_null				/* br if yes */
	frestore fppNullStateFrame		/* reset per Mot 040 bug F23 */

#else				/* FP LITERAL */
	.word	0xf328,0x006c			/* fsave   a0@(STATEFRAME) */
	tstb	a0@(STATEFRAME)			/* check for null frame */
	jne	so_not_null			/* br if not NULL */
	moveb	fppCPVersion,a0@(STATEFRAME)	/* stick in correct version # */
	clrb	a0@(STATEFRAME+1)		/* make idle, Mot 040 bug F33 */
so_not_null:
	.word	0xf228,0xf0ff,0x000c		/* fmovemx f0-f7,a0@(FPX) */
	.word	0xf228,0xbc00,0x0000		/* fmoveml #<cregs.>,a0@(FPCR)*/
	tstb	a0@(STATEFRAME+1)		/* is it an IDLE frame? */
	jeq	so_null				/* br if yes */
	.word	0xf379				/* frestore nullStateFrame */
	.long	fppNullStateFrame		/* reset per Mot 040 bug F23 */

#endif			/* END MC68040 VERSION */
#else			/* START MC680[01236]0 VERSION */

#if FPP_ASSEM			/* FP ASSEMBLY */
	fsave	a0@(STATEFRAME)			/* get MC68881 state frame */
	tstb	a0@(NULLFRAME)			/* check for null frame */
	jeq	so_null				/* if NULL skip saving state */
so_not_null:
	fmovemx	f0-f7,a0@(FPX)			/* save data registers */
	fmoveml	#<fpcr,fpsr,fpiar>,a0@(FPCR)	/* save control registers */

#else				/* FP LITERAL */
	.word	0xf328,0x006c			/* fsave   a0@(STATEFRAME) */
	tstb	a0@(NULLFRAME)
	jeq	so_null				/* if NULL skip saving state */
so_not_null:
	.word	0xf228,0xf0ff,0x000c		/* fmovemx f0-f7,a0@(FPX) */
	.word	0xf228,0xbc00,0x0000		/* fmoveml #<cregs.>,a0@(FPCR)*/
#endif
#endif			/* END MC680[0123]0 VERSION */

so_null:
	unlk	a6
	rts

/*******************************************************************************
*
* fppRestore - restore the floating-point coprocessor context
*
* This routine restores the floating-point coprocessor context.
* The context restored is:
*
*	- registers fpcr, fpsr, and fpiar
*	- registers f0 - f7
*	- internal state frame
*
* If the internal state frame is null, the other registers are not restored.
*
* RETURNS: N/A
*
* SEE ALSO: fppSave(), MC68881/MC68882 Floating-Point Coprocessor User's Manual

* void fppRestore
*    (
*    FP_CONTEXT *  pFpContext  /* from where to restore context *
*    )

*/

_fppRestore:
	link	a6,#0

	movel	a6@(ARG1),a0		/* from where to restore registers */
	tstb	a0@(NULLFRAME)		/* is it a null frame? */
	jeq	si_null			/* yes, so restore only state frame */

#if FPP_ASSEM

	fmovemx	a0@(FPX),f0-f7			/* restore data registers */
	fmoveml	a0@(FPCR),#<fpcr,fpsr,fpiar>	/* restore control registers */
si_null:
	frestore	a0@(STATEFRAME)		/* restore state frame */

#else

	.word	0xf228,0xd0ff,0x000c	/* fmovemx a0@(FPX),f0-f7 */
	.word	0xf228,0x9c00,0x0000	/* fmoveml a0@(FPCR),#<fpcr,fpsr,fpiar*/
si_null:
	.word	0xf368,0x006c		/* frestore a0@(STATEFRAME) */

#endif

	unlk	a6
	rts
/*******************************************************************************
*
* fppDtoDx - convert double to extended double precision
*
* The MC68881 uses a special extended double precision format
* (12 bytes as opposed to 8 bytes) for internal operations.
* The routines fppSave and fppRestore must preserve this precision.
*
* NOMANUAL

* void fppDtoDx (pDx, pDouble)
*     DOUBLEX *pDx;	 /* where to save result    *
*     double *pDouble;	 /* ptr to value to convert *

*/

_fppDtoDx:
	link	a6,#0

#if FPP_ASSEM
	fmovex	f0,a7@-		/* save regs */
	movel	a6@(ARG1),a1	/* to Dx */
	movel	a6@(ARG2),a0	/* from D */
	fmoved	a0@,f0		/* convert double */
	fmovex	f0,a1@		/* to extended */
	fmovex	a7@+,f0		/* restore regs */
#else
	.word	0xf227,0x6800	/* fmovex  f0,a7@- */
	movel	a6@(ARG1),a1
	movel	a6@(ARG2),a0
	.word	0xf210,0x5400	/* fmoved  a0@,f0 */
	.word	0xf211,0x6800	/* fmovex  f0,a1@ */
	.word	0xf21f,0x4800	/* fmovex  a7@+,f0 */
#endif

	unlk	a6
	rts
/*******************************************************************************
*
* fppDxtoD - convert extended double precisoion to double
*
* The MC68881 uses a special extended double precision format
* (12 bytes as opposed to 8 bytes) for internal operations.
* The routines fppSave and fppRestore must preserve this precision.
*
* NOMANUAL

* void fppDxtoD (pDouble, pDx)
*     double *pDouble;		/* where to save result    *
*     DOUBLEX *pDx;		/* ptr to value to convert *

*/

_fppDxtoD:
	link	a6,#0

#if FPP_ASSEM
	fmovex	f0,a7@-		/* save regs */
	movel	a6@(ARG1),a1	/* to D */
	movel	a6@(ARG2),a0	/* from Dx */
	fmovex	a0@,f0		/* convert extended */
	fmoved	f0,a1@		/* to double */
	fmovex	a7@+,f0		/* restore regs */
#else
	.word	0xf227,0x6800	/* fmovex  f0,a7@- */
	movel	a6@(ARG1),a1
	movel	a6@(ARG2),a0
	.word	0xf210,0x4800	/* fmovex  a0@,f0 */
	.word	0xf211,0x7400	/* fmoved  f0,a1@ */
	.word	0xf21f,0x4800	/* fmovex  a7@+,f0 */
#endif

	unlk	a6
	rts
/*******************************************************************************
*
* fppProbeSup - fppProbe support routine
*
* This routine executes some coprocessor instruction which will cause a
* bus error if a coprocessor is not present.  A handler, viz. fppProbeTrap,
* should be installed at that vector.  If the coprocessor is present this
* routine returns OK.
*
* SEE ALSO: MC68881/MC68882 User's Manual, page 5-15
*
* NOMANUAL

* STATUS fppProbeSup ()
*/

_fppProbeSup:
	link	a6,#0
	clrl	d0		/* set status to OK */
	nop
#if FPP_ASSEM
	fmovecr	#0,fp0		/* 040 unimplemented instruction */
#else
	.word	0xf200,0x5c00	/* fmovecr #0,fp0 */
#endif
	nop

fppProbeSupEnd:
	unlk    a6
	rts

/****************************************************************************
*
* fppProbeTrap - fppProbe support routine
*
* This entry point is momentarily attached to the coprocessor illegal opcode
* error exception vector.  Usually it simply sets d0 to ERROR to indicate that
* the illegal opcode error did occur, and returns from the interrupt.  The
* 68040/68060 version, however, stores the coprocessor version number for pseudo
* frame construction in fppSave, and leaves d0 with the value of OK.
*
* NOMANUAL
*/

_fppProbeTrap:			/* coprocessor illegal error trap */

#if ((CPU==MC68040) || (CPU==MC68060) || CPU==MC68LC040)
	movew	a7@(0x6),d0		/* get the vector offset from the esf */
	andw	#0xf000,d0		/* clear the format */
	cmpw	#0x4000,d0		/* format $4 means mc68EC040 */
	jeq	fppNone			/* which doesn't have a fpu */
	clrl	d0			/* set d0 back to OK */
	link	a6,#0			/* now we can link */
	fsave	a7@-			/* save the unimplemented frame */
	frestore fppNullStateFrame	/* reset FPCP */
	moveb	a7@,fppCPVersion	/* get the version number out of it */
	unlk	a6			/* clean up stack */
	movel	#fppProbeSupEnd,a7@(2)	/* change PC to bail out */
	rte				/* return to fppProbeSupEnd */
#endif	/* CPU==MC68040 || CPU==MC68060 || CPU==MC68LC040 */

fppNone:				/* 68020/68EC040 */
	movel	#-1,d0			/* set status to ERROR */
	movel	#fppProbeSupEnd,a7@(2)	/* change PC to bail out */
	rte				/* return to fppProbeSupEnd */

/******************************************************************************
*
* fpcrGet - get the value of the fpcr register
*
* RETURNS: the value of the fpcr register
*
* SEE ALSO: fpcrSet(), MC68881/MC68882 Floating-Point Coprocessor
* User's Manual

* int fpcrGet (void)

*/ 

	.globl _fpcrGet

_fpcrGet:
#if FPP_ASSEM
	fmovel	fpcr, d0
#else
	.word	0xf200, 0xb000
#endif
	rts

/******************************************************************************
*
* fpcrSet - set the value of the fpcr register
*
* The fpcr register controls which exceptions can be generated
* by the coprocessor. By default no exceptions can be generated.
* This routine can be used to enable some or all floating point
* excpetions.
*
* RETURNS: N/A
*
* SEE ALSO: fpcrGet(), MC68881/MC68882 Floating-Point Coprocessor
* User's Manual

* void fpcrSet
*    (
*    int	value
*    )

*/ 

	.globl _fpcrSet

_fpcrSet:
#if FPP_ASSEM
	fmovel	sp@(0x4), fpcr
#else
	.word	0xf22f, 0x9000, 0x0004
#endif
	rts
