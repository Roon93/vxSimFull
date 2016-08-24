/* dbgI86Lib.h - header file for arch dependent portion of debugger */

/* Copyright 1981-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,16sep02,pai  Updated with commonly used manifest constants.
01f,30may02,hdn  added WDB_CTX_LOAD() define (spr 75694)
01e,11mar98,dbt  corrected problems introduced by the SENS merge
                 added Copyright.
01d,13feb98,hdn  fixed BRK_DATAxx macros.
01d,30dec97,dbt  modified and added some defines for new breakpoint scheme.
01c,29nov93,hdn  added a definition EDI - EFLAGS.
01b,27aug93,hdn  added support for c++
01a,11sep91,hdn  written based on TRON version.
*/

#ifndef __INCI86dbgh
#define __INCI86dbgh

#ifdef __cplusplus
extern "C" {
#endif


/* includes */

#include "arch/i86/esfI86.h"


/* defines */

#define BRK_INST            (0x00)    /* instruction hardware breakpoint   */
#define BRK_DATAW1          (0x01)    /* data write 1 byte breakpoint      */
#define BRK_DATAW2          (0x05)    /* data write 2 byte breakpoint      */
#define BRK_DATAW4          (0x0d)    /* data write 4 byte breakpoint      */
#define BRK_DATARW1         (0x03)    /* data read-write 1 byte breakpoint */
#define BRK_DATARW2         (0x07)    /* data read-write 2 byte breakpoint */
#define BRK_DATARW4         (0x0f)    /* data read-write 4 byte breakpoint */

#define DEFAULT_HW_BP       (BRK_DATARW1)   /* default hardware breakpoint */

#define BRK_HARDWARE        (0x10)    /* hardware breakpoint bit */
#define BRK_HARDMASK        (0x0f)    /* hardware breakpoint mask */

#define SAVED_DBGREGS       (0x18)    /* 6 debug registers */
#define SAVED_REGS          (0x38)    /* 8 + 6 registers */

#define TRACE_FLAG          (0x0100)  /* TF in EFLAGS */
#define INT_FLAG            (0x0200)  /* IF in EFLAGS */

#define DBG_INST_ALIGN      (1)

/* instruction patterns and masks */

#define ADDI08_0            (0x83)
#define ADDI08_1            (0xc4)
#define ADDI32_0            (0x81)
#define ADDI32_1            (0xc4)
#define LEAD08_0            (0x8d)
#define LEAD08_1            (0x64)
#define LEAD08_2            (0x24)
#define LEAD32_0            (0x8d)
#define LEAD32_1            (0xa4)
#define LEAD32_2            (0x24)
#define JMPD08              (0xeb)
#define JMPD32              (0xe9)
#define ENTER               (0xc8)
#define PUSH_EBX            (0x53)
#define PUSH_EBP            (0x55)
#define PUSH_ESI            (0x56)
#define MOV_ESP0            (0x89)
#define MOV_ESP1            (0xe5)
#define MOV_ESP_ESI         (0xe689)
#define LEAVE               (0xc9)
#define RET                 (0xc3)
#define RETADD              (0xc2)
#define CALL_DIR            (0xe8)
#define CALL_INDIR0         (0xff)
#define CALL_INDIR1         (0x10)
#define CALL_INDIR_REG_EAX  (0xd0)
#define CALL_INDIR_REG_ECX  (0xd1)
#define CALL_INDIR_REG_EDX  (0xd2)
#define CALL_INDIR_REG_EBX  (0xd3)

#define ADDI08_0_MASK       (0xff)
#define ADDI08_1_MASK       (0xff)
#define ADDI32_0_MASK       (0xff)
#define ADDI32_1_MASK       (0xff)
#define LEAD08_0_MASK       (0xff)
#define LEAD08_1_MASK       (0xff)
#define LEAD08_2_MASK       (0xff)
#define LEAD32_0_MASK       (0xff)
#define LEAD32_1_MASK       (0xff)
#define LEAD32_2_MASK       (0xff)
#define JMPD08_MASK         (0xff)
#define JMPD32_MASK         (0xff)
#define ENTER_MASK          (0xff)
#define PUSH_EBX_MASK       (0xff)
#define PUSH_EBP_MASK       (0xff)
#define PUSH_ESI_MASK       (0xff)
#define MOV_ESP0_MASK       (0xff)
#define MOV_ESP1_MASK       (0xff)
#define MOV_ESP_ESI_MASK    (0xffff)
#define LEAVE_MASK          (0xff)
#define RET_MASK            (0xff)
#define RETADD_MASK         (0xff)
#define CALL_DIR_MASK       (0xff)
#define CALL_INDIR0_MASK    (0xff)
#define CALL_INDIR1_MASK    (0x38)
#define CALL_INDIR_REG_MASK (0xdf)


#ifndef _ASMLANGUAGE

#define DBG_HARDWARE_BP     (1)       /* hardware breakpoint support */
#define DBG_BREAK_INST      (0xcc)    /* int 3 */

/* offsets to register fields in type REG_SET */

#define EDI                 (0)
#define ESI                 (1)
#define EBP                 (2)
#define ESP                 (3)
#define EBX                 (4)
#define EDX                 (5)
#define ECX                 (6)
#define EAX                 (7)
#define EFLAGS              (8)

#define WDB_CTX_LOAD(pRegs) (_wdbDbgCtxLoad (pRegs))


/* macros */

#define DSM(addr,inst,mask) ((*(addr) & (mask)) == (inst))


/* typedefs */

/* aliases for type <ESF0> */

typedef ESF0                BREAK_ESF;
typedef ESF0                TRACE_ESF;

/* hardware breakpoint registers */

typedef struct              /* DBG_REGS */
    {
    unsigned int    db0;    /* debug register 0 */
    unsigned int    db1;    /* debug register 1 */
    unsigned int    db2;    /* debug register 2 */
    unsigned int    db3;    /* debug register 3 */
    unsigned int    db6;    /* debug register 6 */
    unsigned int    db7;    /* debug register 7 */

    } DBG_REGS;


/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

extern void    _wdbDbgCtxLoad (const REG_SET *);

#else     /* __STDC__ */

extern void    _wdbDbgCtxLoad ();

#endif    /* __STDC__ */

#endif    /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __INCI86dbgh */
