/* mathSoftLib.c - high-level floating-point emulation library */

/* Copyright 1984-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01p,04sep98,yh   fixed mathSoftCeil for spr#22152.
01o,21nov95,gbj  switched to ansi-style function declarations
01n,07jul95,rcs  changed LOG2E back to correct value.
01m,03feb95,rhp  doc: warn that sw f/p is not available for all archs
01l,12jan95,rhp  doc: correct config macro to call mathSoftInit() (SPR#3733).
01k,14jul94,dvs  fixed argument order and doc for mathSoftAtan2/mathSoftAtan2f 
		 (SPR# 2580)
01j,21nov92,jdi	 documentation cleanup.
01i,13nov92,jcf	 made logMsg calls indirect to reduce coupling.
01h,13oct92,jdi	 doc, and NOMANUAL for mathSoftSincos() and mathSoftSincosf(). 
01g,30jul92,kdl	 changed to ANSI single precision names (e.g. sinf -> sinf).
01f,26jul92,gae  fixed number of parameters to logMsg().
01e,04jul92,jcf  ansi clean up.
01d,03jul92,smb  removed include of vxTypes.h.
01c,26may92,rrr  the tree shuffle
01b,04oct91,rrr  passed through the ansification filter
		  -changed VOID to void
		  -changed copyright notice
01a,28jan91,kdl	 original version, based on library written by S. Huddleston.
*/

/*
DESCRIPTION
This library provides software emulation of various high-level
floating-point operations.  This emulation is generally for use in systems
that lack a floating-point coprocessor.

WARNING
Software floating point is not supported for all architectures.  See
the architecture-specific appendices of the
.I VxWorks Programmer's Guide.

INCLUDE FILES: math.h

SEE ALSO: mathHardLib, mathALib,
.I VxWorks Programmer's Guide
architecture-specific appendices

INTERNAL
Authors:
Major portions by Scott Huddleston, Computer Research Lab, Tektronix.
  Copyright 1990, Tektronix Inc.

Cube-root function (cbrt) written by K.C. Ng, UC Berkeley.
  Copyright 1985, Regents of the University of California.

Hyperbolic trig functions (cosh, sinh, tanh) written by Fred Fish.

*/

#include "vxWorks.h"
#include "math.h"
#include "logLib.h"
#include "private/funcBindP.h"

/* make sure PI is defined */
#ifndef PI
#define PI      3.14159265358979323846264338327950
#endif

/* constant PI in single precision */
static float PI_SINGLE = PI;

#define INF_HiShort		0x7ff0
#define INF_HiShort_SINGLE	0x7f80

/* tests for NaN (Not a Number) and INF (infinity) */
#define isNaN(z)	(((*(short*)&z) & 0x7fff) > INF_HiShort)
#define isINF(z)	(((*(short*)&z) & 0x7fff) == INF_HiShort)
#define isINFNaN(z)	(((*(short*)&z) & 0x7fff) >= INF_HiShort)

#define isNaN_SINGLE(z)		(((*(short*)&z) & 0x7fff) > INF_HiShort_SINGLE)
#define isINF_SINGLE(z)		(((*(short*)&z) & 0x7fff) == INF_HiShort_SINGLE)
#define isINFNaN_SINGLE(z)	(((*(short*)&z) & 0x7fff) >= INF_HiShort_SINGLE)

#define LOGE_MAXDOUBLE  7.09782712893383970e+02
#define LOGE_MINDOUBLE  -7.09089565712824080e+02
#define TANH_MAXARG	16
#define LOG2E		1.4426950408889634074	/* Log to base 2 of e */





/* Externals */

extern double	mathSoftAtan ();	/* functions in mathSoftALib.s */
extern double	mathSoftCos ();
extern double	mathSoftExp ();
extern double	mathSoftFloor ();
extern double	mathSoftInfinity ();
extern double	mathSoftLog ();
extern double	mathSoftLog10 ();
extern double	mathSoftRealtoint ();
extern double	mathSoftSin ();
extern double	mathSoftSqrt ();
extern double	mathSoftTan ();

extern float	mathSoftAtanf ();
extern float	mathSoftCosf ();
extern float	mathSoftExpf ();
extern float	mathSoftFloorf ();
extern float	mathSoftInfinityf ();
extern float	mathSoftLogf ();
extern float	mathSoftLog10f ();
extern float	mathSoftRealtointf ();
extern float	mathSoftSinf ();
extern float	mathSoftSqrtf ();
extern float	mathSoftTanf ();


