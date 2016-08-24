/* mathHardALib.s - */

/* Copyright 1984-1992 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01v,29jun94,tpr  added MC68060 cpu support.
01u,29oct92,jcf  MC68020/MC68040 now both use assembly code.  fixed SPR #1422.
01t,23aug92,jcf  changed bxxx to jxx.
01s,26may92,rrr  the tree shuffle
01r,20apr92,kdl  Fixed cut/paste typo in .word version of mathHardIrint
		 (SPR #1422).
01q,20jan92,kdl  Put in conditional code so 68040 calls emulation library
		 for missing fp instructions; change fp register naming
		 from "fN" to "fpN"; use fp0 (not fp7) as scratch register.
01p,04oct91,rrr  passed through the ansification filter
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01o,27aug91,wmd	 added #include "vxWorks.h".
01n,23jul91,gae  changed 68k/asm.h to asm.h.
01m,13jun91,kdl	 fixed offset problems when dereferencing multiple double
		 arguments; now uses new "DARGx" symbolic offsets; removed
		 cut/paste artifact "ftanhd" call from mathHardSincos.
01l,28jan91,kdl	 changed name from "mathALib.s" to "mathHardALib.s";
		 all functions renamed with "mathHard" prefix, to be
		 called via jumps from the new "mathALib.s".
01k,27oct90,dnw  changed to include "68k/asm.h" explicitly.
01j,23sep90,elr  Renamed rmod to fmod to conform to ANSI.
	   +mau	 Added atan2, floor, infinity to conform to ANSI.
                 Removed entier - function was never documented, used, or
		   understood.  Included errno.h.
		 Added #else (ifndef) MATH_ASM throughout this module module.
01i,14mar90,jdi  documentation cleanup.
01h,10feb89,gae  fixed d1 bug in all functions.
	   +pxm  added sincos, rmod, floor, ceil, trunc, round, entier
                 irint & iround.
01g,19aug88,gae  documentation.
01f,28may88,dnw  removed extra spaces in .word declarations that may have
		   been screwing up the VMS port.
01e,13feb88,dnw  added .data before .asciz above, for Intermetrics assembler.
01d,04dec87,gae  removed most HOST_TYPE dependencies - fp codes hand assembled.
01c,23nov87,gae  added included asm.h and special HOST_IRIS version.
01b,18nov87,jlf  documentation and alphabetized
01a,16nov87,terry arden + rdc   written
*/

/*
DESCRIPTION
This library provides a C interface to the high-level math functions
on the MC68881/MC68882 floating-point coprocessor.  All angle-related
parameters and return values are expressed in radians.  Functions
capable errors, will set errno upon an error. All functions
included in this library whos names correspond to the ANSI C specification
are, indeed, ANSI-compatable. In the spirit of ANSI, HUGE_VAL is now
supported.

WARNING
This library works only if an MC68881/MC68882 coprocessor is in the
system! Attempts to use these routines with no coprocessor present
will result in illegal instruction traps.

SEE ALSO:
fppLib (1), floatLib (1), The C Programming Language - Second Edition

INCLUDE FILE: math.h

INTERNAL
Each routine has the following format:
    o save register f7
    o calculate floating-point function using double parameter
    o transfer result to parameter storage
    o store result to d0 and d1 registers
    o restore old register f7
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "errno.h"

#if (CPU == MC68060)
#include "fplsp060Lib.h"
#endif /* (CPU == MC68060) */

        .text
	.even

        .globl  _mathHardTanh
        .globl  _mathHardCosh
        .globl  _mathHardSinh
        .globl  _mathHardLog2
        .globl  _mathHardLog10
        .globl  _mathHardLog
        .globl  _mathHardExp
        .globl  _mathHardAtan
	.globl  _mathHardAtan2
        .globl  _mathHardAcos
        .globl  _mathHardAsin
        .globl  _mathHardTan
        .globl  _mathHardCos
        .globl  _mathHardSin
        .globl  _mathHardPow
        .globl  _mathHardSqrt
        .globl  _mathHardFabs
	.globl  _mathHardFmod

	.globl  _mathHardSincos
	.globl  _mathHardFloor
	.globl  _mathHardCeil
	.globl  _mathHardTrunc
	.globl  _mathHardRound
	.globl  _mathHardIround
	.globl  _mathHardIrint
	.globl  _mathHardInfinity

