/* bALib.s - buffer manipulation library assembly language routines */

/* Copyright 1984-1994 Wind River Systems, Inc. */
	.data
	.globl  _copyright_wind_river
	.long   _copyright_wind_river


/*
modification history
--------------------
01y,26oct94,tmk  added mc68LC040 support
01x,30may94,tpr  added MC68060 cpu support.
		 tweak comment modifications.
01x,17oct94,rhp  delete obsolete doc references to strLib, spr#3712
01w,23aug92,jcf  changed bxxx to jxx.
		 changed all signed comparisons to unsigned.
01v,26may92,rrr  the tree shuffle
01u,07jan92,JLF	 fixed Motorola 68040 ERRATA I7 for move16 instruction.
	    shl  fixed mod history number.
01t,17oct91,yao  added support for CPU32.
01s,04oct91,rrr  passed through the ansification filter
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01r,24jul91,JLF   added 68040 support and optimization.
01q,14mar90,jdi   documentation cleanup; library declared NOMANUAL.
01p,09sep89,gae   fixed documentation of bcopy{Word,Long}s;
		    added PORTABLE define.
01o,10feb89,jcf   added bcopyWords (), and bcopyLongs ().
01n,30aug88,gae   more documentation tweaks.
01m,20aug88,gae   documentation.
01l,22jun88,dnw   changed bcopy() and bcopyBytes() to handle overlapping buffers
		    correctly, and deleted bmove() and bmoveBytes().
01k,05jun88,dnw   changed from bufALib to bALib.
01j,30may88,dnw   changed to v4 names.
01i,13feb88,dnw   added .data before .asciz above, for Intermetrics assembler.
01h,05nov87,jlf   documentation
01g,24mar87,jlf   documentation
01f,21dec86,dnw   changed to not get include files from default directories.
01e,31oct86,dnw   Eliminated magic f/b numeric labels which mitToMot can't
		    handle.
		  Changed "moveml" instructions to use Motorola style register
		    lists, which are now handled by "aspp".
		  Changed "mov[bwl]" to "move[bwl]" for compatiblity w/Sun as.
01d,26mar86,dnw   Fixed bugs introduced in 01c w/ not saving enough regs.
01c,18mar86,dnw   Added cpybytes, filbytes, and movbytes.
		  More optimizations.
		  Fixed documentation.
		  Fixed bug in movbuf.
01b,18sep85,jlf   Made cpybuf, filbuf, and movbuf work properly
		      with 0 length strings.
01a,17jul85,jlf   Written, by modifying bufLib.c, v01h.
*/

/*
DESCRIPTION
This library contains optimized versions of the routines in bLib.c
for manipulating buffers of variable-length byte arrays.

NOMANUAL

SEE ALSO: bLib, ansiString
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"


#ifndef	PORTABLE

	/* exports */

	.globl	_bcopy
	.globl	_bcopyBytes
	.globl	_bcopyWords
	.globl	_bcopyLongs
	.globl	_bfill
	.globl	_bfillBytes

	.text
	.even

/*******************************************************************************
*
* bcopy - copy one buffer to another
*
* This routine copies the first `nbytes' characters from
* `source' to `destination'.  Overlapping buffers are handled correctly.
* The copy is optimized by copying 4 bytes at a time if possible,
* (see bcopyBytes (2) for copying a byte at a time only).
*
* SEE ALSO: bcopyBytes (2)
*
* NOMANUAL - manual entry in bLib (1)

* void bcopy (source, destination, nbytes)
*     char *	source;		/* pointer to source buffer      *
*     char *	destination	/* pointer to destination buffer *
*     int 	nbytes;		/* number of bytes to copy       *

*/

_bcopy:
	link	a6,#0
	movel	d2,a7@-			/* save d2 */

	/* put src in a0, dest in a1, and count in d0 */

	movel	a6@(ARG1),a0		/* source */
	movel	a6@(ARG2),a1		/* destination */
	movel	a6@(ARG3),d0		/* nbytes */

	/* Find out if there is an overlap problem.
	 * We have to copy backwards if destination lies within source,
	 * i.e. ((destination - source) > 0 && < nbytes) */

	movel	a1,d1			/* destination */
	subl	a0,d1			/* - source */
	jls	cFwd			/* <= 0 means copy forward */
	cmpl	d0,d1			/* compare to nbytes */
	jcs	cBak			/* < nbytes means copy backwards */

