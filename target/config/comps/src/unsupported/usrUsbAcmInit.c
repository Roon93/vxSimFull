/* usrUsbAcmInit.c - USB ACM class driver initialization */

/* Copyright 2000-2001 Wind River Systems, Inc. */

/*
Modification history
--------------------
01a,09jan00,wef  Created
*/
 
/*
DESCRIPTION
 
This configlette initializes the USB ACM Communication class driver.  This
driver is intended for USB modems.  This configlette assumes the USB host 
stack has already been initialized and has a host controller driver attached.

This file also contains routines that are used to issue AT commands to the
modem.  These are given here for testing purposes and are intended to show
how communication to the modem should be in a typical application.


Parameters 


ACM_TYCO_CHAN_NUMBER - channel that the ACM device will be installed on.
*/

/* includes */

#include "vxWorks.h"
#include "string.h"
#include "sioLib.h"
#include "errno.h"
#include "ctype.h"
#include "logLib.h"

#include "usb/usbPlatform.h"
#include "usb/ossLib.h"		    /* operations system srvcs */
#include "usb/usb.h"		    /* general USB definitions */
#include "usb/usbListLib.h"	    /* linked list functions */
#include "usb/usbdLib.h"	    /* USBD interface */
#include "usb/usbLib.h"		    /* USB utility functions */

#include "usb/usbCommdevices.h"
#include "drv/usb/usbAcmLib.h"      /* our API */


/* locals */

int		noOfAcmChans = 0;	/* Count of acm channel structs */


/***************************************************************************
*
* usbAcmAttachCallback- ACM class driver Registration
*
* This function the what happens when a new modem is plugged in or an 
* existing modem is disconnected.  It is responsible for creating the tty
* device and installing it into the I/O system.
*
*
* RETURNS: OK or Error
*/


STATUS usbAcmAttachCallback 
    (
    pVOID arg, 
    SIO_CHAN *pChan,
    UINT16 callbackType, 
    UINT8* ptr1, 
    UINT16 ct
    )
    {

    char tyName [20];		/* string for device name */
    char tyPtr [20];
    DEV_HDR * pDevice;


    USB_ACM_SIO_CHAN * pSioChan = (USB_ACM_SIO_CHAN *)pChan;

    tyName[0] = '\0';		/* initialize to null string */

    strcat (tyName, "/tyCo/");

    if(callbackType == USB_ACM_CALLBACK_ATTACH)
	{

	logMsg (" New modem attached \n",0 ,0 ,0, 0, 0, 0);

	pSioChan->unitNo = noOfAcmChans;	
	
	/* after 2 default serial ports */
	
	strcat (tyName, itos (pSioChan->unitNo + ACM_TYCO_CHAN_NUMBER));     

	if(ttyDevCreate (tyName, (SIO_CHAN *)pSioChan, 512, 512) == ERROR)
	    {
	    logMsg (" %s could not be added to the dev list \n", tyName,
							        0 ,0 ,0, 0, 0);
	    return ERROR;
	    }
	else
	    logMsg (" %s successfully added to dev list \n", tyName,
							    0 ,0 ,0, 0, 0);
	    
	noOfAcmChans++;

	}
    else if(callbackType == USB_ACM_CALLBACK_DETACH)
	{
	logMsg ("Modem removed \n",0 ,0 ,0, 0, 0, 0);

	strcat (tyName, itos (pSioChan->unitNo + ACM_TYCO_CHAN_NUMBER));     /* after 2 default serial ports */

	pDevice = iosDevFind(tyName, (char *) &tyPtr);

	if (pDevice == NULL)
	    {
	    logMsg ("%s not found", tyName,0 ,0 ,0, 0, 0);
	    return ERROR;
	    }
	
	iosDevDelete(pDevice);

	logMsg (" %s removed from dev list \n", tyName,0 ,0 ,0, 0, 0);


	noOfAcmChans--;
	}
	
    return OK;
    }

/*************************************************************************
*
* usrUsbAcmInit - initializes USB ACM class driver.
*
* RETURNS: Nothing
*/


void usrUsbAcmInit (void)
    {

    /* Initialize the BULK class driver */
 
    if (usbAcmLibInit () != OK)
	logMsg ("usbAcmLibInit() returned ERROR\n",0 ,0 ,0, 0, 0, 0);
 
    /* Register for acm device attachment/removal */

    if( usbAcmCallbackRegister (NULL,
			       	USB_ACM_CALLBACK_ATTACH, 
			       	(FUNCPTR) usbAcmAttachCallback,	
			       	0) 
			     == ERROR)
	logMsg ("usbAcmCallbackRegister() returned ERROR\n",0 ,0 ,0, 0, 0, 0);

    }
