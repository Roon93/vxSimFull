/* Copyright 1984-1992 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01e,23aug92,jcf  changed bxxx to jxx.
01d,30jul92,kdl	 changed to ANSI single precision names (e.g. sinf -> sinf)
01c,26may92,rrr  the tree shuffle
01b,04oct91,rrr  passed through the ansification filter
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01a,28jan91,kdl	 original version, based on library developed by S. Huddleston.
*/

/*
DESCRIPTION

This file contains assembly language interface routines which allow the
US Software floating point emulation package to be used from C code.
The US Software package uses non-C conventions for parameter passing
on the stack, and some functions use the CPU's status register bits to
indicate results.  These routines convert these non-standard interfaces
to normal C parameter passing.  The names for functions are those which
are generated by the gcc compiler.

The routines contained in this library provide high-level floating point
operations, including triginometric functions.

INTERNAL
Many of these routines were machine-generated by a special tool which
used a template describing the interface for each call.

AUTHOR
Scott Huddleston, Computer Research Lab, Tektronix.
  Copyright 1990, Tektronix Inc.

*/
#define	_ASMLANGUAGE

.globl	_mathSoftFloor
.globl	_mathSoftSqrt
.globl	_mathSoftExp
.globl	_mathSoftInfinity
.globl	_mathSoftLog
.globl	_mathSoftLog10
.globl	_mathSoftSin
.globl	_mathSoftCos
.globl	_mathSoftTan
.globl	_mathSoftAtan
.globl	_mathSoftRealtoint
.globl	_mathSoftFloorf
.globl	_mathSoftSqrtf
.globl	_mathSoftExpf
.globl	_mathSoftInfinityf
.globl	_mathSoftLogf
.globl	_mathSoftLog10f
.globl	_mathSoftSinf
.globl	_mathSoftCosf
.globl	_mathSoftTanf
.globl	_mathSoftAtanf
.globl	_mathSoftRealtointf
.globl	XDPXTOI
.globl	XFPXTOI

	.text
	.even

/*******************************************************************************
*
* mathSoftFloor - ANSI-compatable software floating-point floor
*
* Performs a 'round-to-negative-infinity'.
*
* RETURNS:
* The largest integral value less than or equal to dblParam,
* result is returned in double precision.
*
* SEE ALSO:
* floatLib (1), "The C Programming Language - Second Edition"
*
*
* double mathSoftFloor (dblParam)
*     double dblParam;	/* argument *
*
*/

_mathSoftFloor:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	jbsr	DAINT
	movel	sp@+,d0
	movel	sp@+,d1
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
*
* mathSoftSqrt - ANSI-compatable software floating-point square root
*
* RETURNS: The floating-point square root of dblParam.
*
* SEE ALSO: floatLib(1), pow (2)
*
* double mathSoftSqrt (dblParam)
*     double dblParam;	/* argument *
*
*/

_mathSoftSqrt:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	jbsr	DPSQRT
	movel	sp@+,d0
	movel	sp@+,d1
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
*
* mathSoftExp - software floating-point exponential function
*
* RETURNS:
*    Floating-point inverse natural logarithm (e ** (dblExponent)).
*
* SEE ALSO:
* floatLib (1), "The C Programming Language - Second Edition"
*
* double mathSoftExp (dblExponent)
*     double dblExponent;	/* argument *
*
*/

_mathSoftExp:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	jbsr	DPEXP
	movel	sp@+,d0
	movel	sp@+,d1
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
*
* mathSoftLog - ANSI-compatable software floating-point natural logarithm
*
* RETURNS: The natural logarithm of dblParam.
*
* SEE ALSO:
* floatLib (1), "The C Programming Language - Second Edition"
*
* double mathSoftLog (dblParam)
*     double dblParam;	/* argument *
*
*/

_mathSoftLog:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	jbsr	DPLN
	movel	sp@+,d0
	movel	sp@+,d1
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
*
* mathSoftLog10 - ANSI-compatable software floating-point base 10 logarithm
*
* RETURNS: The logarithm (base 10) of dblParam.
*
* SEE ALSO: floatLib (1), log2 (2)
*
* double mathSoftLog10 (dblParam)
*     double dblParam;	/* argument *
*
*/

