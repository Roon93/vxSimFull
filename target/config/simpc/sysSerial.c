/* sysSerial.c - vxSim for Windows serial device initialization */

/* Copyright 1984-2001 Wind River Systems, Inc. */

#include "copyright_wrs.h"

/*
modification history
--------------------
01d,13nov01,jmp  removed INCLUDE_OLD_PIPE references.
01c,21sep98,fle  added library description
01b,03jun98,cym  cleaned up to fix compiler warnings.
01a,29apr98,cym  written based on pc386 version.
*/

/*
DESCRIPTION

This library contains routines for vxSim for Windows serial device
initialization
*/

/* includes */

#include "vxWorks.h"
#include "iv.h"
#include "intLib.h"
#include "config.h"
#include "sysLib.h"
#include "winSio.h"

/* locals */

static WIN_CHAN  winChan;

/******************************************************************************
*
* sysSerialHwInit - initialize the BSP serial devices to a quiescent state
*
* This routine initializes the BSP serial device descriptors and puts the
* devices in a quiescent state.  It is called from sysHwInit() with
* interrupts locked.
*
* RETURNS: N/A
*
* SEE ALSO: sysHwInit()
*/

void sysSerialHwInit (void)
    {
    winDevInit(&winChan);
    sysSerialHwInit2();
    }

/******************************************************************************
*
* sysSerialHwInit2 - connect BSP serial device interrupts
*
* This routine connects the BSP serial device interrupts.  It is called from
* sysHwInit2().  
* 
* Serial device interrupts cannot be connected in sysSerialHwInit() because
* the kernel memory allocator is not initialized at that point, and
* intConnect() calls malloc().
*
* RETURNS: N/A
*
* SEE ALSO: sysHwInit2()
*/

void sysSerialHwInit2 (void)
    {
    /* connect serial interrupts */

    intConnect((VOIDFUNCPTR *)0x102,(VOIDFUNCPTR)&winIntRcv,(int)&winChan);
    winDevInit2(0); 
    }

/******************************************************************************
*
* sysSerialChanGet - get the SIO_CHAN device associated with a serial channel
*
* This routine gets the SIO_CHAN device associated with a specified serial
* channel.
*
* RETURNS: A pointer to the SIO_CHAN structure for the channel, or ERROR
* if the channel is invalid.
*/

SIO_CHAN * sysSerialChanGet
    (
    int channel		/* serial channel */
    )
    {
    return ( (SIO_CHAN *)&winChan );
    }