cFwd:
	/* if length is less than 16, it's cheaper to do a byte copy */

	cmpl	#16,d0			/* test count */
	jcs	cFwd10			/* do byte copy */

#if (CPU==MC68000 || CPU==MC68010 || CPU==MC68020 || CPU==CPU32)
	/* 
	 * If destination and source are not both odd, or both even,
	 * we must do a byte copy, rather than a long copy.
	 *
	 * Note: this restriction does NOT apply to the 68040 and 68060.
	 */

	movew	a0,d1
	movew	a1,d2
	eorb	d2,d1			/* d1 = destination ^ source */
	btst	#0,d1
	jne	cFwd10
#endif	/* (CPU==MC68000 || CPU==MC68010 || CPU==MC68020 || CPU==CPU32) */

#if (CPU==MC68040 || CPU==MC68060 || CPU==MC68LC040)
	movew   a1,d2			/* copy address needed for next test */
#endif	/* (CPU==MC68040 || CPU==MC68060 || CPU==MC68LC040) */

	/* If the buffers are odd-aligned, copy the first byte */

	btst	#0,d2			/* d2 has source */
	jeq	cFwd0			/* if even-aligned */
	moveb	a0@+,a1@+		/* copy the byte */
	subl	#1,d0			/* decrement count by 1 */

cFwd0:
#if (CPU == MC68040 || CPU==MC68060 || CPU==MC68LC040)
	movew	a0,d1
	movew	a1,d2
	orw	d1,d2			/* OR the addresses together */
	andw	#0x000f,d2		/* if all are mod-16, we can MOVE16 */
	jne 	cFwd0d			/* else, skip to next test */

	movl	d0,d1			/* work with "d1" */
	lsrl	#4,d1			/* divide count by 16 */
	nop				/* ERRATA: I7 */
	jra 	cFwd0b			/* and jump into loop */

cFwd0a:
	.word   0xf620,0x9000		/* move16 (a0)+,(a1)+ */
cFwd0b:
	dbra	d1,cFwd0a		/* loop until (short) rollover */

	subl	#0x10000,d1		/* then subtract from MSW of count */
	jpl 	cFwd0a			/* loop until result is negative */

	andl	#0x0f,d0		/* then mask all but lower 16 bytes */
	jra	cFwd10			/* then finish with byte copy */

#endif	/* (CPU == MC68040 || CPU==MC68060 || CPU==MC68LC040) */

	/* 
	 * Since we're copying 16 bytes at a crack, divide count by 16.
	 * Keep the remainder in d0, so we can do those bytes at the
	 * end of the loop.
	 */

cFwd0d:
	movel	d0,d2
#if (CPU==MC68010)
	/* 
	 * Note: because of the DBRA pipeline on the 68010, it is faster
	 * for it to limit the loop to 1 move instruction.  For all other
	 * chips, it is more efficient to use 4.
	 */
	andl	#0x03,d0		/* remainder in d0 */
	asrl	#2,d2			/* count /= 4 */
#endif	/* (CPU==MC68010) */

#if (CPU==MC68000 || CPU==MC68020 || CPU==MC68040 || CPU==MC68060 || \
     CPU==MC68LC040 || CPU==CPU32)
	andl	#0x0f,d0		/* remainder in d0 */
	asrl	#4,d2			/* count /= 16 */
#endif	/* (CPU==MC68000 || CPU==MC68020 || CPU==MC68040 || CPU==MC68060 || \
	 *  CPU==MC68LC040 || CPU==CPU32) */

	/* 
	 * The fastest way to do the copy is with a dbra loop, but dbra
	 * uses only a 16 bit counter.  Therefore, break up count into
	 * two pieces, to be used as an inner loop and an outer loop
	 */

	movel	d2,d1			/* Set up d1 as the outer loop ctr */
	swap	d1			/* get upper word into dbra counter */
	jra	cFwd3			/* do the test first */

cFwd2:
	movel	a0@+,a1@+		/* move 4 bytes */
#if (CPU==MC68000 || CPU==MC68020 || CPU==MC68040 || CPU==MC68060 || \
     CPU==MC68LC040 || CPU==CPU32)
	movel	a0@+,a1@+		/* no, make that 16 bytes */
	movel	a0@+,a1@+
	movel	a0@+,a1@+
#endif  /* (CPU==MC68000 || CPU==MC68020 || CPU==MC68040 || CPU==MC68060 || \
	 *  CPU==MC68LC040 || CPU==CPU32) */