/*******************************************************************************
*
* mathHardAcos - ANSI-compatable hardware floating-point arc-cosine
*
* RETURNS: The arc-cosine in the range -pi/2 to pi/2 radians.

* double mathHardAcos (dblParam)
*     double dblParam;	/* angle in radians *

*
*/

_mathHardAcos:
	link    a6,#0
	fmovex	fp0,sp@-
#if (CPU==MC68040 || CPU==MC68060)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
#if (CPU == MC68040)
	jsr	__l_facosd		/* returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FACOSD_	/* returns in fp0 */
#endif /* (CPU == MC68060) */
	addql	#8,sp
#else
	facosd	a6@(DARG1),fp0
#endif
	fmoved	fp0,a6@(DARG1)
	movel   a6@(DARG1),d0
	movel   a6@(DARG1L),d1
	fmovex	sp@+,fp0
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardAsin - ANSI-compatable hardware floating-point arc-sine
*
* RETURNS: The arc-sine in the range 0.0 to pi radians.
*
* SEE ALSO:
* floatLib (1), "The C Programming Language - Second Edition"

* double mathHardAsin (dblParam)
*     double dblParam;	/* angle in radians *

*
*/

_mathHardAsin:
	link    a6,#0
	fmovex  fp0,sp@-
#if (CPU==MC68040 || CPU==MC68060)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
#if (CPU == MC68040)
	jsr	__l_fasind		/* returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FASIND_	/* returns in fp0 */
#endif /* (CPU == MC68060) */
	addql	#8,sp
#else
	fasind  a6@(DARG1),fp0
#endif
	fmoved  fp0,a6@(DARG1)
	movel   a6@(DARG1),d0
	movel   a6@(DARG1L),d1
	fmovex  sp@+,fp0
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardAtan - ANSI-compatable hardware floating-point arc-tangent
*
* RETURNS: The arc-tangent of dblParam in the range -pi/2 to pi/2.
*
* SEE ALSO: floatLib (1), acos (2), asin (2)

* double mathHardAtan (dblParam)
*     double dblParam;	/* angle in radians *

*
*/

_mathHardAtan:
	link    a6,#0
	fmovex  fp0,sp@-
#if (CPU==MC68040 || CPU==MC68060)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
#if (CPU == MC68040)
	jsr	__l_fatand		/* returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FATAND_	/* returns in fp0 */
#endif /* (CPU == MC68060) */
	addql	#8,sp
#else
	fatand  a6@(DARG1),fp0
#endif
	fmoved  fp0,a6@(DARG1)
	movel   a6@(DARG1),d0
	movel   a6@(DARG1L),d1
	fmovex  sp@+,fp0
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardAtan2 - hardware floating point function for arctangent of (dblY/dblX)
*
* RETURNS:
*    The arc-tangent of (dblY/dblX) in the range -pi to pi.
*
* SEE ALSO:
* floatLib (1), "The C Programming Language - Second Edition"

* double mathHardAtan2 (dblY, dblX)
*     double dblY;		/* Y *
*     double dblX;		/* X *

*
*/

_mathHardAtan2:
	link	a6, #0
	fmoved	a6@(DARG1), fp0		/* y */
	fmoved	a6@(DARG2), fp1		/* x */
	fdivx	fp1,fp0			/* y/x */
	fatanx	fp0, fp0		/* atan (y/x) */
	ftstx	fp1			/* x */
	fbge	atan2Exit		/* x >= 0 */
	fmovecr	#0, fp1			/* pi */
	ftstd	a6@(DARG1)		/* y  */
	fblt	atan2NegY 		/* y < 0 ? */
atan2PosY:
	faddx	fp1, fp0		/* atan(y/x) + pi */
	jra	atan2Exit
atan2NegY:
	fsubx	fp1, fp0		/* atan(y/x) - pi */
