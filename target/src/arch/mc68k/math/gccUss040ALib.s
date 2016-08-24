/* Copyright 1991-1992 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01d,02oct92,jcf  commented out .set.
01c,23aug92,jcf  changed bxxx to jxx.
01b,26may92,rrr  the tree shuffle
01a,07nov91,shl  created by combining selected part of gccUssALib.s
		 and uss_dpopns.s.
*/

/*
This library is a combination of gccUssALib.s and uss_dpopns.s.
The routines contained in this library provide basic low-level
floating point conversion function.

This library should only be used in asscoiation with the Motorola floating
point emulation package under the GNU C environment to provide
a complete set of floating point funtionality for the MC68040 architecture.

This file contains assembly language interface routines which allow the
US Software floating point emulation package to be used with the Gnu
C compiler (gcc).  The US Software package uses non-C conventions for
parameter passing on the stack, and some functions use the CPU's
status register bits to indicate results.  These routines convert
these non-standard interfaces to normal C parameter passing.  The
names for functions are those which are generated by the gcc compiler.


NOMANUAL
*/

#define	_ASMLANGUAGE
#include "vxWorks.h"
#include "uss_fp.h"


        .globl  DFIX
        .globl  DINT
	.globl	XDINT
	.globl  _gccUss040Init
	.globl	___fixunsdfsi

|       .set    DBIAS,1023              | Double precision format exponent bias
|
|
|       .set    CCRC,0x01               | Carry bit in CCR
|       .set    CCRV,0x02               | Overflow bit in CCR
|       .set    CCRZ,0x04               | Zero bit in CCR
|       .set    CCRN,0x08               | Negative bit in CCR
|       .set    CCRX,0x10               | Extend bit in CCR

	.text
	.even


/*******************************************************************************
* gccUss040Init - initialize US Software floating point package for MC68040
*
*/

_gccUss040Init:
	nop			/* dump initialization routine */
	rts

/*******************************************************************************
* fixunsdfsi - convert double-precision floating point to int, rounding down
*
* RETURNS: integer representation of double precision value.
*
* AUTHOR: Scott Huddleston, Tektronix.
*
* NOMANUAL
*/

___fixunsdfsi:
	lea	sp@(4),a0
	moveml	d2-d7/a1-a6,sp@-
	subaw	#8,sp
	movel	a0@(4),sp@-
	movel	a0@(0),sp@-
	jbsr	XDINT
	movel	sp@+,d0
	moveml	sp@+,d2-d7/a1-a6
	rts

/*******************************************************************************
* XDINT - convert double to int (signed or unsigned)
*
* RETURNS: integer value
*
* AUTHOR: Scott Huddleston, Tektronix.
*
* NOMANUAL
*/

XDINT:
	movel	sp@+,sp@(8)		| stuff rtn address under 1 dbl arg
	jbsr	DINT			| call USS DINT
	movel	d1,sp@(4)		| put lo order 32 bits of d0:d1 on stack
	rts

/*******************************************************************************
|
|       page
|
|  DINT
|  ====
|  Return the largest integer smaller than the argument provided
|
*/

DINT:
        bsr     GETDP1
        bsr     DFIX00
|
        jcc     DINT00          | J/ no bits lost
        cmpaw   #0,a2
        jeq     DINT00          | J/ not negative
        subql   #1,d1           | Decrement integer value
        jcc     DINT00          | J/ no borrow
        subql   #1,d0
DINT00:
        jmp     a0@

/*******************************************************************************
|
|
|       page
|
|  GETDP1
|  ======
|  Routine called to extract a double precision argument from the
|  system stack and place it (unpacked) into the 68000's registers.
|
*/

GETDP1:
        moveal  sp@+,a1 | /* Get GETDP1's return address */
        moveal  sp@+,a0 | Get calling routines return address
|
        movel   sp@+,d0 | Get argument
        movel   sp@+,d3
        subw    d2,d2           | Clear carry
        roxll   #1,d0           | Sign bit into carry
        subxw   d2,d2           | Replicate sign bit throughout d2
        moveaw  d2,a2           | Sign bit info into a2
        roll    #8,d0           | Left justify mantissa, position exp
        roll    #3,d0
        movel   d0,d2
        andiw   #0x7FF,d0       | Mask to exponent field
        jeq     GETD11          | J/ zero value
|
        eorw    d0,d2           | Zero exponent bits in d2
        lsrl    #1,d2           | Position mantissa
        bset    #31,d2          | Set implicit bit
        roll    #8,d3           | Position lo long word of mantissa
        roll    #3,d3
        eorw    d3,d2           | Clever use of EOR to move bits
        andiw   #0xF800,d3      | Trim off bits moved to d3
        eorw    d3,d2           | Remove noise in d2
|
GETD11:
        jmp     a1@             | Return to caller, its ret addr in a0

/*******************************************************************************
|
|       page
|
|  DFIX
|  ====
|  Routine to convert the double precision argument on the stack
|  to an integer value (with a dropoff flag).
|
*/

DFIX:
        bsr     GETDP1          | Extract/unpack one double prec val
        bsr     DFIX00          | Use internal routine
        jmp     a0@             | Return to caller
|
|
DFIX00:
        andw    d0,d0
        jne     DFIX01          | J/ value <> 0.0
|
        subl    d0,d0           | Return a zero value, no drop off
        clrl    d1
        rts
|
DFIX01:
        cmpiw   #DBIAS,d0
        jcc     DFIX02          | J/ abs() >= 1.0  [BCC == BHS]
|
        clrl    d0              | Return a zero value
        clrl    d1
        orib    #0x01+CCRX,ccr  | Set carry/extend bits
| ##    ORI     #$11,CCR
        rts
|
DFIX02:
        subiw   #DBIAS+63,d0
        jlt     DFIX03          | J/ abs() < 2^63
|
        moveq   #-1,d0          | Set d0:d1 to the maximum integer value
        moveq   #-1,d1
        lsrl    #1,d0           | d0:d1 = 0x7FFFFFFFFFFFFFFF
        movel   a2,d2
        subl    d2,d1           | Account for the sign of the arg.
        subxl   d2,d0
        rts
|
DFIX03:
        clrl    d1              | Clear bit drop off accum
|
        negw    d0              | Positive shift count
        cmpiw   #32,d0
        jlt     DFIX04          | J/ less than a word shift
|
        andl    d3,d3
        sne     d1              | Set d1 = 0FFH if d3 <> 0
|
        movel   d2,d3
        clrl    d2
|
        subiw   #32,d0
|
DFIX04:
        cmpiw   #16,d0
        jlt     DFIX05          | J/ less than a swap left
|
        orw     d3,d1           | Accum any bits dropped off
|
        movew   d2,d3           | Do a swap shift (16 bits)
        swap    d3
        clrw    d2
        swap    d2
|
        subiw   #16,d0
|
DFIX05:
        subqw   #1,d0
        jlt     DFIX07          | J/ shifting complete
|
DFIX06:
        lsrl    #1,d2
        roxrl   #1,d3
|
        roxll   #1,d1
|
        dbra    d0,DFIX06
|
DFIX07:
        cmpaw   #0,a2           | Check for negative value
        jeq     DFIX08          | J/ positive
|
        negl    d3
        negxl   d2
|
DFIX08:
        moveq   #-1,d0
        addl    d1,d0           | Set carry if bits lost
|
        exg     d2,d0           | Move integer result to d0:d1
        exg     d3,d1
        rts