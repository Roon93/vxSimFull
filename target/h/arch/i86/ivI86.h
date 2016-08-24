/* ivI86.h - I80x86 interrupt vectors */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,29aug01,hdn  added support for streaming SIMD exception.
01d,29may94,hdn  removed I80486 conditional.
01c,08jun93,hdn  added support for c++. updated to 5.1.
01b,26mar93,hdn  added a new vector for 486.
01a,28feb92,hdn  written based on TRON, 68k version.
*/

#ifndef __INCivI86h
#define __INCivI86h

#ifdef __cplusplus
extern "C" {
#endif


/* 0 - 19	exception vector number */

#define IN_DIVIDE_ERROR			0
#define IN_DEBUG			1
#define IN_NON_MASKABLE			2
#define IN_BREAKPOINT			3
#define IN_OVERFLOW			4
#define IN_BOUND			5
#define IN_INVALID_OPCODE		6
#define IN_NO_DEVICE			7
#define IN_DOUBLE_FAULT			8
#define IN_CP_OVERRUN			9
#define IN_INVALID_TSS			10
#define IN_NO_SEGMENT			11
#define IN_STACK_FAULT			12
#define IN_PROTECTION_FAULT		13
#define IN_PAGE_FAULT			14
#define IN_RESERVED			15
#define IN_CP_ERROR			16
#define IN_ALIGNMENT			17
#define IN_MACHINE_CHECK		18
#define IN_SIMD				19

/* 20 - 31	unassigned, Intel reserved exceptions */

/* 32 - 255	user defined interrupt vectors */


#ifndef	_ASMLANGUAGE

/* macros to convert interrupt vectors <-> interrupt numbers */

#define IVEC_TO_INUM(intVec)	((int) (intVec) >> 3)
#define INUM_TO_IVEC(intNum)	((VOIDFUNCPTR *) ((intNum) << 3))

/* exception vector address/offset */

#define IV_DIVIDE_ERROR			INUM_TO_IVEC (IN_DIVIDE_ERROR)
#define IV_DEBUG			INUM_TO_IVEC (IN_DEBUG)
#define IV_NON_MASKABLE			INUM_TO_IVEC (IN_NON_MASKABLE)
#define IV_BREAKPOINT			INUM_TO_IVEC (IN_BREAKPOINT)
#define IV_OVERFLOW			INUM_TO_IVEC (IN_OVERFLOW)
#define IV_BOUND			INUM_TO_IVEC (IN_BOUND)
#define IV_INVALID_OPCODE		INUM_TO_IVEC (IN_INVALID_OPCODE)
#define IV_NO_DEVICE			INUM_TO_IVEC (IN_NO_DEVICE)
#define IV_DOUBLE_FAULT			INUM_TO_IVEC (IN_DOUBLE_FAULT)
#define IV_CP_OVERRUN			INUM_TO_IVEC (IN_CP_OVERRUN)
#define IV_INVALID_TSS			INUM_TO_IVEC (IN_INVALID_TSS)
#define IV_NO_SEGMENT			INUM_TO_IVEC (IN_NO_SEGMENT)
#define IV_STACK_FAULT			INUM_TO_IVEC (IN_STACK_FAULT)
#define IV_PROTECTION_FAULT		INUM_TO_IVEC (IN_PROTECTION_FAULT)
#define IV_PAGE_FAULT			INUM_TO_IVEC (IN_PAGE_FAULT)
#define IV_RESERVED			INUM_TO_IVEC (IN_RESERVED)
#define IV_CP_ERROR			INUM_TO_IVEC (IN_CP_ERROR)
#define IV_ALIGNMENT			INUM_TO_IVEC (IN_ALIGNMENT)
#define IV_MACHINE_CHECK		INUM_TO_IVEC (IN_MACHINE_CHECK)
#define IV_SIMD				INUM_TO_IVEC (IN_SIMD)

#endif	/* _ASMLANGUAGE */


#ifdef __cplusplus
}
#endif

#endif	/* __INCivI86h */