atan2Exit:
	fmoved	fp0, a6@(DARG1)		/* return result in d0:d1 */
	movl	a6@(DARG1), d0
	movl	a6@(DARG1L), d1
	unlk	a6
	rts

/*******************************************************************************
*
* mathHardCos - ANSI-compatable hardware floating-point cosine
*
* RETURNS: the cosine of the radian argument dblParam
*
* SEE ALSO:
* floatLib (1), sin (2), cos (2), tan(2),
* "The C Programming Language - Second Edition"

* double mathHardCos (dblParam)
*     double dblParam;	/* angle in radians *

*
*/
_mathHardCos:
	link    a6,#0
	fmovex  fp0,sp@-
#if (CPU==MC68040 || CPU==MC68060)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
#if (CPU == MC68040)
	jsr	__l_fcosd		/* returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FCOSD_	/* returns in fp0 */
#endif /* (CPU == MC68060) */
	addql	#8,sp
#else
	fcosd   a6@(DARG1),fp0
#endif
	fmoved  fp0,a6@(DARG1)
	movel   a6@(DARG1),d0
	movel   a6@(DARG1L),d1
	fmovex  sp@+,fp0
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardCosh - ANSI-compatable hardware floating-point hyperbolic cosine
*
* RETURNS:
*    The hyperbolic cosine of dblParam if (dblParam > 1.0), or
*    NaN if (dblParam < 1.0)
*
* SEE ALSO: "The C Programming Language - Second Edition"

* double mathHardCosh (dblParam)
*     double dblParam;	/* angle in radians *

*
*/

_mathHardCosh:
	link    a6,#0
	fmovex  fp0,sp@-
#if (CPU==MC68040 || CPU==MC68060)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
#if (CPU == MC68040)
	jsr	__l_fcoshd		/* returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FCOSHD_	/* returns in fp0 */
#endif /* (CPU == MC68060) */
	addql	#8,sp
#else
	fcoshd  a6@(DARG1),fp0
#endif
	fmoved  fp0,a6@(DARG1)
	movel   a6@(DARG1),d0
	movel   a6@(DARG1L),d1
	fmovex  sp@+,fp0
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardExp - hardware floating-point exponential function
*
* RETURNS:
*    Floating-point inverse natural logarithm (e ** (dblExponent)).
*
* SEE ALSO:
* floatLib (1), "The C Programming Language - Second Edition"

* double mathHardExp (dblExponent)
*     double dblExponent;	/* argument *

*
*/

_mathHardExp:
	link    a6,#0
	fmovex  fp0,sp@-
#if (CPU==MC68040 || CPU==MC68060)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
#if (CPU == MC68040)
	jsr	__l_fetoxd		/* returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FETOXD_	/* returns in fp0 */
#endif /* (CPU == MC68060) */
	addql	#8,sp
#else
	fetoxd  a6@(DARG1),fp0
#endif
	fmoved  fp0,a6@(DARG1)
	movel   a6@(DARG1),d0
	movel   a6@(DARG1L),d1
	fmovex  sp@+,fp0
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardFabs - ANSI-compatable hardware floating-point absolute value
*
* RETURNS: The floating-point absolute value of dblParam.
*
* SEE ALSO:
* floatLib (1), "The C Programming Language - Second Edition"

* double mathHardFabs (dblParam)
*     double dblParam;	/* argument *

*
*/

_mathHardFabs:
	link    a6,#0
	fmovex  fp0,sp@-
#if (CPU==MC68040)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
	jsr	__l_fabsd		/* returns in fp0 */
	addql	#8,sp
#else
	fabsd   a6@(DARG1),fp0
#endif
	fmoved  fp0,a6@(DARG1)
	movel   a6@(DARG1),d0
	movel   a6@(DARG1L),d1
	fmovex  sp@+,fp0
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardLog - ANSI-compatable hardware floating-point natural logarithm
*
* RETURNS: The natural logarithm of dblParam.
*
* SEE ALSO:
* floatLib (1), "The C Programming Language - Second Edition"

* double mathHardLog (dblParam)
*     double dblParam;	/* argument *

*
*/

