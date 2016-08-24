/* mathALib.s - C interface library to high-level math functions */

/* Copyright 1984-1995 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01w,09may02,wsl  fix warning about incomplete implementation on some arch's
01v,10oct01,dgp  doc: fix formatting for doc build
01u,25nov95,jdi  doc: removed references to 68k-specific docs.
01t,03feb95,rhp  warn not all functions available on all architectures
01s,09dec94,rhp  fix man pages for inverse trig, hyperbolic fns
01r,09dec94,rhp  mention include file math.h in man pages (SPR #2502)
01q,26nov94,kdl  added 68LC040 support.
01p,02aug94,tpr  added MC68060 cpu support.
01o,15feb93,jdi  made NOMANUAL: hypot() and hypotf().
01n,12feb93,smb  removed pow and fmod for the MC68040
01m,05feb93,jdi  changed sqrt() to sqrtf() in doc for hypotf().
01l,02dec92,jdi  documentation; added modhist entry 01k omitted by jcf in 
		 last checkin (mistakenly called 01j in scl). 
01k,13nov92,jcf  made logMsg calls indirect to reduce coupling.
01j,13oct92,jdi  made mathErrNoInit() nomanual.
01i,26sep92,yp   added title so doc run would work
01h,23aug92,jcf  changed bxxx to jxx.
01g,30jul92,kdl	 changed to ANSI single precision names (e.g. fsin -> sinf)
01f,26may92,rrr  the tree shuffle
01e,04oct91,rrr  passed through the ansification filter
		  -changed VOID to void
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01d,27aug91,wmd	 added #include "vxWorks.h".
01c,23jul91,gae  changed 68k/asm.h to asm.h.
01b,30jan91,kdl	 mangen fixes.
01a,28jan91,kdl	 written; old "mathALib" renamed to "mathHardALib".
*/

/*
DESCRIPTION
This library provides a C interface to high-level floating-point math
functions, which can use either a hardware floating-point unit or a
software floating-point emulation library.  The appropriate routine is
called based on whether mathHardInit() or mathSoftInit() or both have
been called to initialize the interface.

All angle-related parameters are expressed in radians.  All functions in
this library with names corresponding to ANSI C specifications are ANSI
compatible.

WARNING
Not all functions in this library are available on all architectures.  For 
information on available math functions, consult the VxWorks architecture 
supplement for your processor.
	
INTERNAL
When generating man pages, the man pages from this library should be
built BEFORE those from ansiMath.  Thus, the equivalent man pages in 
ansiMath will overwrite those from this library, which is the correct
behavior.  This ordering is set up in the overall makefile system.

Even though this library is 68K, the man pages are meant to service
all architectures, within the limitation described in the above WARNING.

INTERNAL
See "MC68881/MC68882 Floating-Point Coprocessor User's Manual, 2nd Edition"

INCLUDE FILES: math.h

SEE ALSO:
'ansiMath', fppLib, floatLib, mathHardLib, mathSoftLib, the various
.I "Architecture Supplements,"
Kernighan & Ritchie:
.I "The C Programming Language, 2nd Edition"
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "errno.h"

	.data
	.even

	.globl _mathAcosFunc	/* double-precision function pointers */
	.globl _mathAsinFunc
	.globl _mathAtanFunc
	.globl _mathAtan2Func
	.globl _mathCbrtFunc
	.globl _mathCeilFunc
	.globl _mathCosFunc
	.globl _mathCoshFunc
	.globl _mathExpFunc
	.globl _mathFabsFunc
	.globl _mathFloorFunc
	.globl _mathFmodFunc
	.globl _mathHypotFunc
	.globl _mathInfinityFunc
	.globl _mathIrintFunc
	.globl _mathIroundFunc
	.globl _mathLogFunc
	.globl _mathLog2Func
	.globl _mathLog10Func
	.globl _mathPowFunc
	.globl _mathRoundFunc
	.globl _mathSinFunc
	.globl _mathSincosFunc
	.globl _mathSinhFunc
	.globl _mathSqrtFunc
	.globl _mathTanFunc
	.globl _mathTanhFunc
	.globl _mathTruncFunc

	.globl _mathAcosfFunc	/* single-precision function pointers */
	.globl _mathAsinfFunc
	.globl _mathAtanfFunc
	.globl _mathAtan2fFunc
	.globl _mathCbrtfFunc
	.globl _mathCeilfFunc
	.globl _mathCosfFunc
	.globl _mathCoshfFunc
	.globl _mathExpfFunc
	.globl _mathFabsfFunc
	.globl _mathFloorfFunc
	.globl _mathFmodfFunc
	.globl _mathHypotfFunc
	.globl _mathInfinityfFunc
	.globl _mathIrintfFunc
	.globl _mathIroundfFunc
	.globl _mathLogfFunc
	.globl _mathLog2fFunc
	.globl _mathLog10fFunc
	.globl _mathPowfFunc
	.globl _mathRoundfFunc
	.globl _mathSinfFunc
	.globl _mathSincosfFunc
	.globl _mathSinhfFunc
	.globl _mathSqrtfFunc
	.globl _mathTanfFunc
	.globl _mathTanhfFunc
	.globl _mathTruncfFunc


