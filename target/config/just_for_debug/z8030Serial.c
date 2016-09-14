/* z8530Serial.c - Z8530 SCC (Serial Communications Controller) tty driver*/

/* Copyright 1984-1993 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,23feb93,eve  started from version 01n of z8530Serial.c.
*/

/*
DESCRIPTION
This is the driver for the Z8030 SCC (Serial Communications Controller).
It uses the SCCs in asynchronous mode only.

USER-CALLABLE ROUTINES
Most of the routines in this driver are accessible only through the I/O
system.  Two routines, however, must be called directly: tyCoDrv() to
initialize the driver, and tyCoDevCreate() to create devices.

Before the driver can be used, it must be initialized by calling tyCoDrv().
This routine should be called exactly once, before any reads, writes, or
calls to tyCoDevCreate().  Normally, it is called by usrRoot() in usrConfig.c.

Before a terminal can be used, it must be created using tyCoDevCreate().
Each port to be used should have exactly one device associated with it by
calling this routine.

IOCTL FUNCTIONS
This driver responds to the same ioctl() codes as a normal tty driver; for
more information, see the manual entry for tyLib.  Available baud rates
range from 50 to 38400.

SEE ALSO
tyLib
*/

#include "vxWorks.h"
#include "iv.h"
#include "ioLib.h"
#include "iosLib.h"
#include "tyLib.h"
#include "intLib.h"
#include "errnoLib.h"
#include "drv/serial/z8030.h"

#define DEFAULT_BAUD	9600

IMPORT TY_CO_DEV tyCoDv []; /* device descriptors */

LOCAL int tyCoDrvNum;		/* driver number assigned to this driver */

/* forward declarations */

LOCAL void   tyCoStartup ();
LOCAL int    tyCoOpen ();
LOCAL STATUS tyCoIoctl ();
LOCAL void   tyCoHrdInit ();
LOCAL void   tyCoInitChannel ();
LOCAL void   tyCoResetChannel ();
LOCAL void   delayRegAccess();

/*******************************************************************************
*
* tyCoDrv - initialize the tty driver
*
* This routine initializes the serial driver, sets up interrupt vectors, and
* performs hardware initialization of the serial ports.
*
* This routine should be called exactly once, before any reads, writes, or
* calls to tyCoDevCreate().  Normally, it is called by usrRoot() in
* usrConfig.c.
*
* RETURNS: OK, or ERROR if the driver cannot be installed.
*
* SEE ALSO: tyCoDevCreate()
*/

STATUS tyCoDrv (void)

    {
    /* check if driver already installed */
    
    if (tyCoDrvNum > 0)
	return (OK);

    tyCoHrdInit ();

    tyCoDrvNum = iosDrvInstall (tyCoOpen, (FUNCPTR) NULL, tyCoOpen,
				(FUNCPTR) NULL, tyRead, tyWrite, tyCoIoctl);

    return (tyCoDrvNum == ERROR ? ERROR : OK);
    }

/*******************************************************************************
*
* tyCoDevCreate - create a device for an on-board serial port
*
* This routine creates a device on a specified serial port.  Each port
* to be used should have exactly one device associated with it by calling
* this routine.
*
* For instance, to create the device "/tyCo/0", with buffer sizes of 512 bytes,
* the proper call would be:
* .CS
*     tyCoDevCreate ("/tyCo/0", 0, 512, 512);
* .CE
*
* RETURNS: OK, or ERROR if the driver is not installed, the channel is
* invalid, or the device already exists.
*
* SEE ALSO: tyCoDrv()
*/

STATUS tyCoDevCreate
    (
    char *      name,           /* name to use for this device      */
    FAST int    channel,        /* physical channel for this device */
    int         rdBufSize,      /* read buffer size, in bytes       */
    int         wrtBufSize      /* write buffer size, in bytes      */
    )
    {
    FAST TY_CO_DEV *pTyCoDv;

    if (tyCoDrvNum <= 0)
	{
	errnoSet (S_ioLib_NO_DRIVER);
	return (ERROR);
	}

    /* if this doesn't represent a valid channel, don't do it */

    if (channel < 0 || channel >= tyCoDv [0].numChannels)
	return (ERROR);

    pTyCoDv = &tyCoDv [channel];

    /* if there is a device already on this channel, don't do it */

    if (pTyCoDv->created)
	return (ERROR);

    /* initialize the ty descriptor */

    if (tyDevInit (&pTyCoDv->tyDev, rdBufSize, wrtBufSize,
		   (FUNCPTR) tyCoStartup) != OK)
	{
	return (ERROR);
	}

    /* initialize the channel hardware */

    tyCoInitChannel (channel);

    /* mark the device as created, and add the device to the I/O system */

    pTyCoDv->created = TRUE;

    return (iosDevAdd (&pTyCoDv->tyDev.devHdr, name, tyCoDrvNum));
    }

