/* esfI86.h - I80x86 exception stack frames */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,31aug01,hdn  added ESF[123] macros and its offset/size.
01b,08jun93,hdn  added support for c++. updated to 5.1.
01a,28feb92,hdn  written based on TRON, 68k version.
*/

#ifndef	__INCesfI86h
#define	__INCesfI86h

#ifdef __cplusplus
extern "C" {
#endif

#ifndef	_ASMLANGUAGE

/* Exception stack frames.  Most of these can happen only for one
   CPU or another, but they are all defined for all CPU's */

/* Format-0 - no privilege change, no error code */

typedef struct
    {
    INSTR * pc;				/* 0x00 : PC */
    unsigned short cs;			/* 0x04 : code segment */
    unsigned short pad0;		/* 0x06 : padding */
    unsigned long eflags;		/* 0x08 : EFLAGS */
    } ESF0;				/* sizeof(ESF0) -> 0x0c */

/* Format-1 - no privilege change, error code */

typedef struct
    {
    unsigned long errCode;		/* 0x00 : error code */
    INSTR * pc;				/* 0x04 : PC */
    unsigned short cs;			/* 0x08 : code segment */
    unsigned short pad0;		/* 0x0a : padding */
    unsigned long eflags;		/* 0x0c : EFLAGS */
    } ESF1;				/* sizeof(ESF1) -> 0x10 */

/* Format-2 - privilege change, no error code */

typedef struct
    {
    INSTR * pc;				/* 0x00 : PC */
    unsigned short cs;			/* 0x04 : code segment */
    unsigned short pad0;		/* 0x06 : padding */
    unsigned long eflags;		/* 0x08 : EFLAGS */
    unsigned long esp;			/* 0x0c : ESP */
    unsigned long ss;			/* 0x10 : SS */
    } ESF2;				/* sizeof(ESF2) -> 0x14 */

/* Format-3 - privilege change, error code */

typedef struct
    {
    unsigned long errCode;		/* 0x00 : error code */
    INSTR * pc;				/* 0x04 : PC */
    unsigned short cs;			/* 0x08 : code segment */
    unsigned short pad0;		/* 0x0a : padding */
    unsigned long eflags;		/* 0x0c : EFLAGS */
    unsigned long esp;			/* 0x10 : ESP */
    unsigned long ss;			/* 0x14 : SS */
    } ESF3;				/* sizeof(ESF3) -> 0x18 */

#endif	/* _ASMLANGUAGE */


#define	ESF0_EIP	0x00		/* 0x00 : PC */
#define	ESF0_CS		0x04		/* 0x04 : CS */
#define	ESF0_EFLAGS	0x08		/* 0x08 : EFLAGS */

#define	ESF1_ERROR	0x00		/* 0x00 : ERROR */
#define	ESF1_EIP	0x04		/* 0x04 : PC */
#define	ESF1_CS		0x08		/* 0x08 : CS */
#define	ESF1_EFLAGS	0x0c		/* 0x0c : EFLAGS */

#define	ESF2_EIP	0x00		/* 0x00 : PC */
#define	ESF2_CS		0x04		/* 0x04 : CS */
#define	ESF2_EFLAGS	0x08		/* 0x08 : EFLAGS */
#define	ESF2_ESP	0x0c		/* 0x0c : ESP */
#define	ESF2_SS		0x10		/* 0x10 : SS */

#define	ESF3_ERROR	0x00		/* 0x00 : ERROR */
#define	ESF3_EIP	0x04		/* 0x04 : PC */
#define	ESF3_CS		0x08		/* 0x08 : CS */
#define	ESF3_EFLAGS	0x0c		/* 0x0c : EFLAGS */
#define	ESF3_ESP	0x10		/* 0x10 : ESP */
#define	ESF3_SS		0x14		/* 0x14 : SS */

#define	ESF0_NBYTES	0x0c		/* sizeof(ESF0) */
#define	ESF1_NBYTES	0x10		/* sizeof(ESF1) */
#define	ESF2_NBYTES	0x14		/* sizeof(ESF2) */
#define	ESF3_NBYTES	0x18		/* sizeof(ESF3) */

#define	ESF0_NLONGS	0x03		/* sizeof(ESF0) / sizeof(int) */
#define	ESF1_NLONGS	0x04		/* sizeof(ESF1) / sizeof(int) */
#define	ESF2_NLONGS	0x05		/* sizeof(ESF2) / sizeof(int) */
#define	ESF3_NLONGS	0x06		/* sizeof(ESF3) / sizeof(int) */

#ifdef __cplusplus
}
#endif

#endif	/* __INCesfI86h */