cFwd3:
	dbra	d2,cFwd2		/* inner loop test */
	dbra	d1,cFwd2		/* outer loop test */


	/* byte by byte copy */

cFwd10:	movel	d0,d1			/* Set up d1 as the outer loop ctr */
	swap	d1			/* get upper word into dbra counter */
	jra	cFwd13			/* do the test first */

cFwd12:	moveb	a0@+,a1@+		/* move a byte */
cFwd13:	dbra	d0,cFwd12		/* inner loop test */
	dbra	d1,cFwd12		/* outer loop test */

	movel	a7@+,d2			/* restore d2 */
	unlk	a6
	rts


	/* -------------------- copy backwards ---------------------- */
cBak:
	addl	d0,a0			/* make a0 point at end of from buf */
	addl	d0,a1			/* make a1 point at end of to buffer */

	/* if length is less than 10, cheaper to do a byte move */

	cmpl	#10,d0			/* test count */
	jcs	cBak10			/* do byte move */

#if (CPU==MC68000 || CPU==MC68010 || CPU==MC68020 || CPU==CPU32)
	/*
	 * If destination and source are not both odd, or both even,
	 * we must do a byte copy, rather than a long copy.
	 *
	 * Note: this restriction does NOT apply to the 68040 and 68060.
	 */

	movew	a0,d1
	movew	a1,d2
	eorb	d2,d1			/* d1 = destination ^ source */
	btst	#0,d1
	jne	cBak10
#endif	/* (CPU==MC68000 || CPU==MC68010 || CPU==MC68020 || CPU==CPU32) */

#if (CPU==MC68040 || CPU==MC68060 || CPU==MC68LC040)
	movew	a1,d2			/* set this up for the next test */
#endif	/* (CPU==MC68040 || CPU==MC68060 || CPU==MC68LC040) */

	/* If the buffers are odd-aligned, copy the first byte */

	btst	#0,d2			/* d2 has source */
	jeq	cBak0			/* if even-aligned */
	moveb	a0@-,a1@-		/* copy the byte */
	subl	#1,d0			/* decrement count by 1 */

	/* Since we're copying 4 bytes at a crack, divide count by 4.
	 * Keep the remainder in d0, so we can do those bytes at the
	 * end of the loop. */

cBak0:
	movel	d0,d2

#if (CPU==MC68010)
	/* 
	 * Note: because of the DBRA pipeline on the 68010, it is faster
	 * for it to limit the loop to 1 move instruction.  For all other
	 * chips, it is more efficient to use 4.
	 */
	andl	#0x03,d0		/* remainder in d0 */
	asrl	#2,d2			/* count /= 4 */
#endif	/* (CPU==MC68010) */
#if (CPU==MC68000 || CPU==MC68020 || CPU==MC68040 || CPU==MC68060 || \
     CPU==MC68LC040 || CPU==CPU32)
	andl	#0x0f,d0		/* remainder in d0 */
	asrl	#4,d2			/* count /= 16 */
#endif  /* (CPU==MC68000 || CPU==MC68020 || CPU==MC68040 || CPU==MC68060 || \
	 *  CPU==MC68LC040 || CPU==CPU32) */

	/* 
	 * The fastest way to do the copy is with a dbra loop, but dbra
	 * uses only a 16 bit counter.  Therefore, break up count into
	 * two pieces, to be used as an inner loop and an outer loop
	 */

	movel	d2,d1			/* Set up d1 as the outer loop ctr */
	swap	d1			/* get upper word into dbra counter */
	jra	cBak3			/* do the test first */

cBak2:
	movel	a0@-,a1@-		/* move 4 bytes */
#if (CPU==MC68000 || CPU==MC68020 || CPU==MC68040 || CPU==MC68060 || \
     CPU==MC68LC040 || CPU==CPU32)
	movel	a0@-,a1@-		/* no, make that 16 bytes */
	movel	a0@-,a1@-
	movel	a0@-,a1@-
#endif  /* (CPU==MC68000 || CPU==MC68020 || CPU==MC68040 || CPU==MC68060 || \
	 *  CPU==MC68LC040 || CPU==CPU32) */
cBak3:
	dbra	d2,cBak2		/* inner loop test */
	dbra	d1,cBak2		/* outer loop test */


	/* byte by byte copy */

cBak10:	movel	d0,d1			/* Set up d1 as the outer loop ctr */
	swap	d1			/* get upper word into dbra counter */
	jra	cBak13			/* do the test first */

