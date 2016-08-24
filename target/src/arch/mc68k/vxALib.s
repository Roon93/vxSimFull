/* vxALib.s - miscellaneous assembly language routines */

/* Copyright 1984-1994 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01u,23aug95,ms   memProbe on invalid address no longer hangs the CPU32
01t,24mar95,ism  fixed BERR infinite loop problem in vxTas() (SPR#4118)
01s,31oct94,tmk  added MC68LC040 support
01r,30may94,tpr  added MC68060 cpu support.
		 tweak comments.
		 added vxSSEnable() and vxSSDisable().
01q,15sep92,jdi  ansified declarations for mangenable routines.
01p,14sep92,yao  documentation.
01o,23aug92,jcf  changed bxxx to jxx.
01n,26may92,rrr  the tree shuffle
01m,04oct91,rrr  passed through the ansification filter
		  -fixed #else and #endif
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01l,25sep91,yao  added support for CPU32.
01k,29aug91,JLF  added support for MC68040. added nop to force write queue
		 flush and immediate exception processing.
01j,28aug91,shl  added support for MC68040.
01i,30mar91,jdi  documentation cleanup; doc review by dnw.
01h,02aug90,jcf  removed vxIncrement() and vxDecrement().
01g,05may90,gae  removed import of sysKernelTrap().
01f,20dec89,jcf  added atomic operations vxIncrement(), and vxDecrement().
01e,10apr89,dab  fixed bug in vxTas() - changed bne to bmi.
01d,01sep88,gae  documentation.
01c,05jun88,dnw  changed from kALib to vxALib.
01b,30may88,dnw  changed to v4 names.
01a,15mar88,jcf  written and soon to be redistributed.
*/

/*
DESCRIPTION
This module contains miscellaneous VxWorks support routines.

SEE ALSO: vxLib
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"

	.text
	.even

	/* internals */

	.globl _vxMemProbeSup
	.globl _vxMemProbeTrap
	.globl _vxTas

	/* externals */

	.globl _exit
#if (CPU == MC68060)
	.globl _vxSSEnable
	.globl _vxSSDisable
#endif /* (CPU == MC68060) */

	.text
	.even

/*******************************************************************************
*
* vxMemProbeSup - vxMemProbe support routine
*
* This routine is called to try to read byte, word, or long, as specified
* by length, from the specified source to the specified destination.
*
* NOMANUAL

STATUS vxMemProbeSup (length, src, dest)
    (
    int 	length,	// length of cell to test (1, 2, 4) *
    char *	src,	// address to read *
    char *	dest	// address to write *
    )

*/

_vxMemProbeSup:
	link	a6,#0

	movel	a6@(ARG2),a0	/* get source address */
	movel	a6@(ARG3),a1	/* get destination address */

	clrl	d0		/* preset status = OK */

	movel	a6@(ARG1),d1	/* get length */
	cmpl	#1,d1
	jne	vmp10
	nop                     /* force write queue flush first */
	moveb	a0@,a1@		/* move byte */
	nop			/* force immediate exception processing */
	jra	vmpRtn

vmp10:
	cmpl	#2,d1
	jne	vmp20
	nop                     /* force write queue flush first */
	movew	a0@,a1@		/* move word */
	nop                     /* force immediate exception processing */
	jra	vmpRtn

vmp20:
	nop                     /* force write queue flush first */
	movel	a0@,a1@		/* move long */
	nop                     /* force immediate exception processing */

	/* 
	 * NOTE: vmpRtn is known by vxMemProbTrap for 68000 because 68000
	 * can't know where to return exactly.
	 */
vmpRtn:
	unlk	a6
	rts

/*******************************************************************************
*
* vxMemProbeTrap - vxMemProbe support routine
*
* This entry point is momentarily attached to the bus error exception
* vector.  It simply sets d0 to ERROR to indicate that the bus error did
* occur, and returns from the interrupt.
*
* 68010 & 68020 NOTE:
* The instruction that caused the bus error must not be run again so we
* have to set some special bits in the exception stack frame.
*
* 68000 NOTE:
* On the 68000, the pc in the exception stack frame is NOT necessarily
* the address of the offending instruction, but is merely "in the vicinity".
* Thus the 68000 version of this trap has to patch the exception stack
* frame to return to a known address before doing the RTE.
*
* NOMANUAL
*/

