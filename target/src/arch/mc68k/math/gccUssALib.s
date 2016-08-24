/* Copyright 1984-1994 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01g,28apr94,kdl  changed fixdfsi() to call correct routine to round-toward-zero.
01f,07sep92,smb  added gccUssInit()
01e,23aug92,jcf  changed bxxx to jxx.
01d,05jun92,kdl  changed external branches to jumps (and bsr's to jsr's).
01c,26may92,rrr  the tree shuffle
01b,04oct91,rrr  passed through the ansification filter
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01a,28jan91,kdl	 original version, based on library developed by S. Huddleston.
*/

/*
DESCRIPTION

This file contains assembly language interface routines which allow the
US Software floating point emulation package to be used with the Gnu
C compiler (gcc).  The US Software package uses non-C conventions for
parameter passing on the stack, and some functions use the CPU's
status register bits to indicate results.  These routines convert
these non-standard interfaces to normal C parameter passing.  The
names for functions are those which are generated by the gcc compiler.

The routines contained in this library provide basic low-level floating point
operations, including multiplication, addition, comparison, etc.

AUTHOR
Scott Huddleston, Computer Research Lab, Tektronix.
  Copyright 1990, Tektronix Inc.

NOMANUAL
*/

#define	_ASMLANGUAGE

	.globl	FPERR,NANFLG,INFFLG,UNFFLG
|
	.data			| On-board RWM
|
FPERR:	.space	1
NANFLG:	.space	1
INFFLG:	.space	1
UNFFLG:	.space	1

	.text
	.even

	.globl	___adddf3
	.globl	___subdf3
	.globl	___muldf3
	.globl	___divdf3
	.globl	___cmpdf2
	.globl	___negdf2
	.globl	___addsf3
	.globl	___subsf3
	.globl	___mulsf3
	.globl	___divsf3
	.globl	___cmpsf2
	.globl	___negsf2
	.globl	___truncdfsf2
	.globl	___extendsfdf2
	.globl	___fixdfsi
	.globl	___fixunsdfsi
	.globl	___floatsidf
	.globl	XDPSUB
	.globl	XDPNEG
	.globl	XDPCMP
	.globl	XFPSUB
	.globl	XFPNEG
	.globl	XFPCMP
	.globl	XDINT
	.globl	XDFLOAT

	.globl	_gccUssInit


/*******************************************************************************
*
* gccUssInit -  This routine will cause this module to be included in
*	         the vxWorks build.
*
* RETURNS: 
*
* NOMANUAL
*/

_gccUssInit:
	link	a6,#-4
	unlk	a6
	rts

/*******************************************************************************
* adddf3 - double-precision floating point addition function using gcc
*
* RETURNS: double precision addition result.
*
* NOMANUAL
*/

___adddf3:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	movel	a0@(12),sp@-
	movel	a0@(8),sp@-
	jsr	DPADD
	movel	sp@+,d0
	movel	sp@+,d1
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
* subdf3 - double-precision floating point subtraction function using gcc
*
* RETURNS: double precision subtraction result.
*
* NOMANUAL
*/

___subdf3:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	movel	a0@(12),sp@-
	movel	a0@(8),sp@-
	jsr	XDPSUB
	movel	sp@+,d0
	movel	sp@+,d1
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
* muldf3 - double-precision floating point multiplication function using gcc
*
* RETURNS: double precision multiplication result.
*
* NOMANUAL
*/

___muldf3:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	movel	a0@(12),sp@-
	movel	a0@(8),sp@-
	jsr	DPMUL
	movel	sp@+,d0
	movel	sp@+,d1
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
* divdf3 - double-precision floating point division function using gcc
*
* RETURNS: double precision division result.
*
* NOMANUAL
*/

___divdf3:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	movel	a0@(12),sp@-
	movel	a0@(8),sp@-
	jsr	DPDIV
	movel	sp@+,d0
	movel	sp@+,d1
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
* cmpdf2 - double-precision floating point comparison function using gcc
*
* RETURNS: double precision comparison result.
*
* NOMANUAL
*/