/*******************************************************************************
*
* tyCoHrdInit - initialize the USART
*/

LOCAL void tyCoHrdInit (void)

    {
    FAST int   oldlevel;	/* current interrupt level mask */
    int        ix;

    oldlevel = intLock ();	/* disable interrupts during init */

    for (ix=0; ix < tyCoDv [0].numChannels; ix++)
	tyCoResetChannel (ix);	/* reset channel */

    intUnlock (oldlevel);
    }

/********************************************************************************
*
* tyCoResetChannel - reset a single channel
*/

LOCAL void tyCoResetChannel
    (
    int channel
    )
    {
    volatile char *cr = tyCoDv [channel].cr;        /* SCC control reg adr */
    int delay;

    *Z8030_WR0(cr) = SCC_WR0_ERR_RST  ;
    delayRegAccess();
    *Z8030_WR0(cr) = SCC_WR0_RST_INT  ;

     
    if ( (delay = channel % 2 ) == 0 )
	*Z8030_WR9(cr) = SCC_WR9_CH_A_RST;
    else
        *Z8030_WR9(cr) = SCC_WR9_CH_B_RST; 

    for (delay = 0; delay < 1000; delay++)
	;     /* spin wheels for a moment */
    }

/*******************************************************************************
*
* tyCoInitChannel - initialize a single channel
*/

LOCAL void tyCoInitChannel
    (
    int channel
    )
    {
    FAST TY_CO_DEV *pTyCoDv = &tyCoDv [channel];
    volatile char  *cr = pTyCoDv->cr;	/* SCC control reg adr */
    FAST int   	    baudConstant;
    FAST int        oldlevel;		/* current interrupt level mask */
    int	            zero = 0;

    oldlevel = intLock ();		/* disable interrupts during init */

    /* initialize registers */

    *Z8030_WR4(cr) = SCC_WR4_1_STOP | SCC_WR4_16_CLOCK;

    delayRegAccess();
    *Z8030_WR1(cr) = SCC_WR1_INT_ALL_RX | SCC_WR1_TX_INT_EN;

    delayRegAccess();
    *Z8030_WR3(cr) = SCC_WR3_RX_8_BITS;

    delayRegAccess();
    *Z8030_WR5(cr) = SCC_WR5_TX_8_BITS | SCC_WR5_DTR | SCC_WR5_RTS;

    delayRegAccess();
    *Z8030_WR10(cr)= zero;		/* clear sync, loop, poll */

    delayRegAccess();
    *Z8030_WR11(cr) = pTyCoDv->clockModeWR11;

    delayRegAccess();
    *Z8030_WR15(cr) = zero;

    /* Calculate the baud rate constant for the default baud rate
     * from the input clock frequency.  This assumes that the
     * divide-by-16 bit is set (done in WR4 above).
     */

    baudConstant = ((pTyCoDv->baudFreq / 32) / DEFAULT_BAUD) - 2;

    *Z8030_WR12(cr) = (char) baudConstant	;	/* write LSB */

    delayRegAccess();
    *Z8030_WR13(cr) = (char) (baudConstant >> 8);	/* write MSB */

    delayRegAccess();
    *Z8030_WR14(cr) = pTyCoDv->clockModeWR14;

    delayRegAccess();
    *Z8030_WR15(cr) = zero;

    /* reset external interrupts */

    delayRegAccess();
    *Z8030_WR0(cr) = SCC_WR0_RST_INT  ;

    /* reset errors */

    delayRegAccess();
    *Z8030_WR0(cr) = SCC_WR0_ERR_RST  ;  

    delayRegAccess();
    *Z8030_WR3(cr) = SCC_WR3_RX_8_BITS | SCC_WR3_RX_EN;

    delayRegAccess();
    *Z8030_WR5(cr) = SCC_WR5_TX_8_BITS | SCC_WR5_TX_EN |
                     SCC_WR5_DTR       | SCC_WR5_RTS;

    delayRegAccess();
    *Z8030_WR2(cr) = pTyCoDv->intVec;

    delayRegAccess();
    *Z8030_WR1(cr) = SCC_WR1_INT_ALL_RX | SCC_WR1_TX_INT_EN;

    /* enable interrupts */

    delayRegAccess();
    *Z8030_WR9(cr) = SCC_WR9_MIE | pTyCoDv->intType;

    delayRegAccess();
    *Z8030_WR0(cr) = SCC_WR0_RST_TX_CRC  ;

    delayRegAccess();
    *Z8030_WR0(cr) = SCC_WR0_RST_INT  ;

    delayRegAccess();
    *Z8030_WR0(cr) = SCC_WR0_RST_INT  ;

    delayRegAccess();
    *Z8030_WR0(cr) = 0  ;

    intUnlock (oldlevel);
    }

