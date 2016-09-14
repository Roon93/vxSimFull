/* usbTargKbdLib.c - USB keyboard target exerciser/demonstration */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01b,23nov99,rcb  Change #include ../xxx references to lower case.
01a,18aug99,rcb  First.
*/

/*
DESCRIPTION

This module contains code to exercise the usbTargLib by emulating a rudimentary
USB keyboard.  This module will generally be invoked by usbTool or a similar USB
test/exerciser application.

It is the caller's responsibility to initialize usbTargLib and attach a USB TCD 
to it.	When attaching a TCD to usbTargLib, the caller must pass a pointer to a
table of callbacks required by usbTargLib.  The address of this table and the
"callback parameter" required by these callbacks may be obtained by calling
usbTargKbdCallbackInfo().  It is not necessary to initialize the usbTartKbdLib or
to shut it down.  It performs all of its operations in response to callbacks from
usbTargLib.

This module also exports a function called usbTargKbdInjectReport().  This 
function allows the caller to inject a "boot report" into the interrupt pipe.
This allows for rudimentary emulation of keystrokes.
*/


/* includes */

#include "usb/usbPlatform.h"

#include "string.h"

#include "usb/usb.h"
#include "usb/usbHid.h" 		    /* USB HID definitions */
#include "usb/usbDescrCopyLib.h"			/* USB utility functions */
#include "usb/target/usbTargLib.h"	    /* USB target functions */
#include "drv/usb/target/usbTargKbdLib.h"   /* our API */




/* defines */

/* string identifiers */

#define UNICODE_ENGLISH     0x409

#define ID_STR_MFG	    1
#define ID_STR_MFG_VAL	    "Wind River Systems"

#define ID_STR_PROD	    2
#define ID_STR_PROD_VAL     "USB keyboard emulator"


/* keyboard device */

#define KBD_USB_VERSION 		0x0100
#define KBD_NUM_CONFIG			1


/* keyboard configuration */

#define KBD_CONFIG_VALUE		1
#define KBD_NUM_INTERFACES		1


/* keyboard interface */

#define KBD_INTERFACE_NUM		0
#define KBD_INTERFACE_ALT_SETTING	1
#define KBD_NUM_ENDPOINTS		1


/* keyboard interrupt endpoint */

#define KBD_INTERRUPT_ENDPOINT_NUM	0x01
#define KBD_INTERRUPT_ENDPOINT_INTERVAL 20	/* 20 milliseconds */


/* typedefs */


/* forward declarations */

LOCAL STATUS mngmtFunc
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT16 mngmtCode
    );

LOCAL STATUS featureClear
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT8 requestType,
    UINT16 feature,
    UINT16 index
    );

LOCAL STATUS featureSet
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT8 requestType,
    UINT16 feature,
    UINT16 index
    );

LOCAL STATUS configurationGet
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    pUINT8 pConfiguration
    );

LOCAL STATUS configurationSet
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT8 configuration
    );

LOCAL STATUS descriptorGet
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT8 requestType,
    UINT8 descriptorType,
    UINT8 descriptorIndex,
    UINT16 languageId,
    UINT16 length,
    pUINT8 pBfr,
    pUINT16 pActLen
    );

LOCAL STATUS interfaceGet
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT16 interfaceIndex,
    pUINT8 pAlternateSetting
    );

LOCAL STATUS interfaceSet
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT16 interfaceIndex,
    UINT8 alternateSetting
    );


LOCAL STATUS vendorSpecific
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT8 requestType,
    UINT8 request,
    UINT16 value,
    UINT16 index,
    UINT16 length
    );


/* locals */

LOCAL USB_TARG_CALLBACK_TABLE usbTargKbdCallbackTable =
    {
    mngmtFunc,		/* mngmtFunc */
    featureClear,	/* featureClear */
    featureSet, 	/* featureSet */
    configurationGet,	/* configurationGet */
    configurationSet,	/* configurationSet */
    descriptorGet,	/* descriptorGet */
    NULL,		/* descriptorSet */
    interfaceGet,	/* interfaceGet */
    interfaceSet,	/* interfaceSet */
    NULL,		/* statusGet */
    NULL,		/* addressSet */
    NULL,		/* synchFrameGet */
    vendorSpecific,	/* vendorSpecific */
    };


LOCAL USB_TARG_CHANNEL channel;
LOCAL UINT16 numEndpoints;
LOCAL pUSB_TARG_ENDPOINT_INFO pEndpoints;

LOCAL UINT8 curConfiguration;
LOCAL UINT8 curAlternateSetting;
LOCAL USB_TARG_PIPE intPipeHandle;