extern DBLFUNCPTR	mathAcosFunc;	/* double-precision function ptrs */
extern DBLFUNCPTR	mathAsinFunc;
extern DBLFUNCPTR	mathAtanFunc;
extern DBLFUNCPTR	mathAtan2Func;
extern DBLFUNCPTR	mathCbrtFunc;
extern DBLFUNCPTR	mathCeilFunc;
extern DBLFUNCPTR	mathCosFunc;
extern DBLFUNCPTR	mathCoshFunc;
extern DBLFUNCPTR	mathExpFunc;
extern DBLFUNCPTR	mathFabsFunc;
extern DBLFUNCPTR	mathFloorFunc;
extern DBLFUNCPTR	mathFmodFunc;
extern DBLFUNCPTR	mathHypotFunc;
extern DBLFUNCPTR	mathInfinityFunc;
extern FUNCPTR		mathIrintFunc;
extern FUNCPTR		mathIroundFunc;
extern DBLFUNCPTR	mathLogFunc;
extern DBLFUNCPTR	mathLog2Func;
extern DBLFUNCPTR	mathLog10Func;
extern DBLFUNCPTR	mathPowFunc;
extern DBLFUNCPTR	mathRoundFunc;
extern DBLFUNCPTR	mathSinFunc;
extern VOIDFUNCPTR	mathSincosFunc;
extern DBLFUNCPTR	mathSinhFunc;
extern DBLFUNCPTR	mathSqrtFunc;
extern DBLFUNCPTR	mathTanFunc;
extern DBLFUNCPTR	mathTanhFunc;
extern DBLFUNCPTR	mathTruncFunc;

extern FLTFUNCPTR	mathAcosfFunc;	/* single-precision function ptrs */
extern FLTFUNCPTR	mathAsinfFunc;
extern FLTFUNCPTR	mathAtanfFunc;
extern FLTFUNCPTR	mathAtan2fFunc;
extern FLTFUNCPTR	mathCbrtfFunc;
extern FLTFUNCPTR	mathCeilfFunc;
extern FLTFUNCPTR	mathCosfFunc;
extern FLTFUNCPTR	mathCoshfFunc;
extern FLTFUNCPTR	mathExpfFunc;
extern FLTFUNCPTR	mathFabsfFunc;
extern FLTFUNCPTR	mathFmodfFunc;
extern FLTFUNCPTR	mathFloorfFunc;
extern FLTFUNCPTR	mathHypotfFunc;
extern FLTFUNCPTR	mathInfinityfFunc;
extern FUNCPTR		mathIrintfFunc;
extern FUNCPTR		mathIroundfFunc;
extern FLTFUNCPTR	mathLogfFunc;
extern FLTFUNCPTR	mathLog2fFunc;
extern FLTFUNCPTR	mathLog10fFunc;
extern FLTFUNCPTR	mathPowfFunc;
extern FLTFUNCPTR	mathRoundfFunc;
extern FLTFUNCPTR	mathSinfFunc;
extern VOIDFUNCPTR	mathSincosfFunc;
extern FLTFUNCPTR	mathSinhfFunc;
extern FLTFUNCPTR	mathSqrtfFunc;
extern FLTFUNCPTR	mathTanfFunc;
extern FLTFUNCPTR	mathTanhfFunc;
extern FLTFUNCPTR	mathTruncfFunc;


/* Locals */

LOCAL char	*errMsgString = "ERROR - %s function not supported\n";

/* Forward references */

LOCAL double 	mathSoftAcos (double dblParam);
LOCAL double 	mathSoftAsin (double dblParam);
LOCAL double 	mathSoftAtan2 (double dblY, double dblX);
LOCAL double 	mathSoftCbrt (double x);
LOCAL double 	mathSoftCeil (double dblParam);
LOCAL double 	mathSoftCosh (double dblParam);
LOCAL double 	mathSoftFabs (double dblParam);
LOCAL double 	mathSoftHypot (double dblX, double dblY);
LOCAL double 	mathSoftLog2 (double dblParam);
LOCAL double 	mathSoftPow (double dblX, double dblY);
LOCAL double 	mathSoftSinh (double dblParam);
LOCAL void 	mathSoftSincos (double dblParam, double *pSinResult, 
				double *pCosResult);
LOCAL double 	mathSoftTanh (double dblParam);

LOCAL float 	mathSoftAcosf (float fltParam);
LOCAL float 	mathSoftAsinf (float fltParam);
LOCAL float 	mathSoftAtan2f (float fltY, float fltX);
LOCAL float 	mathSoftCbrtf (float x);
LOCAL float 	mathSoftCeilf (float fltParam);
LOCAL float 	mathSoftFabsf (float fltParam);
LOCAL float 	mathSoftHypotf (float fltX, float fltY);
LOCAL float	mathSoftLog2f (float fltParam);
LOCAL float 	mathSoftPowf (float fltX, float fltY);
LOCAL void 	mathSoftSincosf (float fltParam, float *pSinResult, 
				 float *pCosResult);


                      				/* unsupported */
LOCAL void 	mathSoftFmod (double dblParam, double dblDivisor);
                      				/* unsupported */
LOCAL void 	mathSoftIrint (double dblParam);
                      				/* unsupported */
LOCAL void 	mathSoftIround (double dblParam);
                      				/* unsupported */
LOCAL void 	mathSoftRound (double dblParam);
                      				/* unsupported */
LOCAL void 	mathSoftTrunc (double dblParam);
                      				/* unsupported */
LOCAL void 	mathSoftCoshf (float fltParam);
                      				/* unsupported */
LOCAL void 	mathSoftFmodf (float fltParam, float fltDivisor);
                      				/* unsupported */
LOCAL void 	mathSoftIrintf (float fltParam);
                      				/* unsupported */
LOCAL void 	mathSoftIroundf (float fltParam);
                      				/* unsupported */
LOCAL void 	mathSoftRoundf (float fltParam);
                      				/* unsupported */
LOCAL void 	mathSoftSinhf (float fltParam);
                      				/* unsupported */