_mathAcosFunc:			/* double-precision function pointers */
	.long	_mathErrNoInit
_mathAsinFunc:
	.long	_mathErrNoInit
_mathAtanFunc:
	.long	_mathErrNoInit
_mathAtan2Func:
	.long	_mathErrNoInit
_mathCbrtFunc:
	.long	_mathErrNoInit
_mathCeilFunc:
	.long	_mathErrNoInit
_mathCosFunc:
	.long	_mathErrNoInit
_mathCoshFunc:
	.long	_mathErrNoInit
_mathExpFunc:
	.long	_mathErrNoInit
_mathFabsFunc:
	.long	_mathErrNoInit
_mathFloorFunc:
	.long	_mathErrNoInit
_mathFmodFunc:
	.long	_mathErrNoInit
_mathHypotFunc:
	.long	_mathErrNoInit
_mathInfinityFunc:
	.long	_mathErrNoInit
_mathIrintFunc:
	.long	_mathErrNoInit
_mathIroundFunc:
	.long	_mathErrNoInit
_mathLogFunc:
	.long	_mathErrNoInit
_mathLog2Func:
	.long	_mathErrNoInit
_mathLog10Func:
	.long	_mathErrNoInit
_mathPowFunc:
	.long	_mathErrNoInit
_mathRoundFunc:
	.long	_mathErrNoInit
_mathSinFunc:
	.long	_mathErrNoInit
_mathSincosFunc:
	.long	_mathErrNoInit
_mathSinhFunc:
	.long	_mathErrNoInit
_mathSqrtFunc:
	.long	_mathErrNoInit
_mathTanFunc:
	.long	_mathErrNoInit
_mathTanhFunc:
	.long	_mathErrNoInit
_mathTruncFunc:
	.long	_mathErrNoInit

_mathAcosfFunc:			/* single-precision function pointers */
	.long	_mathErrNoInit
_mathAsinfFunc:
	.long	_mathErrNoInit
_mathAtanfFunc:
	.long	_mathErrNoInit
_mathAtan2fFunc:
	.long	_mathErrNoInit
_mathCbrtfFunc:
	.long	_mathErrNoInit
_mathCeilfFunc:
	.long	_mathErrNoInit
_mathCosfFunc:
	.long	_mathErrNoInit
_mathCoshfFunc:
	.long	_mathErrNoInit
_mathExpfFunc:
	.long	_mathErrNoInit
_mathFabsfFunc:
	.long	_mathErrNoInit
_mathFloorfFunc:
	.long	_mathErrNoInit
_mathFmodfFunc:
	.long	_mathErrNoInit
_mathHypotfFunc:
	.long	_mathErrNoInit
_mathInfinityfFunc:
	.long	_mathErrNoInit
_mathIrintfFunc:
	.long	_mathErrNoInit
_mathIroundfFunc:
	.long	_mathErrNoInit
_mathLogfFunc:
	.long	_mathErrNoInit
_mathLog2fFunc:
	.long	_mathErrNoInit
_mathLog10fFunc:
	.long	_mathErrNoInit
_mathPowfFunc:
	.long	_mathErrNoInit
_mathRoundfFunc:
	.long	_mathErrNoInit
_mathSinfFunc:
	.long	_mathErrNoInit
_mathSincosfFunc:
	.long	_mathErrNoInit
_mathSinhfFunc:
	.long	_mathErrNoInit
_mathSqrtfFunc:
	.long	_mathErrNoInit
_mathTanfFunc:
	.long	_mathErrNoInit
_mathTanhfFunc:
	.long	_mathErrNoInit
_mathTruncfFunc:
	.long	_mathErrNoInit


