/* usrUsbUglMseInit.c - Initialization of a USB Mouse in the UGL stack*/

/* Copyright 1999-2001 Wind River Systems, Inc. */

/*
Modification history
--------------------
01b,05feb01,wef	 Created
*/

/*
DESCRIPTION

This configlette initializes the USB mouse driver for use with the UGL
stack.  This assumes the USB host stack has already been initialized and 
has a host controller driver attached.   

Refer to the Zinc for Tornado users guide for help on configuring the 
graphics stack.

*/


/* includes */

#include "stdlib.h"
#include "string.h"

#include "ugl/uglos.h"
#include "ugl/uglin.h"

#include "usb/usbPlatform.h"		/* Basic definitions */
#include "usb/usbdLib.h"		/* USBD interface */

#include "usb/usbHid.h"			/* USB HID definitions */
#include "drv/usb/usbMouseLib.h"	/* USB mouse SIO driver */

/* defines */

#define USB_UGLMSE_DRV	"usbUglMse"

/* typedefs */

/* Usb to ugl mouse driver structure */

typedef struct usb_ugl_mouse_driver	
    {
    UGL_INPUT_DRIVER 		inputDriver;
    SIO_CHAN 			*pMseSioChan;
    USBD_CLIENT_HANDLE 		usbdHandle;
    UGL_UINT16			oldButtonState;
    } USB_UGL_MOUSE_DRIVER;


/* Globals */

extern UGL_INPUT_Q_ID uglInputQ;


/* forward static declarations */

LOCAL UGL_STATUS usbUglMouseDevDestroy ( UGL_INPUT_DRIVER * pInputDriver );

LOCAL VOID usbMouseDynamicCallback ( pVOID arg, 
				     SIO_CHAN *pChan, 
				     UINT16 attachCode );

LOCAL STATUS usbMouseReportCallback (void *arg, pHID_MSE_BOOT_REPORT pReport);



/******************************************************************************
*
* usbUglMouseDevCreate
*
* This routine is call from UGL stack to initialize a mouse conntected to 
* an USB port.
*
* RETURNS: Pointer USB_UGL_MOUSE_DRIVER or UGL_NULL
*
*/

void usbUglMouseDevCreate 
    ( 
    void 
    )
    {
    USB_UGL_MOUSE_DRIVER * pDriver;


    /* allocate memory for driver structure */

    pDriver = (USB_UGL_MOUSE_DRIVER *) UGL_CALLOC 
				(sizeof(USB_UGL_MOUSE_DRIVER),1);


    if (pDriver == UGL_NULL)
	{
	printf ("Cant allocate USB / UGL mouse driver\n");
	return;
	}

    /* Register client to usb stack */

    if (usbdClientRegister (USB_UGLMSE_DRV, &(pDriver->usbdHandle)) != OK)
	{
	UGL_FREE(pDriver);
	printf ("usbdClientRegister () returned ERROR\n");
	return;
	}

    /* Initialize mouse library */

    if (usbMouseDevInit() != OK)
	{	
	printf ("usbMouseDevInit () returned ERROR\n");
	UGL_FREE(pDriver);
	return;
	}

    /* attach dynamic callback to mouse library */
    if (usbMouseDynamicAttachRegister(usbMouseDynamicCallback,
				      (pVOID)pDriver) 
				    != OK)
	{
	printf ("usbMouseDynamicAttachRegister () returned ERROR\n");
	usbMouseDevShutdown();
	UGL_FREE(pDriver);
	}


    /* Set destroy function pointer */	

    pDriver->inputDriver.destroy = usbUglMouseDevDestroy;
    pDriver->oldButtonState = 0;

    /* Attach to UGL stack */

    pDriver->inputDriver.handler.qid = uglInputQ;
    pDriver->inputDriver.handler.formatter = UGL_NULL;
    pDriver->inputDriver.handler.device.isr.name = UGL_NULL;
    pDriver->inputDriver.handler.device.isr.interruptId = UGL_NULL;
    pDriver->inputDriver.handler.frame.xMin = 0;
    pDriver->inputDriver.handler.frame.yMin = 0;
    pDriver->inputDriver.handler.frame.xMax = (UGL_ORD) USB_UGL_CONFIG_SCREEN_WIDTH;
    pDriver->inputDriver.handler.frame.yMax = (UGL_ORD) USB_UGL_CONFIG_SCREEN_HEIGHT;

    }

