/* archI86.h - Intel 80X86 specific header */

/* Copyright 1984-2002 Wind River Systems, Inc. */
/*
modification history
--------------------
01i,12nov02,hdn  made CR4 initialization only for P5 or later (spr 83992)
01h,29oct02,hdn  assigned reserved1 to (FP_CONTEXT *) (spr 70252)
01g,07oct02,hdn  added the architecture extension of the TCB
01f,13mar01,sn   SPR 73723 - define supported toolchains
01e,04oct01,hdn  enclosed ARCH_REGS_INIT macro with _ASMLANGUAGE
01d,21aug01,hdn  added PENTIUM4's _CACHE_ALIGN_SIZE, ARCH_REGS_INIT
01c,10aug01,hdn  added PENTIUM2/3/4 support
01b,25feb99,hdn  added _CACHE_ALIGN_SIZE for Pentium and PentiumPro
01a,07jun93,hdn  written based on arch68k.h
*/

#ifndef __INCarchI86h
#define __INCarchI86h

#ifdef __cplusplus
extern "C" {
#endif

#define _ARCH_SUPPORTS_GCC

#define	_BYTE_ORDER		_LITTLE_ENDIAN
#define	_DYNAMIC_BUS_SIZING	FALSE		/* require alignment for swap */

#if	(CPU==PENTIUM4)
#define _CACHE_ALIGN_SIZE	64
#elif	(CPU==PENTIUM || CPU==PENTIUM2 || CPU==PENTIUM3)
#define _CACHE_ALIGN_SIZE	32
#else
#define _CACHE_ALIGN_SIZE	16
#endif	/* (CPU==PENTIUM || CPU==PENTIUM[234]) */


#ifdef	_ASMLANGUAGE

/* 
 * system startup macros used by sysInit(sysALib.s) and romInit(romInnit.s).
 * - no function calls to outside of BSP is allowed.
 * - no references to the data segment is allowed.
 * the CR4 is introduced in the Pentium processor.
 */

#define ARCH_REGS_INIT							\
	xorl	%eax, %eax;		/* zero EAX */			\
	movl	%eax, %dr7;		/* initialize DR7 */		\
	movl	%eax, %dr6;		/* initialize DR6 */		\
	movl	%eax, %dr3;		/* initialize DR3 */		\
	movl	%eax, %dr2;		/* initialize DR2 */		\
	movl	%eax, %dr1;		/* initialize DR1 */		\
	movl	%eax, %dr0;		/* initialize DR0 */		\
	movl    %cr0, %edx;		/* get CR0 */			\
	andl    $0x7ffafff1, %edx;	/* clear PG, AM, WP, TS, EM, MP */ \
	movl    %edx, %cr0;		/* set CR0 */			\
									\
	pushl	%eax;			/* initialize EFLAGS */		\
	popfl;

#define ARCH_CR4_INIT							\
	xorl	%eax, %eax;		/* zero EAX */			\
	movl	%eax, %cr4;		/* initialize CR4 */


#else

/* architecture extension of the TCB (pTcb->reserved2) */

typedef struct x86Ext		/* architecture specific TCB extension */
    {
    unsigned int	reserved0;	/* (DS_CONFIG *) */
    unsigned int	reserved1;	/* (FP_CONTEXT *) for FPU exception */
    unsigned int	reserved2;
    unsigned int	reserved3;
    unsigned int	reserved4;
    unsigned int	reserved5;
    unsigned int	reserved6;
    unsigned int	reserved7;
    } X86_EXT;

/* no function declarations for makeSymTlb/symTbl.c */


#endif	/* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __INCarchI86h */