_mathErrNoInitString:
	.asciz	"ERROR - floating point math not initialized!\n"

        .text
	.even
        .globl  _acos			/* double-precision functions */
        .globl  _asin
        .globl  _atan
	.globl  _atan2
	.globl  _cbrt
	.globl  _ceil
        .globl  _cos
        .globl  _cosh
        .globl  _exp
        .globl  _fabs
	.globl  _floor
#if (CPU!=MC68040) && (CPU!=MC68LC040) && (CPU!=MC68060)
	.globl  _fmod
#endif	/* (CPU!=MC68040) && (CPU!=MC68LC040) && (CPU!=MC68060) */
	.globl	_hypot
	.globl  _infinity
	.globl  _irint
	.globl  _iround
        .globl  _log
        .globl  _log2
        .globl  _log10
#if (CPU!=MC68040) && (CPU!=MC68LC040) && (CPU!=MC68060)
        .globl  _pow
#endif	/* (CPU!=MC68040) && (CPU!=MC68LC040) && (CPU!=MC68060) */
	.globl  _round
        .globl  _sin
	.globl  _sincos
        .globl  _sinh
        .globl  _sqrt
        .globl  _tan
        .globl  _tanh
	.globl  _trunc

        .globl  _acosf			/* single-precision functions */
        .globl  _asinf
        .globl  _atanf
	.globl  _atan2f
	.globl  _cbrtf
	.globl  _ceilf
        .globl  _cosf
        .globl  _coshf
        .globl  _expf
        .globl  _fabsf
	.globl  _floorf
	.globl  _fmodf
	.globl	_hypotf
	.globl  _infinityf
	.globl  _irintf
	.globl  _iroundf
        .globl  _logf
        .globl  _log2f
        .globl  _log10f
        .globl  _powf
	.globl  _roundf
        .globl  _sinf
	.globl  _sincosf
        .globl  _sinhf
        .globl  _sqrtf
        .globl  _tanf
        .globl  _tanhf
	.globl  _truncf

	.globl	_mathErrNoInit		/* default routine (log error msg) */
	.globl	__func_logMsg		/* logMsg virtual function */

/*******************************************************************************
*
* acos - compute an arc cosine (ANSI)
*
* SYNOPSIS
* \ss
* double acos
*     (
*     double x    /@ angle in radians @/
*     )
* \se
*
* INCLUDE FILES: math.h 
*
* RETURNS: The double-precision arc cosine of <x> in the range 0.0
* to pi radians.
*
* SEE ALSO:
* Kernighan & Ritchie,
* .I "The C Programming Language, 2nd Edition"
*
*/

_acos:
	movel	_mathAcosFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* asin - compute an arc sine (ANSI)
*
* SYNOPSIS
* \ss
* double asin
*     (
*     double x    /@ angle in radians @/
*     )
* \se
*
* INCLUDE FILES: math.h 
*
* RETURNS: The double-precision arc sine of <x> in the range
* -pi/2 to pi/2 radians.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/

_asin:
	movel	_mathAsinFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* atan - compute an arc tangent (ANSI)
*
* SYNOPSIS
* \ss
* double atan
*     (
*     double x    /@ angle in radians @/
*     )
* \se
*
* INCLUDE FILES: math.h 
*
* RETURNS: The double-precision arc tangent of <x> in the range -pi/2 to pi/2.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/
_atan:
	movel	_mathAtanFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* atan2 - compute the arc tangent of y/x (ANSI)
*
* SYNOPSIS
* \ss
* double atan2
*     (
*     double  y,  /@ numerator @/
*     double  x   /@ denominator @/
*     )
* \se
*
* INCLUDE FILES: math.h 
*
* RETURNS:
* The double-precision arc tangent of <y>/<x> in the range -pi to pi.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/

_atan2:

	movel	_mathAtan2Func,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* cbrt - compute a cube root
*
* SYNOPSIS
* \ss
* double cbrt
*     (
*     double x	/@ value to compute the cube root of @/
*     )
* \se
*
* This routine returns the cube root of <x> in double precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The double-precision cube root of <x>.
*/

_cbrt:
	movel	_mathCbrtFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* ceil - compute the smallest integer greater than or equal to a specified value (ANSI)
*
* SYNOPSIS
* \ss
* double ceil
*     (
*     double v    /@ value to return the ceiling of @/
*     )
* \se
*
* Performs a round-to-positive-infinity.
*
* INCLUDE FILES: math.h 
*
* RETURNS:
* The smallest integral value greater than or equal to <v>,
* represented in double precision.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/