LOCAL void 	mathSoftTanhf (float fltParam);
                      				/* unsupported */
LOCAL void 	mathSoftTruncf (float fltParam);

/******************************************************************************
*
* mathSoftInit - initialize software floating-point math support
*
* This routine places the addresses of the emulated high-level math
* functions (trigonometric functions, etc.) in a set of global variables.
* This allows the standard math functions (e.g., sin(), pow()) to have a
* single entry point but be dispatched to the hardware or software support
* routines, as specified.
*
* This routine is called from usrConfig.c if INCLUDE_SW_FP is defined.
* This definition causes the linker to include the floating-point
* emulation library.
*
* If the system is to use some combination of emulated as well as hardware
* coprocessor floating points, then this routine should be called before calling
* mathHardInit().
*
* RETURNS: N/A
*
* SEE ALSO: mathHardInit()
*
*/

void mathSoftInit ()
    {

    /* Load software routine addresses into global variables
     * defined in mathALib.s.
     */

    /* Double-precision routines */

    mathAcosFunc     = mathSoftAcos;
    mathAsinFunc     = mathSoftAsin;
    mathAtanFunc     = mathSoftAtan;
    mathAtan2Func    = mathSoftAtan2;
    mathCbrtFunc     = mathSoftCbrt;
    mathCeilFunc     = mathSoftCeil;
    mathCosFunc      = mathSoftCos;
    mathCoshFunc     = mathSoftCosh;
    mathExpFunc      = mathSoftExp;
    mathFabsFunc     = mathSoftFabs;
    mathFmodFunc     = (DBLFUNCPTR) mathSoftFmod;	/* unsupported */
    mathFloorFunc    = mathSoftFloor;
    mathHypotFunc    = mathSoftHypot;
    mathInfinityFunc = mathSoftInfinity;
    mathIrintFunc    = (FUNCPTR) mathSoftIrint;		/* unsupported */
    mathIroundFunc   = (FUNCPTR) mathSoftIround;	/* unsupported */
    mathLogFunc      = mathSoftLog;
    mathLog2Func     = mathSoftLog2;
    mathLog10Func    = mathSoftLog10;
    mathPowFunc      = mathSoftPow;
    mathRoundFunc    = (DBLFUNCPTR) mathSoftRound;	/* unsupported */
    mathSinFunc      = mathSoftSin;
    mathSincosFunc   = mathSoftSincos;
    mathSinhFunc     = mathSoftSinh;
    mathSqrtFunc     = mathSoftSqrt;
    mathTanFunc      = mathSoftTan;
    mathTanhFunc     = mathSoftTanh;
    mathTruncFunc    = (DBLFUNCPTR) mathSoftTrunc;	/* unsupported */

    /* Single-precision routines */

    mathAcosfFunc     = (FLTFUNCPTR)mathSoftAcosf;
    mathAsinfFunc     = (FLTFUNCPTR)mathSoftAsinf;
    mathAtanfFunc     = mathSoftAtanf;
    mathAtan2fFunc    = (FLTFUNCPTR)mathSoftAtan2f;
    mathCbrtfFunc     = (FLTFUNCPTR)mathSoftCbrtf;
    mathCeilfFunc     = (FLTFUNCPTR)mathSoftCeilf;
    mathCosfFunc      = mathSoftCosf;
    mathCoshfFunc     = (FLTFUNCPTR) mathSoftCoshf;	/* unsupported */
    mathExpfFunc      = mathSoftExpf;
    mathFabsfFunc     = (FLTFUNCPTR)mathSoftFabsf;
    mathFmodfFunc     = (FLTFUNCPTR) mathSoftFmodf;	/* unsupported */
    mathFloorfFunc    = mathSoftFloorf;
    mathHypotfFunc    = (FLTFUNCPTR)mathSoftHypotf;
    mathInfinityfFunc = mathSoftInfinityf;
    mathIrintfFunc    = (FUNCPTR) mathSoftIrintf;	/* unsupported */
    mathIroundfFunc   = (FUNCPTR) mathSoftIroundf;	/* unsupported */
    mathLogfFunc      = mathSoftLogf;
    mathLog2fFunc     = (FLTFUNCPTR)mathSoftLog2f;
    mathLog10fFunc    = mathSoftLog10f;
    mathPowfFunc      = (FLTFUNCPTR)mathSoftPowf;
    mathRoundfFunc    = (FLTFUNCPTR) mathSoftRoundf;	/* unsupported */
    mathSinfFunc      = mathSoftSinf;
    mathSincosfFunc   = (VOIDFUNCPTR)mathSoftSincosf;
    mathSinhfFunc     = (FLTFUNCPTR) mathSoftSinhf;	/* unsupported */
    mathSqrtfFunc     = mathSoftSqrtf;
    mathTanfFunc      = mathSoftTanf;
    mathTanhfFunc     = (FLTFUNCPTR) mathSoftTanhf;	/* unsupported */
    mathTruncfFunc    = (FLTFUNCPTR) mathSoftTruncf;	/* unsupported */

    }

/******************************************************************************
*
* mathSoftFabs - software floating point absolute value
*
* This routine takes the input double-precision floating point
* parameter and returns the absolute value.
*
* RETURNS: double-precision absolute value.
*/