LOCAL USB_ERP reportErp;
LOCAL BOOL reportInUse;
LOCAL HID_KBD_BOOT_REPORT reportBfr;


LOCAL USB_LANGUAGE_DESCR langDescr =
    {sizeof (USB_LANGUAGE_DESCR), USB_DESCR_STRING, 
	{TO_LITTLEW (UNICODE_ENGLISH)}};

LOCAL char *pStrMfg = ID_STR_MFG_VAL;
LOCAL char *pStrProd = ID_STR_PROD_VAL;


LOCAL USB_DEVICE_DESCR devDescr =
    {
    USB_DEVICE_DESCR_LEN,	    /* bLength */
    USB_DESCR_DEVICE,		    /* bDescriptorType */
    TO_LITTLEW (KBD_USB_VERSION),   /* bcdUsb */
    0,				    /* bDeviceClass */
    0,				    /* bDeviceSubclass */
    0,				    /* bDeviceProtocol */
    USB_MIN_CTRL_PACKET_SIZE,	    /* maxPacketSize0 */
    0,				    /* idVendor */
    0,				    /* idProduct */
    0,				    /* bcdDevice */
    ID_STR_MFG, 		    /* iManufacturer */
    ID_STR_PROD,		    /* iProduct */
    0,				    /* iSerialNumber */
    KBD_NUM_CONFIG		    /* bNumConfigurations */
    };


LOCAL USB_CONFIG_DESCR configDescr =
    {
    USB_CONFIG_DESCR_LEN,	    /* bLength */
    USB_DESCR_CONFIGURATION,	    /* bDescriptorType */
    TO_LITTLEW (USB_CONFIG_DESCR_LEN + USB_INTERFACE_DESCR_LEN + 
    USB_ENDPOINT_DESCR_LEN),
				    /* wTotalLength */
    KBD_NUM_INTERFACES, 	    /* bNumInterfaces */
    KBD_CONFIG_VALUE,		    /* bConfigurationValue */
    0,				    /* iConfiguration */
    USB_ATTR_SELF_POWERED,	    /* bmAttributes */
    0				    /* MaxPower */
    };

LOCAL USB_INTERFACE_DESCR ifDescr =
    {
    USB_INTERFACE_DESCR_LEN,	    /* bLength */
    USB_DESCR_INTERFACE,	    /* bDescriptorType */
    KBD_INTERFACE_NUM,		    /* bInterfaceNumber */
    KBD_INTERFACE_ALT_SETTING,	    /* bAlternateSetting */
    KBD_NUM_ENDPOINTS,		    /* bNumEndpoints */
    USB_CLASS_HID,		    /* bInterfaceClass */
    USB_SUBCLASS_HID_BOOT,	    /* bInterfaceSubClass */
    USB_PROTOCOL_HID_BOOT_KEYBOARD, /* bInterfaceProtocol */
    0				    /* iInterface */
    };

LOCAL USB_ENDPOINT_DESCR epDescr =
    {
    USB_ENDPOINT_DESCR_LEN,	    /* bLength */
    USB_DESCR_ENDPOINT, 	    /* bDescriptorType */
    KBD_INTERRUPT_ENDPOINT_NUM,     /* bEndpointAddress */
    USB_ATTR_INTERRUPT, 	    /* bmAttributes */
    sizeof (HID_KBD_BOOT_REPORT),   /* maxPacketSize */
    KBD_INTERRUPT_ENDPOINT_INTERVAL /* bInterval */
    };


/* functions */

/***************************************************************************
*
* usbTargKbdCallbackInfo - returns usbTargKbdLib callback table
*
* RETURNS: N/A
*/

VOID usbTargKbdCallbackInfo
    (
    pUSB_TARG_CALLBACK_TABLE *ppCallbacks,
    pVOID *pCallbackParam
    )

    {
    if (ppCallbacks != NULL)
	*ppCallbacks = &usbTargKbdCallbackTable;

    if (pCallbackParam != NULL)
	*pCallbackParam = NULL;
    }


/***************************************************************************
*
* reportErpCallback - called when report ERP terminates
*
* RETURNS: N/A
*/

LOCAL VOID reportErpCallback
    (
    pVOID p			/* pointer to ERP */
    )

    {
    reportInUse = FALSE;
    }


/***************************************************************************
*
* usbTargKbdInjectReport - injects a "boot report" into the interrupt pipe
*
* RETURNS: OK, or ERROR if unable to inject report
*/