_ceil:
	movel	_mathCeilFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* cos - compute a cosine (ANSI)
*
* SYNOPSIS
* \ss
* double cos
*     (
*     double x    /@ angle in radians @/
*     )
* \se
*
* INCLUDE FILES: math.h 
*
* RETURNS: The double-precision cosine of <x>.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/

_cos:
	movel	_mathCosFunc,a0		/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* cosh - compute a hyperbolic cosine (ANSI)
*
* SYNOPSIS
* \ss
* double cosh
*     (
*     double x    /@ angle in radians @/
*     )
* \se
*
* INCLUDE FILES: math.h 
*
* RETURNS:
* The double-precision hyperbolic cosine of <x> if the parameter is greater
* than 1.0, or NaN if the parameter is less than 1.0.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/

_cosh:
	movel	_mathCoshFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* exp - compute an exponential value (ANSI)
*
* SYNOPSIS
* \ss
* double exp
*     (
*     double x	/@ exponent @/
*     )
* \se
*
* This routine returns the exponential value of <x> -- the inverse natural
* logarithm (e ** <x>) -- in double precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS:
* The double-precision exponential value of <x>.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/

_exp:
	movel	_mathExpFunc,a0		/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* fabs - compute an absolute value (ANSI)
*
* SYNOPSIS
* \ss
* double fabs
*     (
*     double v    /@ number to return the absolute value of @/
*     )
* \se
*
* INCLUDE FILES: math.h 
*
* RETURNS: The double-precision absolute value of <v>.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/

_fabs:
	movel	_mathFabsFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* floor - compute the largest integer less than or equal to a specified value (ANSI)
*
* SYNOPSIS
* \ss
* double floor
*     (
*     double v    /@ value to return the floor of @/
*     )
* \se
*
* Performs a round-to-negative-infinity.
*
* INCLUDE FILES: math.h 
*
* RETURNS:
* The largest integral value less than or equal to <v>,
* in double precision.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/

_floor:
	movel	_mathFloorFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

#if (CPU!=MC68040) && (CPU!=MC68LC040) && (CPU!=MC68060)
/*******************************************************************************
*
* fmod - compute the remainder of x/y (ANSI)
*
* SYNOPSIS
* \ss
* double fmod
*     (
*     double x,   /@ numerator   @/
*     double y    /@ denominator @/
*     )
* \se
*
* INCLUDE FILES: math.h 
*
* RETURNS:
* The double-precision modulus of <x>/<y> with the sign of <x>.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/

_fmod:
	movel	_mathFmodFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

#endif	/* (CPU!=MC68040) && (CPU!=MC68LC040) && (CPU!=MC68060) */

/*******************************************************************************
*
* hypot - compute a Euclidean distance (hypotenuse)
*
* SYNOPSIS
* \ss
* double hypot
*     (
*     double x,		/@ argument @/
*     double y		/@ argument @/
*     )
* \se
*
* This routine returns the length of the Euclidean distance
* (hypotenuse) of two double-precision parameters.
*
* The distance is calculated as:
* .CS
*     sqrtf ((x * x) + (y * y))
* .CE
*
* INCLUDE FILES: math.h 
*
* RETURNS: The double-precision hypotenuse of <x> and <y>.
*
* NOMANUAL
*/

_hypot:
	movel	_mathHypotFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* infinity - return a very large double
*
* SYNOPSIS
* \ss
* double infinity (void)
* \se
*
* This routine returns a very large double.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The double-precision representation of positive infinity.
*/

_infinity:
	movel	_mathInfinityFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */


/*******************************************************************************
*
* irint - convert a double-precision value to an integer
*
* SYNOPSIS
* \ss
* int irint
*     (
*     double x	/@ argument @/
*     )
* \se
*
* This routine converts a double-precision value <x> to an integer
* using the selected IEEE rounding direction.
*
* CAVEAT:
* The rounding direction is not pre-selectable and is fixed for
* round-to-the-nearest.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The integer representation of <x>.
*/

_irint:
	movel	_mathIrintFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* iround - round a number to the nearest integer
*
* SYNOPSIS
* \ss
* int iround
*     (
*     double x    /@ argument @/
*     )
* \se
*
* This routine rounds a double-precision value <x> to the nearest
* integer value.
*
* NOTE:
* If <x> is spaced evenly between two integers, it returns the even integer.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The integer nearest to <x>.
*/

_iround:
	movel	_mathIroundFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* log - compute a natural logarithm (ANSI)
