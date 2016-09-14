/* usbTargPhilipsD12EvalLib.c - emulates Philips PDIUSBD12 eval firmware */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01d,19mar00,rcb  Add handling for CLEAR_FEATURE...Newer Philips test
		 program now invokes this function.
01c,23nov99,rcb  Change #include ../xxx references to lower case.
01b,12nov99,rcb  Shorted path names...affects "#include" directives.
01a,03sep99,rcb  First.
*/

/*
DESCRIPTION

The Philips PDIUSBD12 evaluation kit is shipped with sample firmware.  The
"firmware" is a program which runs under MS-DOS and responds to certain standard
and "vendor specific" USB requests.  A custom Windows application - also provided
by Philips - can be used to exercise this "firmware".

In order to provide a different example of working target behavoir, this module
emulates the behavoir of the Philips evaluation firmware, allowing this target
module to work with the Philips Windows-based exerciser program.  The "scan",
"print", and "loopback" portions of that test work correctly.  The "LED buttons"
supported by the Philips firmware are not emulated, and do nothing.

It is the caller's responsibility to initialize usbTargLib and attach a USB TCD 
to it.	When attaching a TCD to usbTargLib, the caller must pass a pointer to a
table of callbacks required by usbTargLib.  The address of this table and the
"callback parameter" required by these callbacks may be obtained by calling
usbTargPhilipsD12EvalCallbackInfo().  It is not necessary to initialize the 
usbTartPhilipsD12EvalLib or to shut it down.  It performs all of its operations
in response to callbacks from usbTargLib.
*/


/* includes */

#include "usb/usbPlatform.h"

#include "string.h"

#include "usb/usb.h"
#include "usb/usbDescrCopyLib.h"			    /* USB utility functions */
#include "usb/target/usbTargLib.h"		/* USB target functions */

#include "drv/usb/target/usbPdiusbd12.h"	/* Philips definitions */
#include "drv/usb/target/usbTargPhilipsD12EvalLib.h"	/* our API */




/* defines */

#define BULK_BFR_LEN	    64000		    /* size used by Philips */


/* string identifiers */

#define UNICODE_ENGLISH     0x409

#define ID_STR_MFG	    1
#define ID_STR_MFG_VAL	    "Wind River Systems"

#define ID_STR_PROD	    2
#define ID_STR_PROD_VAL     "Philips firmware emulator"


/* descriptor constants */

#define CONFIG_VAL	1

#define NUM_ENDPOINTS	4

#define CONFIG_DESCRIPTOR_LENGTH    \
    USB_CONFIG_DESCR_LEN + USB_INTERFACE_DESCR_LEN + \
    (NUM_ENDPOINTS * USB_ENDPOINT_DESCR_LEN)


/* vendor specific commands */

#define READ_WRITE_REGISTER	12


/* wIndex parameters to the vendor specific READ_WRITE_REGISTER cmd */

#define SETUP_DMA_REQUEST	0x0471
#define GET_FIRMWARE_VERSION	0x0472
#define GET_SET_TWAIN_REQUEST	0x0473


/* typedefs */

/* IO_REQUEST is the parameter to the SETUP_DMA_REQUEST vendor-specific
 * command.
 */

#define IO_REQUEST_LEN	6

typedef struct io_request {
    UINT8 bytes [IO_REQUEST_LEN];
} IO_REQUEST, *pIO_REQUEST;


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


LOCAL STATUS initBulkErp 
    (
    USB_TARG_PIPE pipeHandle,
    UINT16 pid,
    UINT16 bfrOffset,
    UINT16 length
    );


/* locals */

