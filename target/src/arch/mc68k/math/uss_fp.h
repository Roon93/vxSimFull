/* uss_fp.h - US Software 68k floating point emulation package defines */

/* Copyright 1992-1992 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01b,26may92,rrr  the tree shuffle
01a,30mar92,kdl  written, from .set directives in USS source files (SPR #1398).
*/

/*
DESCRIPTION

This file contains various symbolic constants used in the US Software
floating point emulation library.

INTERNAL
Prior to creation of this header file, the assembly source files used
.set directives to define these constants, which had the undesirable
side effect of causing the symbols to go into the VxWorks symbol table.

*/


#define    CCRC		0x01      /* Carry bit in CCR */
#define    CCRN		0x08      /* Negative bit in CCR */
#define    CCRV		0x02      /* Overflow bit in CCR */
#define    CCRX		0x10      /* Extend bit in CCR */
#define    CCRZ		0x04      /* Zero bit in CCR */

#define    DBIAS	1023      /* Double precision format exponent bias */
#define    DFUZZ	51        /* Fifty-one bits of fuzz */
#define    DNEXPC	16        /* Number of constants in DEXP poly */
#define    DNLNCN	5         /* Number of constants in DLN poly */

#define    ERNAN	3         /* FPERR INVALID OPERATION ERROR CODE */
#define    EROVF	2         /* FPERR OVERFLOW ERROR CODE */
#define    ERUNF	1         /* FPERR UNDERFLOW ERROR CODE */

#define    FBIAS	127       /* Single precision format exponent bias */
#define    FFUZZ	20        /* Twenty bits of fuzz */
#define    FNEXPC	10        /* Number of constants in FEXP poly */
#define    FNLNCN	4         /* Number of constants in FLN poly */

#define    NDATNC	5
#define    NDCOSC	9
#define    NDSINC	8
#define    NDTNPC	4
#define    NDTNQC	4
#define    NFATNC	4
#define    NFCOSC	5
#define    NFSINC	5
#define    NFTNPC	4
#define    NFTNQC	4

#define    XBIAS	127       /* *** for assembler bug *** */
#define    comp64	0         /*flag for 64 bit multiply/divide */