*
* SYNOPSIS
* \ss
* double log
*     (
*     double x    /@ value to compute the natural logarithm of @/
*     )
* \se
*
* INCLUDE FILES: math.h 
*
* RETURNS: The double-precision natural logarithm of <x>.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/

_log:
	movel	_mathLogFunc,a0		/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* log10 - compute a base-10 logarithm (ANSI)
*
* SYNOPSIS
* \ss
* double log10
*     (
*     double x    /@ value to compute the base-10 logarithm of @/
*     )
* \se
*
* INCLUDE FILES: math.h 
*
* RETURNS: The double-precision base-10 logarithm of <x>.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/

_log10:
	movel	_mathLog10Func,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* log2 - compute a base-2 logarithm
*
* SYNOPSIS
* \ss
* double log2
*     (
*     double x	/@ value to compute the base-two logarithm of @/
*     )
* \se
*
* This routine returns the base-2 logarithm of <x> in double precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The double-precision base-2 logarithm of <x>.
*/

_log2:
	movel	_mathLog2Func,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

#if (CPU!=MC68040) && (CPU!=MC68LC040) && (CPU!=MC68060)
/*******************************************************************************
*
* pow - compute the value of a number raised to a specified power (ANSI)
*
* SYNOPSIS
* \ss
* double pow
*     (
*     double x,   /@ operand  @/
*     double y    /@ exponent @/
*     )
* \se
*
* INCLUDE FILES: math.h 
*
* RETURNS: The double-precision value of <x> to the power of <y>.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/

_pow: 					/* pow (x,y) = exp (y * log(x)) */
	movel	_mathPowFunc,a0		/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

#endif	/* (CPU!=MC68040) && (CPU!=MC68LC040) && (CPU!=MC68060) */

/*******************************************************************************
*
* round - round a number to the nearest integer
*
* SYNOPSIS
* \ss
* double round
*     (
*     double x	/@ value to round @/
*     )
* \se
*
* This routine rounds a double-precision value <x> to the nearest
* integral value.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The double-precision representation of <x> rounded to the
* nearest integral value.
*/

_round:
	movel	_mathRoundFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* sin - compute a sine (ANSI)
*
* SYNOPSIS
* \ss
* double sin
*     (    
*     double x    /@ angle in radians @/
*     )
* \se
*
* INCLUDE FILES: math.h 
*
* RETURNS: The double-precision floating-point sine of <x>.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/

_sin:
	movel	_mathSinFunc,a0		/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* sincos - compute both a sine and cosine
*
* SYNOPSIS
* \ss
* void sincos
*     (
*     double x,		 /@ angle in radians @/
*     double *sinResult, /@ sine result buffer @/
*     double *cosResult	 /@ cosine result buffer @/
*     )
* \se
*
* This routine computes both the sine and cosine of <x> in double precision.
* The sine is copied to <sinResult> and the cosine is copied to <cosResult>.
*
* INCLUDE FILES: math.h 
*
* RETURNS: N/A
*/

_sincos:
	movel	_mathSincosFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* sinh - compute a hyperbolic sine (ANSI)
*
* SYNOPSIS
* \ss
* double sinh
*     (
*     double x    /@ angle in radians @/
*     )
* \se
*
* INCLUDE FILES: math.h 
*
* RETURNS: The double-precision hyperbolic sine of <x>.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/
_sinh:
	movel	_mathSinhFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* sqrt - compute a non-negative square root (ANSI)
*
* INCLUDE FILES: math.h 
*
* SYNOPSIS
* \ss
* double sqrt
*     (
*     double x    /@ value to compute the square root of @/
*     )
* \se
*
* RETURNS: The double-precision square root of <x>.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/

_sqrt:
	movel	_mathSqrtFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* tan - compute a tangent (ANSI)
*
* SYNOPSIS
* \ss
* double tan
*     (
*     double x    /@ angle in radians @/
*     )
* \se
*
* INCLUDE FILES: math.h 
*
* RETURNS: The double-precision tangent of <x>.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/

_tan:
	movel	_mathTanFunc,a0		/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* tanh - compute a hyperbolic tangent (ANSI)
*
* SYNOPSIS
* \ss
* double tanh
*     (
*     double x    /@ angle in radians @/
*     )
* \se
*
* INCLUDE FILES: math.h 
*
* RETURNS: The double-precision hyperbolic tangent of <x>.
*
* SEE ALSO:
* Kernighan & Ritchie:
* .I "The C Programming Language, 2nd Edition"
*/