LOCAL double mathSoftFabs
    (
    double	dblParam
    )
    {

    return ((dblParam < 0.0) ? -dblParam : dblParam);
    }

/******************************************************************************
*
* mathSoftCeil - software floating point ceiling
*
* This routine takes the input double-precision floating point
* parameter and returns (in double-precision floating point format)
* the integer immediately greater than the input parameter.
*
* RETURNS: double-precision representation of next largest integer.
*/

LOCAL double mathSoftCeil 
    (
    double	dblParam
    )
    {
    if (dblParam <= 0.0 && dblParam > -1.0)
	return 0.0;
    else
	return (-floor (-dblParam));
    }

/******************************************************************************
*
* mathSoftPow - software floating point power function
*
* This routine takes two input double-precision floating point
* parameters, <dblX> and <dblY>, and returns the double-precision
* value of <dblX> to the <dblY> power.
*
* INTERNAL
* The US Software emulation library has a special function for taking
* a floating point number to an integer power.  This routine therefore
* checks to see if the <dblY> parameter is an integer, and uses the
* US Software function (XTOI) if it is.
*
* RETURNS: double-precision value of <dblX> to <dblY> power.
*/

LOCAL double mathSoftPow 
    (
    double	dblX,
    double	dblY 
    )
    {

    if (isNaN(dblY))
	return (dblY);			/* dblY = NaN --> NaN */

    if (dblX == 1.0)
	return (1.0);			/* dblX = 1 --> 1 */

    if (dblY == floor (dblY))		/* int dblY --> XTOI(dblX,dblY) */
	return (mathSoftRealtoint (dblX,(long int) dblY));

    return (exp (dblY * log(dblX)));
    }

/******************************************************************************
*
* mathSoftAsin - software floating point arc sine
*
* This routine takes the input double-precision floating point
* parameter and returns the arc sine.
*
* RETURNS: double-precision arc sine value.
*/

LOCAL double mathSoftAsin 
    (
    double	dblParam 
    )
    {

    return (atan (dblParam / sqrt (1.0 - dblParam * dblParam)));
    }

/******************************************************************************
*
* mathSoftAcos - software floating point arc cosine
*
* This routine takes the input double-precision floating point
* parameter and returns the arc cosine.
*
* RETURNS: double-precision arc cosine value.
*/

LOCAL double mathSoftAcos 
    (
    double	dblParam 
    )
    {
    double 	result;

    result = atan (sqrt (1.0 - dblParam * dblParam) / dblParam);

    if (dblParam < 0.0)
	return (result + PI);		/* [pi/2 .. pi] */
    else
	return (result);		/* [0 .. pi/2) */
    }

/******************************************************************************
*
* mathSoftAtan2 - software floating point arc tangent of two arguments
*
* This routine takes two input double-precision floating point
* parameters, <dblY> and <dblX>, and returns the double precision
* arc tangent of <dblY> / <dblX>.
*
* RETURNS: double-precision arc tangent of <dblY> / <dblX>.
*/

LOCAL double mathSoftAtan2 
    (
    double	dblY,
    double	dblX 
    )
    {

    double	result;


    result = atan (dblY/dblX);

    if (dblX >= 0.0)
	return (result);		/* [-pi/2 .. pi/2) */
    else if (result < 0.0)
	return (result + PI);		/* [pi/2 .. pi) */
    else
	return (result - PI);		/* [-pi .. -pi/2) */
    }

/******************************************************************************
*
* mathSoftHypot - software floating point Euclidean distance
*
* This routine takes two input double-precision floating point
* parameters and returns length of the corresponding Euclidean distance
* (hypotenuse).
*
* RETURNS: double-precision hypotenuse.
*/

LOCAL double mathSoftHypot 
    (
    double	dblX,
    double	dblY 
    )
    {

    return (sqrt (dblX * dblX  +  dblY * dblY));
    }


/******************************************************************************
*
* mathSoftCbrt - software floating point cube root
*
* This routine takes the input double-precision floating point
* parameter and returns the cube root.
*
* kahan's cube root (53 bits IEEE double precision)
* for IEEE machines only
* coded in C by K.C. Ng, 4/30/85
*
* Accuracy:
*	better than 0.667 ulps according to an error analysis. Maximum
* error observed was 0.666 ulps in an 1,000,000 random arguments test.
*
* Warning: this code is semi machine dependent; the ordering of words in
* a floating point number must be known in advance. I assume that the
* long interger at the address of a floating point number will be the
* leading 32 bits of that floating point number (i.e., sign, exponent,
* and the 20 most significant bits).
* On a National machine, it has different ordering; therefore, this code
* must be compiled with flag -DNATIONAL.
*
* RETURNS: double-precision cube root.
*
* AUTHOR:  Kahan's cube root, coded in C by K.C. Ng, UC Berkeley, 4/30/85.
*	   Copyright (c) 1985 Regents of the University of California.
*	   Adapted by Scott Huddleston, Computer Research Lab, Tektronix.
*
* Use and reproduction of this software are granted  in  accordance  with
* the terms and conditions specified in  the  Berkeley  Software  License
* Agreement (in particular, this entails acknowledgement of the programs'
* source, and inclusion of this notice) with the additional understanding
* that  all  recipients  should regard themselves as participants  in  an
* ongoing  research  project and hence should  feel  obligated  to report
* their  experiences (good or bad) with these elementary function  codes,
* using "sendbug 4bsd-bugs@BERKELEY", to the authors.
*/