_mathSoftLog10:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	jbsr	DPLOG
	movel	sp@+,d0
	movel	sp@+,d1
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
*
* mathSoftSin - ANSI-compatable software floating-point sine
*
* RETURNS: The floating-point sine of dblParam.
*
* SEE ALSO:
*   floatLib (1), cos (2), tan (2),
*   "The C Programming Language - Second Edition"
*
* double mathSoftSin (dblParam)
*     double dblParam;	/* angle in radians *
*
*/

_mathSoftSin:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	jbsr	DPSIN
	movel	sp@+,d0
	movel	sp@+,d1
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
*
* mathSoftCos - ANSI-compatable software floating-point cosine
*
* RETURNS: the cosine of the radian argument dblParam
*
* SEE ALSO:
* floatLib (1), sin (2), cos (2), tan(2),
* "The C Programming Language - Second Edition"
*
* double mathSoftCos (dblParam)
*     double dblParam;	/* angle in radians *
*
*/

_mathSoftCos:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	jbsr	DPCOS
	movel	sp@+,d0
	movel	sp@+,d1
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
*
* mathSoftTan - ANSI-compatable software floating-point tangent
*
* RETURNS: Floating-point tangent of dblParam.
*
* SEE ALSO: floatLib (1), cos (2), sin (2),
* "The C Programming Language - Second Edition"
*
* double mathSoftTan (dblParam)
*     double dblParam;	/* angle in radians *
*
*/

_mathSoftTan:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	jbsr	DPTAN
	movel	sp@+,d0
	movel	sp@+,d1
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
*
* mathSoftAtan - ANSI-compatable software floating-point arc-tangent
*
* RETURNS: The arc-tangent of dblParam in the range -pi/2 to pi/2.
*
* SEE ALSO: floatLib (1), acos (2), asin (2)
*
* double mathSoftAtan (dblParam)
*     double dblParam;	/* angle in radians *
*
*/

_mathSoftAtan:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	jbsr	DPATN
	movel	sp@+,d0
	movel	sp@+,d1
	moveml	sp@+,d2-d7/a1-a6
	rts


.text
	.even
/*******************************************************************************
*
* mathSoftRealtoint - double-precision floating point to integer power
*
* RETURNS: The double-precision result of <dblParam> to <intExp> power
*
* SEE ALSO: pow ()
*
* double mathSoftRealtoint (dblParam, intExp)
*     double dblParam;	/* argument      *
*     int    intExp;	/* integer power *
*
*/

_mathSoftRealtoint:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	movel	a0@(8),sp@-
	jbsr	XDPXTOI
	movel	sp@+,d0
	movel	sp@+,d1
	moveml	sp@+,d2-d7/a1-a6
	rts

XDPXTOI:
	movel	sp@(4),d0		| put integer power in d0
	movel	sp@+,sp@		| stuff rtn address lower on stack
	jra	DPXTOI			| call USS routine


/********************************************************************************
* mathSoftInfinity - software floating-point return of a very large double
*
* SEE ALSO: floatLib(1)
*
* double mathSoftInfinity ()
*
*/

_mathSoftInfinity:
	movel	#0x7ff00000,d0		| exponent = max; mantissa = 0
	clrl	d1			| mantissa = 0
	rts

/*******************************************************************************
*
* mathSoftFloorf - single-precision software floating-point floor
*
* Performs a 'round-to-negative-infinity'.
*
* RETURNS:
* The largest integral value less than or equal to fltParam,
* result is returned in single precision.
*
* SEE ALSO:
* floatLib (1), "The C Programming Language - Second Edition"
*
*
* float mathSoftFloorf (fltParam)
*     float fltParam;	/* argument *
*
*/

_mathSoftFloorf:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(0),sp@-
	jbsr	AINT
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
*
* mathSoftSqrtf - single-precision software floating-point square root
*
* RETURNS: The floating-point square root of fltParam.
*
* SEE ALSO: floatLib(1), pow (2)
*
* float mathSoftSqrtf (fltParam)
*     float fltParam;	/* argument *
*
*/

