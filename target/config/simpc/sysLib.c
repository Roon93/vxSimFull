/* sysLib.c - vxSim for Windows dependent library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01e,08apr02,jmp  added sysBootParams initialization if INCLUDE_NET_INIT and
		 INCLUDE_BOOT_LINE_INIT are not defined (SPR #75207).
01d,21sep01,jmp  imported bootline initialization from arch/simnt/simLib.c,
                 added dynamic Ulip IP addressing (SPRs #67687, #28810).
01c,09aug01,hbh  Added definition of USER_RESERVED_MEM in sysMemTop() routine
                 (SPR 24151)
01b,02dec98,cym  changing reboot message.
01a,21sep98,cym  adding sysExcMsgAdrs
01e 17sep98,cym  getting bootline from boot parameters,
		 Implemented sysToMonitor.
01d 30jul98,cym  replaced nullNvRam and nullVme.
01c 13jul98,cym  removed NT from Board name.
01b 03jun98,cym  cleaned up compiler warnings.
01a 29apr98,cym  written.
*/

/*
DESCRIPTION

INCLUDE FILES: sysLib.h

SEE ALSO:
.pG "Configuration"
*/

/* includes */

#define FULL_SIMULATOR
#include "vxWorks.h"
#include "vme.h"
#include "memLib.h"
#include "sysLib.h"
#include "string.h"
#include "intLib.h"
#include "config.h"
#include "logLib.h"
#include "taskLib.h"
#include "vxLib.h"
#include "errnoLib.h"
#include "stdio.h"
#include "cacheLib.h"
#include "timer/simPcTimer.c"
#include "mem/nullNvRam.c"
#include "vme/nullVme.c"

/* To make man pages for network support routines */

/* imports */

IMPORT VOIDFUNCPTR intVecSetEnt;  /* entry hook for intVecSet() */
IMPORT VOIDFUNCPTR intVecSetExit; /* exit  hook for intVecSet() */
IMPORT int winOut();
IMPORT void simDie();
IMPORT int ulipIpAddrGet ();

extern int simProcnum;
extern char fullExePath[];

/* globals */

char 	sysExcMsgAdrs[256];		/* catastrophic message area */
char	*sysExcMsg = (char *)&sysExcMsgAdrs;
int	sysCpu		= CPU;		/* system cpu type (MC680x0) */
char    sysBootString[256];
char	*sysBootLine	= (char *)&sysBootString; /* address of boot line */
int	sysProcNum;			/* processor number of this cpu */
int	sysFlags;			/* boot flags */
char	sysBootHost [BOOT_FIELD_LEN];	/* name of host from which we booted */
char	sysBootFile [BOOT_FIELD_LEN];	/* name of file from which we booted */
UINT	sysIntIdtType	= 0x0000ef00;	/* trap gate, 0x0000ee00=int gate */
UINT	sysProcessor	= NONE;		/* 0=386, 1=486, 2=Pentium */
UINT	sysCoprocessor	= 0;		/* 0=none, 1=387, 2=487 */
UINT	sysStrayIntCount = 0;		/* Stray interrupt count */
char	*memTopPhys	= NULL;		/* top of memory */
int	sysCodeSelector	= 0;		/* code selector for context switch */

/* last nibble depend on processor number and is initialized by _sysInit() */

unsigned char ntEnetAddr [6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x1};

/* locals */

/* forward declarations */

char *sysPhysMemTop (void);
static int sysUlipIpBaseAddrGet (char * ulipIpBaseAddr, char * ulipIpAddr);

#include "sysSerial.c"

/*******************************************************************************
*
* sysModel - return the model name of the CPU board
*
* This routine returns the model name of the CPU board.
*
* RETURNS: A pointer to the string "PC 386" or "PC 486".
*/

char *sysModel (void)

    {
    return("VxSim for Windows");
    }

/*******************************************************************************
*
* sysBspRev - return the BSP version and revision number
*
* This routine returns a pointer to a BSP version and revision number, for
* example, 1.1/0. BSP_REV is concatenated to BSP_VERSION and returned.
*
* RETURNS: A pointer to the BSP version/revision string.
*/

char * sysBspRev (void)
    {
    return (BSP_VERSION BSP_REV);
    }

/*******************************************************************************
*
* sysHwInit - initialize the system hardware
*
* This routine initializes various features of the i386/i486 board.
* It is called from usrInit() in usrConfig.c.
*
* NOTE: This routine should not be called directly by the user application.
*
* RETURNS: N/A
*/

void sysHwInit (void)
    {
    char ulipIpAddr[16];	/* Ip address of the Ulip Adapter */
    char ulipIpBaseAddr[13];	/* base address of the Ulip Adapter */

    /* initialize boot line */

    if (ulipIpAddrGet (ulipIpAddr) != -1)
	{
	/* get the ulip base address (eg: "90.0.0.") */

	sysUlipIpBaseAddrGet (ulipIpBaseAddr, ulipIpAddr);

	sprintf ((char *) &sysBootString, "nt(0,%d)%s:%s h=%s e=%s%d u=user",
		simProcnum, "host", (char *) &fullExePath, ulipIpAddr,
		ulipIpBaseAddr, simProcnum + 1);
	}
    else
	{
	sprintf ((char *) &sysBootString, "nt(0,%d)%s:%s u=user",
		simProcnum, "host", (char *) &fullExePath);
	}

    /* set simulator ethernet address according to the processor number */

    ntEnetAddr[5] = simProcnum + 1;

#if (!defined (INCLUDE_NET_INIT) && !defined (INCLUDE_BOOT_LINE_INIT))
    /* sysBootParams init is required by INCLUDE_NET_SYM_TBL */
    bootStringToStruct (sysBootLine, &sysBootParams);
#endif	/* !defined (INCLUDE_NET_INIT) && !defined (INCLUDE_BOOT_LINE_INIT) */

    /* initializes the serial devices */

    sysSerialHwInit ();      /* initialize serial data structure */
    }