_mathHardLog:
	link    a6,#0
	fmovex  fp0,sp@-
#if (CPU==MC68040 || CPU==MC68060)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
#if (CPU == MC68040)
	jsr	__l_flognd		/* returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FLOGND_	/* returns in fp0 */
#endif /* (CPU == MC68060) */
	addql	#8,sp
#else
	flognd  a6@(DARG1),fp0
#endif
	fmoved  fp0,a6@(DARG1)
	movel   a6@(DARG1),d0
	movel   a6@(DARG1L),d1
	fmovex  sp@+,fp0
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardLog10 - ANSI-compatable hardware floating-point base 10 logarithm
*
* RETURNS: The logarithm (base 10) of dblParam.
*
* SEE ALSO: floatLib (1), log2 (2)

* double mathHardLog10 (dblParam)
*     double dblParam;	/* argument *

*
*/

_mathHardLog10:
	link    a6,#0
	fmovex  fp0,sp@-
#if (CPU==MC68040 || CPU==MC68060)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
#if (CPU == MC68040)
	jsr	__l_flog10d		/* returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FLOG10D_	/* returns in fp0 */
#endif /* (CPU == MC68060) */
	addql	#8,sp
#else
	flog10d a6@(DARG1),fp0
#endif
	fmoved  fp0,a6@(DARG1)
	movel   a6@(DARG1),d0
	movel   a6@(DARG1L),d1
	fmovex  sp@+,fp0
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardLog2 - ANSI-compatable hardware floating-point logarithm base 2
*
* RETURNS: The logarithm (base 2) of dblParam.
*
* SEE ALSO: floatLib (1), log10 (2)

* double mathHardLog2 (dblParam)
*     double dblParam;	/* argument *

*
*/

_mathHardLog2:
	link    a6,#0
	fmovex  fp0,sp@-
#if (CPU==MC68040 || CPU==MC68060)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
#if (CPU == MC68040)
	jsr	__l_flog2d		/* returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FLOG2D_	/* returns in fp0 */
#endif /* (CPU == MC68060) */
	addql	#8,sp
#else
	flog2d  a6@(DARG1),fp0
#endif
	fmoved  fp0,a6@(DARG1)
	movel   a6@(DARG1),d0
	movel   a6@(DARG1L),d1
	fmovex  sp@+,fp0
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardPow - ANSI-compatable hardware floating-point power function
*
* RETURNS: The floating-point value of dblX to the power of dblY.
*
* SEE ALSO: floatLib (1), sqrt (2)

* double mathHardPow (dblX, dblY)
*     double dblX;	/* X *
*     double dblY;	/* X *

*
*/

_mathHardPow: 				/* pow (x,y) = exp (y * log(x)) */
	link	a6, #0
	fmoved	a6@(DARG1), fp0		/* x */
	fble	powNonPosX		/* is (x =< 0) ? */

powPosX:				/* x > 0 */
#if (CPU==MC68040 || CPU==MC68060)
	fmovex	fp0,sp@-
#if (CPU == MC68040)
	jsr	__l_flognx		/* ln(x)  -- returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FLOGNX_	/* ln(x)  -- returns in fp0 */
#endif /* (CPU == MC68060) */
	addql	#8,sp
	fmuld	a6@(DARG2), fp0		/* y * ln(x) */
	fmovex	fp0,sp@-
#if (CPU == MC68040)
	jsr	__l_fetoxx		/* exp( y * ln(x))  -- returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FETOXX_	/* exp( y * ln(x))  -- returns in fp0 */
#endif /* (CPU == MC68060) */
	addql	#8,sp
#else
    	flognx	fp0, fp0		/* ln(x) */
	fmuld	a6@(DARG2), fp0		/* y * ln(x) */
	fetoxx	fp0, fp0		/* exp( y * ln(x) ) */
#endif
	jra	powExit			/* finished */

powNonPosX:				/* x =< 0 */
	fbne	powNegX			/* is (x < 0) ? */