LOCAL double mathSoftCbrt 
    (
    double	x 
    )
    {
	double r,s,t=0.0,w;
	unsigned long *px = (unsigned long *) &x,
	              *pt = (unsigned long *) &t,
		      mexp,sign;

static long B0 = 0x43500000;	/* (double) 2**54 */
static long B1 = 0x2a9f7893,
			B2 = 0x297f7893;	/* B1 / (double) 2**18 */

static double
	    C= 19./35.,
	    D= -864./1225.,
	    E= 99./70.,
	    F= 45./28.,
	    G= 5./14.;

#ifdef NATIONAL /* ordering of words in a floating points number */
	int n0=1,n1=0;
#else
	int n0=0,n1=1;
#endif

	mexp = px[n0]&0x7ff00000;
	if(isINFNaN(x)) return(x); /* cbrt(NaN,INF) is itself */
	if(x==0.0) return(x);	/* cbrt(0) is itself */

	sign=px[n0]&0x80000000; /* sign= sign(x) */
	px[n0] ^= sign;		/* x=|x| */

    /* rough cbrt to 5 bits */
	if(mexp==0) 		/* subnormal number */
	  {pt[n0]=B0;		/* scale t to be normal, 2^54 or 2^18 */
	   t*=x; pt[n0]=pt[n0]/3+B2;
	  }
	else
	  pt[n0]=px[n0]/3+B1;

    /* new cbrt to 23 bits, may be implemented in single precision */
	r=t*t/x;
	s=C+r*t;
	t*=G+F/(s+E+D/s);

    /* chopped to 20 bits and make it larger than cbrt(x) */
	pt[n1]=0; pt[n0]+=0x00000001;

    /* one step newton iteration to 53 bits with error less than 0.667 ulps */
	s=t*t;		/* t*t is exact */
	r=x/s;
	w=t+t;
	r=(r-t)/(w+r);	/* r-s is exact */
	t=t+t*r;

    /* restore the sign bit */
	pt[n0] |= sign;
	return(t);
    }

/*******************************************************************************
*
* mathSoftCosh - software emulation for floating point hyperbolic cosine
*
*
*	Computes hyperbolic cosine from:
*
*		cosh(X) = 0.5 * (exp(X) + exp(-X))
*
*	Returns double precision hyperbolic cosine of double precision
*	floating point number.
*
*	Inputs greater than log(DBL_MAX) result in overflow.
*	Inputs less than log(DBL_MIN) result in underflow.
*
*	For precision information refer to documentation of the
*	floating point library routines called.
*
* RETURNS: double-precision hyperbolic cosine
*
* AUTHOR: Fred Fish
*
*				N O T I C E
*
*			Copyright Abandoned, 1987, Fred Fish
*
*	This previously copyrighted work has been placed into the
*	public domain by the author (Fred Fish) and may be freely used
*	for any purpose, private or commercial.  I would appreciate
*	it, as a courtesy, if this notice is left in all copies and
*	derivative works.  Thank you, and enjoy...
*
*/

LOCAL double mathSoftCosh 
    (
    double	dblParam 
    )
    {
    double	retValue;

    if (dblParam > LOGE_MAXDOUBLE)
	{
	retValue = HUGE_VAL;
        }
    else if (dblParam < LOGE_MINDOUBLE)
	{
	retValue = -HUGE_VAL;
    	}
    else
	{
	dblParam = exp (dblParam);
	retValue = 0.5 * (dblParam + 1.0/dblParam);
    	}

    return (retValue);
    }

/*******************************************************************************
*
* mathSoftSinh - software emulation for floating point hyperbolic sine
*
*	Computes hyperbolic sine from:
*
*		sinh(x) = 0.5 * (exp(x) - 1.0/exp(x))
*
*
*	Returns double precision hyperbolic sine of double precision
*	floating point number.
*
*	Inputs greater than ln(DBL_MAX) result in overflow.
*	Inputs less than ln(DBL_MIN) result in underflow.
*
* RETURNS: double-precision hyperbolic sine
*
* AUTHOR: Fred Fish
*
*				N O T I C E
*
*			Copyright Abandoned, 1987, Fred Fish
*
*	This previously copyrighted work has been placed into the
*	public domain by the author (Fred Fish) and may be freely used
*	for any purpose, private or commercial.  I would appreciate
*	it, as a courtesy, if this notice is left in all copies and
*	derivative works.  Thank you, and enjoy...
*
*/

LOCAL double mathSoftSinh 
    (
    double	dblParam 
    )
    {
    double	retValue;


    if (dblParam > LOGE_MAXDOUBLE)
	{
	retValue = HUGE_VAL;
    	}
    else if (dblParam < LOGE_MINDOUBLE)
	{
	retValue = -HUGE_VAL;
    	}
    else
	{
	dblParam = exp (dblParam);
	retValue = 0.5 * (dblParam - (1.0 / dblParam));
    	}

    return (retValue);
    }

