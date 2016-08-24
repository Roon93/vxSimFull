/* winnt/config.h - vxSim for Windows configuration file */ 

/* Copyright 1998-2002, Wind River Systems, Inc. */

/*
modification history
--------------------
02j,23may02,jmp  forced WDB_MODE to WDB_MODE_TASK on full simulator.
02i,03may02,jmp  added INCLUDE_HW_FP (SPR #76719).
02h,15apr02,hbh  Enabled system mode debugging for pipe backend.
02g,20nov01,jmp  no longer allow to switch to system mode.
02f,12nov01,jmp  no longer allow to switch to system mode when INCLUDE_END.
02e,31oct01,hbh  Updated BSP revision for T2.2. Code cleanup.
02d,20oct98,pr   added WindView 2.0 defines
01c,21sep98,cym  added declaration for simMemSize.
01b,11aug98,sbs  changed WDB_COMM_TYPE to be WDB_COMM_END if END 
                 support is included.
01a,29apr98,cym  written based on pc386.
*/

/*
This module contains the configuration parameters for the
Windows simulator.
*/

#ifndef	INCconfigh
#define	INCconfigh

/* BSP version/revision identification, before configAll.h */

#define BSP_VER_1_1	1
#define BSP_VER_1_2	1
#define BSP_VERSION	"1.2"
#define BSP_REV		"/1"	/* 0 for first revision */

#include "configAll.h"

IMPORT char * sysBootLine;
#undef  BOOT_LINE_ADRS
#define BOOT_LINE_ADRS          sysBootLine
#undef  DEFAULT_BOOT_LINE
#define DEFAULT_BOOT_LINE       sysBootLine

#undef NUM_TTY
#define NUM_TTY 1
#undef  INCLUDE_AOUT
#define INCLUDE_PECOFF

/* XXX - need to implement shared memory network */

#undef  INCLUDE_SM_NET

/*
 * The default configuration is no network. If you've installed the
 * full featured simulator (unbundled), change TRUE to FALSE below.
 */

#define INCLUDE_NTPASSFS

#if	TRUE
#undef	INCLUDE_NETWORK
#undef	INCLUDE_NET_INIT
#undef  WDB_COMM_TYPE
#define WDB_COMM_TYPE WDB_COMM_PIPE
#undef	WDB_TTY_TEST
#else 	/* !TRUE */
#define INCLUDE_END
#undef  INCLUDE_WDB_SYS	/* system mode not supported on full simulator */
#undef	WDB_MODE
#define	WDB_MODE	WDB_MODE_TASK	/* do not allow switch to system mode */
#endif 	/* TRUE */

#ifdef  INCLUDE_END
#undef  WDB_COMM_TYPE   /* WDB agent communication path is END device */ 
#define WDB_COMM_TYPE	WDB_COMM_END    
#define INCLUDE_NT_ULIP
#endif  /* INCLUDE_END */

#ifdef INCLUDE_LOADER
#ifdef INCLUDE_PECOFF
IMPORT int loadPecoffInit();
#endif
#endif

/* Misc. options */

#undef	VM_PAGE_SIZE
#define VM_PAGE_SIZE	4096

#define INCLUDE_HW_FP			/* hardware fpp support */

/* miscellaneous definitions */

#define NV_RAM_SIZE     NONE            /* no NVRAM */

/* Windows Timers only have a resolution of 1ms, limiting clock rate */

#define SYS_CLK_RATE_MIN  19            /* minimum system clock rate */
#define SYS_CLK_RATE_MAX  1000		/* maximum system clock rate */
#define AUX_CLK_RATE_MIN  2             /* minimum auxiliary clock rate */
#define AUX_CLK_RATE_MAX  1000          /* maximum auxiliary clock rate */


/* User reserved memory.  See sysMemTop(). */

#define	USER_RESERVED_MEM	0

/*
 * Local-to-Bus memory address constants:
 * the local memory address always appears at 0 locally;
 * it is not dual ported.
 */

IMPORT char *	simMemBlock;
IMPORT int 	simMemSize;

#define LOCAL_MEM_LOCAL_ADRS	(simMemBlock)	/* fixed */
#define LOCAL_MEM_BUS_ADRS	0x0		/* fixed */
#define LOCAL_MEM_SIZE		(simMemSize)    /* parameter */

/*
 * The following parameters are defined here and in the Makefile.
 * The must be kept synchronized; effectively config.h depends on Makefile.
 * Any changes made here must be made in the Makefile and vice versa.
 */

#define ROM_BASE_ADRS		0		/* base address of ROM */
#define ROM_TEXT_ADRS		(ROM_BASE_ADRS)	/* booting from EPROM */
#define ROM_SIZE		0		/* size of ROM */

#define RAM_LOW_ADRS		0x00108000	/* VxWorks image entry point */
#define RAM_HIGH_ADRS		0x00008000	/* Boot image entry point */

#define INCLUDE_TRIGGERING      /* event triggering tool */

#define INCLUDE_WINDVIEW        /* windview control tool */
#define INCLUDE_WDB_TSFS        /* target-server file system */

/* windview upload paths */

#define  INCLUDE_WVUPLOAD_FILE
#define INCLUDE_WVUPLOAD_TSFSSOCK

#endif	/* INCconfigh */
#if defined(PRJ_BUILD)
#include "prjParams.h"
#endif
