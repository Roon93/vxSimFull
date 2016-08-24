/* usrUsbUglKbdInit.c - Initialization of a USB Keyboard in the UGL stack*/
 
/* Copyright 1999-2001 Wind River Systems, Inc. */
 
/*
Modification history
--------------------
01b,05feb01,wef  Created
*/
 
/*
DESCRIPTION
 
This configlette initializes the USB keyboard driver for use with the UGL
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

#include "usb/usbPlatform.h"				/* Basic definitions */
#include "usb/usbdLib.h"					/* USBD interface */

#include "usb/usbHid.h"						/* USB HID definitions */
#include "drv/usb/usbKeyboardLib.h"			/* USB keyoard SIO driver */

#include "logLib.h"

/* defines */

#define UGL_USBKBD_DRV	"usbUglKbd"

/* typedefs */

/* Usb to ugl eyboard driver structure */

typedef struct ugl_usb_kbd_driver	
    {
    UGL_INPUT_DRIVER 	inputDriver;
    SIO_CHAN 			*pKbdSioChan;
    USBD_CLIENT_HANDLE 	usbdHandle;
    } UGL_USB_KBD_DRIVER;

/* Globals */

extern UGL_INPUT_Q_ID uglInputQ;


/* forward static declarations */

LOCAL UGL_STATUS usbUglKeyboardDevDestroy( UGL_INPUT_DRIVER * pInputDriver );

LOCAL VOID usbKeyboardDynamicCallback (pVOID arg, 
				       SIO_CHAN *pChan, 
				       UINT16 attachCode );

LOCAL STATUS usbKeyboardReportCallback( void *arg, char rxChar );

/******************************************************************************
*
* usbUglKbdDevCreate
*
* This routine is call from UGL stack to initialize a keyboard conntected to 
* an USB port.
*
* RETURNS: Pointer UGL_USB_KBD_DRIVER or UGL_NULL
*
*/
void usbUglKeyboardDevCreate	
    (
    void 
    )

    {
    UGL_USB_KBD_DRIVER * pDriver;

    /* allocate memory for driver structure */

    pDriver = (UGL_USB_KBD_DRIVER *)
		    UGL_CALLOC (sizeof(UGL_USB_KBD_DRIVER),1);

    if (pDriver == UGL_NULL)
	{
	printf ("Cant allocate USB / UGL keyboard driver\n");
	return;
	}

    /* register client to usb stack */

    if (usbdClientRegister (UGL_USBKBD_DRV, &(pDriver->usbdHandle)) != OK)
	{
	printf ("usbdClientRegister () returned ERROR\n");
	UGL_FREE(pDriver);
	return;
	}

    /* initialize keyboard library */
    if (usbKeyboardDevInit() != OK)
	{	

	printf ("usbKeyboardDevInit () returned ERROR\n");
	UGL_FREE(pDriver);
	return;
	}

    /* attach dynamic callback to keyboard library */

    if (usbKeyboardDynamicAttachRegister (usbKeyboardDynamicCallback,
					  (pVOID)pDriver) != OK)
	{

	printf ("usbKeyboardDynamicAttachRegister () returned ERROR\n");
	usbKeyboardDevShutdown();
	UGL_FREE(pDriver);
	return;
	}


    /* set destroy function pointer */	

    pDriver->inputDriver.destroy = usbUglKeyboardDevDestroy;

    /* Attach to UGL stack */

    pDriver->inputDriver.version = 1;
    pDriver->inputDriver.handler.qid = uglInputQ;
    pDriver->inputDriver.handler.formatter = UGL_NULL;
    pDriver->inputDriver.handler.device.isr.name = UGL_NULL;
    pDriver->inputDriver.handler.device.isr.interruptId = UGL_NULL;

    /*return (UGL_INPUT_DRIVER *)pDriver;*/
    }

/******************************************************************************
*
* usbUglKeyoardDevDestroy
*
* This routine is call from UGL stack to uninstall all register callback and
* free driver structure.
*
* RETURNS: UGL_STATUS_ERROR or UGL_STATUS_OK
*
*/

