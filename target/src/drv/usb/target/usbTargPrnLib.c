/* usbTargPrnLib.c - USB printer target exerciser/demonstration */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01b,23nov99,rcb  Change #include ../xxx references to lower case.
01a,30aug99,rcb  First.
*/

/*
DESCRIPTION

This module contains code to exercise the usbTargLib by emulating a rudimentary
USB printer.  This module will generally be invoked by usbTool or a similar USB
test/exerciser application.

It is the caller's responsibility to initialize usbTargLib and attach a USB TCD 
to it.	When attaching a TCD to usbTargLib, the caller must pass a pointer to a
table of callbacks required by usbTargLib.  The address of this table and the
"callback parameter" required by these callbacks may be obtained by calling
usbTargPrnCallbackInfo().  It is not necessary to initialize the usbTartPrnLib or
to shut it down.  It performs all of its operations in response to callbacks from
usbTargLib.

This module also exports a function, usbTargPrnBfrInfo(), which allows a test
application to retrieve the current status of the bulk output buffer.
*/


/* includes */

#include "usb/usbPlatform.h"

#include "string.h"

#include "usb/usb.h"
#include "usb/usbPrinter.h"		    /* USB PRINTER definitions */
#include "usb/usbDescrCopyLib.h"	    /* USB utility functions */
#include "usb/target/usbTargLib.h"	    /* USB target functions */
#include "drv/usb/target/usbTargPrnLib.h"   /* our API */




/* defines */

/* Define USE_DMA_ENDPOINT to direct printer data to the Philips PDIUSBD12
 * "main" endpoint (#2) which uses DMA.  Un-define USE_DMA_ENDPOINT to direct
 * printer data to the generic endpoint (#1) which uses programmed-IO.
 */

#define USE_DMA_ENDPOINT


/* string identifiers */

#define UNICODE_ENGLISH     0x409

#define ID_STR_MFG	    1
#define ID_STR_MFG_VAL	    "Wind River Systems"

#define ID_STR_PROD	    2
#define ID_STR_PROD_VAL     "USB printer emulator"


/* printer "capabilities" string */

#define PRN_CAPS    "mfg:WRS;model=emulator;"


/* printer device */

#define PRN_USB_VERSION 		0x0100
#define PRN_NUM_CONFIG			1


/* printer configuration */

#define PRN_CONFIG_VALUE		1
#define PRN_NUM_INTERFACES		1


/* printer interface */

#define PRN_INTERFACE_NUM		0
#define PRN_INTERFACE_ALT_SETTING	1
#define PRN_NUM_ENDPOINTS		1


/* printer BULK OUT endpoint */

#ifdef	USE_DMA_ENDPOINT
#define PRN_BULK_OUT_ENDPOINT_ID	4	/* PDIUSBD12 "main out" endpoint */
#define PRN_BULK_OUT_ENDPOINT_NUM	0x02
#else
#define PRN_BULK_OUT_ENDPOINT_ID	2	/* PDIUSBD12 "generic out" endpoint */
#define PRN_BULK_OUT_ENDPOINT_NUM	0x01
#endif

#define BULK_BFR_LEN			4096


/* typedefs */


/* forward declarations */

LOCAL STATUS mngmtFunc
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT16 mngmtCode
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

