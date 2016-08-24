/* mathHardLib.c - hardware floating-point math library */

/* Copyright 1984-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01o,28feb97,tam  installed overflow and underflow handlers back for 68060.  
01n,23may96,ms   fixed SPR 4963 by not installing motorola fpp exc handlers
01m,29mar95,kdl  made MC68060 use gccUss040Lib support.
01l,03feb95,rhp  doc: warn not all archs support hw float pt
01k,27jun94,tpr  added MC68060 cpu support.
01j,21nov92,jdi	 documentation cleanup.
01i,13nov92,jcf	 made logMsg calls indirect to reduce coupling.
01h,13oct92,jdi	 documentation.
01g,19sep92,kdl	 made mathHardInit() call fppInit() and fppProbe().
01f,30jul92,kdl	 made mathHardInit() call gccUss040Init() for 68040.
01e,30jul92,kdl	 changed to ANSI single precision names (e.g. fsin -> sinf)
01d,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01c,20jan92,kdl Added 68040 floating point software package (FPSP) support.
	    shl ANSI cleanup.
01b,04oct91,rrr  passed through the ansification filter
		  -changed VOID to void
		  -changed copyright notice
01a,28jan91,kdl	 written.
*/

/*
DESCRIPTION
This library provides support routines for using hardware floating-point
units with high-level math functions.  The high-level functions include
triginometric operations, exponents, and so forth.

The routines in this library are used automatically for high-level
math functions only if mathHardInit() has been called previously.

WARNING
Not all architectures support hardware floating-point.  See the
architecture-specific appendices of the
.I VxWorks Programmer's Guide.

INCLUDE FILES: math.h

SEE ALSO: mathSoftLib, mathALib,
.I VxWorks Programmer's Guide
architecture-specific appendices
*/

#include "vxWorks.h"
#include "math.h"
#include "logLib.h"
#include "fppLib.h"
#include "intLib.h"
#include "private/funcBindP.h"

#if ((CPU==MC68040) || (CPU==MC68060))
#include "arch/mc68k/ivMc68k.h"
#endif

#if (CPU==MC68060)
#include "fpsp060Lib.h"
#endif

/* Externals */

extern double  mathHardAcos ();	/* functions in mathHardALib.s */
extern double  mathHardAsin ();
extern double  mathHardAtan ();
extern double  mathHardAtan2 ();
extern double  mathHardCeil ();
extern double  mathHardCos ();
extern double  mathHardCosh ();
extern double  mathHardExp ();
extern double  mathHardFabs ();
extern double  mathHardFloor ();
extern double  mathHardFmod ();
extern double  mathHardInfinity ();
extern int     mathHardIrint ();
extern int     mathHardIround ();
extern double  mathHardLog ();
extern double  mathHardLog2 ();
extern double  mathHardLog10 ();
extern double  mathHardPow ();
extern double  mathHardRound ();
extern double  mathHardSin ();
extern void    mathHardSincos ();
extern double  mathHardSinh ();
extern double  mathHardSqrt ();
extern double  mathHardTan ();
extern double  mathHardTanh ();
extern double  mathHardTrunc ();

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
extern FLTFUNCPTR	mathFloorfFunc;
extern FLTFUNCPTR	mathFmodfFunc;
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

extern void		mathErrNoInit ();
					/* initial value of function ptrs */


#if ((CPU==MC68040) || (CPU==MC68060))

/* Exception handlers which must be installed for 68040 support: */
IMPORT void     _x_fpsp_ill_inst ();
IMPORT void     _x_fpsp_fline ();
IMPORT void     _x_fpsp_bsun ();
IMPORT void     _x_fpsp_inex ();
IMPORT void     _x_fpsp_dz ();
IMPORT void     _x_fpsp_unfl ();
IMPORT void     _x_fpsp_operr ();
IMPORT void     _x_fpsp_ovfl ();
IMPORT void     _x_fpsp_snan ();
IMPORT void     _x_fpsp_unsupp ();

#endif /* ((CPU==MC68040) || (CPU==MC68060)) */

/* Forward declarations */

LOCAL void	mathHardNoSingle ();
LOCAL void	mathHardCbrt ();
LOCAL void	mathHardHypot ();


/******************************************************************************
*
* mathHardInit - initialize hardware floating-point math support
*
* This routine places the addresses of the hardware high-level math
* functions (trigonometric functions, etc.) in a set of global variables.
* This allows the standard math functions (e.g., sin(), pow()) to have a
* single entry point but to be dispatched to the hardware or software
* support routines, as specified.
*
* This routine is called from usrConfig.c if INCLUDE_HW_FP is defined.  This
* definition causes the linker to include the floating-point hardware
* support library.
*
* Certain routines in the floating-point software emulation library do not
* have equivalent hardware support routines.  (These are primarily routines
* that handle single-precision floating-point numbers.)  If no emulation
* routine address has already been put in the global variable for this
* function, the address of a dummy routine that logs an error message is
* placed in the variable; if an emulation routine address is present (the
* emulation initialization, via mathSoftInit(), must be done prior to
* hardware floating-point initialization), the emulation routine address is
* left alone.  In this way, hardware routines will be used for all available
* functions, while emulation will be used for the missing functions.
*
* RETURNS: N/A
*
* SEE ALSO: mathSoftInit()
*/

