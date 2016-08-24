/* template.h - template board header */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,18apr02,rhe  Added C++ Protection
TODO -	Remove the template modification history and begin a new history
	starting with version 01a and growing the history upward with
	each revision.

01c,08apr99,dat  SPR 26491, PCI macros
01b,27aug97,dat  code review comments, added documentation
01a,10jul97,dat  written (from template68k/template.h, ver 01d)
*/

/*
This file contains I/O addresses and related constants for the
template BSP.
*/

#ifndef	INCtemplateh
#define	INCtemplateh

#ifdef __cplusplus
extern "C" {
#endif

#define BUS		NONE		/* no off-board bus interface */
#define N_SIO_CHANNELS	2		/* Number of serial I/O channels */

/* Local I/O address map */

#define	TEMPLATE_SIO_ADRS	((volatile char *) 0xfffe0000)
#define	TEMPLATE_TIMER_ADRS	((volatile char *) 0xfffa0000)
#define BBRAM			((volatile char *) 0xfffb0000)

/* timer constants */

#define	SYS_CLK_RATE_MIN  	3	/* minimum system clock rate */
#define	SYS_CLK_RATE_MAX  	5000	/* maximum system clock rate */
#define	AUX_CLK_RATE_MIN  	3	/* minimum auxiliary clock rate */
#define	AUX_CLK_RATE_MAX  	5000	/* maximum auxiliary clock rate */

/* create a single macro INCLUDE_MMU */

#if defined(INCLUDE_MMU_BASIC) || defined(INCLUDE_MMU_FULL)
#define INCLUDE_MMU
#endif

/* Only one can be selected, FULL overrides BASIC */

#ifdef INCLUDE_MMU_FULL
#   undef INCLUDE_MMU_BASIC
#endif

/* Static interrupt vectors/levels (configurable defs go in config.h) */

#if     FALSE
#define INT_VEC_ABORT           xxx
#define INT_VEC_CLOCK           xxx
...
#define INT_LVL_ABORT           xxx
#define INT_LVL_CLOCK           xxx
...
#endif

#ifdef INCLUDE_PCI
    /* Translate PCI addresses to virtual addresses (master windows) */

#   define	PCI_MEM2LOCAL(x) \
	((void *)((UINT)(x) - PCI_MSTR_MEM_BUS + PCI_MSTR_MEM_LOCAL))

#   define PCI_IO2LOCAL(x) \
	((void *)((UINT)(x) - PCI_MSTR_IO_BUS + PCI_MSTR_IO_LOCAL))

#   define PCI_MEMIO2LOCAL(x) \
	((void *)((UINT)(x) - PCI_MSTR_MEMIO_BUS + PCI_MSTR_MEMIO_LOCAL))

    /* Translate local memory address to PCI address (slave window) */

#   define LOCAL2PCI_MEM(x) \
	((void *)((UINT)(x) - PCI_SLV_MEM_LOCAL + PCI_SLV_MEM_BUS))
#endif /* INCLUDE_PCI */

/*
 * Miscellaneous definitions go here. For example, macro definitions
 * for various devices.
 */

/* programmable interrupt controller (PIC) */

#include "drv/intrCtl/i8259a.h"	/* TODO - select actual driver */

/* TODO - These are dummy values. */

#define	PIC1_BASE_ADR		0x20
#define	PIC2_BASE_ADR		0xa0
#define	PIC_REG_ADDR_INTERVAL	1	/* address diff of adjacent regs. */

#define	COMMAND_8042		0x64
#define	DATA_8042		0x60
#define	STATUS_8042		COMMAND_8042
#define COMMAND_8048		0x61	/* out Port PC 61H in the 8255 PPI */
#define	DATA_8048		0x60	/* input port */
#define	STATUS_8048		COMMAND_8048

/* constant values in romInit.s */

#define ROM_IDTR		0xaf		/* offset to romIdtr  */
#define ROM_GDTR		0xb5		/* offset to romGdtr  */
#define ROM_GDT			0xc0		/* offset to romGdt   */
#define ROM_INIT2		0xf0		/* offset to romInit2 */
#define ROM_STACK		0x7000		/* initial stack pointer */
#define ROM_WARM_HIGH		0x10		/* warm start entry p */
#define ROM_WARM_LOW		0x20		/* warm start entry p */

#define INT_NUM_IRQ0		0x20		/* vector number for IRQ0 */

#ifdef __cplusplus
}
#endif

#endif	/* INCtemplateh */