_tanh:
	movel	_mathTanhFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */


/*******************************************************************************
*
* trunc - truncate to integer
*
* SYNOPSIS
* \ss
* double trunc
*     (
*     double x	/@ value to truncate @/
*     )
* \se
*
* This routine discards the fractional part of a double-precision value <x>.
*
* INCLUDE FILES: math.h 
*
* RETURNS:
* The integer portion of <x>, represented in double-precision.
*/

_trunc:
	movel	_mathTruncFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* acosf - compute an arc cosine (ANSI)
*
* SYNOPSIS
* \ss
* float acosf
*     (
*     float x	/@ number between -1 and 1 @/
*     )
* \se
*
* This routine computes the arc cosine of <x> in single precision.
* If <x> is the cosine of an angle <T>, this function returns <T>.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The single-precision arc cosine of <x> in the range 0
* to pi radians.
*/

_acosf:
	movel	_mathAcosfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* asinf - compute an arc sine (ANSI)
*
* SYNOPSIS
* \ss
* float asinf
*     (
*     float x	/@ number between -1 and 1 @/
*     )
* \se
*
* This routine computes the arc sine of <x> in single precision.
* If <x> is the sine of an angle <T>, this function returns <T>.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The single-precision arc sine of <x> in the range
* -pi/2 to pi/2 radians.
*/

_asinf:
	movel	_mathAsinfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* atanf - compute an arc tangent (ANSI)
*
* SYNOPSIS
* \ss
* float atanf
*     (
*     float x	/@ tangent of an angle @/
*     )
* \se
*
* This routine computes the arc tangent of <x> in single precision.
* If <x> is the tangent of an angle <T>, this function returns <T> 
* (in radians).
*
* INCLUDE FILES: math.h 
*
* RETURNS: The single-precision arc tangent of <x> in the range -pi/2 to pi/2.
*/

_atanf:
	movel	_mathAtanfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* atan2f - compute the arc tangent of y/x (ANSI)
*
* SYNOPSIS
* \ss
* float atan2f
*     (
*     float y,		/@ numerator @/
*     float x		/@ denominator @/
*     )
* \se
*
* This routine returns the principal value of the arc tangent of <y>/<x>
* in single precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS:
* The single-precision arc tangent of <y>/<x> in the range -pi to pi.
*/

_atan2f:

	movel	_mathAtan2fFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* cbrtf - compute a cube root
*
* SYNOPSIS
* \ss
* float cbrtf
*     (
*     float x  /@ argument @/
*     )
* \se
*
* This routine returns the cube root of <x> in single precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The single-precision cube root of <x>.
*/

_cbrtf:
	movel	_mathCbrtfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* ceilf - compute the smallest integer greater than or equal to a specified value (ANSI)
*
* SYNOPSIS
* \ss
* float ceilf
*     (
*     float v	/@ value to find the ceiling of @/
*     )
* \se
*
* This routine returns the smallest integer greater than or equal to <v>,
* in single precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The smallest integral value greater than or equal to <v>,
* in single precision.
*/

_ceilf:
	movel	_mathCeilfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* cosf - compute a cosine (ANSI)
*
* SYNOPSIS
* \ss
* float cosf
*     (
*     float x	/@ angle in radians @/
*     )
* \se
*
* This routine returns the cosine of <x> in single precision.
* The angle <x> is expressed in radians.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The single-precision cosine of <x>.
*/

_cosf:
	movel	_mathCosfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* coshf - compute a hyperbolic cosine (ANSI)
*
* SYNOPSIS
* \ss
* float coshf
*     (
*     float x	/@ value to compute the hyperbolic cosine of @/
*     )
* \se
*
* This routine returns the hyperbolic cosine of <x> in single precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS:
* The single-precision hyperbolic cosine of <x> if the parameter is greater
* than 1.0, or NaN if the parameter is less than 1.0.
* 	
* Special cases:
*  If <x> is +INF, -INF, or NaN, coshf() returns <x>.
*/

_coshf:
	movel	_mathCoshfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* expf - compute an exponential value (ANSI)
*
* SYNOPSIS
* \ss
* float expf
*     (
*     float x	/@ exponent @/
*     )
* \se
*
* This routine returns the exponential of <x> in single precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS:
* The single-precision exponential value of <x>.
*/

_expf:
	movel	_mathExpfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* fabsf - compute an absolute value (ANSI)