powZeroX:				/* x == 0 */
	ftstd	a6@(DARG2)		/* y */
	fbgt	powExit		 	/* if (y>0), 0^(y>0) --> 0 */
	fbeq	powNaN			/* if (y==0), 0^0 --> undefined */
	fmovecr	#0x3f, fp0		/* if (y<0), --> HUGE_VAL */
	jra	powErrExit

powNegX:
#if (CPU==MC68040)
	fmovex	fp0,sp@-
	jsr	__l_fabsx		/* returns in fp0 */
	addql	#8,sp
	fmovex	fp0,sp@-
	jsr	__l_flognx		/* ln (|x|)  -- returns in fp0 */
	addql	#8,sp
	fmovex	fp0,sp@-		/* save fp0 temporarily */
	movel	a6@(DARG2L),sp@-
	movel	a6@(DARG2),sp@-
	jsr	__l_fintd		/* returns in fp0 */
	addql	#8,sp
	fmovex	fp0,fp1			/* copy result to fp1 */
	fmovex	sp@+,fp0		/* restore fp0 */
#else
	fabsx	fp0, fp0
#if (CPU == MC68060)
	fmovex	fp0,sp@-
	jsr	FPLSP_060__FLOGNX_	/* ln(|x|) */
	addql	#8,sp
#else	/* (CPU == MC68060) */
    	flognx	fp0, fp0		/* ln(|x|) */
#endif	/* (CPU == MC68060) */
	fintd	a6@(DARG2), fp1
#endif

	fcmpd	a6@(DARG2), fp1		/* y == int(y) ? */
	fbne	powNaN
	fmulx	fp1, fp0		/* y * ln(|x|) */

#if (CPU==MC68040 || CPU==MC68060)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
#if (CPU == MC68040)
	jsr	__l_fetoxx		/* exp(y * ln(|x|)) -- returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FETOXX_	/* exp(y * ln(|x|)) -- returns in fp0 */
#endif /* (CPU == MC68060) */
	addql	#8,sp
#else
	fetoxx	fp0, fp0		/* exp( y * ln(|x|) ) */
#endif

	fmodw	#2, fp1			/* modulus 2 */
	fbeq	powExit			/* All done */
pow_odd_y:
	fnegx	fp0, fp0		/* inverse mantissa (negate) */
	jra	powExit

powNaN:
	fmovecr	#0x0f, fp0		/* Zero fp register */
	jra	powErrExit

powErrExit:
	pea	EDOM			/* Domain Error	*/
	jsr	_errnoSet		/* Set the error */
	addqw	#0x4, sp		/* tidy up */

powExit:
	fmoved	fp0, a6@(DARG1)
	movl	a6@(DARG1), d0		/* return result in d0:d1 */
	movl	a6@(DARG1L), d1
	unlk	a6
	rts

/*******************************************************************************
*
* mathHardSin - ANSI-compatable hardware floating-point sine
*
* RETURNS: The floating-point sine of dblParam.
*
* SEE ALSO:
*   floatLib (1), cos (2), tan (2),
*   "The C Programming Language - Second Edition"

* double mathHardSin (dblParam)
*     double dblParam;	/* angle in radians *

*
*/

_mathHardSin:
	link    a6,#0
	fmovex  fp0,sp@-
#if (CPU==MC68040 || CPU==MC68060)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
#if (CPU == MC68040)
	jsr	__l_fsind		/* returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FSIND_	/* returns in fp0 */
#endif /* (CPU == MC68060) */
	addql	#8,sp
#else
	fsind   a6@(DARG1),fp0
#endif
	fmoved  fp0,a6@(DARG1)
	movel   a6@(DARG1),d0
	movel   a6@(DARG1L),d1
	fmovex  sp@+,fp0
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardSinh - ANSI-compatable hardware floating-point hyperbolic sine
*
* RETURNS: The floating-point hyperbolic sine of dblParam.
*
* SEE ALSO:
* floatLib (1), "The C Programming Language - Second Edition"

* double mathHardSinh (dblParam)
*     double dblParam;	/* angle in radians *

*
*/
_mathHardSinh:
	link    a6,#0
	fmovex  fp0,sp@-