void mathHardInit ()
    {

#if (CPU==MC68040) 

    /*
     * Load exception vectors with addresses of routines from
     * Motorola 68040 floating point library.
     *
     * The 040 floating point emulation software needs to filter the
     * following vectors, itself.  If an error is caught that must be
     * passed back to the OS, then that software package will do it
     * by re-establishing the exception frame and jumping directly
     * into "excStub".
     */

    intVecSet ((FUNCPTR *)IV_ILLEGAL_INSTRUCTION,  (FUNCPTR) _x_fpsp_ill_inst);
    intVecSet ((FUNCPTR *)IV_LINE_1111_EMULATOR,   (FUNCPTR) _x_fpsp_fline);
    intVecSet ((FUNCPTR *)IV_FPCP_B_S_U_CONDITION, (FUNCPTR) _x_fpsp_bsun);
    intVecSet ((FUNCPTR *)IV_FPCP_INEXACT_RESULT,  (FUNCPTR) _x_fpsp_inex);
    intVecSet ((FUNCPTR *)IV_DIVIDE_BY_ZERO,	   (FUNCPTR) _x_fpsp_dz);
    intVecSet ((FUNCPTR *)IV_UNDERFLOW,	   	   (FUNCPTR) _x_fpsp_unfl);
    intVecSet ((FUNCPTR *)IV_OPERAND_ERROR,	   (FUNCPTR) _x_fpsp_operr);
    intVecSet ((FUNCPTR *)IV_OVERFLOW,		   (FUNCPTR) _x_fpsp_ovfl);
    intVecSet ((FUNCPTR *)IV_SIGNALING_NAN,	   (FUNCPTR) _x_fpsp_snan);
    intVecSet ((FUNCPTR *)IV_UNIMP_DATA_TYPE,	   (FUNCPTR) _x_fpsp_unsupp);

#endif /* (CPU==MC68040) */

#if (CPU==MC68060)

    /*
     * Load exception vectors with addresses of routines from
     * Motorola 68060 floating point exception handlers.
     *
     * The 060 floating point emulation software needs to filter the
     * following vectors, itself.  If an error is caught that must be
     * passed back to the OS, then that software package will do it
     * by re-establishing the exception frame and jumping directly
     * into "excStub".
     */

    fpsp060COTblInit ();

    intVecSet ((FUNCPTR *)IV_LINE_1111_EMULATOR,   (FUNCPTR) (FPSP_060_FLINE));
    intVecSet ((FUNCPTR *)IV_UNIMP_DATA_TYPE,	   (FUNCPTR) (FPSP_060_UNSUPP));
    intVecSet ((FUNCPTR *)IV_UNIMP_EFFECTIVE_ADDRESS,(FUNCPTR) (FPSP_060_EFFADD));

    /*
     * underflow and overflow exceptions are non maskable: therefore we need
     * exception handlers to be installed by default to trap these exception.
     */

    intVecSet ((FUNCPTR *)IV_UNDERFLOW,	   	   (FUNCPTR) (FPSP_060_UNFL));
    intVecSet ((FUNCPTR *)IV_OVERFLOW,		   (FUNCPTR) (FPSP_060_OVFL));

#if 0
    /*
     *
     */

    intVecSet ((FUNCPTR *)IV_FPCP_INEXACT_RESULT,  (FUNCPTR) (FPSP_060_INEX));
    intVecSet ((FUNCPTR *)IV_DIVIDE_BY_ZERO,	   (FUNCPTR) (FPSP_060_DZ));
    intVecSet ((FUNCPTR *)IV_OPERAND_ERROR,	   (FUNCPTR) (FPSP_060_OPERR));
    intVecSet ((FUNCPTR *)IV_SIGNALING_NAN,	   (FUNCPTR) (FPSP_060_SNAN));
#endif
#endif /* (CPU==MC68060) */


#if ((CPU==MC68040) || (CPU==MC68060))
    gccUss040Init ();		/* harmless call to drag in gccUss040Lib.o */

#endif /* (CPU==MC68040 || CPU==MC68060) */


    /* Don't do more unless there really is hardware support */

    fppInit();			/* attempt to init hardware support */

    if (fppProbe() != OK)	/* is there a coprocessor? */
	return;			/*  exit if no */


    /* Load hardware routine addresses into global variables
     * defined in mathALib.s.
     */

    /* Double-precision routines */

    mathAcosFunc     = mathHardAcos;
    mathAsinFunc     = mathHardAsin;
    mathAtanFunc     = mathHardAtan;
    mathAtan2Func    = mathHardAtan2;
    mathCeilFunc     = mathHardCeil;
    mathCosFunc      = mathHardCos;
    mathCoshFunc     = mathHardCosh;
    mathExpFunc      = mathHardExp;
    mathFabsFunc     = mathHardFabs;
    mathFmodFunc     = mathHardFmod;
    mathFloorFunc    = mathHardFloor;
    mathInfinityFunc = mathHardInfinity;
    mathIrintFunc    = mathHardIrint;
    mathIroundFunc   = mathHardIround;
    mathLogFunc      = mathHardLog;
    mathLog2Func     = mathHardLog2;
    mathLog10Func    = mathHardLog10;
    mathPowFunc      = mathHardPow;
    mathRoundFunc    = mathHardRound;
    mathSinFunc      = mathHardSin;
    mathSincosFunc   = mathHardSincos;
    mathSinhFunc     = mathHardSinh;
    mathSqrtFunc     = mathHardSqrt;
    mathTanFunc      = mathHardTan;
    mathTanhFunc     = mathHardTanh;
    mathTruncFunc    = mathHardTrunc;


    /* Single-precision functions (unsupported) */

    if (mathAcosfFunc == (FLTFUNCPTR) mathErrNoInit)
    	mathAcosfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathAsinfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathAsinfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathAtanfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathAtanfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathAtan2fFunc == (FLTFUNCPTR) mathErrNoInit)
	mathAtan2fFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathCbrtfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathCbrtfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathCeilfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathCeilfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathCosfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathCosfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathCoshfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathCoshfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathExpfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathExpfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFabsfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFabsfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFmodfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFmodfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathFloorfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathFloorfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathHypotfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathHypotfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathInfinityfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathInfinityfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathIrintfFunc == (FUNCPTR) mathErrNoInit)
	mathIrintfFunc = (FUNCPTR) mathHardNoSingle;

    if (mathIroundfFunc == (FUNCPTR) mathErrNoInit)
	mathIroundfFunc = (FUNCPTR) mathHardNoSingle;

    if (mathLogfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathLogfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathLog2fFunc == (FLTFUNCPTR) mathErrNoInit)
	mathLog2fFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathLog10fFunc == (FLTFUNCPTR) mathErrNoInit)
	mathLog10fFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathPowfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathPowfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathRoundfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathRoundfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathSinfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathSinfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathSincosfFunc == mathErrNoInit)
	mathSincosfFunc = mathHardNoSingle;

    if (mathSinhfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathSinhfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathSqrtfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathSqrtfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathTanfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathTanfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathTanhfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathTanhfFunc = (FLTFUNCPTR) mathHardNoSingle;

    if (mathTruncfFunc == (FLTFUNCPTR) mathErrNoInit)
	mathTruncfFunc = (FLTFUNCPTR) mathHardNoSingle;


    /* Miscellaneous unsupported functions */

    if (mathCbrtFunc == (DBLFUNCPTR) mathErrNoInit)
    	mathCbrtFunc = (DBLFUNCPTR) mathHardCbrt;

    if (mathHypotFunc == (DBLFUNCPTR) mathErrNoInit)
    	mathHypotFunc = (DBLFUNCPTR) mathHardHypot;

    }

