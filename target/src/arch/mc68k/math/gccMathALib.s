/* gccMathALib.s - math support routines for gcc */

/* Copyright 1984-1992 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01g,20sep92,kdl  moved gcc v2.2 support routines to gccMathLib.c; restored 
		 missing title line.
01f,07sep92,smb  added gccMathInit.
01e,23aug92,jcf  changed bxxx to jxx.
01d,30jul92,kdl  added gcc v2.2 support; made previous file contents 
		 conditional for 68000/10 only.
01c,26may92,rrr  the tree shuffle
01b,04oct91,rrr  passed through the ansification filter
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01a,28jan91,kdl	 original version, based on routines by K.U. Bloem.
*/

/*
DESCRIPTION

This library contains various support routines for using gcc.  The
compiler will generate subroutine calls to the functions in this file
when the software floating point option is used.  The function names 
are those which are automatically generated by the gcc compiler.

This library also contains routines to perform long (32-bit) arithmetic
on a Motorola 68000 or 68010 processor.  Multiply, divide, and
modulus operations are provided, each in signed and unsigned modes.

AUTHOR
Compare routines supplied by Cygnus Support.

Original 32-bit multiply and divide routines for 68000/10 written by 
Kai-Uwe Bloem (I5110401@dbstu1.bitnet).

NOMANUAL
*/


#define	_ASMLANGUAGE

#include "vxWorks.h"


#if  (CPU==MC68000 || CPU==MC68010)
	.globl	___mulsi3
	.globl	___umulsi3
	.globl	___divsi3
	.globl	___udivsi3
	.globl	___modsi3
	.globl	___umodsi3
#endif  /* CPU==MC68000 || CPU==MC68010 */

	.globl	_gccMathInit

	.text
	.even


/*******************************************************************************
*
* gccMathInit -  This routine will cause this module to be included in
*	         the vxWorks build.
*
* RETURNS: 
*
* NOMANUAL
*/

_gccMathInit:
	link	a6,#-4
	unlk	a6
	rts


#if  (CPU==MC68000 || CPU==MC68010)
/*******************************************************************************
*
* mulsi3 - long integer multiplication routine for 68000/10 using gcc
*
* Revision 1.1, kub 03-90
* first version, replaces the appropriate routine from fixnum.s.
* This one is longer, but normally faster because __umulsi3 is no longer
* called for multiplication. Rather, the code is inlined here. See the
* comments in _umulsi3.s
*
* RETURNS: signed 32-bit multiplication result.
*
* AUTHOR:
* written by Kai-Uwe Bloem (I5110401@dbstu1.bitnet).
*
* NOMANUAL
*/

___mulsi3:
	movel	d2,a0		| save registers
	movel	d3,a1
	movemw	sp@(4),d0-d3	| get the two longs. u = d0-d1, v = d2-d3
	movew	d0,sp@-		| sign flag
	jpl	0f		| is u negative ?
	negw	d1		| yes, force it positive
	negxw	d0
0:	tstw	d2		| is v negative ?
	jpl	0f
	negw	d3		| yes, force it positive ...
	negxw	d2
	notw	sp@		|  ... and modify flag word
0:
	extl	d0		| u.h <> 0 ?
	jeq	1f
	mulu	d3,d0		| r  = v.l * u.h
1:	tstw	d2		| v.h <> 0 ?
	jeq	2f
	mulu	d1,d2		| r += v.h * u.l
	addw	d2,d0
2:	swap	d0
	clrw	d0
	mulu	d3,d1		| r += v.l * u.l
	addl	d1,d0
	movel	a1,d3
	movel	a0,d2
	tstw	sp@+		| should the result be negated ?
	jpl	3f		| no, just return
	negl	d0		| else r = -r
3:	rts