*
* SYNOPSIS
* \ss
* float fabsf
*     (
*     float v	/@ number to return the absolute value of @/
*     )
* \se
*
* This routine returns the absolute value of <v> in single precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The single-precision absolute value of <v>.
*/

_fabsf:
	movel	_mathFabsfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* floorf - compute the largest integer less than or equal to a specified value (ANSI)
*
* SYNOPSIS
* \ss
* float floorf
*     (
*     float v    /@ value to find the floor of @/
*     )
* \se
*
* This routine returns the largest integer less than or equal to <v>,
* in single precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS:
* The largest integral value less than or equal to <v>,
* in single precision.
*/

_floorf:
	movel	_mathFloorfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* fmodf - compute the remainder of x/y (ANSI)
*
* SYNOPSIS
* \ss
* float fmodf
*     (
*     float x,   /@ numerator   @/
*     float y    /@ denominator @/
*     )
* \se
*
* This routine returns the remainder of <x>/<y> with the sign of <x>,
* in single precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS:
* The single-precision modulus of <x>/<y>.
*/

_fmodf:
	movel	_mathFmodfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* hypotf - compute a Euclidean distance (hypotenuse)
*
* SYNOPSIS
* \ss
* float hypotf
*     (
*     float x,		/@ argument @/
*     float y		/@ argument @/
*     )
* \se
*
* This routine returns the length of the Euclidean distance (hypotenuse)
* of two single-precision parameters.
*
* The distance is calculated as:
* .CS
*     sqrtf ((x * x) + (y * y))
* .CE
*
* INCLUDE FILES: math.h 
*
* RETURNS: The single-precision hypotenuse of <x> and <y>.
*
* NOMANUAL
*/

_hypotf:
	movel	_mathHypotfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* infinityf - return a very large float
*
* SYNOPSIS
* \ss
* float infinityf (void)
* \se
*
* This routine returns a very large float.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The single-precision representation of positive infinity.
*/

_infinityf:
	movel	_mathInfinityfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */


/*******************************************************************************
*
* irintf - convert a single-precision value to an integer
*
* SYNOPSIS
* \ss
* int irintf
*     (
*     float x	/@ argument @/
*     )
* \se
*
* This routine converts a single-precision value <x> to an integer using the 
* selected IEEE rounding direction.
*
* CAVEAT:
* The rounding direction is not pre-selectable
* and is fixed as round-to-the-nearest.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The integer representation of <x>.
*/

_irintf:
	movel	_mathIrintfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* iroundf - round a number to the nearest integer
*
* SYNOPSIS
* \ss
* int iroundf
*     (
*     float x	/@ argument @/
*     )
* \se
*
* This routine rounds a single-precision value <x> to the nearest 
* integer value.
*
* NOTE:
* If <x> is spaced evenly between two integers, the even integer is
* returned.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The integer nearest to <x>.
*/

_iroundf:
	movel	_mathIroundfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* logf - compute a natural logarithm (ANSI)
*
* SYNOPSIS
* \ss
* float logf
*     (
*     float x	/@ value to compute the natural logarithm of @/
*     )
* \se
*
* This routine returns the logarithm of <x> in single precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The single-precision natural logarithm of <x>.
*/

_logf:
	movel	_mathLogfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* log10f - compute a base-10 logarithm (ANSI)
*
* SYNOPSIS
* \ss
* float log10f
*     (
*     float x    /@ value to compute the base-10 logarithm of @/
*     )
* \se
*
* This routine returns the base-10 logarithm of <x> in single precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The single-precision base-10 logarithm of <x>.
*/

_log10f:
	movel	_mathLog10fFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* log2f - compute a base-2 logarithm
*
* SYNOPSIS
* \ss
* float log2f
*     (
*     float x    /@ value to compute the base-2 logarithm of @/
*     )
* \se
*
* This routine returns the base-2 logarithm of <x> in single precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The single-precision base-2 logarithm of <x>.
*/

_log2f:
	movel	_mathLog2fFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* powf - compute the value of a number raised to a specified power (ANSI)
*
* SYNOPSIS
* \ss
* float powf
*     (
*     float x,   /@ operand  @/
*     float y    /@ exponent @/
*     )
* \se
*
* This routine returns the value of <x> to the power of <y> in
* single precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The single-precision value of <x> to the power of <y>.
*/

_powf: 					/* pow (x,y) = exp (y * log(x)) */
	movel	_mathPowfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* roundf - round a number to the nearest integer