LOCAL USB_TARG_CALLBACK_TABLE usbTargPrnCallbackTable =
    {
    mngmtFunc,		/* mngmtFunc */
    NULL,		/* featureClear */
    NULL,		/* featureSet */
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
LOCAL USB_TARG_PIPE bulkPipeHandle;

LOCAL USB_ERP bulkErp;
LOCAL BOOL bulkInUse;
LOCAL pUINT8 bulkBfr;
LOCAL BOOL bulkBfrValid;

LOCAL char capsString [] = PRN_CAPS;
LOCAL USB_PRINTER_PORT_STATUS portStatus = 
    {USB_PRN_STS_SELECTED | USB_PRN_STS_NOT_ERROR};

LOCAL USB_LANGUAGE_DESCR langDescr =
    {sizeof (USB_LANGUAGE_DESCR), USB_DESCR_STRING, 
	{TO_LITTLEW (UNICODE_ENGLISH)}};

LOCAL char *pStrMfg = ID_STR_MFG_VAL;
LOCAL char *pStrProd = ID_STR_PROD_VAL;


LOCAL USB_DEVICE_DESCR devDescr =
    {
    USB_DEVICE_DESCR_LEN,	    /* bLength */
    USB_DESCR_DEVICE,		    /* bDescriptorType */
    TO_LITTLEW (PRN_USB_VERSION),   /* bcdUsb */
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
    PRN_NUM_CONFIG		    /* bNumConfigurations */
    };


LOCAL USB_CONFIG_DESCR configDescr =
    {
    USB_CONFIG_DESCR_LEN,	    /* bLength */
    USB_DESCR_CONFIGURATION,	    /* bDescriptorType */
    TO_LITTLEW (USB_CONFIG_DESCR_LEN + USB_INTERFACE_DESCR_LEN + 
    USB_ENDPOINT_DESCR_LEN),
				    /* wTotalLength */
    PRN_NUM_INTERFACES, 	    /* bNumInterfaces */
    PRN_CONFIG_VALUE,		    /* bConfigurationValue */
    0,				    /* iConfiguration */
    USB_ATTR_SELF_POWERED,	    /* bmAttributes */
    0				    /* MaxPower */
    };

LOCAL USB_INTERFACE_DESCR ifDescr =
    {
    USB_INTERFACE_DESCR_LEN,	    /* bLength */
    USB_DESCR_INTERFACE,	    /* bDescriptorType */
    PRN_INTERFACE_NUM,		    /* bInterfaceNumber */
    PRN_INTERFACE_ALT_SETTING,	    /* bAlternateSetting */
    PRN_NUM_ENDPOINTS,		    /* bNumEndpoints */
    USB_CLASS_PRINTER,		    /* bInterfaceClass */
    USB_SUBCLASS_PRINTER,	    /* bInterfaceSubClass */
    USB_PROTOCOL_PRINTER_UNIDIR,    /* bInterfaceProtocol */
    0				    /* iInterface */
    };

LOCAL USB_ENDPOINT_DESCR epDescr =
    {
    USB_ENDPOINT_DESCR_LEN,	    /* bLength */
    USB_DESCR_ENDPOINT, 	    /* bDescriptorType */
    PRN_BULK_OUT_ENDPOINT_NUM,	    /* bEndpointAddress */
    USB_ATTR_BULK,		    /* bmAttributes */
    USB_MIN_CTRL_PACKET_SIZE,	    /* maxPacketSize */
    0				    /* bInterval */
    };


/* functions */

/***************************************************************************
*
* bulkErpCallback - called when report ERP terminates
*
* RETURNS: N/A
*/

LOCAL VOID bulkErpCallback
    (
    pVOID p			/* pointer to ERP */
    )

    {
    bulkInUse = FALSE;
    bulkBfrValid = TRUE;
    }


/***************************************************************************
*
* initBulkOutErp - listen for output printer data
*
* RETURNS: OK, or ERROR if unable to submit ERP.
*/

LOCAL STATUS initBulkOutErp (void)
    {
    if (bulkBfr == NULL)
	return ERROR;

    if (bulkInUse)
	return OK;

    /* Initialize bulk ERP */

    memset (&bulkErp, 0, sizeof (bulkErp));

    bulkErp.erpLen = sizeof (bulkErp);
    bulkErp.userCallback = bulkErpCallback;
    bulkErp.bfrCount = 1;

    bulkErp.bfrList [0].pid = USB_PID_OUT;
    bulkErp.bfrList [0].pBfr = bulkBfr;
    bulkErp.bfrList [0].bfrLen = BULK_BFR_LEN;

    bulkInUse = TRUE;
    bulkBfrValid = FALSE;

    if (usbTargTransfer (bulkPipeHandle, &bulkErp) != OK)
	{
	bulkInUse = FALSE;
	return ERROR;
	}

    return OK;
    }


/***************************************************************************
*
* usbTargPrnCallbackInfo - returns usbTargPrnLib callback table
*
* RETURNS: N/A
*/

VOID usbTargPrnCallbackInfo
    (
    pUSB_TARG_CALLBACK_TABLE *ppCallbacks,
    pVOID *pCallbackParam
    )

    {
    if (ppCallbacks != NULL)
	*ppCallbacks = &usbTargPrnCallbackTable;

    if (pCallbackParam != NULL)
	*pCallbackParam = NULL;
    }


/***************************************************************************
*
* usbTargPrnDataInfo - returns buffer status/info
*
* RETURNS: OK if buffer has valid data, else ERROR
*/

STATUS usbTargPrnDataInfo
    (
    pUINT8 *ppBfr,
    pUINT16 pActLen
    )

    {
    if (!bulkBfrValid)
	return ERROR;

    if (ppBfr != NULL)
	*ppBfr = bulkBfr;

    if (pActLen != NULL)
	*pActLen = bulkErp.bfrList [0].actLen;

    return OK;
    }


/***************************************************************************
*
* usbTargPrnDataRestart - restarts listening ERP
*
* RETURNS: OK, or ERROR if unable to re-initiate ERP
*/

STATUS usbTargPrnDataRestart (void)
    {
    return initBulkOutErp ();
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
	    bulkPipeHandle = NULL;

	    /* Initialize control pipe maxPacketSize. */

	    devDescr.maxPacketSize0 = pEndpoints [0].maxPacketSize;

	    /* Initialize bulk endpoint max packet size. */

	    epDescr.maxPacketSize = 
		pEndpoints [PRN_BULK_OUT_ENDPOINT_ID].bulkOutMaxPacketSize;

	    /* Allocate buffer */

	    if ((bulkBfr = OSS_MALLOC (BULK_BFR_LEN)) == NULL)
		return ERROR;

	    break;

	case TCD_MNGMT_DETACH:

	    /* De-allocate buffer */

	    if (bulkBfr != NULL)
		{
		OSS_FREE (bulkBfr);
		bulkBfr = NULL;
		}

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
    if (configuration > PRN_CONFIG_VALUE)
	return ERROR;

    if ((curConfiguration = configuration) == PRN_CONFIG_VALUE)
	{
	/* Enable the interrupt status pipe if not already enabled. */

	if (bulkPipeHandle == NULL)
	    {
	    /* Create a pipe */

	    if (usbTargPipeCreate (targChannel, PRN_BULK_OUT_ENDPOINT_ID, 0,
		PRN_BULK_OUT_ENDPOINT_NUM, configuration, PRN_INTERFACE_NUM, 
		USB_XFRTYPE_BULK, USB_DIR_OUT, &bulkPipeHandle) != OK)
		return ERROR;

	    /* Initialize ERP to listen for data */

	    initBulkOutErp ();
	    }
	}
    else
	{
	/* Disable the interrupt status pipe if it's enabled */

	if (bulkPipeHandle != NULL)
	    {
	    usbTargPipeDestroy (bulkPipeHandle);
	    bulkPipeHandle = NULL;
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
    UINT8 bfr [USB_MAX_DESCR_LEN];
    pUSB_PRINTER_CAPABILITIES pCaps = (pUSB_PRINTER_CAPABILITIES) bfr;
    UINT16 capsLen = strlen (capsString) + 2;


    if (requestType == (USB_RT_DEV_TO_HOST | USB_RT_CLASS | USB_RT_INTERFACE))
	{
	switch (request)
	    {
	    case USB_REQ_PRN_GET_DEVICE_ID:

		/* Send the IEEE-1284-style "device id" string. */

		pCaps->length = TO_BIGW (capsLen);
		memcpy (pCaps->caps, capsString, strlen (capsString));

		usbTargControlResponseSend (targChannel, capsLen, (pUINT8) pCaps);
		return OK;

	    case USB_REQ_PRN_GET_PORT_STATUS:

		/* This emulator simply acknowledges these HID requests...no
		 * processing is required. 
		 */

		usbTargControlResponseSend (targChannel, 
		    sizeof (portStatus), (pUINT8) &portStatus);

		return OK;

	    default:
		break;
	    }
	}

    else if (requestType == (USB_RT_HOST_TO_DEV | USB_RT_CLASS | USB_RT_OTHER))
	{
	switch (request)
	    {
	    case USB_REQ_PRN_SOFT_RESET:

		/* We accept the SOFT_RESET and return OK. */

		return OK;

	    default:
		break;
	    }
	}

    return ERROR;
    }




/* End of file. */

