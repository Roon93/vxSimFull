/* sysAmp.c - PC [34]86/Pentium/Pentium[234] system-dependent library */

/* Copyright 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,10apr02,hdn  written
*/

/*
DESCRIPTION
This library provides routines for the AMP (asymmetric multi processing).

*/


/* includes */

#ifdef  INCLUDE_END
#   include "muxLib.h"
#endif  /* INCLUDE_END */


/* defines */

#define SYSAMP_DBG
#ifdef  SYSAMP_DBG
#   define SYSAMP_DBG_MSG(STR,VALUE)   logMsg(STR,VALUE,0,0,0,0,0);
#else
#   define SYSAMP_DBG_MSG(STR,VALUE)
#endif  /* SYSAMP_DEBUG */


/* imports */

IMPORT int	ifreset ();	 /* for sysApShutdown() */


/* globals */

volatile UINT32 * sysLockSem = (UINT32 *)(MP_LOCK_SEM);
volatile UINT32 * sysNcpu = (UINT32 *)(MP_N_CPU);
volatile UINT32 * sysNipi = (UINT32 *)(MP_N_IPI);


/***************************************************************************
*
* sysApStartup - start the application processor.
*
* This routine loads a boot file and sends a Start Inter Processor Interrupt
* to a AP.
*
* RETURNS: OK or ERROR
*
*/

STATUS sysApStartup
    (
    UINT32 loApicIdAp,		/* AP's local APIC ID */
    char * bootFile		/* bootfile name , e,g "/fd0/bootrom.sys" */
    )
    {
    INT32 fd;			/* file descriptor */
    INT32 nCpu;			/* # of CPUs */
    INT32 status = OK;		/* return value */
    INT32 retry = 0;		/* retry counter */
    INT32 oldResetVector;	/* warm reset vector */
    UINT8 regVal;		/* CMOS register */
    UINT8 oldShutdownCode;	/* CMOS shutdown code */
    UINT32 entryPoint = 0x8000;	/* entry point of the bootrom image */


    /* open the binary image file */

    if ((fd = open (bootFile, O_RDONLY, 0644)) == ERROR)
        {
        printErr ("Boot File open failed. \n");
        return (ERROR);
        }

    /* read 622K bytes to the lower memory at 0x8000 */

    if (read (fd, (char *)entryPoint, 0x98000) == ERROR)
        {
        printErr ("Error during read file: %x\n", errnoGet ());
        return (ERROR);
        }

    close(fd);

    /* get the CPU counter value */

    nCpu = *sysNcpu;

    /* set the AP entry point address in WARM_REST_VECTOR */

    oldResetVector = *(volatile UINT32 *)WARM_RESET_VECTOR;
    *(volatile UINT32 *)WARM_RESET_VECTOR = entryPoint;

    /* initialze the BIOS shutdown code to be 0xA */

    regVal = sysInByte (RTC_INDEX);	/* only 5 LSB'its are used */
    regVal = ((regVal & 0xE0) | BIOS_SHUTDOWN_STATUS);
    sysOutByte (RTC_INDEX, regVal);	/* selects Shutdown Status Register */
    oldShutdownCode = sysInByte (RTC_DATA);	/* get BIOS Shutdown code */
    sysOutByte (RTC_INDEX, regVal);	/* selects Shutdown Status Register */
    sysOutByte (RTC_DATA, 0xA);		/* set BIOS Shutdown code to 0x0A  */

    /* BSP sends AP an INIT-IPI and STARTUP-IPI */

    if (ipiStartup (loApicIdAp, entryPoint, 1) != OK)
	{
	SYSAMP_DBG_MSG ("ipiStartup failed: %d\n", loApicIdAp)
        return (ERROR);
	}

    /* the CPU counter will be incremented by the AP */
 
    while (*sysNcpu != (nCpu + 1))
        {
        taskDelay (sysClkRateGet ());
	if (retry++ > MP_BOOT_TIMEOUT)
	    {
            status = ERROR;
	    break;
	    }
        }

    /* restore the WARM_REST_VECTOR and the BIOS shutdown code */

    *(volatile UINT32 *)WARM_RESET_VECTOR = oldResetVector;
    sysOutByte (RTC_INDEX, regVal);		/* Shutdown Status Reg */
    sysOutByte (RTC_DATA, oldShutdownCode);	/* set BIOS Shutdown code */

    if (status == OK)
	{
        SYSAMP_DBG_MSG ("AP start OK. %d\n", loApicIdAp)
	}
    else
	{
        SYSAMP_DBG_MSG ("AP start failed. %d\n", loApicIdAp)
	}

    return (status);
    }

/***************************************************************************
*
* sysApShutdown - shutdown the application processor.
*
* This routine shutdowns the specified application processor.	
*
* RETURNS: OK or ERROR
*
*/

STATUS sysApShutdown
    (
    UINT32 loApicIdAp		/* AP's local APIC ID */
    )
    {
    INT32 nCpu;			/* # of CPUs */
    INT32 status = OK;		/* return value */
    INT32 retry = 0;		/* retry counter */


    /* get the CPU counter value */

    nCpu = *sysNcpu;

    /* BSP sends AP the shutdown IPI */

    status = ipiShutdown (loApicIdAp, INT_NUM_LOAPIC_IPI + 0);

    /* the CPU counter will be decremented by the AP */
 
    if (status == OK)
	{
        while (*sysNcpu != (nCpu - 1))
            {
            taskDelay (sysClkRateGet ());
	    if (retry++ > MP_BOOT_TIMEOUT)
	        {
                status = ERROR;
	        break;
	        }
            }
	}

    if (status == OK)
	{
        SYSAMP_DBG_MSG ("AP shutdown OK. %d\n", loApicIdAp)
	}
    else
	{
        SYSAMP_DBG_MSG ("AP shutdown failed. %d\n", loApicIdAp)
	}

    return (status);
    }