/*******************************************************************************
*
* umulsi3 - unsigned long integer multiplication routine for 68000/10 using gcc
*
* Revision 1.1, kub 03-90
* first version, replaces the appropriate routine from fixnum.s.
* This one is short and fast for the common case of both longs <= 0x0000ffff,
* but the case of a zero lowword is no longer recognized.
* (besides it's easier to read this source 8-)
*
* RETURNS: unsigned 32-bit multiplication result.
*
* AUTHOR:
* written by Kai-Uwe Bloem (I5110401@dbstu1.bitnet).
*
* NOMANUAL
*/

___umulsi3:
	movel	d2,a0		| save registers
	movel	d3,a1
	movemw	sp@(4),d0-d3	| get the two longs. u = d0-d1, v = d2-d3
	extl	d0		| u.h <> 0 ?
	jeq	1f
	mulu	d3,d0		| r  = v.l * u.h
1:	tstw	d2		| v.h <> 0 ?
	jeq	2f
	mulu	d1,d2		| r += v.h * u.l
	addw	d2,d0
2:	swap	d0
	clrw	d0
	mulu	d3,d1		| r += v.l * u.l
	addl	d1,d0
	movel	a1,d3
	movel	a0,d2
	rts

/*******************************************************************************
*
*
* divsi3 - long integer division routine for 68000/10 using gcc
*
* Revision 1.1, kub 03-90
* first version, replaces the appropriate routine from fixnum.s.
* Should be faster in more common cases. Division is done by 68000 divu
* operations if divisor is only 16 bits wide. Otherwise the normal division
* algorithm as described in various papers takes place. The division routine
* delivers the quotient in d0 and the remainder in d1, thus the implementation
* of the modulo operation is trivial. We gain some extra speed by inlining
* the division code here instead of calling ___udivsi3.
*
* RETURNS: signed 32-bit division result.
*
* AUTHOR:
* written by Kai-Uwe Bloem (I5110401@dbstu1.bitnet).
*
* NOMANUAL
*/

___divsi3:
	movel	d2,a0		| save registers
	movel	d3,a1
	clrw	sp@-		| sign flag
	clrl	d0		| prepare result
	movel	sp@(10),d2	| get divisor
	jeq	9f		| divisor = 0 causes a division trap
	jpl	0f		| divisor < 0 ?
	negl	d2		| negate it
	notw	sp@		| remember sign
0:	movel	sp@(6),d1	| get dividend
	jpl	0f		| dividend < 0 ?
	negl	d1		| negate it
	notw	sp@		| remember sign
0:
|== case 1) divident < divisor
	cmpl	d2,d1		| is divident smaller then divisor ?
	jcs	8f		| yes, return immediately
|== case 2) divisor has <= 16 significant bits
	tstw	sp@(10)
	jne	2f		| divisor has only 16 bits
	movew	d1,d3		| save dividend
	clrw	d1		| divide dvd.h by dvs
	swap	d1
	jeq	0f		| (no division necessary if dividend zero)
	divu	d2,d1
0:	movew	d1,d0		| save quotient.h
	swap	d0
	movew	d3,d1		| (d0.h = remainder of prev divu)
	divu	d2,d1		| divide dvd.l by dvs
	movew	d1,d0		| save quotient.l
	clrw	d1		| get remainder
	swap	d1
	jra	8f		| and return
|== case 3) divisor > 16 bits (corollary is dividend > 16 bits, see case 1)
2:
	moveq	#31,d3		| loop count
3:
	addl	d1,d1		| shift divident ...
	addxl	d0,d0		|  ... into d0
	cmpl	d2,d0		| compare with divisor
	jcs	0f
	subl	d2,d0		| big enough, subtract
	addw	#1,d1		| and note bit into result
0:
	dbra	d3,3b
	exg	d0,d1		| put quotient and remainder in their registers
8:
	tstw	sp@(6)		| must the remainder be corrected ?
	jpl	0f
	negl	d1		| yes, apply sign
| the following line would be correct if modulus is defined as in algebra
|	addl	sp@(6),d1	| algebraic correction: modulus can only be >= 0
0:	tstw	sp@+		| result should be negative ?
	jpl	0f
	negl	d0		| yes, negate it