/******************************************************************************
*
* mathHardNoSingle - log error message for unsupported single-precision fp
*
* This routine will generate a log message to the VxWorks console if
* an attempt is made to use single-precision math functions with the
* hardware floating point coprocessor (not supported).
*
*/

LOCAL void mathHardNoSingle ()
    {

    if (_func_logMsg != NULL)
	(* _func_logMsg) ("ERROR - single-precision flt. point not supported\n",
	    		  0,0,0,0,0,0);
    }

/******************************************************************************
*
* mathHardCbrt - log error message for unsupported cube root function
*
* This routine will generate a log message to the VxWorks console if
* an attempt is made to use the cbrt() cube root function with the
* hardware floating point coprocessor (not supported).
*
*/

LOCAL void mathHardCbrt ()
    {

    if (_func_logMsg != NULL)
	(* _func_logMsg) ("ERROR - floating point cube root not supported\n",
	   		  0,0,0,0,0,0);
    }

/******************************************************************************
*
* mathHardHypot - log error message for unsupported hypot function
*
* This routine will generate a log message to the VxWorks console if
* an attempt is made to use the hypot() Euclidean distance function with
* the hardware floating point coprocessor (not supported).
*
*/

LOCAL void mathHardHypot ()
    {

    if (_func_logMsg != NULL)
	(* _func_logMsg) ("ERROR - h/w floating point hypot not supported\n",
	   		  0,0,0,0,0,0);
    }