LOCAL UGL_STATUS usbUglKeyboardDevDestroy
    (
    UGL_INPUT_DRIVER * pInputDriver	/* USB keyboard driver structure*/
    )

    {
    UGL_USB_KBD_DRIVER * pDriver =
	    (UGL_USB_KBD_DRIVER *)pInputDriver;

    /* unregister dynamic callback from usb stack */

    if (usbKeyboardDynamicAttachUnRegister (usbKeyboardDynamicCallback, 
					    (pVOID)pDriver) 
					 != OK)
	return UGL_STATUS_ERROR;

    /* desinitialize keyboard library */

    if (usbKeyboardDevShutdown() != OK)
	return UGL_STATUS_ERROR;

    /* unregister driver from usb stack */

    if (pDriver->usbdHandle != NULL)
	{
	usbdClientUnregister (pDriver->usbdHandle);
	pDriver->usbdHandle = NULL;	
	}

    /* free structure */

    UGL_FREE (pInputDriver);

    return UGL_STATUS_OK;

    }

/******************************************************************************
*
* usbKeyboardDynamicCallback
*
* This routine is the callback function to notify when a keyboard is attached 
* or removed from USB port.
*
* RETURNS: N/A
*
*/
LOCAL VOID usbKeyboardDynamicCallback
    ( 
    pVOID arg,			/* user-defined arg to callback */
    SIO_CHAN *pChan, 		/* 
				 * struc. of the channel being 
				 * created/destroyed 
				 */
    UINT16 attachCode		/* attach code */
    )

    {

    UGL_USB_KBD_DRIVER * pDriver = (UGL_USB_KBD_DRIVER *)arg;


    if (attachCode == USB_KBD_ATTACH)		/* keyboard device attached */
	{
	if (pDriver->pKbdSioChan == NULL)
	    {
	    if (usbKeyboardSioChanLock(pChan) == OK)
		    pDriver->pKbdSioChan = pChan;

	    if (pDriver->pKbdSioChan != NULL)
		{
		/* Install the callback report function */

		(*pDriver->pKbdSioChan->pDrvFuncs->callbackInstall) 
						(pDriver->pKbdSioChan,	
						 SIO_CALLBACK_PUT_RCV_CHAR,
						 usbKeyboardReportCallback, 
						 (pVOID)pDriver);

		/* Start keyboard in supported mode */
		
		(*pDriver->pKbdSioChan->pDrvFuncs->ioctl)(pDriver->pKbdSioChan,
							  SIO_MODE_SET, 
							  (pVOID)SIO_MODE_INT);
		}
	    }
	}
    else				/* keyboard device removed */
	{
	if (pChan == pDriver->pKbdSioChan)
	    if (usbKeyboardSioChanUnlock(pChan) == OK)
		    pDriver->pKbdSioChan = NULL;

	}
    }

/******************************************************************************
*
* usbKeyboardReportCallback(
*
* This routine is the event callback. It notify when the USB keyboard key as
* been press. It then post the message to the UGL stack.
*
* RETURNS: UGL_STATUS_DROP or OK
*
*/
LOCAL STATUS usbKeyboardReportCallback
    ( 
    void *arg, 
    char rxChar
    )

    {
    UGL_INPUT_EVENT * pInputEvent;
    UGL_USB_KBD_DRIVER * pDriver = 
	    (UGL_USB_KBD_DRIVER *)arg;

    /* get a input event handler from UGL */
    if (uglInputQEventStorageGet (pDriver->inputDriver.handler.qid,
				  &pInputEvent) 
				!= UGL_STATUS_OK)
	{
	/* Out of room. Throw out event. */
	return UGL_STATUS_DROP;
	}

    if (pInputEvent == UGL_NULL)
	    return UGL_STATUS_DROP;

    memset(pInputEvent, 0, sizeof(*pInputEvent));

    /* fill InputEvent message */

    pInputEvent->id = UGL_INPUT_EVENT_TYPE_KEYBOARD;
    pInputEvent->sourceDevice = (UGL_INPUT_HANDLER *) pDriver;
    pInputEvent->event.keyboard.key = rxChar;
    pInputEvent->event.keyboard.modifiers |= 
		    UGL_INPUT_EVENT_KEYBOARD_KEYCAP_STATE;

    /* send input message to UGL input queue */
    uglInputQEventPost (&pInputEvent);

    return OK;
    }