/*******************************************************************************
*
* mathSoftTanh - software emulation for floating point hyperbolic tangent
*
*
*	Computes hyperbolic tangent from:
*
*		tanh(x) = 1.0 for x > TANH_MAXARG
*			= -1.0 for x < -TANH_MAXARG
*			=  sinh(x) / cosh(x) otherwise
*
*	Returns double precision hyperbolic tangent of double precision
*	floating point number.
*
* RETURNS: double-precision hyperbolic tangent
*
* AUTHOR: Fred Fish
*
*				N O T I C E
*
*			Copyright Abandoned, 1987, Fred Fish
*
*	This previously copyrighted work has been placed into the
*	public domain by the author (Fred Fish) and may be freely used
*	for any purpose, private or commercial.  I would appreciate
*	it, as a courtesy, if this notice is left in all copies and
*	derivative works.  Thank you, and enjoy...
*/

LOCAL double mathSoftTanh 
    (
    double	dblParam 
    )
    {
    double	retValue;

    if (dblParam > TANH_MAXARG || dblParam < -TANH_MAXARG)
	{
	if (dblParam > 0.0)
	    retValue = 1.0;
	else
	    retValue = -1.0;
    	}
    else
	{
	retValue = sinh (dblParam) / cosh (dblParam);
    	}

    return (retValue);
    }

/*******************************************************************************
*
* mathSoftLog2 - software emulation for floating point log base 2
*
* This routine returns a double-precision log base 2 of a double-precision
* floating point number.
*
*	Computes log2(x) from:
*
*		log2(x) = log2(e) * log(x)
*
*
* RETURNS: double-precision log base 2
*
*/

LOCAL double mathSoftLog2 
    (
    double	dblParam 
    )
    {

    return (LOG2E * log (dblParam));
    }

/*******************************************************************************
*
* mathSoftSincos - software emulation for simultaneous sine and cosine
*
* This routine obtains both the sine and cosine for the specified
* floating point value and returns both.
*
* NOMANUAL
*/

void mathSoftSincos 
    (
    double	dblParam,		/* angle in radians */
    double	*pSinResult,		/* sine result buffer */
    double	*pCosResult 		/* cosine result buffer */
    )
    {

    *pSinResult = sin (dblParam);
    *pCosResult = cos (dblParam);
    }


/******************************************************************************
*
* mathSoftFabsf - single-precision software floating point absolute value
*
* This routine takes the input single-precision floating point
* parameter and returns the absolute value.
*
* RETURNS: single-precision absolute value.
*/

LOCAL float mathSoftFabsf 
    (
    float	fltParam 
    )
    {

    return ((fltParam < 0.0) ? -fltParam : fltParam);
    }

/******************************************************************************
*
* mathSoftCeilf - single-precision software floating point ceiling
*
* This routine takes the input single-precision floating point
* parameter and returns (in single-precision floating point format)
* the integer immediately greater than the input parameter.
*
* RETURNS: single-precision representation of next largest integer.
*/

LOCAL float mathSoftCeilf 
    (
    float	fltParam 
    )
    {

    return (-floorf (-fltParam));
    }

/******************************************************************************
*
* mathSoftPowf - single-precision software floating point power function
*
* This routine takes two input single-precision floating point
* parameters, <fltX> and <fltY>, and returns the single-precision
* value of <fltX> to the <fltY> power.
*
* INTERNAL
* The US Software emulation library has a special function for taking
* a floating point number to an integer power.  This routine therefore
* checks to see if the <fltY> parameter is an integer, and uses the
* US Software function (XTOI) if it is.
*
* RETURNS: single-precision value of <fltX> to <fltY> power.
*/

LOCAL float mathSoftPowf 
    (
    float	fltX,
    float	fltY 
    )
    {

    if (isNaN_SINGLE (fltY))
	return (fltY);			/* fltY = NaN --> NaN */

    if (fltX == 1.0)
	return (1.0);			/* fltX = 1 --> 1 */

    if (fltY == floorf (fltY))		/* int fltY --> XTOI(fltX,fltY) */
	return (mathSoftRealtointf (fltX, (long int) fltY));

    return (expf (fltY * logf (fltX)));
    }

/******************************************************************************
*
* mathSoftAsinf - single-precision software floating point arc sine
*
* This routine takes the input single-precision floating point
* parameter and returns the arc sine.
*
* RETURNS: single-precision arc sine value.
*/

LOCAL float mathSoftAsinf 
    (
    float	fltParam 
    )
    {

    return atanf (fltParam / sqrtf (1.0 - fltParam * fltParam));
    }

/******************************************************************************
*
* mathSoftAcosf - single-precision software floating point arc cosine
*
* This routine takes the input single-precision floating point
* parameter and returns the arc cosine.
*
* RETURNS: single-precision arc cosine value.
*/

LOCAL float mathSoftAcosf 
    (
    float	fltParam 
    )
    {
    float 	result;

    result = atanf (sqrtf(1.0 - fltParam * fltParam) / fltParam);

    if (fltParam < 0.0)
	return (result + PI_SINGLE);	/* [pi/2 .. pi] */
    else
	return (result);		/* [0 .. pi/2) */
    }

/******************************************************************************
*
* mathSoftAtan2f - single-precision software arc tangent of two arguments
*
* This routine takes two input single-precision floating point
* parameters, <fltY> and <fltX>, and returns the single precision
* arc tangent of <fltY> / <fltX>.
*
* RETURNS: single-precision arc tangent of <fltY> / <fltX>.
*/