/*******************************************************************************
*
* tyCoOpen - open file to USART
*/

LOCAL int tyCoOpen
    (
    TY_CO_DEV *pTyCoDv,
    char      *name,
    int        mode
    )
    {
    return ((int) pTyCoDv);
    }

/*******************************************************************************
*
* tyCoIoctl - special device control
*
* This routine handles FIOBAUDRATE requests and passes all others to tyIoctl().
*
* RETURNS: OK, or ERROR if invalid baud rate, or whatever tyIoctl() returns.
*/

LOCAL STATUS tyCoIoctl
    (
    TY_CO_DEV *pTyCoDv,		/* device to control */
    int        request,		/* request code */
    int        arg		/* some argument */
    )
    {
    FAST int     oldlevel;		/* current interrupt level mask */
    FAST int     baudConstant;
    FAST STATUS  status;
    volatile char *cr;			/* SCC control reg adr */

    switch (request)
	{
	case FIOBAUDRATE:

	    if (arg < 50 || arg > 38400)
	        {
		status = ERROR;		/* baud rate out of range */
		break;
	        }

	    /* Calculate the baud rate constant for the new baud rate
	     * from the input clock frequency.  This assumes that the
	     * divide-by-16 bit is set in the Z8530 WR4 register (done
	     * in tyCoInitChannel).
	     */

	    baudConstant = ((pTyCoDv->baudFreq / 32) / arg) - 2;

	    cr = pTyCoDv->cr;

	    /* disable interrupts during chip access */

	    oldlevel = intLock ();

	    *Z8030_WR12(cr) = (char)baudConstant;	/* write LSB */

            delayRegAccess();
	    *Z8030_WR13(cr) = (char)(baudConstant >> 8); /* write MSB */

	    intUnlock (oldlevel);

	    status = OK;
	    break;

	default:
	    status = tyIoctl (&pTyCoDv->tyDev, request, arg);
	    break;
	}
    return (status);
    }

/*******************************************************************************
*
* tyCoIntWr - interrupt level processing
*
* This routine handles write interrupts from the SCC.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void tyCoIntWr
    (
    int channel			/* interrupting serial channel */
    )
    {
    char            outChar;
    FAST TY_CO_DEV *pTyCoDv = &tyCoDv [channel];
    volatile char  *cr = pTyCoDv->cr;

    if (pTyCoDv->created && tyITx (&pTyCoDv->tyDev, &outChar) == OK)
	{
	*pTyCoDv->dr = outChar;
	}
    else
	{
	/* no more chars to xmit now.  reset the tx int,
	 * so the SCC does not keep interrupting.
	 */

	*Z8030_WR0(cr)= SCC_WR0_RST_TX_INT  ;
	}

    /* end the interrupt acknowledge */

    delayRegAccess();
    *Z8030_WR0(cr)= SCC_WR0_RST_HI_IUS  ;

    }

/*****************************************************************************
*
* tyCoIntRd - interrupt level input processing
*
* This routine handles read interrupts from the SCC
*
* RETURNS: N/A
*
* NOMANUAL
*/