cBak12:	moveb	a0@-,a1@-		/* move a byte */
cBak13:	dbra	d0,cBak12		/* inner loop test */
	dbra	d1,cBak12		/* outer loop test */

	movel	a7@+,d2			/* restore d2 */
	unlk	a6
	rts

/*******************************************************************************
*
* bcopyBytes - copy one buffer to another a byte at a time
*
* This routine copies the first `nbytes' characters from
* `source' to `destination'.
* It is identical to bcopy except that the copy is always performed
* a byte at a time.  This may be desirable if one of the buffers
* can only be accessed with byte instructions, as in certain byte-wide
* memory-mapped peripherals.
*
* SEE ALSO: bcopy (2)
*
* NOMANUAL - manual entry in bLib (1)

* void bcopyBytes (source, destination, nbytes)
*     char *	source;		/* pointer to source buffer      *
*     char *	destination;	/* pointer to destination buffer *
*     int 	nbytes;		/* number of bytes to copy       *

*/


_bcopyBytes:
	link	a6,#0

	/* put src in a0, dest in a1, and count in d0 */

	movel	a6@(ARG1),a0		/* source */
	movel	a6@(ARG2),a1		/* destination */
	movel	a6@(ARG3),d0		/* count */

	/* 
	 * Find out if there is an overlap problem.
	 * We have to copy backwards if destination lies within source,
	 * i.e. ((destination - source) > 0 && < nbytes)
	 */

	movel	a1,d1			/* destination */
	subl	a0,d1			/* - source */
	jls	cbFwd			/* <= 0 means copy forward */
	cmpl	d0,d1			/* compare to nbytes */
	jcs	cbBak			/* < nbytes means copy backwards */

	/* Copy the whole thing forward, byte by byte */

cbFwd:
	movel	d0,d1			/* Set up d1 as the outer loop ctr */
	swap	d1			/* get upper word into dbra counter */
	jra	cbFwd3			/* do the test first */

cbFwd1:	movel	#0xffff,d0		/* set to copy another 64K */

cbFwd2:	moveb	a0@+,a1@+		/* move a byte */
cbFwd3:	dbra	d0,cbFwd2		/* inner loop test */

	dbra	d1,cbFwd1		/* outer loop test */

	unlk	a6
	rts


	/* Copy the whole thing backward, byte by byte */

cbBak:
	addl	d0,a0			/* make a0 point at end of from buffer*/
	addl	d0,a1			/* make a1 point at end of to buffer */

	movel	d0,d1			/* Set up d1 as the outer loop ctr */
	swap	d1			/* get upper word into dbra counter */
	jra	cbBak3			/* do the test first */

cbBak1:	movel	#0xffff,d0		/* set to copy another 64K */

cbBak2:	moveb	a0@-,a1@-		/* move a byte */
cbBak3:	dbra	d0,cbBak2		/* inner loop test */

	dbra	d1,cbBak1		/* outer loop test */

	unlk	a6
	rts

/*******************************************************************************
*
* bcopyWords - copy one buffer to another a word at a time
*
* This routine copies the first `nwords' words from
* `source' to `destination'.
* It is similar to bcopy except that the copy is always performed
* a word at a time.  This may be desirable if one of the buffers
* can only be accessed with word instructions, as in certain word-wide
* memory-mapped peripherals.  The source and destination must be word-aligned.
*
* SEE ALSO: bcopy (2)
*
* NOMANUAL - manual entry in bLib (1)

* void bcopyWords (source, destination, nwords)
*     char *	source;		/* pointer to source buffer      *
*     char *	destination;	/* pointer to destination buffer *
*     int	nwords;		/* number of words to copy       *

*/


_bcopyWords:
	link	a6,#0

	/* put src in a0, dest in a1, and count in d0 */

	movel	a6@(ARG1),a0		/* source */
	movel	a6@(ARG2),a1		/* destination */
	movel	a6@(ARG3),d0		/* count */

	asll	#1,d0			/* convert count to bytes */

	/*
	 * Find out if there is an overlap problem.
	 * We have to copy backwards if destination lies within source,
	 * i.e. ((destination - source) > 0 && < nbytes)
	 */

	movel	a1,d1			/* destination */
	subl	a0,d1			/* - source */
	jls	cwFwd			/* <= 0 means copy forward */
	cmpl	d0,d1			/* compare to nbytes */
	jcs	cwBak			/* < nbytes means copy backwards */

	/* Copy the whole thing forward, word by word */