*
* SYNOPSIS
* \ss
* float roundf
*     (
*     float x	/@ argument @/
*     )
* \se
*
* This routine rounds a single-precision value <x> to the nearest 
* integral value.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The single-precision representation of <x> rounded to the
* nearest integral value.
*/

_roundf:
	movel	_mathRoundfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* sinf - compute a sine (ANSI)
*
* SYNOPSIS
* \ss
* float sinf
*     (
*     float x    /@ angle in radians @/
*     )
* \se
*
* This routine returns the sine of <x> in single precision.
* The angle <x> is expressed in radians.
*
* INCLUDE FILES: math.h 
*
* RETURNS:
* The single-precision sine of <x>.
*/

_sinf:
	movel	_mathSinfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* sincosf - compute both a sine and cosine
*
* SYNOPSIS
* \ss
* void sincosf
*     (
*     float x,		/@ angle in radians @/
*     float *sinResult,	/@ sine result buffer @/
*     float *cosResult	/@ cosine result buffer @/
*     )
* \se
*
* This routine computes both the sine and cosine of <x> in single precision.
* The sine is copied to <sinResult> and the cosine is copied to <cosResult>.
* The angle <x> is expressed in radians.
*
* INCLUDE FILES: math.h 
*
* RETURNS: N/A
*/

_sincosf:
	movel	_mathSincosfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* sinhf - compute a hyperbolic sine (ANSI)
*
* SYNOPSIS
* \ss
* float sinhf
*     (
*     float x	/@ number whose hyperbolic sine is required @/
*     )
* \se
*
* This routine returns the hyperbolic sine of <x> in single precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The single-precision hyperbolic sine of <x>.
*/

_sinhf:
	movel	_mathSinhfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* sqrtf - compute a non-negative square root (ANSI)
*
* SYNOPSIS
* \ss
* float sqrtf
*     (
*     float x    /@ value to compute the square root of @/
*     )
* \se
*
* This routine returns the non-negative square root of <x> in single
* precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The single-precision square root of <x>.
*/

_sqrtf:
	movel	_mathSqrtfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* tanf - compute a tangent (ANSI)
*
* SYNOPSIS
* \ss
* float tanf
*     (
*     float x    /@ angle in radians @/
*     )
* \se
*
* This routine returns the tangent of <x> in single precision.
* The angle <x> is expressed in radians.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The single-precision tangent of <x>.
*/

_tanf:
	movel	_mathTanfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* tanhf - compute a hyperbolic tangent (ANSI)
*
* SYNOPSIS
* \ss
* float tanhf
*     (
*     float x    /@ number whose hyperbolic tangent is required @/
*     )
* \se
*
* This routine returns the hyperbolic tangent of <x> in single precision.
*
* INCLUDE FILES: math.h 
*
* RETURNS: The single-precision hyperbolic tangent of <x>.
*/

_tanhf:
	movel	_mathTanhfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */


/*******************************************************************************
*
* truncf - truncate to integer
*
* SYNOPSIS
* \ss
* float truncf
*     (
*     float x	/@ value to truncate @/
*     )
* \se
*
* This routine discards the fractional part of a single-precision value <x>.
*
* INCLUDE FILES: math.h 
*
* RETURNS:
* The integer portion of <x>, represented in single precision.
*/

_truncf:
	movel	_mathTruncfFunc,a0	/* get appropriate function addr */
	jmp	a0@			/* jump, let that routine rts */

/*******************************************************************************
*
* mathErrNoInit - default routine for uninitialized floating-point functions
*
* SYNOPSIS
* \ss
* void mathErrNoInit ()
* \se
*
* This routine is called if floating point math operations are attempted
* before either a mathSoftInit() or mathHardInit() call is performed.  The
* address of this routine is the initial value of the various mathXXXFunc
* pointers, which are re-initialized with actual function addresses during
* either of the floating point initialization calls.
*
* SEE ALSO: mathHardInit(), mathSoftInit()
*
* NOMANUAL
*/

_mathErrNoInit:
	tstl	__func_logMsg
	beqs	_mathErrDone
	clrl	sp@-
	clrl	sp@-
	clrl	sp@-
	clrl	sp@-
	clrl	sp@-
	clrl	sp@-
	movel	#_mathErrNoInitString,sp@-
	movel	__func_logMsg,a0
	jsr	a0@			/* log the message if logger avail */
	addl	#0x1c,sp		/* cleanup stack */
_mathErrDone:
	rts