#if (CPU==MC68040 || CPU==MC68060)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
#if (CPU == MC68040)
	jsr	__l_fsinhd		/* returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FSINHD_	/* returns in fp0 */
#endif /* (CPU == MC68060) */
	addql	#8,sp
#else
	fsinhd  a6@(DARG1),fp0
#endif
	fmoved  fp0,a6@(DARG1)
	movel   a6@(DARG1),d0
	movel   a6@(DARG1L),d1
	fmovex  sp@+,fp0
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardSqrt - ANSI-compatable hardware floating-point square root
*
* RETURNS: The floating-point square root of dblParam.
*
* SEE ALSO: floatLib(1), pow (2)

* double mathHardSqrt (dblParam)
*     double dblParam;	/* argument *

*
*/

_mathHardSqrt:
	link    a6,#0
	fmovex  fp0,sp@-
#if (CPU==MC68040)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
	jsr	__l_fsqrtd		/* returns in fp0 */
	addql	#8,sp
#else
	fsqrtd  a6@(DARG1),fp0
#endif
	fmoved  fp0,a6@(DARG1)
	movel   a6@(DARG1),d0
	movel   a6@(DARG1L),d1
	fmovex  sp@+,fp0
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardTan - ANSI-compatable hardware floating-point tangent
*
* RETURNS: Floating-point tangent of dblParam.
*
* SEE ALSO: floatLib (1), cos (2), sin (2),
* "The C Programming Language - Second Edition"

* double mathHardTan (dblParam)
*     double dblParam;	/* angle in radians *

*
*/

_mathHardTan:
	link    a6,#0
	fmovex  fp0,sp@-
#if (CPU==MC68040 || CPU==MC68060)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
#if (CPU == MC68040)
	jsr	__l_ftand		/* returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FTAND_	/* returns in fp0 */
#endif /* (CPU == MC68060) */
	addql	#8,sp
#else
	ftand   a6@(DARG1),fp0
#endif
	fmoved  fp0,a6@(DARG1)
	movel   a6@(DARG1),d0
	movel   a6@(DARG1L),d1
	fmovex  sp@+,fp0
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardTanh - ANSI-compatable hardware floating-point hyperbolic tangent
*
* RETURNS: Floating-point hyperbolic tangent of dblParam.
*
* SEE ALSO:
* floatLib (1), cosh (2), sinh (2)
* "The C Programming Language - Second Edition"

* double mathHardTanh (dblParam)
*     double dblParam;	/* angle in radians *

*
*/

_mathHardTanh:
	link    a6,#0
	fmovex  fp0,sp@-
#if (CPU==MC68040 || CPU==MC68060)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
#if (CPU == MC68040)
	jsr	__l_ftanhd		/* returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FTANHD_	/* returns in fp0 */
#endif /* (CPU == MC68060) */
	addql	#8,sp
#else
	ftanhd  a6@(DARG1),fp0
#endif
	fmoved  fp0,a6@(DARG1)
	movel   a6@(DARG1),d0
	movel   a6@(DARG1L),d1
	fmovex  sp@+,fp0
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardSincos - simultaneous hardware floating-point sine and cosine
*
* RETURNS:
* The simultaeous floating point results of sine and cosine of the
* radian argument  The dblParam must be in range of -1.0 to +1.0.
*
* CAVEAT:
* Supported for the MC68881/68882 only.
*
* SEE ALSO: floatLib (1), "MC68881/68882 Floating-Point User's Manual"

* VOID mathHardSincos (dblParam, sinResult, cosResult)
*     double dblParam;		/* angle in radians *
*     double *sinResult;	/* sine result buffer *
*     double *cosResult;	/* cosine result buffer *

*
*/

_mathHardSincos:
	link    a6,#0
#if (CPU == MC68060)
	fmovex	 fp0,sp@-
	fmovex	 fp1,sp@-
#else /* (CPU == MC68060) */
	fmovemx  fp0/fp1,sp@-
#endif /* (CPU == MC68060) */

#if (CPU==MC68040 || CPU==MC68060)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
#if (CPU == MC68040)
	jsr	__l_fcosd		/* returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FCOSD_	/* returns in fp0 */