/******************************************************************************
*
* usbUglMouseDevDestroy
*
* This routine is call from UGL stack to uninstall all register callback and
* free driver structure.
*
* RETURNS: UGL_STATUS_ERROR or UGL_STATUS_OK
*
*/
LOCAL UGL_STATUS usbUglMouseDevDestroy
    (
    UGL_INPUT_DRIVER * pInputDriver		/* USB mouse driver structure */
    )
    {

    USB_UGL_MOUSE_DRIVER * pDriver = (USB_UGL_MOUSE_DRIVER *)pInputDriver;

    /* Unregister dynamic callback from usb stack */

    if (usbMouseDynamicAttachUnRegister (usbMouseDynamicCallback, 
					(pVOID)pDriver) 
				      != OK)
	    return UGL_STATUS_ERROR;

    /* Desinitialize mouse library */

    if (usbMouseDevShutdown() != OK)
	    return UGL_STATUS_ERROR;

    /* Unregister driver from usb stack */

    if (pDriver->usbdHandle != NULL)
	{
	usbdClientUnregister (pDriver->usbdHandle);
	pDriver->usbdHandle = NULL;	
	}

    /* Free structure */

    UGL_FREE (pInputDriver);

    return UGL_STATUS_OK;
    }

/******************************************************************************
*
* usbMouseDynamicCallback
*
* This routine is the callback function to notify when a mouse is attached or
* removed from USB port.
*
* RETURNS: N/A
*
*/
LOCAL VOID usbMouseDynamicCallback( 
	pVOID arg,			/* user-defined arg to callback */
	SIO_CHAN *pChan, 		/* struc. of the channel being 
					 * created/destroyed 
					 */
	UINT16 attachCode		/* attach code */
	)
    {
    USB_UGL_MOUSE_DRIVER * pDriver = (USB_UGL_MOUSE_DRIVER *)arg;


    if (attachCode == USB_MSE_ATTACH)
	{
	if (usbMouseSioChanLock (pChan) != OK)
	    printf ("usbMouseSioChanLock () returned ERROR\n");
	else
	    {
	    pDriver->pMseSioChan = pChan;

	    if (pDriver->pMseSioChan != NULL)
		{
		/* Install the callback report function */

		(*pDriver->pMseSioChan->pDrvFuncs->callbackInstall)
					(pDriver->pMseSioChan,	
					SIO_CALLBACK_PUT_MOUSE_REPORT,
					usbMouseReportCallback, 
					(pVOID)pDriver);
		}

	    }
	}
    else if (attachCode == USB_MSE_REMOVE)	/* mouse device removed */
	{
	if (pChan == pDriver->pMseSioChan)
	    if (usbMouseSioChanUnlock(pChan) == OK)
		    pDriver->pMseSioChan = NULL;

	}
    }

/******************************************************************************
*
* usbMouseReportCallback
*
* This routine is the event callback. It notify when the USB mouse as move or
* a button was click. It then post the message to the UGL stack.
*
* RETURNS: UGL_STATUS_DROP or OK
*
*/
LOCAL STATUS usbMouseReportCallback
    ( 
    void *arg, 
    pHID_MSE_BOOT_REPORT pReport 
    )

    {

    UGL_INPUT_EVENT * pInputEvent;
    USB_UGL_MOUSE_DRIVER * pDriver = (USB_UGL_MOUSE_DRIVER *)arg;

	    
    if (uglInputQEventStorageGet(pDriver->inputDriver.handler.qid,
				     &pInputEvent) 
				   != UGL_STATUS_OK)
	return UGL_STATUS_DROP;

    if (pInputEvent == UGL_NULL)
	return UGL_STATUS_DROP;

    memset(pInputEvent, 0, sizeof(*pInputEvent));

    pInputEvent->id = UGL_INPUT_EVENT_TYPE_INCREMENTAL_POINTER;
    pInputEvent->sourceDevice = (UGL_INPUT_HANDLER *)pDriver;

    pInputEvent->event.pointer.dx = (signed char) pReport->xDisplacement; 
    pInputEvent->event.pointer.dy = (signed char) pReport->yDisplacement;

    if ((pReport->buttonState & MOUSE_BUTTON_1) != 0)
	    pInputEvent->event.pointer.buttonState |= 1;

    if ((pReport->buttonState & MOUSE_BUTTON_2) != 0)
	    pInputEvent->event.pointer.buttonState |= 2;

    if ((pReport->buttonState & MOUSE_BUTTON_3) != 0)
	    pInputEvent->event.pointer.buttonState |= 4;


    /* Save button state changes */

    pInputEvent->event.pointer.buttonChange = 
			(pInputEvent->event.pointer.buttonState ^ 
			 pDriver->oldButtonState);

    pDriver->oldButtonState = pInputEvent->event.pointer.buttonState;

    /* Post to ugl stack the notification event */

    if (uglInputQEventPost(&pInputEvent) != UGL_STATUS_OK)
	{
	printf ("uglInputQEventPost () returned ERRROR.\n");
	return UGL_STATUS_DROP;
	}

    
    return OK;
    }