_vxMemProbeTrap:		/* we get here via the bus error trap */

#if (CPU==MC68000)
	addql	#8,a7		/* throw away extra bus error info on stack */
	movel	#vmpRtn,a7@(2)	/* patch return address (see note above) */
#endif

#if (CPU==MC68010)
	/* 
	 * The special status word needs to have the rr bit set, only
	 * on the 68010.  This prevents the offending instruction from
	 * being run again.
	 */

	moveb	#0x80,d0	/* rr bit is the upper bit */
	orb	d0,a7@(8)	/* Set it in special status register */
#endif

#if (CPU==MC68020)
	/* 
	 * In the 68020, we reset the RC and RB flags of the special
	 * status word to prevent the bus cycle from being re-run.
	 * We'll also reset the DF flag just in case
	 */

	moveb	#0xce,d0	/* reset bits 4, 5 and 0 */
	andb	d0,a7@(0xa)	/* ssw is always at offset 0xa */
#endif

#if (CPU==CPU32)
	/* 
	 * In the CPU32, we reset the RR flag of the special status word
	 * to prevent the bus cycle from being re-run.
	 */

	moveb	#0xfd,d0	/* reset bit 9 of ssw */
	andb	d0,a7@(0x16)	/* ssw is always at offset 0x16 */
#endif	/* CPU==CPU32 */

#if ((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040)) || (CPU==CPU32)
	movel	#vmpRtn,a7@(2)	/* patch return address (see note above) */
#endif	/* ((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040)) */

	movel	#-1,d0		/* set status to ERROR */
	rte			/* return to the subroutine */
/*******************************************************************************
*
* vxTas - C-callable atomic test-and-set primitive
*
* This routine provides a C-callable interface to the 680x0 test-and-set
* instruction.  The "tas" instruction is executed on the specified
* address.
*
* RETURNS:
* TRUE if the value had not been set, but now is;
* FALSE if the value was already set.

* BOOL vxTas 
*     (
*     void *	address		/* address to be tested *
*     )

*/
_vxTas:
	moveq	#0,d0
	movel	sp@(4),a0
#if (CPU==MC68000 || CPU==MC68010 || CPU==MC68020 || CPU==CPU32)
	tas	a0@
	jmi	vxT1
	moveq	#1,d0
vxT1:	rts

#endif	/* (CPU==MC68000 || CPU==MC68010 || CPU==MC68020 || CPU==CPU32) */

#if ((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040))
vxT1r:
	moveq   #0,d0		/* make sure d0 is cleared  for BERR test */
	nop			/* NOP used to flush pended writes */
	tas	a0@
	nop			/* and to force immediate exception proc */
	jmi 	vxT1

	tstl	d0		/* a BERR set b31 of d0 */
	jmi 	vxT1r
	moveq	#1,d0
	rts
vxT1:
	tstl	d0		/* a BERR set b31 of d0 */
	jmi 	vxT1r
	rts
#endif	/* ((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040)) */

#if (CPU == MC68060)
/*******************************************************************************
*
* vxSSEnable - Enable the superscalar dispatch (MC68060 only)
*
* This function sets the ESS bit of the Processor Configuration Register (PCR)
* to enable the superscalar dispatch.
*
* RETURNS : N/A

* void vxSSEnable (void)

*/

_vxSSEnable:
	.word	0x4e7a,0x0808	/* movec pcr,d0 */
	oril	#0x00000001,d0	/* enable superscalar dispatcher */
	.word	0x4e7b,0x0808	/* movec d0,pcr */
	rts			/* return */

/*******************************************************************************
*
* vxSSDisable - disable the superscalar dispatch (MC68060 only)
*
* This function resets the ESS bit of the Processor Configuration Register (PCR)
* to disable the superscalar dispatch.
*
* RETURNS : N/A

* void vxSSDisable (void)

*/

_vxSSDisable:
	.word	0x4e7a,0x0808	/* movec pcr,d0 */
	andil	#0xfffffffe,d0	/* disable superscalar dispatch */
	.word	0x4e7b,0x0808	/* movec d0,pcr */
	rts			/* return */

#endif /* (CPU == MC68060) */