STATUS usbTargKbdInjectReport
    (
    pHID_KBD_BOOT_REPORT pReport,
    UINT16 reportLen
    )

    {
    /* If the report pipe isn't enabled, return an error. */

    if (intPipeHandle == NULL)
	return ERROR;

    /* If a report is already queued, return an error. */

    if (reportInUse)
	return ERROR;

    reportInUse = TRUE;

    /* Copy the report and set up the transfer. */

    reportLen = min (sizeof (reportBfr), reportLen);
    memcpy (&reportBfr, pReport, reportLen);

    memset (&reportErp, 0, sizeof (reportErp));

    reportErp.erpLen = sizeof (reportErp);
    reportErp.userCallback = reportErpCallback;
    reportErp.bfrCount = 1;
    reportErp.bfrList [0].pid = USB_PID_IN;
    reportErp.bfrList [0].pBfr = (pUINT8) &reportBfr;
    reportErp.bfrList [0].bfrLen = reportLen;

    return usbTargTransfer (intPipeHandle, &reportErp);
    }


/***************************************************************************
*
* mngmtFunc - invoked by usbTargLib for connection management events
*
* RETURNS: OK if able to handle event, or ERROR if unable to handle event
*/

LOCAL STATUS mngmtFunc
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT16 mngmtCode			    /* management code */
    )

    {
    switch (mngmtCode)
	{
	case TCD_MNGMT_ATTACH:

	    /* Initialize global data */

	    channel = targChannel;
	    usbTargEndpointInfoGet (targChannel, &numEndpoints, &pEndpoints);

	    curConfiguration = 0;
	    curAlternateSetting = 0;
	    intPipeHandle = NULL;

	    /* Initialize control pipe maxPacketSize. */

	    devDescr.maxPacketSize0 = pEndpoints [0].maxPacketSize;
	    break;

	case TCD_MNGMT_DETACH:
	    break;

	case TCD_MNGMT_BUS_RESET:
	case TCD_MNGMT_VBUS_LOST:

	    /* revert to power-ON configuration */

	    configurationSet (param, targChannel, 0);
	    break;	    

	default:
	    break;
	}

    return OK;
    }


/***************************************************************************
*
* featureClear - invoked by usbTargLib for CLEAR_FEATURE request
*
* RETURNS: OK, or ERROR if unable to clear requested feature
*/

LOCAL STATUS featureClear
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT8 requestType,
    UINT16 feature,
    UINT16 index
    )

    {
    return OK;
    }


/***************************************************************************
*
* featureSet - invoked by usbTargLib for SET_FEATURE request
*
* RETURNS: OK, or ERROR if unable to set requested feature
*/

LOCAL STATUS featureSet
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT8 requestType,
    UINT16 feature,
    UINT16 index
    )

    {
    return OK;
    }


/***************************************************************************
*
* configurationGet - invoked by usbTargLib for GET_CONFIGURATION request
*
* RETURNS: OK, or ERROR if unable to return configuration setting
*/

LOCAL STATUS configurationGet
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    pUINT8 pConfiguration
    )

    {
    *pConfiguration = curConfiguration;
    return OK;
    }


/***************************************************************************
*
* configurationSet - invoked by usbTargLib for SET_CONFIGURATION request
*
* RETURNS: OK, or ERROR if unable to set specified configuration
*/

LOCAL STATUS configurationSet
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT8 configuration
    )

    {
    UINT16 i;


    if (configuration > KBD_CONFIG_VALUE)
	return ERROR;

    if ((curConfiguration = configuration) == KBD_CONFIG_VALUE)
	{
	/* Enable the interrupt status pipe if not already enabled. */

	if (intPipeHandle == NULL)
	    {
	    /* Find a suitable endpoint */

	    for (i = 2; i < numEndpoints; i++)
		if ((pEndpoints [i].flags & TCD_ENDPOINT_INT_OK) != 0 &&
		    (pEndpoints [i].flags & TCD_ENDPOINT_IN_OK) != 0)
		    break;

	    if (i == numEndpoints)
		return ERROR;

	    /* Create a pipe */

	    if (usbTargPipeCreate (targChannel, pEndpoints [i].endpointId, 0,
		KBD_INTERRUPT_ENDPOINT_NUM, configuration, KBD_INTERFACE_NUM, 
		USB_XFRTYPE_INTERRUPT, USB_DIR_IN, &intPipeHandle) != OK)
		return ERROR;

	    reportInUse = FALSE;
	    }
	}
    else
	{
	/* Disable the interrupt status pipe if it's enabled */

	if (intPipeHandle != NULL)
	    {
	    usbTargPipeDestroy (intPipeHandle);
	    intPipeHandle = NULL;
	    }
	}


    return OK;
    }