void tyCoIntRd
    (
    int channel			/* interrupting serial channel */
    )
    {
    FAST TY_CO_DEV *pTyCoDv = &tyCoDv [channel];
    volatile char  *cr = pTyCoDv->cr;
    char            inchar;

    inchar = *pTyCoDv->dr;

    if (pTyCoDv->created)
	tyIRd (&pTyCoDv->tyDev, inchar);

    /* reset the interrupt in the Z8530 */

    *Z8030_WR0(cr) = SCC_WR0_RST_HI_IUS  ;
    delayRegAccess(); 
    }

/**********************************************************************
*
* tyCoIntEx - miscellaneous interrupt processing
*
* This routine handles miscellaneous interrupts on the SCC
*
* RETURNS: N/A
*
* NOMANUAL
*/

void tyCoIntEx
    (
    int channel			/* interrupting serial channel */
    )
    {

    FAST TY_CO_DEV *pTyCoDv = &tyCoDv [channel];
    volatile char  *cr = pTyCoDv->cr;



   /* reset the interrupt in the Z8530 */

    *Z8030_WR0(cr) = SCC_WR0_ERR_RST  ;	  

    delayRegAccess();
    *Z8030_WR0(cr) = SCC_WR0_RST_INT  ;	  

    delayRegAccess();
    *Z8030_WR0(cr) = SCC_WR0_RST_HI_IUS  ;
    }

/********************************************************************************
*
* tyCoInt - interrupt level processing
*
* This routine handles interrupts from both of the SCCs.
* We determine from the parameter which SCC interrupted, then look at
* the code to find out which channel and what kind of interrupt.
* 
* NOMANUAL
*/

void tyCoInt
    (
    int sccNum
    )
    {
    volatile char   *cr;
    FAST char        intStatus;
    FAST TY_CO_DEV  *pTyCoDv;
    char             outChar;

    /* We need to find out which channel interrupted.  We need to read
     * the B channel of the interrupting SCC to find out which channel
     * really interrupted.  Note that things are set up so that the A
     * channel is channel 0, even though on the chip it is the one with
     * the higher address
     */  

    pTyCoDv = &tyCoDv [(sccNum * 2) + 1];
    cr = pTyCoDv->cr;

    intStatus = *Z8030_WR2(cr);

    if ((intStatus & 0x08) != 0)
        {                               /* the A channel interrupted */
        --pTyCoDv;
        cr = pTyCoDv->cr;
        }

    switch (intStatus & 0x06)
        {
        case 0x00:                      /* Tx Buffer Empty */
            if (pTyCoDv->created && (tyITx (&pTyCoDv->tyDev, &outChar) == OK))
                *pTyCoDv->dr = outChar;
            else
                {
                /* no more chars to xmit now.  reset the tx int,
                 * so the SCC doesn't keep interrupting us.
		 */
 
                *Z8030_WR0(cr) = SCC_WR0_RST_TX_INT  ;
                }
            break;
 
        case 0x04:                      /* RxChar Avail */
            if (pTyCoDv->created)
                tyIRd (&pTyCoDv->tyDev, *pTyCoDv->dr);
            break;
 
        case 0x02:                      /* External Status Change */
            *Z8030_WR0(cr) = SCC_WR0_ERR_RST  ;
            outChar = *pTyCoDv->dr;          /* throw away char */
            break;
 
        case 0x06:                      /* Special receive condition */
            *Z8030_WR0(cr) = SCC_WR0_ERR_RST  ;
            break;                      /* ignore */
 
        }

    /* Reset the interrupt in the Z8530 */

    delayRegAccess();
    *Z8030_WR0(cr) = SCC_WR0_RST_HI_IUS  ;
    }

/*******************************************************************************
*
* tyCoStartup - transmitter startup routine
*
* Call interrupt level character output routine.
*/

LOCAL void tyCoStartup
    (
    TY_CO_DEV *pTyCoDv 		/* ty device to start up */
    )
    {
    char outChar;

    if (tyITx (&pTyCoDv->tyDev, &outChar) == OK)
        {
	*pTyCoDv->dr = outChar;
        delayRegAccess();
        }
    }

/*******************************************************************************
*
* delayRegAccess - delay routine for register access.
*
*/

LOCAL void delayRegAccess(void)
    {
    int delay;

    for (delay = 0; delay < 2; delay++);
    }
