/* regsSimnt.h - simnt registers header */

/* Copyright 1995-2003 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,16jan03,elp  added REGS defines (SPR #84278).
01b,29apr98,cym  added intLockKey
01a,27aug97,cym  written
*/

#ifndef __INCregsSimnth
#define __INCregsSimnth

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#define GREG_NUM 8

typedef struct		/* REG_SET - simnt register set (based on x86)*/
    {
    ULONG edi;		/* general register */
    ULONG esi;		/* general register */
    ULONG ebp;		/* general register */
    ULONG esp;		/* general register */
    ULONG ebx;		/* general register */
    ULONG edx;		/* general register */
    ULONG ecx;		/* frame pointer register */
    ULONG eax;		/* stack pointer register */
    ULONG eflags;	/* status register */
    INSTR *pc;          /* program counter */
    ULONG intLockKey;
    CONTEXT windowsContext;
    } REG_SET;

#define spReg   esp
#define fpReg	ebp
#define reg_pc pc
#define reg_sp spReg
#define reg_fp fpReg

#define G_REG_BASE	0x00
#define G_REG_OFFSET(n) (G_REG_BASE + (n)*sizeof(ULONG))
#define SR_OFFSET	G_REG_OFFSET(GREG_NUM)
#define PC_OFFSET	(SR_OFFSET+sizeof(ULONG))
#endif  /* _ASMLANGUAGE */

#define REG_EDI         0x00
#define REG_ESI         0x04
#define REG_EBP         0x08
#define REG_ESP         0x0c
#define REG_EBX         0x10
#define REG_EDX         0x14
#define REG_ECX         0x18
#define REG_EAX         0x1c
#define REG_EFLAGS      0x20
#define REG_PC          0x24
#define REG_INT_LOCK    0x28

#ifdef __cplusplus
}
#endif

#endif /* __INCregsSimnth */