/***************************************************************************
*
* descriptorGet - invoked by usbTargLib for GET_DESCRIPTOR request
*
* RETURNS: OK, or ERROR if unable to return requested descriptor
*/

LOCAL STATUS descriptorGet
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT8 requestType,
    UINT8 descriptorType,
    UINT8 descriptorIndex,
    UINT16 languageId,
    UINT16 length,
    pUINT8 pBfr,
    pUINT16 pActLen
    )

    {
    UINT8 bfr [USB_MAX_DESCR_LEN];
    UINT16 actLen;


    /* Determine type of descriptor being requested. */

    if (requestType == (USB_RT_DEV_TO_HOST | USB_RT_STANDARD | USB_RT_DEVICE))
	{
	switch (descriptorType)
	    {
	    case USB_DESCR_DEVICE:  
		usbDescrCopy (pBfr, &devDescr, length, pActLen);
		break;

	    case USB_DESCR_CONFIGURATION:
		memcpy (bfr, &configDescr, USB_CONFIG_DESCR_LEN);
		memcpy (&bfr [USB_CONFIG_DESCR_LEN], &ifDescr,
		    USB_INTERFACE_DESCR_LEN);
		memcpy (&bfr [USB_CONFIG_DESCR_LEN + USB_INTERFACE_DESCR_LEN],
		    &epDescr, USB_ENDPOINT_DESCR_LEN);

		actLen = min (length, USB_CONFIG_DESCR_LEN +
		    USB_INTERFACE_DESCR_LEN + USB_ENDPOINT_DESCR_LEN);

		memcpy (pBfr, bfr, actLen);
		*pActLen = actLen;
		break;

	    case USB_DESCR_INTERFACE:
		usbDescrCopy (pBfr, &ifDescr, length, pActLen);
		break;

	    case USB_DESCR_ENDPOINT:
		usbDescrCopy (pBfr, &epDescr, length, pActLen);
		break;
		    
	    case USB_DESCR_STRING:
		switch (descriptorIndex)
		    {
		    case 0: /* language descriptor */
			usbDescrCopy (pBfr, &langDescr, length, pActLen);
			break;

		    case ID_STR_MFG:
			usbDescrStrCopy (pBfr, pStrMfg, length, pActLen);
			break;

		    case ID_STR_PROD:
			usbDescrStrCopy (pBfr, pStrProd, length, pActLen);
			break;

		    default:
			return ERROR;
		    }
		break;

	    default:
		return ERROR;
	    }
	}
    else
	{
	return ERROR;
	}

    return OK;
    }


/***************************************************************************
*
* interfaceGet - invoked by usbTargLib for GET_INTERFACE request
*
* RETURNS: OK, or ERROR if unable to return interface setting
*/

LOCAL STATUS interfaceGet
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT16 interfaceIndex,
    pUINT8 pAlternateSetting
    )

    {
    *pAlternateSetting = curAlternateSetting;
    return OK;
    }


/***************************************************************************
*
* interfaceSet - invoked by usbTargLib for SET_INTERFACE request
*
* RETURNS: OK, or ERROR if unable to set specified interface
*/

LOCAL STATUS interfaceSet
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT16 interfaceIndex,
    UINT8 alternateSetting
    )

    {
    curAlternateSetting = alternateSetting;
    return OK;
    }


/***************************************************************************
*
* vendorSpecific - invoked by usbTargLib for VENDOR_SPECIFIC request
*
* RETURNS: OK, or ERROR if unable to process vendor-specific request
*/

LOCAL STATUS vendorSpecific
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT8 requestType,
    UINT8 request,
    UINT16 value,
    UINT16 index,
    UINT16 length
    )

    {
    if (requestType == (USB_RT_HOST_TO_DEV | USB_RT_CLASS | USB_RT_INTERFACE))
	{
	switch (request)
	    {
	    case USB_REQ_HID_SET_PROTOCOL:
	    case USB_REQ_HID_SET_IDLE:

		/* This emulator simply acknowledges these HID requests...no
		 * processing is required. 
		 */

		usbTargControlResponseSend (targChannel, 0, NULL);
		return OK;

	    case USB_REQ_HID_SET_REPORT:

		/* This eumulator does not support LED settings.  However, this
		 * does give us an opportunity to test usbTargPipeStatusSet().
		 */

		usbTargPipeStatusSet (targChannel, NULL, TCD_ENDPOINT_STALL);
		return ERROR;

	    default:
		break;
	    }
	}

    return ERROR;
    }




/* End of file. */