___cmpdf2:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	subaw	#8,sp
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	movel	a0@(12),sp@-
	movel	a0@(8),sp@-
	jsr	XDPCMP
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
* negdf2 - double-precision floating point negation function using gcc
*
* RETURNS: double precision negation result.
*
* NOMANUAL
*/

___negdf2:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	jsr	XDPNEG
	movel	sp@+,d0
	movel	sp@+,d1
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
* addsf3 - single-precision floating point addition function using gcc
*
* RETURNS: single precision addition result.
*
* NOMANUAL
*/

___addsf3:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(0),sp@-
	movel	a0@(4),sp@-
	jsr	FPADD
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
* subsf3 - single-precision floating point subtraction function using gcc
*
* RETURNS: single precision subtraction result.
*
* NOMANUAL
*/

___subsf3:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(0),sp@-
	movel	a0@(4),sp@-
	jsr	XFPSUB
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
* mulsf3 - single-precision floating point multiplication function using gcc
*
* RETURNS: single precision multiplication result.
*
* NOMANUAL
*/

___mulsf3:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(0),sp@-
	movel	a0@(4),sp@-
	jsr	FPMUL
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
* divsf3 - single-precision floating point division function using gcc
*
* RETURNS: single precision division result.
*
* NOMANUAL
*/

___divsf3:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(0),sp@-
	movel	a0@(4),sp@-
	jsr	FPDIV
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
* cmpsf2 - single-precision floating point comparison function using gcc
*
* RETURNS: single precision comparison result.
*
* NOMANUAL
*/

___cmpsf2:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	subaw	#8,sp
	movel	a0@(0),sp@-
	movel	a0@(4),sp@-
	jsr	XFPCMP
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
* negsf2 - single-precision floating point negation function using gcc
*
* RETURNS: single precision negation result.
*
* NOMANUAL
*/

___negsf2:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(0),sp@-
	jsr	XFPNEG
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
* truncdfsf2 - double-precision to single-precision floating point conversion.
*
* RETURNS: single precision representation of double-precision value.
*
* NOMANUAL
*/

___truncdfsf2:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	jsr	SINGLE
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
* extendsfdf2 - single-precision to double-precision floating point conversion.
*
* RETURNS: double precision representation of single precision value.
*
* NOMANUAL
*/

___extendsfdf2:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(0),sp@-
	jsr	DOUBLE
	movel	sp@+,d0
	movel	sp@+,d1
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
* fixdfsi - convert double-precision floating point to int, rounding toward zero
*
* RETURNS: integer representation of double precision value.
*
* NOMANUAL
*/

___fixdfsi:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	subaw	#8,sp
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	jsr	XDFIX
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
* fixunsdfsi - convert double-precision floating point to int, rounding down
*
* RETURNS: integer representation of double precision value.
*
* NOMANUAL
*/

___fixunsdfsi:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	subaw	#8,sp
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	jsr	XDINT
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
* floatsidf - convert integer to double-precision floatin point
*
* RETURNS: double precision representation of integer value.
*
* NOMANUAL
*/

___floatsidf:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	movel	a0@(0),sp@-
	jsr	XDFLOAT
	movel	sp@+,d0
	movel	sp@+,d1
	moveml	sp@+,d2-d7/a1-a6
	rts


/*******************************************************************************
* XDPNEG0 - negate a double-precision floating point number
*
* Internal subroutine to negate a double precision number
* Assumes address of arg is in a0
*
* RETURNS: negated double precision value.
*
* NOMANUAL
*/

XDPNEG0:
	movew	a0@,d0		| get sign/exp/mant word
	jeq	XDPNEG1		| J/ arg = 0.0

	bclr	#15,d0		| clear sign bit
	cmpiw	#0x7ff0,d0
	jgt	XDPNEG1		| J/ arg = NaN

	bchg	#7,a0@		| flip sign bit
XDPNEG1:
	rts


/*******************************************************************************
* XFPNEG0 - negate a single-precision floating point number
*
* Internal subroutine to negate a single precision number
* Assumes address of arg is in a0
*
* RETURNS: negated double precision value.
*
* NOMANUAL
*/