#endif /* (CPU == MC68060) */
	fmovex	fp0,fp1			/* put cosine in fp1 */
	addql	#8,sp

	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
#if (CPU == MC68040)
	jsr	__l_fsind		/* get sine in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FSIND_	/* get sine in fp0 */
#endif /* (CPU == MC68060) */
	addql	#8,sp
#else
	fsincosd  a6@(DARG1),fp1:fp0	/* fp0 gets sine; fp1 gets cosine */
#endif

	movel   a6@(ARG3),a0		/* because of double, it's like ARG3 */
	fmoved  fp0,a0@			/* copy sine result */
	movel   a6@(ARG4),a0
	fmoved  fp1,a0@			/* copy cosine result */
#if (CPU == MC68060)
	fmovex	sp@+,fp1
	fmovex	sp@+,fp0
#else /* (CPU == MC68060) */
	fmovemx sp@+,fp0/fp1
#endif /* (CPU == MC68060) */
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardFmod - ANSI-compatable hardware floating-point modulus
*
* RETURNS:
* Floating-point modulus of (dblParam / dblDivisor) with the sign of dblParam.
*
* SEE ALSO:
* floatLib (1), "The C Programming Language - Second Edition"

* double mathHardFmod (dblParam, dblDivisor)
*     double dblParam;		/* argument *
*     double dblDivisor;	/* divisor *

*
*/

_mathHardFmod:
	link    a6,#0
	fmovex  fp0,sp@-
#if (CPU==MC68040 || CPU==MC68060)
	movel	a6@(DARG2L),sp@-
	movel	a6@(DARG2),sp@-
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
#if (CPU == MC68040)
	jsr	__l_fmodd		/* returns in fp0 */
#else /* (CPU == MC68060) */
	jsr	FPLSP_060__FMODD_	/* returns in fp0 */
#endif /* (CPU == MC68060) */
	addl	#16,sp
#else
	fmoved  a6@(DARG1),fp0
	fmodd   a6@(DARG2),fp0
#endif
	fmoved  fp0,a6@(DARG1)
	movel   a6@(DARG1),d0
	movel   a6@(DARG1L),d1
	fmovex  sp@+,fp0
	unlk    a6
	rts

/* MC68881/68882 Rounding modes */

#define	tonearest	0x0
#define	tozero		0x10
#define	minusinf	0x20
#define	plusinf		0x30

/*******************************************************************************
*
* mathHardFloor - ANSI-compatable hardware floating-point floor
*
* Performs a 'round-to-negative-infinity'.
*
* RETURNS:
* The largest integral value less than or equal to dblParam,
* result is returned in double precision.
*
* SEE ALSO:
* floatLib (1), "The C Programming Language - Second Edition"

* double mathHardFloor (dblParam)
*     double dblParam;	/* argument *

*
*/

_mathHardFloor:
	link    a6,#0
	fmovex  fp0,sp@-
	fmovel  fpcr,d0		/* set FPCR for round-to-minus-infinity */
        movel   d0,d1
        movb    #minusinf,d1
        fmovel  d1,fpcr
#if (CPU==MC68040)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
	jsr	__l_fintd		/* returns in fp0 */
	addql	#8,sp
#else
        fintd   a6@(DARG1),fp0
#endif
        fmoved  fp0,a6@(DARG1)
        fmovex  sp@+,fp0
        fmovel  d0,fpcr
        movel   a6@(DARG1),d0
        movel   a6@(DARG1L),d1
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardCeil - ANSI-compatable hardware floating-point ceiling
*
* Performs a 'round-to-positive-infinity'
*
* RETURNS:
* The least integral value greater than or equal to dblParam,
* result is returned in double precision.
*
* SEE ALSO:
* floatLib (1), "The C Programming Language - Second Edition"

* double mathHardCeil (dblParam)
*     double dblParam;	/* argument *

*
*/

_mathHardCeil:
	link    a6,#0
	fmovex  fp0,sp@-
	fmovel  fpcr,d0		/* set FPCR for round-to-plus-infinity */
        movel   d0,d1
        movb    #plusinf,d1
        fmovel  d1,fpcr