LOCAL USB_TARG_CALLBACK_TABLE usbTargPrnCallbackTable =
    {
    mngmtFunc,		/* mngmtFunc */
    featureClear,	/* featureClear */
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

LOCAL UINT16 curConfiguration;
LOCAL UINT16 curAlternateSetting;

LOCAL USB_TARG_PIPE bulkOutPipeHandle;
LOCAL USB_TARG_PIPE bulkInPipeHandle;

LOCAL USB_ERP payloadErp;
LOCAL IO_REQUEST ioRequest;

LOCAL USB_ERP bulkErp;
LOCAL pUINT8 bulkBfr;
LOCAL BOOL bulkInUse;

LOCAL UINT32 bulkCanceled;
LOCAL UINT32 bulkErrors;

LOCAL UINT8 fwVersion = 0x01;	    /* version for PDIUSBD12 PC kit */


/* descriptors */

LOCAL USB_LANGUAGE_DESCR langDescr =
    {sizeof (USB_LANGUAGE_DESCR), USB_DESCR_STRING, 
	{TO_LITTLEW (UNICODE_ENGLISH)}};

LOCAL char *pStrMfg = ID_STR_MFG_VAL;
LOCAL char *pStrProd = ID_STR_PROD_VAL;


LOCAL USB_DEVICE_DESCR devDescr =
    {
    USB_DEVICE_DESCR_LEN,	    /* bLength */
    USB_DESCR_DEVICE,		    /* bDescriptorType */
    TO_LITTLEW (0x0100),	    /* bcdUsb */
    0xdc, /* test class */	    /* bDeviceClass */
    0,				    /* bDeviceSubclass */
    0,				    /* bDeviceProtocol */
    USB_MIN_CTRL_PACKET_SIZE,	    /* maxPacketSize0 */
    TO_LITTLEW (0x0471),	    /* idVendor */
    TO_LITTLEW (0x0222),	    /* idProduct */
    TO_LITTLEW (0x0100),	    /* bcdDevice */
    ID_STR_MFG, 		    /* iManufacturer */
    ID_STR_PROD,		    /* iProduct */
    0,				    /* iSerialNumber */
    1				    /* bNumConfigurations */
    };

LOCAL USB_CONFIG_DESCR configDescr =
    {
    USB_CONFIG_DESCR_LEN,	    /* bLength */
    USB_DESCR_CONFIGURATION,	    /* bDescriptorType */
    TO_LITTLEW (CONFIG_DESCRIPTOR_LENGTH), /* wTotalLength */
    1,				    /* bNumInterfaces */
    CONFIG_VAL, 		    /* bConfigurationValue */
    0,				    /* iConfiguration */
    USB_ATTR_SELF_POWERED,	    /* bmAttributes */
    0				    /* MaxPower */
    };

LOCAL USB_INTERFACE_DESCR ifDescr =
    {
    USB_INTERFACE_DESCR_LEN,	    /* bLength */
    USB_DESCR_INTERFACE,	    /* bDescriptorType */
    0,				    /* bInterfaceNumber */
    0,				    /* bAlternateSetting */
    NUM_ENDPOINTS,		    /* bNumEndpoints */
    0xdc, /* test class */	    /* bInterfaceClass */
    0xa0,			    /* bInterfaceSubClass */
    0xb0,			    /* bInterfaceProtocol */
    0				    /* iInterface */
    };

LOCAL USB_ENDPOINT_DESCR epDescr1 =
    {
    USB_ENDPOINT_DESCR_LEN,	    /* bLength */
    USB_DESCR_ENDPOINT, 	    /* bDescriptorType */
    0x81,			    /* bEndpointAddress */
    USB_ATTR_INTERRUPT, 	    /* bmAttributes */
    USB_MIN_CTRL_PACKET_SIZE,	    /* maxPacketSize */
    10				    /* bInterval */
    };

LOCAL USB_ENDPOINT_DESCR epDescr2 =
    {
    USB_ENDPOINT_DESCR_LEN,	    /* bLength */
    USB_DESCR_ENDPOINT, 	    /* bDescriptorType */
    0x01,			    /* bEndpointAddress */
    USB_ATTR_INTERRUPT, 	    /* bmAttributes */
    USB_MIN_CTRL_PACKET_SIZE,	    /* maxPacketSize */
    10				    /* bInterval */
    };

LOCAL USB_ENDPOINT_DESCR epDescr3 =
    {
    USB_ENDPOINT_DESCR_LEN,	    /* bLength */
    USB_DESCR_ENDPOINT, 	    /* bDescriptorType */
    0x82,			    /* bEndpointAddress */
    USB_ATTR_BULK,		    /* bmAttributes */
    USB_MIN_CTRL_PACKET_SIZE,	    /* maxPacketSize */
    0				    /* bInterval */
    };

LOCAL USB_ENDPOINT_DESCR epDescr4 =
    {
    USB_ENDPOINT_DESCR_LEN,	    /* bLength */
    USB_DESCR_ENDPOINT, 	    /* bDescriptorType */
    0x02,			    /* bEndpointAddress */
    USB_ATTR_BULK,		    /* bmAttributes */
    USB_MIN_CTRL_PACKET_SIZE,	    /* maxPacketSize */
    0				    /* bInterval */
    };


/* functions */

/***************************************************************************
*
* usbTargPhilipsD12EvalCallbackInfo - returns callback table
*
* RETURNS: N/A
*/

VOID usbTargPhilipsD12EvalCallbackInfo
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
	    bulkInPipeHandle = NULL;
	    bulkOutPipeHandle = NULL;
	    bulkCanceled = 0;
	    bulkErrors = 0;

	    /* Initialize control pipe maxPacketSize. */

	    devDescr.maxPacketSize0 = pEndpoints [0].maxPacketSize;

	    /* Initialize bulk endpoint max packet size. */

	    epDescr1.maxPacketSize = 
		pEndpoints [D12_ENDPOINT_1_IN].intInMaxPacketSize;

	    epDescr2.maxPacketSize =
		pEndpoints [D12_ENDPOINT_1_OUT].intOutMaxPacketSize;

	    epDescr3.maxPacketSize =
		pEndpoints [D12_ENDPOINT_2_IN].bulkInMaxPacketSize;

	    epDescr4.maxPacketSize =
		pEndpoints [D12_ENDPOINT_2_OUT].bulkOutMaxPacketSize;
	    

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
    if (configuration > CONFIG_VAL)
	return ERROR;

    if ((curConfiguration = configuration) == CONFIG_VAL)
	{
	/* Enable the pipes if not already enabled. */

	if (bulkOutPipeHandle == NULL)
	    {
	    /* Create a pipe */

	    if (usbTargPipeCreate (targChannel, D12_ENDPOINT_2_OUT, 0,
		2, configuration, 0, USB_XFRTYPE_BULK, USB_DIR_OUT, 
		&bulkOutPipeHandle) != OK)
		return ERROR;
	    }


	if (bulkInPipeHandle == NULL)
	    {
	    if (usbTargPipeCreate (targChannel, D12_ENDPOINT_2_IN, 0,
		2, configuration, 0, USB_XFRTYPE_BULK, USB_DIR_IN,
		&bulkInPipeHandle) != OK)
		return ERROR;
	    }
	}
    else
	{
	/* Disable the pipes */

	if (bulkOutPipeHandle != NULL)
	    {
	    usbTargPipeDestroy (bulkOutPipeHandle);
	    bulkOutPipeHandle = NULL;
	    }

	if (bulkInPipeHandle != NULL)
	    {
	    usbTargPipeDestroy (bulkInPipeHandle);
	    bulkInPipeHandle = NULL;
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
		    &epDescr1, USB_ENDPOINT_DESCR_LEN);

		memcpy (&bfr [USB_CONFIG_DESCR_LEN + USB_INTERFACE_DESCR_LEN +
		    USB_ENDPOINT_DESCR_LEN], &epDescr2, USB_ENDPOINT_DESCR_LEN);
		memcpy (&bfr [USB_CONFIG_DESCR_LEN + USB_INTERFACE_DESCR_LEN +
		    USB_ENDPOINT_DESCR_LEN * 2], &epDescr3, USB_ENDPOINT_DESCR_LEN);
		memcpy (&bfr [USB_CONFIG_DESCR_LEN + USB_INTERFACE_DESCR_LEN +
		    USB_ENDPOINT_DESCR_LEN * 3], &epDescr4, USB_ENDPOINT_DESCR_LEN);

		actLen = min (length, CONFIG_DESCRIPTOR_LENGTH);

		memcpy (pBfr, bfr, actLen);
		*pActLen = actLen;
		break;

	    case USB_DESCR_INTERFACE:
		usbDescrCopy (pBfr, &ifDescr, length, pActLen);
		break;

	    case USB_DESCR_ENDPOINT:
		switch (descriptorIndex)
		    {
		    case 0: usbDescrCopy (pBfr, &epDescr1, length, pActLen); break;
		    case 1: usbDescrCopy (pBfr, &epDescr2, length, pActLen); break;
		    case 2: usbDescrCopy (pBfr, &epDescr3, length, pActLen); break;
		    case 3: usbDescrCopy (pBfr, &epDescr4, length, pActLen); break;
		    }
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
* bulkErpCallback - called when report ERP terminates
*
* RETURNS: N/A
*/

LOCAL VOID bulkErpCallback
    (
    pVOID p			/* pointer to ERP */
    )

    {
    pUSB_ERP pErp = (pUSB_ERP) p;

    if (pErp->result == S_usbTcdLib_ERP_CANCELED)
	bulkCanceled++;
    else if (pErp->result != OK)
	bulkErrors++;

    bulkInUse = FALSE;
    }


/***************************************************************************
*
* initBulkErp - listen for output printer data
*
* RETURNS: OK, or ERROR if unable to submit ERP.
*/

LOCAL STATUS initBulkErp 
    (
    USB_TARG_PIPE pipeHandle,
    UINT16 pid,
    UINT16 bfrOffset,
    UINT16 length
    )

    {
    if (bulkBfr == NULL)
	return ERROR;

    if (bulkInUse)
	return ERROR;

    /* Initialize bulk ERP */

    memset (&bulkErp, 0, sizeof (bulkErp));

    bulkErp.erpLen = sizeof (bulkErp);
    bulkErp.userCallback = bulkErpCallback;
    bulkErp.bfrCount = 1;

    bulkErp.bfrList [0].pid = pid;
    bulkErp.bfrList [0].pBfr = bulkBfr + bfrOffset;
    bulkErp.bfrList [0].bfrLen = length;

    bulkInUse = TRUE;

    if (usbTargTransfer (pipeHandle, &bulkErp) != OK)
	{
	bulkInUse = FALSE;
	return ERROR;
	}

    return OK;
    }


/***************************************************************************
*
* setupDmaRequestCallback - called when payload received
*
* RETURNS: N/A
*/

LOCAL VOID setupDmaRequestCallback
    (
    pVOID p
    )

    {
    pUSB_ERP pErp = (pUSB_ERP) p;
    UINT16 bfrOffset;
    UINT16 length;
    UINT8 command;

    if (pErp->result == OK)
	{
	/* Examine the IO_REQUEST passed in the payload. */

	bfrOffset = ioRequest.bytes [0] | (ioRequest.bytes [1] << 8);
	length = ioRequest.bytes [3] | (ioRequest.bytes [4] << 8);
	command = ioRequest.bytes [5];

	if ((command & 0x1) != 0)
	    {
	    /* read request */

	    initBulkErp (bulkInPipeHandle, USB_PID_IN, bfrOffset, length);
	    }
	else
	    {
	    /* write request */

	    initBulkErp (bulkOutPipeHandle, USB_PID_OUT, bfrOffset, length);
	    }


	/* Send an acknowledgement of the command. */

	usbTargControlResponseSend (channel, NULL, 0);
	}
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
    if ((requestType & (USB_RT_CATEGORY_MASK | USB_RT_RECIPIENT_MASK)) 
	== (USB_RT_VENDOR | USB_RT_DEVICE))
	{
	switch (request)
	    {
	    case READ_WRITE_REGISTER:

		if ((requestType & USB_RT_DIRECTION_MASK) != 0 &&
		    value == 0 && length == sizeof (fwVersion) &&
		    index == GET_FIRMWARE_VERSION)
		    {
		    /* Send firmware version back to host. */

		    return usbTargControlResponseSend (targChannel,
			sizeof (fwVersion), &fwVersion);
		    }

		else if ((requestType & USB_RT_DIRECTION_MASK) == 0 &&
		    value == 0 && length == IO_REQUEST_LEN && 
		    index == SETUP_DMA_REQUEST)
		    {
		    /* Fetch the payload for the command. */

		    memset (&payloadErp, 0, sizeof (payloadErp));
		    payloadErp.erpLen = sizeof (payloadErp);
		    payloadErp.userCallback = setupDmaRequestCallback;
		    payloadErp.dataToggle = USB_DATA1;
		    payloadErp.bfrCount = 1;
		    payloadErp.bfrList [0].pid = USB_PID_OUT;
		    payloadErp.bfrList [0].pBfr = (pUINT8) &ioRequest;
		    payloadErp.bfrList [0].bfrLen = IO_REQUEST_LEN;

		    return usbTargControlPayloadRcv (targChannel,
			&payloadErp);
		    }

		break;

	    default:
		break;
	    }
	}

    return ERROR;
    }




/* End of file. */

