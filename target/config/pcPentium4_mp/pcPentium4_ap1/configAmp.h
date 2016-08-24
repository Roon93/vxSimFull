/* configAmp.h - PC Pentium4 AMP configuration header */

/* Copyright 2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,14jun02,rhe  Add C++ Protection
01b,22may02,hdn  added INCLUDE_SM_NET_ADDRGET for APs
		 changed SM_MEM_ADRS for romA20on()'s 0x100000 toggling
01a,29mar02,hdn  written
*/

/*
This module contains the configuration parameters for the
PC Pentium4 AMP (asymmetric multi processor)
*/

#ifndef	INCconfigAmph
#define	INCconfigAmph

#ifdef __cplusplus
extern "C" {
#endif


#ifdef	TGT_CPU

/* overriden configuration macros */

#undef	INCLUDE_PC_CONSOLE	/* VGA console */
#undef	PC_CONSOLE		/* console number */
#undef	N_VIRTUAL_CONSOLES	/* shell / application */
#undef	INCLUDE_FEI_END		/* Intel Ether Express PRO100B PCI: END */
#undef  INCLUDE_GEI8254X_END	/* Intel 82543/82544 PCI ethernet */
#undef	INCLUDE_FD		/* floppy disk driver */
#undef	INCLUDE_END		/* Enhanced Network Drivers */
#undef  INCLUDE_BSD	        /* BSD service */
#undef  INCLUDE_WDB	        /* WDB */
#undef	INCLUDE_CONFIGURATION_5_2 /* pre-tornado tools */
#undef	DEFAULT_BOOT_LINE	/* default boot line */
#undef	NUM_TTY			/* number of TTYs */
#undef	LOCAL_MEM_LOCAL_ADRS	/* local mem base */
#undef	LOCAL_MEM_SIZE		/* local mem size */
#undef	RAM_LOW_ADRS		/* VxWorks image entry point */
#undef	RAM_HIGH_ADRS		/* Boot image entry point */

/* AMP (asymmetric multi processor) configuration */

#undef	LOCAL_MEM_AUTOSIZE	/* turned off the memory auto-sizing */
#define	SYMMETRIC_IO_MODE	/* Interrupt Mode: Symmetric IO Mode */
#define	INCLUDE_PCI		/* they all are PCI based */

#undef	INCLUDE_BP_5_0		/* use "sm" interface */
#undef	INCLUDE_SM_OBJ          /* sm objects (unbundled) */
#undef	INCLUDE_SM_SEQ_ADDR     /* sm net adrs auto setup */
#undef	INCLUDE_SM_NET_SHOW     /* include sm net show rtn */
#define	INCLUDE_SM_NET          /* include sm net interface */
#define	INCLUDE_BSD	        /* sm net is BSD service */
#define SM_TAS_TYPE 		SM_TAS_HARD		/* hardware TAS */
#define SM_INT_TYPE		SM_INT_BUS		/* BUS interrupt */
#define SM_INT_ARG1		(loApicId)		/* Local APIC Id */
#define SM_INT_ARG2		(INT_NUM_LOAPIC_SM)     /* intNum for IPI */
#define SM_INT_ARG3		0			/* not used */
#undef	SM_ANCHOR_ADRS					/* sm anchor adrs */
#define SM_ANCHOR_ADRS		((char *) 0x1100)	/* sm anchor adrs */
#define SM_OFF_BOARD		FALSE			/* use on-board mem */
#define SM_MEM_ADRS		0x00101000            	/* fixed sm net */
#define SM_MEM_SIZE		0x00080000              /* 512K sm net pool */
#define SM_OBJ_MEM_ADRS 	(SM_MEM_ADRS + SM_MEM_SIZE) /* fixed sm obj */
#define SM_OBJ_MEM_SIZE		0x0			/* 512K reserved */


#if	(TGT_CPU == MP_BP)		/* BP (Boot Processor: cpu0) */

#   warning "building it for BP"
#   undef  INCLUDE_PC_CONSOLE	/* COM1 is the console */
#   undef  PC_CONSOLE		/* no VGA */
#   undef  N_VIRTUAL_CONSOLES	/* no VGA */
#   undef  INCLUDE_CONFIGURATION_5_2 /* pre-tornado tools */
#   undef  INCLUDE_SM_NET_ADDRGET /* sm net is the primary net */
#   define INCLUDE_FD		/* include floppy disk driver */
#   define INCLUDE_GEI8254X_END	/* include Intel 82543/82544 PCI ethernet */
#   define INCLUDE_END		/* use Enhanced Network Drivers */
#   define INCLUDE_WDB	        /* WDB */
#   define INCLUDE_WDB_TSFS	/* target-server file system */
#   define PIT0_FOR_AUX		/* use PIT0 as an Aux Timer */
#   define DEFAULT_BOOT_LINE \
    "gei(0,0)host:/usr/vw/config/pcPentium4_bp/vxWorks h=90.0.0.3 e=90.0.0.50:ffffff00 b=200.200.200.254:ffffff00 u=target" 
#   define NUM_TTY		(N_UART_CHANNELS)
#   define LOCAL_MEM_LOCAL_ADRS	0x00400000	/* 4KB/4MB aligned */
#   define LOCAL_MEM_SIZE	0x00c00000	/* 12MB (min 3 pages) */
#   define RAM_LOW_ADRS		0x00608000	/* VxWorks image entry point */
#   define RAM_HIGH_ADRS	0x00408000	/* 2MB needed for GEI driver */

#elif	(TGT_CPU == MP_AP1)	/* AP1 (Appl Processor 1: cpu1) */

#   warning "building it for AP1"
#   define INCLUDE_PC_CONSOLE	/* VGA is the console */
#   define PC_CONSOLE		0		/* console number */
#   define N_VIRTUAL_CONSOLES	2		/* shell / application */
#   define INCLUDE_CONFIGURATION_5_2 /* pre-tornado tools */
#   define INCLUDE_SM_NET_ADDRGET /* sm net is the primary net */
#   undef  INCLUDE_FD		/* exclude floppy disk driver */
#   undef  INCLUDE_GEI8254X_END	/* exclude Intel 82543/82544 PCI ethernet */
#   undef  INCLUDE_END		/* not use Enhanced Network Drivers */
#   undef  INCLUDE_WDB	        /* exclude WDB */
#   undef  PIT0_FOR_AUX		/* not use PIT0 as an Aux Timer */
#   define DEFAULT_BOOT_LINE \
    "sm(0,1)host:/usr/vw/config/pcPentium4_ap1/vxWorks h=90.0.0.3 b=200.200.200.1:ffffff00 g=200.200.200.254 u=target" 
#   define NUM_TTY		(0)
#   define LOCAL_MEM_LOCAL_ADRS	0x01000000	/* 4KB/4MB aligned */
#   define LOCAL_MEM_SIZE	0x01000000	/* 16MB (min 3 pages) */
#   define RAM_LOW_ADRS		0x01108000	/* VxWorks image entry point */
#   define RAM_HIGH_ADRS	0x01008000	/* Boot image entry point */

#elif	(TGT_CPU == MP_AP2)	/* AP2 (Appl Processor 2: cpu2) */

#   warning "building it for AP2"
#   define INCLUDE_PC_CONSOLE	/* VGA is the console */
#   define PC_CONSOLE		0		/* console number */
#   define N_VIRTUAL_CONSOLES	2		/* shell / application */
#   define INCLUDE_CONFIGURATION_5_2 /* pre-tornado tools */
#   define INCLUDE_SM_NET_ADDRGET /* sm net is the primary net */
#   undef  INCLUDE_FD		/* exclude floppy disk driver */
#   undef  INCLUDE_GEI8254X_END	/* exclude Intel 82543/82544 PCI ethernet */
#   undef  INCLUDE_END		/* not use Enhanced Network Drivers */
#   undef  INCLUDE_WDB	        /* exclude WDB */
#   undef  PIT0_FOR_AUX		/* not use PIT0 as an Aux Timer */
#   define DEFAULT_BOOT_LINE \
    "sm(0,2)host:/usr/vw/config/pcPentium4_ap2/vxWorks h=90.0.0.3 b=200.200.200.2:ffffff00 g=200.200.200.254 u=target" 
#   define NUM_TTY		(0)
#   define LOCAL_MEM_LOCAL_ADRS	0x02000000	/* 4KB/4MB aligned */
#   define LOCAL_MEM_SIZE	0x01000000	/* 16MB (min 3 pages) */
#   define RAM_LOW_ADRS		0x02108000	/* VxWorks image entry point */
#   define RAM_HIGH_ADRS	0x02008000	/* Boot image entry point */

#elif	(TGT_CPU == MP_AP3)	/* AP3 (Appl Processor 3: cpu3) */

#   warning "building it for AP3"
#   define INCLUDE_PC_CONSOLE	/* VGA is the console */
#   define PC_CONSOLE		0		/* console number */
#   define N_VIRTUAL_CONSOLES	2		/* shell / application */
#   define INCLUDE_CONFIGURATION_5_2 /* pre-tornado tools */
#   define INCLUDE_SM_NET_ADDRGET /* sm net is the primary net */
#   undef  INCLUDE_FD		/* exclude floppy disk driver */
#   undef  INCLUDE_GEI8254X_END	/* exclude Intel 82543/82544 PCI ethernet */
#   undef  INCLUDE_END		/* not use Enhanced Network Drivers */
#   undef  INCLUDE_WDB	        /* exclude WDB */
#   undef  PIT0_FOR_AUX		/* not use PIT0 as an Aux Timer */
#   define DEFAULT_BOOT_LINE \
    "sm(0,3)host:/usr/vw/config/pcPentium4_ap3/vxWorks h=90.0.0.3 b=200.200.200.3:ffffff00 g=200.200.200.254 u=target" 
#   define NUM_TTY		(0)
#   define LOCAL_MEM_LOCAL_ADRS	0x03000000	/* 4KB/4MB aligned */
#   define LOCAL_MEM_SIZE	0x01000000	/* 16MB (min 3 pages) */
#   define RAM_LOW_ADRS		0x03108000	/* VxWorks image entry point */
#   define RAM_HIGH_ADRS	0x03008000	/* Boot image entry point */

#endif	/* (TGT_CPU == MP_BP) */

#endif	/* TGT_CPU */

#ifdef __cplusplus
}
#endif

#endif	/* INCconfigAmph */