/***************************************************************************
*
* sysShutdownSup - shutdown the included components in this BSP.
*
* This routine shutdowns the included components in this BSP.
* This routine is called by the IPI shutdown handler and runs in
* the interrupt level. 
*
* RETURNS: N/A
*
*/

void sysShutdownSup (void)
    {
#ifdef  INCLUDE_NETWORK

    /* Is ifreset () and muxDevStopAll () okay in the int level?
     * Since IPI interrupt handler does not use intEnt/intExit,
     * VxWorks does not know if it is in the interrupt level or not.
     * Thus it should be okay, but what if it happened as a nested 
     * interrupt?  INT_RESTRICT() would return ERROR.
     * How about setting intCnt to zero here?  Too wild?
     */

    ifreset ();				/* reset network to avoid interrupts */

#   ifdef INCLUDE_END
   
    (void) muxDevStopAll (WAIT_FOREVER); /* Stop all ENDs */

#   endif  /* INCLUDE_END */
#endif  /* INCLUDE_NETWORK */

    sysClkDisable ();			/* disable the system clock interrupt */
    }

/******************************************************************************
*
* sysLocalToBusAdrs - convert a local address to a bus address
*
* This routine gets the VMEbus address that accesses a specified local
* memory address.
*
* NOTE: This routine has no effect, since there is no VMEbus.
*
* RETURNS: ERROR, always.
*
* SEE ALSO: sysBusToLocalAdrs()
*/

STATUS sysLocalToBusAdrs
    (
    int    adrsSpace,
    char * busAdrs,
    char * *pLocalAdrs
    )
    {
    return (ERROR);
    }

/******************************************************************************
*
* sysBusToLocalAdrs - convert a bus address to a local address
*
* This routine gets the local address that accesses a specified VMEbus
* address.
*
* NOTE: This routine has no effect, since there is no VMEbus.
*
* RETURNS: ERROR, always.
*
* SEE ALSO: sysLocalToBusAdrs()
*/
 
STATUS sysBusToLocalAdrs
    (
    int    adrsSpace,
    char * busAdrs,
    char * *pLocalAdrs
    )
    {
    return (ERROR);
    }

/******************************************************************************
*
* sysBusIntAck - acknowledge a bus interrupt
*
* This routine acknowledges a specified VMEbus interrupt level.
*
* NOTE: This routine has no effect, since there is no VMEbus.
*
* RETURNS: 0.
*/

int sysBusIntAck
    (
    int intLevel
    )
    {
    return (0);
    }

/********************************************************************************
*
* sysBusTas - test and set a location across the bus
*
* This routine performs a test-and-set (TAS) instruction across the backplane.
*
* NOTE: This routine is equivalent to vxTas(), since there is no VMEbus.
*
* RETURNS: TRUE if the value had not been set but is now,
* or FALSE if the value was set already.
*
* SEE ALSO: vxTas()
*/
 
BOOL sysBusTas
    (
    char * adrs
    )
    {
    return (vxTas(adrs));
    }

/******************************************************************************
*
* sysBusIntGen - generate a bus interrupt
*
* This routine generates a VMEbus interrupt for a specified level with a
* specified vector.
*
* NOTE: This routine has no effect, since there is no VMEbus.
*
* RETURNS: ERROR, always.
*/

STATUS sysBusIntGen
    (
    int apicId,
    int vector
    )
    {
    return (loApicIpi (apicId, 0, 0, 1, 0, 0, vector));
    }

/*******************************************************************************
*
* sysIntEnable - enable a bus interrupt level
*
* This routine enables a specified VMEbus interrupt level.  At startup,
* sysHwInit() should have disabled all interrupts.  Specific interrupts
* must be enabled if interrupts are expected.
*
* RETURNS: OK always
*
* SEE ALSO: sysIntDisable()
*/

STATUS sysIntEnable
    (
    int intLvl
    )
    {
    return (OK);
    }

/*******************************************************************************
*
* sysIntDisable - disable a bus interrupt level
*
* This routine disables a specified VMEbus interrupt level.  SysHwInit()
* should have disabled all interrupts by default.  It should not be
* necessary to disable individual interrupts following startup.
*
* RETURNS: ERROR always
*
* SEE ALSO: sysIntEnable()
*/

STATUS sysIntDisable
    (
    int intLvl
    )
    {
    return (ERROR);
    }

/******************************************************************************
*
* sysMailboxConnect - connect a routine to the mailbox interrupt
*
* This routine specifies the interrupt service routine to be called at each
* mailbox interrupt.
*
* NOTE: This routine has no effect, since the hardware does not support mailbox
* interrupts.
*
* RETURNS: ERROR, always.
*
* SEE ALSO: sysMailboxEnable()
*/

STATUS sysMailboxConnect
    (
    FUNCPTR routine,    /* routine called at each mailbox interrupt */
    int     arg         /* argument with which to call routine      */
    )
    {
    return (ERROR);
    }

/******************************************************************************
*
* sysMailboxEnable - enable the mailbox interrupt
*
* This routine enables the mailbox interrupt.
*
* NOTE: This routine has no effect, since the hardware does not support mailbox
* interrupts.
*
* RETURNS: ERROR, always.
*
* SEE ALSO: sysMailboxConnect()
*/

STATUS sysMailboxEnable
    (
    INT8 *mailboxAdrs           /* mailbox address */
    )
    {
    return (ERROR);
    }