XFPNEG0:
	movew	a0@,d0		| get sign/exp/mant word
	jeq	XFPNEG1		| J/ arg = 0.0

	bclr	#15,d0		| clear sign bit
	cmpiw	#0x7f80,d0
	jgt	XFPNEG1		| J/ arg = NaN

	bchg	#7,a0@		| flip sign bit
XFPNEG1:
	rts


/*******************************************************************************
* XDPSUB - double-precision floating point subtraction
*
* Negates parameter and jumps to addition function.
*
* NOMANUAL
*/

XDPSUB:
	lea	sp@(4),a0
	jsr	XDPNEG0
	jmp	DPADD

/*******************************************************************************
* XDPNEG - double-precision floating point negation
*
* RETURNS: negated double precision value.
*
* NOMANUAL
*/

XDPNEG:
	lea	sp@(4),a0
	jsr	XDPNEG0
	rts

/*******************************************************************************
* XDPCMP - double-precision comparison function
*
* Note USS expects args backwards from C calling conventions.
* Our interface swaps args to the order USS expects.
*
* RETURNS: comparison results.
*
* NOMANUAL
*/

XDPCMP:
	movel	sp@+,sp@(16)	| stuff rtn address under 2 dbl args
	clrl	sp@(20)		| clear return value
	jsr	DPCMP		| call USS DPCMP
	jgt	XDCMPGT
	jeq	XDCMPEQ
	subql	#1,sp@(4)	| arg1 < arg 2
	rts
XDCMPGT:
	addql	#1,sp@(4)		| arg1 > arg 2
XDCMPEQ:
	rts


/*******************************************************************************
* XFPSUB - single-precision floating point subtraction
*
* Negates parameter and jumps to addition function.
*
* NOMANUAL
*/

XFPSUB:
	lea	sp@(4),a0
	jsr	XFPNEG0
	jmp	FPADD

/*******************************************************************************
* XFPNEG - single-precision floating point negation
*
* RETURNS: negated double precision value.
*
* NOMANUAL
*/

XFPNEG:
	lea	sp@(4),a0
	jsr	XDPNEG0
	rts

/*******************************************************************************
* XFPCMP - single-precision comparison function
*
* Note USS expects args backwards from C calling conventions.
* Our interface swaps args to the order USS expects.
*
* RETURNS: comparison results.
*
* NOMANUAL
*/

XFPCMP:
	movel	sp@+,sp@(8)		| stuff rtn address under 2 sngl args
	clrl	sp@(12)			| clear return value
	jsr	FPCMP			| call USS FPCMP
	jgt	XFCMPGT
	jeq	XFCMPEQ
	subql	#1,sp@(4)		| arg1 < arg 2
	rts
XFCMPGT:
	addql	#1,sp@(4)		| arg1 > arg 2
XFCMPEQ:
	rts


/*******************************************************************************
* XDFIX - convert double to int (signed or unsigned), round toward zero
*
* RETURNS: integer value
*
* NOMANUAL
*/

XDFIX:
	movel	sp@+,sp@(8)		| stuff rtn address under 1 dbl arg
	jsr	DFIX			| call USS DFIX
	movel	d1,sp@(4)		| put lo order 32 bits of d0:d1 on stack
	rts

/*******************************************************************************
* XDINT - convert double to int (signed or unsigned), round down
*
* RETURNS: integer value
*
* NOMANUAL
*/

XDINT:
	movel	sp@+,sp@(8)		| stuff rtn address under 1 dbl arg
	jsr	DINT			| call USS DINT
	movel	d1,sp@(4)		| put lo order 32 bits of d0:d1 on stack
	rts

/*******************************************************************************
* XDFLOAT - convert signed int to double
*
* Adjusts sign and jumps to USS conversion routine.
*
* NOMANUAL
*/

XDFLOAT:
	moveql	#-1,d0		| initialize hi word of longlong to negative
	movel	sp@(4),d1	| move 32 bit int to d0:d1 (64 bits)
	jlt	XDFL0
	clrl	d0		| make hi word of longlong positive
XDFL0:
	movel	sp@+,sp@	| stuff rtn address lower on stack
	jmp	DFLOAT