cwFwd:
	asrl	#1,d0			/* convert count to words */

	movel	d0,d1			/* Set up d1 as the outer loop ctr */
	swap	d1			/* get upper word into dbra counter */
	jra	cwFwd3			/* do the test first */

cwFwd1:	movel	#0xffff,d0		/* set to copy another 128K */

cwFwd2:	movew	a0@+,a1@+		/* move a word */
cwFwd3:	dbra	d0,cwFwd2		/* inner loop test */

	dbra	d1,cwFwd1		/* outer loop test */

	unlk	a6
	rts


	/* Copy the whole thing backward, word by word */

cwBak:
	addl	d0,a0			/* make a0 point at end of from buffer*/
	addl	d0,a1			/* make a1 point at end of to buffer */

	asrl	#1,d0			/* convert count to words */

	movel	d0,d1			/* Set up d1 as the outer loop ctr */
	swap	d1			/* get upper word into dbra counter */
	jra	cwBak3			/* do the test first */

cwBak1:	movel	#0xffff,d0		/* set to copy another 128K */

cwBak2:	movew	a0@-,a1@-		/* move a word */
cwBak3:	dbra	d0,cwBak2		/* inner loop test */

	dbra	d1,cwBak1		/* outer loop test */

	unlk	a6
	rts

/*******************************************************************************
*
* bcopyLongs - copy one buffer to another a long at a time
*
* This routine copies the first `nlongs' longs from
* `source' to `destination'.
* It is similar to bcopy except that the copy is always performed
* a long at a time.  This may be desirable if one of the buffers
* can only be accessed with long instructions, as in certain long-wide
* memory-mapped peripherals.  The source and destination must be long-aligned.
*
* SEE ALSO: bcopy (2)
*
* NOMANUAL - manual entry in bLib (1)

* void bcopyLongs (source, destination, nlongs)
*     char *	source;		/* pointer to source buffer      *
*     char *	destination;	/* pointer to destination buffer *
*     int 	nlongs;		/* number of longs to copy       *

*/


_bcopyLongs:
	link	a6,#0

	/* put src in a0, dest in a1, and count in d0 */

	movel	a6@(ARG1),a0		/* source */
	movel	a6@(ARG2),a1		/* destination */
	movel	a6@(ARG3),d0		/* count */

	asll	#2,d0			/* convert count to bytes */

	/* 
	 * Find out if there is an overlap problem.
	 * We have to copy backwards if destination lies within source,
	 * i.e. ((destination - source) > 0 && < nbytes)
	 */

	movel	a1,d1			/* destination */
	subl	a0,d1			/* - source */
	jls	clFwd			/* <= 0 means copy forward */
	cmpl	d0,d1			/* compare to nbytes */
	jcs	clBak			/* < nbytes means copy backwards */

	/* Copy the whole thing forward, long by long */

clFwd:
	asrl	#2,d0			/* convert count to longs */

	movel	d0,d1			/* Set up d1 as the outer loop ctr */
	swap	d1			/* get upper word into dbra counter */
	jra	clFwd3			/* do the test first */

clFwd1:	movel	#0xffff,d0		/* set to copy another 256K */

clFwd2:	movel	a0@+,a1@+		/* move a long */
clFwd3:	dbra	d0,clFwd2		/* inner loop test */

	dbra	d1,clFwd1		/* outer loop test */

	unlk	a6
	rts


	/* Copy the whole thing backward, long by long */

clBak:
	addl	d0,a0			/* make a0 point at end of from buffer*/
	addl	d0,a1			/* make a1 point at end of to buffer */

	asrl	#2,d0			/* convert count to longs */

	movel	d0,d1			/* Set up d1 as the outer loop ctr */
	swap	d1			/* get upper word into dbra counter */
	jra	clBak3			/* do the test first */

clBak1:	movel	#0xffff,d0		/* set to copy another 256K */

clBak2:	movel	a0@-,a1@-		/* move a long */
clBak3:	dbra	d0,clBak2		/* inner loop test */

	dbra	d1,clBak1		/* outer loop test */

	unlk	a6
	rts