_mathSoftSqrtf:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(0),sp@-
	jbsr	FPSQRT
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
*
* mathSoftExpf - single-precision software floating-point exponential function
*
* RETURNS:
*    Floating-point inverse natural logarithm (e ** (fltExponent)).
*
* SEE ALSO:
* floatLib (1), "The C Programming Language - Second Edition"
*
* float mathSoftExpf (fltExponent)
*     float fltExponent;	/* argument *
*
*/

_mathSoftExpf:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(0),sp@-
	jbsr	FPEXP
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
*
* mathSoftLogf - single-precision software floating-point natural logarithm
*
* RETURNS: The natural logarithm of fltParam.
*
* SEE ALSO:
* floatLib (1), "The C Programming Language - Second Edition"
*
* float mathSoftLog (fltParam)
*     float fltParam;	/* argument *
*
*/

_mathSoftLogf:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(0),sp@-
	jbsr	FPLN
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
*
* mathSoftLog10f - single-precision software floating-point base 10 logarithm
*
* RETURNS: The logarithm (base 10) of fltParam.
*
* SEE ALSO: floatLib (1), log2 (2)
*
* float mathSoftLog10 (fltParam)
*     float fltParam;	/* argument *
*
*/

_mathSoftLog10f:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(0),sp@-
	jbsr	FPLOG
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
*
* mathSoftSinf - single-precision software floating-point sine
*
* RETURNS: The floating-point sine of fltParam.
*
* SEE ALSO:
*   floatLib (1), cos (2), tan (2),
*   "The C Programming Language - Second Edition"
*
* float mathSoftSin (fltParam)
*     float fltParam;	/* angle in radians *
*
*/

_mathSoftSinf:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(0),sp@-
	jbsr	FPSIN
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
*
* mathSoftCosf - single-precision software floating-point cosine
*
* RETURNS: the cosine of the radian argument fltParam
*
* SEE ALSO:
* floatLib (1), sin (2), cos (2), tan(2),
* "The C Programming Language - Second Edition"
*
* float mathSoftCosf (fltParam)
*     float fltParam;	/* angle in radians *
*
*/

_mathSoftCosf:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(0),sp@-
	jbsr	FPCOS
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
*
* mathSoftTanf - single-precision software floating-point tangent
*
* RETURNS: Floating-point tangent of fltParam.
*
* SEE ALSO: floatLib (1), cos (2), sin (2),
* "The C Programming Language - Second Edition"
*
* float mathSoftTanf (fltParam)
*     float fltParam;	/* angle in radians *
*
*/

_mathSoftTanf:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(0),sp@-
	jbsr	FPTAN
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
*
* mathSoftAtanf - single-precision software floating-point arc-tangent
*
* RETURNS: The arc-tangent of fltParam in the range -pi/2 to pi/2.
*
* SEE ALSO: floatLib (1), acos (2), asin (2)
*
* float mathSoftAtanf (fltParam)
*     float fltParam;	/* angle in radians *
*
*/

_mathSoftAtanf:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(0),sp@-
	jbsr	FPATN
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
*
* mathSoftRealtointf - single-precision floating point to integer power
*
* RETURNS: The single-precision result of <fltParam> to <intExp> power
*
* SEE ALSO: powf ()
*
* float mathSoftRealtointf (fltParam, intExp)
*     float fltParam;	/* argument      *
*     int   intExp;	/* integer power *
*
*/

_mathSoftRealtointf:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(0),sp@-
	movel	a0@(4),sp@-
	jbsr	XFPXTOI
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

XFPXTOI:
	movel	sp@(4),d0		| put integer power in d0
	movel	sp@+,sp@		| stuff rtn address lower on stack
	jra	FPXTOI			| call USS routine


/********************************************************************************
* mathSoftInfinityf - software floating-point return of a very large float
*
* SEE ALSO: floatLib(1)
*
* float mathSoftInfinityf ()
*
*/

_mathSoftInfinityf:
	movel	#0x7f800000,d0		| exponent = max; mantissa = 0
	rts