0:
	movel	a1,d3
	movel	a0,d2
	rts
9:
	divu	d2,d1		| cause division trap
	jra	8b		| back to user

/*******************************************************************************
*
* udivsi3 - unsigned long integer division routine for 68000/10 using gcc
*
* Revision 1.1, kub 03-90
* first version, replaces the appropriate routine from fixnum.s.
* Should be faster in more common cases. Division is done by 68000 divu
* operations if divisor is only 16 bits wide. Otherwise the normal division
* algorithm as described in various papers takes place. The division routine
* delivers the quotient in d0 and the remainder in d1, thus the implementation
* of the modulo operation is trivial.
*
* RETURNS: unsigned 32-bit division result.
*
* AUTHOR:
* written by Kai-Uwe Bloem (I5110401@dbstu1.bitnet).
*
* NOMANUAL
*/

___udivsi3:
	movel	d2,a0		| save registers
	movel	d3,a1
	clrl	d0		| prepare result
	movel	sp@(8),d2	| get divisor
	jeq	9f		| divisor = 0 causes a division trap
	movel	sp@(4),d1	| get dividend
|== case 1) divident < divisor
	cmpl	d2,d1		| is divident smaller then divisor ?
	jcs	8f		| yes, return immediately
|== case 2) divisor has <= 16 significant bits
	tstw	sp@(8)
	jne	2f		| divisor has only 16 bits
	movew	d1,d3		| save dividend
	clrw	d1		| divide dvd.h by dvs
	swap	d1
	jeq	0f		| (no division necessary if dividend zero)
	divu	d2,d1
0:	movew	d1,d0		| save quotient.h
	swap	d0
	movew	d3,d1		| (d1.h = remainder of prev divu)
	divu	d2,d1		| divide dvd.l by dvs
	movew	d1,d0		| save quotient.l
	clrw	d1		| get remainder
	swap	d1
	jra	8f		| and return
|== case 3) divisor > 16 bits (corollary is dividend > 16 bits, see case 1)
2:
	moveq	#31,d3		| loop count
3:
	addl	d1,d1		| shift divident ...
	addxl	d0,d0		|  ... into d0
	cmpl	d2,d0		| compare with divisor
	jcs	0f
	subl	d2,d0		| big enough, subtract
	addw	#1,d1		| and note bit in result
0:
	dbra	d3,3b
	exg	d0,d1		| put quotient and remainder in their registers
8:
	movel	a1,d3
	movel	a0,d2
	rts
9:
	divu	d2,d1		| cause division trap
	jra	8b		| back to user


/*******************************************************************************
*
* modsi3 - long integer modulo routine for 68000/10 using gcc
*
* Revision 1.1, kub 03-90
*
* RETURNS: signed 32-bit modulo result.
*
* AUTHOR:
* written by Kai-Uwe Bloem (I5110401@dbstu1.bitnet).
*
* NOMANUAL
*/

___modsi3:
	movel	sp@(8),sp@-	| push divisor
	movel	sp@(8),sp@-	| push dividend
	jbsr	___divsi3
	addql	#8,sp
	movel	d1,d0		| return the remainder in d0
	rts

/*******************************************************************************
*
* umodsi3 - unsigned long integer modulo routine for 68000/10 using gcc
*
* Revision 1.1, kub 03-90
*
* RETURNS: unsigned 32-bit modulo result.
*
* AUTHOR:
* written by Kai-Uwe Bloem (I5110401@dbstu1.bitnet).
*
* NOMANUAL
*/

___umodsi3:
	movel	sp@(8),sp@-	| push divisor
	movel	sp@(8),sp@-	| push dividend
	jbsr	___udivsi3
	addql	#8,sp
	movel	d1,d0		| return the remainder in d0
	rts

#endif  /* CPU==MC68000 || CPU==MC68010 */