/*******************************************************************************
*
* sysHwInit2 - additional system configuration and initialization
*
* This routine connects system interrupts and does any additional
* configuration necessary.
*
* RETURNS: N/A
*/

void sysHwInit2 (void)

    {
    /* connect sys clock interrupt and auxiliary clock interrupt*/

    /* connect serial interrupt */  

    sysSerialHwInit2();

    /* connect stray interrupt XXX */  

    }

/*******************************************************************************
*
* sysPhysMemTop - get the address of the top of physical memory
*
* This routine returns the address of the first missing byte of memory,
* which indicates the top of physical memory.
*
* Memory probing begins at the end of BSS; at every 4K boundary a byte
* is read until it finds one that cannot be read, or 4MB have been probed,
* whichever is first.
*
* RETURNS: The address of the top of physical memory.
*
* INTERNAL
* This routine is used by sysHwInit() to differentiate between models.
* It is highly questionable whether vxMemProbe() can be used during the
* initial stage of booting.
*/

char *sysPhysMemTop (void)
    {
    return(FREE_RAM_ADRS+LOCAL_MEM_SIZE);
    }


/*******************************************************************************
*
* sysMemTop - get the address of the top of VxWorks memory
*
* This routine returns a pointer to the first byte of memory not
* controlled or used by VxWorks.
*
* The user can reserve memory space by defining the macro USER_RESERVED_MEM
* in config.h.  This routine returns the address of the reserved memory
* area.  The value of USER_RESERVED_MEM is in bytes.
*
* RETURNS: The address of the top of VxWorks memory.
*/

char * sysMemTop (void)
    {
    return(FREE_RAM_ADRS+LOCAL_MEM_SIZE - USER_RESERVED_MEM);
    }


/*******************************************************************************
*
* sysToMonitor - transfer control to the ROM monitor
*
* This routine transfers control to the ROM monitor.  It is usually called
* only by reboot() -- which services ^X -- and by bus errors at interrupt
* level.  However, in some circumstances, the user may wish to introduce a
* new <startType> to enable special boot ROM facilities.
*
* RETURNS: Does not return.
*/

STATUS sysToMonitor
    (
    int startType   /* passed to ROM to tell it how to boot */
    )
    {
    simDie (startType != BOOT_NO_AUTOBOOT);
    return (ERROR);	/* We should be dead now */
    }


/*******************************************************************************
*
* sysIntDisable - disable a bus interrupt level
*
* This routine disables a specified bus interrupt level.
*
* RETURNS: ERROR, always.
*
* ARGSUSED0
*/

STATUS sysIntDisable
    (
    int intLevel	/* interrupt level to disable */
    )
    {
    return (ERROR);
    }


/*******************************************************************************
*
* sysIntEnable - enable a bus interrupt level
*
* This routine enables a specified bus interrupt level.
*
* RETURNS: ERROR, always.
*
* ARGSUSED0
*/

STATUS sysIntEnable
    (
    int intLevel	/* interrupt level to enable */
    )
    {
    return (ERROR);
    }


/****************************************************************************
*
* sysProcNumGet - get the processor number
*
* This routine returns the processor number for the CPU board, which is
* set with sysProcNumSet().
*
* RETURNS: The processor number for the CPU board.
*
* SEE ALSO: sysProcNumSet()
*/

int sysProcNumGet (void)

    {
    return (sysProcNum);
    }


/****************************************************************************
*
* sysProcNumSet - set the processor number
*
* Set the processor number for the CPU board.  Processor numbers should be
* unique on a single backplane.
*
* NOTE: By convention, only Processor 0 should dual-port its memory.
*
* RETURNS: N/A
*
* SEE ALSO: sysProcNumGet()
*/

void sysProcNumSet
    (
    int procNum		/* processor number */
    )
    {
    sysProcNum = procNum;
    }

/*******************************************************************************
*
* sysDelay - allow recovery time for port accesses
*
* This routine provides a brief delay used between accesses to the same serial
* port chip.
* 
* RETURNS: N/A
*/

void sysDelay (void)
    {
    }

/*******************************************************************************
*
* sysUlipIpBaseAddrGet - get the Ulip Adapter base address
*
* This routines extract the Ulip Adapter base address from the Ulip Adapter
* address by only keeping the first 3 bytes.
*
* RETURNS: OK if success, ERROR otherwise.
*
* ERRNO: N/A
*
*/

static int sysUlipIpBaseAddrGet
    (
    char *	ulipIpBaseAddr,
    char *	ulipIpAddr
    )
    {
    char * p;
    int i;
	
    p = ulipIpAddr;
    for (i = 0; i < 3; i++)
	{
	p = strstr (p, ".");
	if (p == NULL)
	    return (-1);
	p++;
	}

    strncpy (ulipIpBaseAddr, ulipIpAddr, p - ulipIpAddr);
    ulipIpBaseAddr[p - ulipIpAddr] = 0;

    return (0);
    }