LOCAL float mathSoftAtan2f 
    (
    float	fltY,
    float	fltX 
    )
    {
    float 	result;

    result = atanf(fltY/fltX);

    if (fltX >= 0.0)
	return (result);			/* [-pi/2 .. pi/2) */
    else if (result < 0.0)
	return (result + PI_SINGLE);		/* [pi/2 .. pi) */
    else
	return (result - PI_SINGLE);		/* [-pi .. -pi/2) */
    }

/******************************************************************************
*
* mathSoftHypotf - single-precision software floating point Euclidean distance
*
* This routine takes two input single-precision floating point
* parameters and returns length of the corresponding Euclidean distance
* (hypotenuse).
*
* RETURNS: single-precision hypotenuse.
*/

LOCAL float mathSoftHypotf 
    (
    float	fltX,
    float	fltY 
    )
    {

    return (sqrtf (fltX * fltX  +  fltY * fltY));
    }


/******************************************************************************
*
* mathSoftCbrtf - single-precision software floating point cube root
*
* This routine takes the input single-precision floating point
* parameter and returns the cube root.
*
* RETURNS: single-precision cube root.
*
* AUTHOR:  Kahan's cube root, coded in C by K.C. Ng, UC Berkeley, 4/30/85.
*	   Copyright (c) 1985 Regents of the University of California.
*	   Adapted by Scott Huddleston, Computer Research Lab, Tektronix.
*/

LOCAL float mathSoftCbrtf 
    (
    float	x 
    )
    {
	float r,s,t=0.0;
	unsigned long *px = (unsigned long *) &x,
	              *pt = (unsigned long *) &t,
		      mexp,sign;

    static long B0 = 0x4b800000;	/* (float) 2**24 */
    static long B1 = 0x2a4ed9f4,
			B2 = 0x29ced9f4;	/* B1 / (float) 2**8 */

    static float
	    C= 19./35.,
	    D= -864./1225.,
	    E= 99./70.,
	    F= 45./28.,
	    G= 5./14.;

#ifdef NATIONAL /* ordering of words in a floating points number */
	int n0=1;
#else
	int n0=0;
#endif

	mexp = px[n0]&0x7ff00000;
	if(isINFNaN_SINGLE(x)) return(x); /* cbrt(NaN,INF) is itself */
	if(x==0.0) return(x);		/* cbrt(0) is itself */

	sign=px[n0]&0x80000000; /* sign= sign(x) */
	px[n0] ^= sign;		/* x=|x| */

    /* rough cbrt to 5 bits */
	if(mexp==0) 		/* subnormal number */
	  {pt[n0]=B0;		/* scale t to be normal, 2^54 or 2^18 */
	   t*=x; pt[n0]=pt[n0]/3+B2;
	  }
	else
	  pt[n0]=px[n0]/3+B1;

    /* new cbrt to 23 bits, may be implemented in single precision */
	r=t*t/x;
	s=C+r*t;
	t*=G+F/(s+E+D/s);


    /* restore the sign bit */
	pt[n0] |= sign;
	return(t);
    }

/*******************************************************************************
*
* mathSoftLog2f - single-precision emulation for floating point log base 2
*
* This routine returns a single-precision log base 2 of a single-precision
* floating point number.
*
*	Computes log2(x) from:
*
*		log2(x) = log2(e) * log(x)
*
*
* RETURNS: single-precision log base 2
*
*/

LOCAL float mathSoftLog2f 
    (
    float	fltParam 
    )
    {

    return (LOG2E * logf (fltParam));
    }

/*******************************************************************************
*
* mathSoftSincosf - single-precision emulation for simultaneous sine and cosine
*
* This routine obtains both the sine and cosine for the specified
* floating point value and returns both.
*
* NOMANUAL
*/

void mathSoftSincosf 
    (
    float	fltParam,		/* angle in radians */
    float	*pSinResult,		/* sine result buffer */
    float	*pCosResult 		/* cosine result buffer */
    )
    {

    *pSinResult = sinf (fltParam);
    *pCosResult = cosf (fltParam);
    }

/*******************************************************************************
*
* mathSoftFmod - double-precision software floating-point modulus
*
* THIS FUNCTION IS NOT SUPPORTED.  If called, a logMsg() call will display
* an error message on the VxWorks console.
*
*/

LOCAL void mathSoftFmod 
    (
    double	dblParam,		/* argument */
    double	dblDivisor 		/* divisor */
    )
    {
    if (_func_logMsg != NULL)
	(* _func_logMsg) (errMsgString, (int) "fmod", 0, 0, 0, 0, 0);
    }

/*******************************************************************************
*
* mathSoftIrint - double-precision software floating-point round to integer
*
* THIS FUNCTION IS NOT SUPPORTED.  If called, a logMsg() call will display
* an error message on the VxWorks console.
*
*/

LOCAL void mathSoftIrint 
    (
    double	dblParam 
    )
    {
    if (_func_logMsg != NULL)
	(* _func_logMsg) (errMsgString, (int) "irint", 0, 0, 0, 0, 0);
    }