/*******************************************************************************
*
* bfill - fill buffer with character
*
* This routine fills the first `nbytes' characters of the specified buffer
* with the specified character.
* The fill is optimized by filling 4 bytes at a time if possible,
* (see bfillBytes (2) for filling a byte at a time only).
*
* SEE ALSO: bfillBytes (2)
*
* NOMANUAL - manual entry in bLib (1)

* void bfill (buf, nbytes, ch)
*     char *	buf;		/* pointer to buffer              *
*     int 	nbytes;		/* number of bytes to fill        *
*     char 	ch;		/* char with which to fill buffer *

*/

_bfill:
	link	a6,#0
	moveml	d2-d3,a7@-		/* save regs */

	/* put buf in a0, nbytes in d0, and ch in d1 */

	movel	a6@(ARG1),a0		/* get buf */
	movel	a6@(ARG2),d0		/* nbytes */
	movel	a6@(ARG3),d1		/* ch */

	/* if length is less than 20, cheaper to do a byte fill */

	cmpl	#20,d0			/* test count */
	jcs	fb5			/* do byte fill */

	/* Put ch in all four bytes of d1, so we can fill 4 bytes at a crack */

	moveb	d1,d2
	lslw	#8,d1			/* move ch into 2nd byte of d1 */
	orb	d2,d1			/* or ch back into 1st byte of d1 */
	movew	d1,d2
	swap	d1			/* get ch-ch into high word of d1 */
	orw	d2,d1			/* or ch-ch back into low word of d1 */

	/* If the buffer is odd-aligned, copy the first byte */

	movew	a0,d2
	btst	#0,d2			/* d2 has source */
	jeq	fb0			/* if even-aligned */

	moveb	d1,a0@+			/* copy the byte */
	subl	#1,d0			/* decrement count by 1 */

	/* 
	 * Since we're copying 4 bytes at a crack, divide count by 4.
	 * Keep the remainder in d0, so we can do those bytes at the
	 * end of the loop.
	 */

fb0:
	movel	d0,d3
	andl	#3,d0			/* remainder in d0 */
	asrl	#2,d3			/* count /= 4 */

	/* 
	 * The fastest way to do the fill is with a dbra loop, but dbra
	 * uses only a 16 bit counter.  Therefore, break up count into
	 * two pieces, to be used as an inner loop and an outer loop
	 */

	movel	d3,d2			/* Set up d2 as the outer loop ctr */
	swap	d2			/* get upper word into dbra counter */
	jra	fb3			/* do the test first */

fb1:	movel	#0xffff,d3		/* set to fill another 64K */

fb2:	movel	d1,a0@+			/* move 4 bytes */
fb3:	dbra	d3,fb2			/* inner loop test */

	dbra	d2,fb1			/* outer loop test */

	/* do the extras at the end */

	jra	fb5			/* do the test first */
fb4:	moveb	d1,a0@+			/* move 1 byte */
fb5:	dbra	d0,fb4			/* inner loop test */

	moveml	a7@+,d2-d3		/* restore regs */
	unlk	a6
	rts

/*******************************************************************************
*
* bfillBytes - fill buffer with character a byte at a time
*
* This routine fills the first `nbytes' characters of the
* specified buffer with the specified character.
* It is identical to bfill (2) except that the fill is always performed
* a byte at a time.  This may be desirable if the buffer
* can only be accessed with byte instructions, as in certain byte-wide
* memory-mapped peripherals.
*
* SEE ALSO: bfill (2)
*
* NOMANUAL - manual entry in bLib (1)

* void bfillBytes (buf, nbytes, ch)
*     char *	buf;		/* pointer to buffer              *
*     int 	nbytes;		/* number of bytes to fill        *
*     char 	ch;		/* char with which to fill buffer *

*/

_bfillBytes:
	link	a6,#0
	movel	d2,a1			/* save d2 in a1 */

	/* put src in a0, dest in a1, and count in d0 */

	movel	a6@(ARG1),a0		/* get destination */
	movel	a6@(ARG2),d0		/* count */
	movel	a6@(ARG3),d1		/* ch */

	/* Copy the whole thing, byte by byte */

	movel	d0,d2			/* Set up d2 as the outer loop ctr */
	swap	d2			/* get upper word into dbra counter */
	jra	fby3			/* do the test first */

fby1:	movel	#0xffff,d0		/* set to fill another 64K */

fby2:	moveb	d1,a0@+			/* fill a byte */
fby3:	dbra	d0,fby2			/* inner loop test */

	dbra	d2,fby1			/* outer loop test */

	movel	a1,d2			/* restore d2 */
	unlk	a6
	rts
#endif	/* !PORTABLE */