#if (CPU==MC68040)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
	jsr	__l_fintd		/* returns in fp0 */
	addql	#8,sp
#else
        fintd   a6@(DARG1),fp0
#endif
        fmoved  fp0,a6@(DARG1)
        fmovex  sp@+,fp0
        fmovel  d0,fpcr
        movel   a6@(DARG1),d0
        movel   a6@(DARG1L),d1
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardTrunc - hardware floating-point truncation
*
* Performs FINTRZ.
*
* RETURNS:
* The integer portion of a double-precision number,
* result is in double-precision.
*
* SEE ALSO: floatLib (1)

* double mathHardTrunc (dblParam)
*     double dblParam;	/* argument *

*
*/

_mathHardTrunc:
	link    a6,#0
	fmovex  fp0,sp@-
#if (CPU==MC68040)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
	jsr	__l_fintrzd		/* returns in fp0 */
	addql	#8,sp
#else
	fintrzd a6@(DARG1),fp0
#endif
        fmoved  fp0,a6@(DARG1)
        fmovex  sp@+,fp0
        movel   a6@(DARG1),d0
        movel   a6@(DARG1L),d1
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardRound - hardware floating-point rounding
*
* Performs a 'round-to-nearest'.
*
* SEE ALSO: floatLib (1)

* double mathHardRound (dblParam)
*     double dblParam;	/* argument *

*
*/

_mathHardRound:
	link    a6,#0
	fmovex  fp0,sp@-
	fmovel  fpcr,d0		/* set FPCR for round-to-nearest */
        movel   d0,d1
        movb    #tonearest,d1
        fmovel  d1,fpcr
#if (CPU==MC68040)
	movel	a6@(DARG1L),sp@-
	movel	a6@(DARG1),sp@-
	jsr	__l_fintd		/* returns in fp0 */
	addql	#8,sp
#else
        fintd   a6@(DARG1),fp0
#endif
        fmoved  fp0,a6@(DARG1)
        fmovex  sp@+,fp0
        fmovel  d0,fpcr
        movel   a6@(DARG1),d0
        movel   a6@(DARG1L),d1
	unlk    a6
	rts

/*******************************************************************************
*
* mathHardIround - hardware floating-point rounding to nearest integer
*
* Performs a 'round-to-the-nearest' function.
*
* NOTE:
* If dblParam is spaced evenly between two integers,
* then the  even integer will be returned.

* int mathHardIround (dblParam)
*     double dblParam;	/* argument *

*
*/

_mathHardIround:
	link    a6,#0
	fmovex  fp0,sp@-
	fmovel  fpcr,d0		/* set FPCR for round-to-the-nearest */
        movel   d0,d1
        movb    #tonearest,d0
        fmovel  d0,fpcr
        fmoved  a6@(DARG1),fp0
        fmovel  fp0,d0
        fmovex  sp@+,fp0
        fmovel  d1,fpcr
	unlk    a6
	rts

/********************************************************************************
* mathHardIrint - hardware floating-point double to integer conversion
*
* Convert dblParam to an integer using the selected IEEE
* rounding direction.
*
*
* SEE ALSO: floatLib (1)

* int mathHardIrint (dblParam)
*     double dblParam;	/* argument *

*
*/

_mathHardIrint:
        link    a6,#0
	fmovex  fp0,sp@-
        fmoved  a6@(DARG1),fp0
        fmovel  fp0,a6@(DARG1)
        movel   a6@(DARG1),d0
        fmovex  sp@+,fp0
        unlk    a6
        rts

/********************************************************************************
* mathHardInfinity - hardware floating-point return of a very large double
*
* SEE ALSO: floatLib(1)
*

* double mathHardInfinity ()

*
*/

_mathHardInfinity:
        link    a6,#0
	addl	#-8,sp
	fmovecr	#0x3f, fp0	/* 688881/2 rom-based inf. constant */
	fmoved	fp0, sp@
	movl	a6@(-8), d0
	movl	a6@(-4), d1
	addl	#8,sp
        unlk    a6
        rts