/*******************************************************************************
*
* mathSoftIround - double-precision software floating-point round to nearest
*
* THIS FUNCTION IS NOT SUPPORTED.  If called, a logMsg() call will display
* an error message on the VxWorks console.
*
*/

LOCAL void mathSoftIround 
    (
    double	dblParam 
    )
    {
    if (_func_logMsg != NULL)
	(* _func_logMsg) (errMsgString, (int) "iround", 0, 0, 0, 0, 0);
    }

/*******************************************************************************
*
* mathSoftRound - double-precision software floating-point round
*
* THIS FUNCTION IS NOT SUPPORTED.  If called, a logMsg() call will display
* an error message on the VxWorks console.
*
*/

LOCAL void mathSoftRound 
    (
    double	dblParam 
    )
    {
    if (_func_logMsg != NULL)
	(* _func_logMsg) (errMsgString, (int) "round", 0, 0, 0, 0, 0);
    }

/*******************************************************************************
*
* mathSoftTrunc - double-precision software floating-point truncation
*
* THIS FUNCTION IS NOT SUPPORTED.  If called, a logMsg() call will display
* an error message on the VxWorks console.
*
*/

LOCAL void mathSoftTrunc 
    (
    double	dblParam 
    )
    {
    if (_func_logMsg != NULL)
	(* _func_logMsg) (errMsgString, (int) "trunc", 0, 0, 0, 0, 0);
    }

/*******************************************************************************
*
* mathSoftFmodf - single-precision software floating-point modulus
*
* THIS FUNCTION IS NOT SUPPORTED.  If called, a logMsg() call will display
* an error message on the VxWorks console.
*
*/

LOCAL void mathSoftFmodf 
    (
    float	fltParam,		/* argument */
    float	fltDivisor 		/* divisor */
    )
    {
    if (_func_logMsg != NULL)
	(* _func_logMsg) (errMsgString, (int) "fmodf", 0, 0, 0, 0, 0);
    }

/*******************************************************************************
*
* mathSoftIrintf - single-precision software floating-point round to integer
*
* THIS FUNCTION IS NOT SUPPORTED.  If called, a logMsg() call will display
* an error message on the VxWorks console.
*
*/

LOCAL void mathSoftIrintf 
    (
    float	fltParam 
    )
    {
    if (_func_logMsg != NULL)
	(* _func_logMsg) (errMsgString, (int) "irintf", 0, 0, 0, 0, 0);
    }

/*******************************************************************************
*
* mathSoftIroundf - single-precision software floating-point round to nearest
*
* THIS FUNCTION IS NOT SUPPORTED.  If called, a logMsg() call will display
* an error message on the VxWorks console.
*
*/

LOCAL void mathSoftIroundf 
    (
    float	fltParam 
    )
    {
    if (_func_logMsg != NULL)
	(* _func_logMsg) (errMsgString, (int) "iroundf", 0, 0, 0, 0, 0);
    }

/*******************************************************************************
*
* mathSoftRoundf - single-precision software floating-point round
*
* THIS FUNCTION IS NOT SUPPORTED.  If called, a logMsg() call will display
* an error message on the VxWorks console.
*
*/

LOCAL void mathSoftRoundf 
    (
    float	fltParam
    )
    {
    if (_func_logMsg != NULL)
	(* _func_logMsg) (errMsgString, (int) "roundf", 0, 0, 0, 0, 0);
    }

/*******************************************************************************
*
* mathSoftCoshf - single-precision software hyperbolic cosine
*
* THIS FUNCTION IS NOT SUPPORTED.  If called, a logMsg() call will display
* an error message on the VxWorks console.
*
*/

LOCAL void mathSoftCoshf 
    (
    float	fltParam
    )
    {
    if (_func_logMsg != NULL)
	(* _func_logMsg) (errMsgString, (int) "coshf", 0, 0, 0, 0, 0);
    }

/*******************************************************************************
*
* mathSoftSinhf - single-precision software hyperbolic sine
*
* THIS FUNCTION IS NOT SUPPORTED.  If called, a logMsg() call will display
* an error message on the VxWorks console.
*
*/

LOCAL void mathSoftSinhf 
    (
    float	fltParam
    )
    {
    if (_func_logMsg != NULL)
	(* _func_logMsg) (errMsgString, (int) "sinhf", 0, 0, 0, 0, 0);
    }

/*******************************************************************************
*
* mathSoftTanhf - single-precision software hyperbolic tangent
*
* THIS FUNCTION IS NOT SUPPORTED.  If called, a logMsg() call will display
* an error message on the VxWorks console.
*
*/

LOCAL void mathSoftTanhf 
    (
    float	fltParam
    )
    {
    if (_func_logMsg != NULL)
	(* _func_logMsg) (errMsgString, (int) "tanhf", 0, 0, 0, 0, 0);
    }

/*******************************************************************************
*
* mathSoftTruncf - single-precision software floating-point truncation
*
* THIS FUNCTION IS NOT SUPPORTED.  If called, a logMsg() call will display
* an error message on the VxWorks console.
*
*/

LOCAL void mathSoftTruncf 
    (
    float	fltParam
    )
    {
    if (_func_logMsg != NULL)
	(* _func_logMsg) (errMsgString, (int) "truncf", 0, 0, 0, 0, 0);
    }

