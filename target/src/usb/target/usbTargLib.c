/* usbTargLib.c - USB Target Library */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01d,12apr00,wef  Fixed uninitialized variable warning: direction2 in usbTargPipeCreate
01c,23nov99,rcb  Change #include ../xxx references to lower case.
01b,24sep99,rcb  Change packet count calculation in usbTargErpCallback()
		 to handle case where a single 0-length packet is transferred.
01a,05aug99,rcb  First.
*/

/*
DESCRIPTION

This module implements the hardware-independent USB target API.

usbTargLib can manage one or more USB TCDs (Target Controller Drivers).
usbTargLib provides hardware-independent target services and management
while the USB TCDs provide the hardware-dependent USB interface.

usbTargLib must be initialized by calling usbTargInitialize().	Before
operation can begin, at least one TCD must be attached to usbTargLib
by calling usbTargTcdAttach().	In response to a successful TCD
attachment, usbTargLib returns a USB_TARG_CHANNEL handle to the caller.
This handle must be used in all subsequent calls to usbTargLib to
identify a given target channel.

USB devices (targets) almost never initiate activity on the USB (the
exception being RESUME signalling).  So, as part of the call to 
usbTargTcdAttach(), the caller must provide a pointer to a 
USB_TARG_CALLBACK_TABLE structure.  This table contains a collection of
callback function pointers initialized by the caller prior to invoking
the usbTargTcdAttach() function.  Through these callbacks, usbTargLib
notifies the calling application of various USB events and requests from
the host.  The calling application is not required to provide callback
functions for all possible events.  The callback pointers for which
the calling application has no interest should be NULL.

usbTargLib automatically establishes the default control pipe for
each attached target channel and monitors all control pipe requests.
If the calling application provides a non-NULL callback for the
corresponding request, usbTargLib invokes the function.  If the callback
function chooses to handle the control pipe request itself, then it
returns OK, and usbTargLib generally performs no further automatic
processing.  If the callback instead returns FALSE, then usbTargLib
handles the control pipe request automatically using a default behavior.

Generally, requests through the control pipe will result in the need for
the target to establish one or more additional pipes to the host.  Each
pipe is assigned to a particular endpoint on the device.  The calling
application then submits USB_ERPs (endpoint request packets) which cause
data transfers to be performed on specific pipes.
*/


/* includes */

#include "usb/usbPlatform.h"		/* platform defintions */

#include "string.h"

#include "usb/ossLib.h" 		/* o/s services */
#include "usb/usb.h"			/* general USB definitions */
#include "usb/usbListLib.h"		/* linked list functions */
#include "usb/usbHandleLib.h"		/* handle functions */
#include "usb/target/usbTcdLib.h"	/* interface to USB TCD */
#include "usb/target/usbTargLib.h"	/* our API */


/* defines */

#define TARG_VERSION	0x0001			    /* TARG version in BCD */
#define TARG_MFG	"Wind River Systems, Inc."  /* TARG Mfg */

#define TARG_TCD_SIG	((UINT32) 0xaaaa00cd)
#define TARG_PIPE_SIG	((UINT32) 0xaaaa000e)


/* Constants used by resetDataToggle(). */

#define ANY_CONFIGURATION	0xffff
#define ANY_INTERFACE		0xffff
#define ANY_ENDPOINT		0xffff

#define NO_CONFIGURATION	0xfffe
#define NO_INTERFACE		0xfffe
#define NO_ENDPOINT		0xfffe


/* typedefs */

/* TARG_TCD */

typedef struct targ_tcd
    {
    LINK tcdLink;			/* linked list of TCDs */

    USB_TARG_CHANNEL targChannel;	/* handle allocated for channel */

    UINT16 deviceAddress;		/* current device address */

    pUSB_TARG_CALLBACK_TABLE pCallbacks;/* table of application callbacks */
    pVOID callbackParam;		/* caller-defined callback param */

    TCD_NEXUS tcdNexus; 		/* our TCD "nexus" */

    UINT16 speed;			/* target speed: USB_SPEED_xxxx */
    UINT16 numEndpoints;		/* number of endpoints */
    pUSB_TARG_ENDPOINT_INFO pEndpoints; /* ptr to array of endpoints */

    LIST_HEAD pipes;			/* list of pipes on this target */

    USB_TARG_PIPE controlPipe;		/* Pipe opened for control channel */

    USB_ERP setupErp;			/* ERP to receive control pipe setup */
    UINT8 setupBfr [sizeof (USB_SETUP)]; /* bfr for control SETUP packet */
    BOOL setupErpPending;		/* TRUE when ERP is pending */

    USB_ERP dataErp;			/* ERP to send/receive control data */
    UINT8 dataBfr [USB_MAX_DESCR_LEN];	/* bfr for control IN/OUT packet */
    BOOL dataErpPending;		/* TRUE when ERP is pending */

    USB_ERP statusErp;			/* ERP to manage status phase packet */
    BOOL statusErpPending;		/* TRUE when ERP is pending */

    } TARG_TCD, *pTARG_TCD;


/* TARG_PIPE */

typedef struct targ_pipe
    {
    LINK pipeLink;			/* linked list of pipes */

    USB_TARG_PIPE pipeHandle;		/* handle allocated for pipe */
    pTARG_TCD pTcd;			/* TCD for this pipe */

    UINT16 configuration;		/* configuration associated with pipe */
    UINT16 interface;			/* interface associated with pipe */

    pUSB_TARG_ENDPOINT_INFO pEndpoint;	/* ptr to TCD endpoint info */
    pUSB_TARG_ENDPOINT_INFO pEndpoint2; /* 2nd endpoint for ctrl pipes */
    UINT16 dataToggle;			/* current DATA0/DATA1 state */

    LIST_HEAD erps;			/* list of outstanding ERPs */
    pUSB_ERP erpBeingDeleted;		/* ptr to ERP with cancel pending */
    BOOL erpDeleted;			/* TRUE when ERP canceled */

    } TARG_PIPE, *pTARG_PIPE;


/* locals */

LOCAL UINT16 initCount = 0;		/* init count nesting */
LOCAL BOOL ossInitialized;		/* TRUE when ossLib initialized */
LOCAL BOOL handleInitialized;		/* TRUE when usbHandleLib initialized */

LOCAL MUTEX_HANDLE targMutex;		/* guards data structures */

LOCAL LIST_HEAD tcdList;		/* list of attached TCDs */


/* forward declarations */

LOCAL VOID setupErpCallback (pVOID p);
LOCAL VOID responseErpCallback (pVOID p);
LOCAL VOID statusErpCallback (pVOID p);


/* functions */

/***************************************************************************
*
* cancelErp - Cancel an IRP and wait for the ERP callback
*
* RETURNS: OK or S_usbTargLib_xxxx if couldn't cancel ERP
*/

LOCAL int cancelErp
    (
    pTARG_PIPE pPipe,
    pUSB_ERP pErp
    )

    {
    /* Mark ERP being canceled */

    pPipe->erpBeingDeleted = pErp;
    pPipe->erpDeleted = FALSE;

    /* Try to cancel ERP */

    if (usbTcdErpCancel (&pPipe->pTcd->tcdNexus, pErp) != OK)
	return S_usbTargLib_CANNOT_CANCEL;

    while (!pPipe->erpDeleted)
	OSS_THREAD_SLEEP (1);

    return OK;
    }


/***************************************************************************
*
* mngmtFunc - Invokes target applications mngmtFunc callback.
*
* If there is no mngmtFunc, returns OK.
*
* RETURNS: return code (OK, ERROR) from mngmtFunc.
*/

LOCAL STATUS mngmtFunc
    (
    pTARG_TCD pTcd,
    UINT16 mngmtCode
    )

    {
    if (pTcd->pCallbacks->mngmtFunc == NULL)
	return OK;

    return (*pTcd->pCallbacks->mngmtFunc)
	(pTcd->callbackParam, pTcd->targChannel, mngmtCode);
    }


/***************************************************************************
*
* destroyPipe - Shuts down a pipe and releases its resources
*
* RETURNS: N/A
*/

LOCAL VOID destroyPipe
    (
    pTARG_PIPE pPipe
    )

    {
    pUSB_ERP pErp;


    if (pPipe != NULL)
	{
	/* Unlink the pipe */

	usbListUnlink (&pPipe->pipeLink);

	/* Cancel outstanding ERPs */

	while ((pErp = usbListFirst (&pPipe->erps)) != NULL)
	    cancelErp (pPipe, pErp);

	/* Release endpoint(s) assigned to pipe. */

	if (pPipe->pEndpoint != NULL)
	    usbTcdEndpointRelease (&pPipe->pTcd->tcdNexus, 
		pPipe->pEndpoint->endpointId);

	if (pPipe->pEndpoint2 != NULL)
	    usbTcdEndpointRelease (&pPipe->pTcd->tcdNexus,
		pPipe->pEndpoint2->endpointId);

	/* release handle */

	usbHandleDestroy (pPipe->pipeHandle);


	OSS_FREE (pPipe);
	}
    }


/***************************************************************************
*
* destroyTcd - Shuts down a target channel and releases its resources
*
* RETURNS: N/A
*/

LOCAL VOID destroyTcd
    (
    pTARG_TCD pTcd
    )

    {
    pTARG_PIPE pPipe;


    if (pTcd != NULL)
	{
	/* Unlink TCD */

	usbListUnlink (&pTcd->tcdLink);


	/* Notify target application that the channel is going down. */

	mngmtFunc (pTcd, TCD_MNGMT_DETACH);


	/* Destroy outstanding pipes */

	while ((pPipe = usbListFirst (&pTcd->pipes)) != NULL)
	    destroyPipe (pPipe);


	/* Detach TCD */

	usbTcdDetach (&pTcd->tcdNexus);


	/* release handle */

	if (pTcd->targChannel != NULL)
	    usbHandleDestroy (pTcd->targChannel);


	ossFree (pTcd);
	}
    }


/***************************************************************************
*
* validateTarg - validate a target channel and the state of usbTargLib
*
* If <ppTcd> is not NULL, then validate the <targChannel> parameter.
* Also checks to make sure usbTargLib has been properly initialized.
*
* RETURNS: OK or ERROR if unable to validate target channel
*
* ERRNO:
*   S_usbTargLib_NOT_INITIALIZED
*   S_usbTargLib_BAD_HANDLE
*/

LOCAL int validateTarg
    (
    USB_TARG_CHANNEL targChannel,
    pTARG_TCD *ppTcd
    )

    {
    if (initCount == 0)
	return ossStatus (S_usbTargLib_NOT_INITIALIZED);

    if (ppTcd != NULL)
	{
	if (usbHandleValidate (targChannel, TARG_TCD_SIG, (pVOID *) ppTcd) != OK)
	    return ossStatus (S_usbTargLib_BAD_HANDLE);
	}

    return OK;
    }


/***************************************************************************
*
* validatePipe - validates a pipe handle
*
* RETURNS: OK or ERROR if unable to validate pipe handle
*
* ERRNO:
*   S_usbTargLib_BAD_HANDLE
*/

LOCAL STATUS validatePipe
    (
    USB_TARG_PIPE pipeHandle,
    pTARG_PIPE *ppPipe
    )

    {
    if (usbHandleValidate (pipeHandle, TARG_PIPE_SIG, (pVOID *) ppPipe) != OK)
	return ossStatus (S_usbTargLib_BAD_HANDLE);

    return OK;
    }


/***************************************************************************
*
* findEndpointById - Return pUSB_TARG_ENDPOINT_INFO for endpointId
*
* RETURNS: pointer to USB_TARG_ENDPOINT_INFO or NULL if invalid endpointId
*/

LOCAL pUSB_TARG_ENDPOINT_INFO findEndpointById
    (
    pTARG_TCD pTcd,
    UINT16 endpointId
    )

    {
    pUSB_TARG_ENDPOINT_INFO pEndpoint;
    UINT16 i;

    for (i = 0; i < pTcd->numEndpoints; i++)
	if ((pEndpoint = &pTcd->pEndpoints [i])->endpointId == endpointId)
	    return pEndpoint;

    return NULL;
    }


/***************************************************************************
*
* validateEndpoint - checks if an endpointId can be used as requested
*
* Checks if <endpointId> is a valid endpoint ID and if the endpoint it
* indicates supports the <transferType> and <direction>.  If the endpoint
* is valid and supports the requested transfer type/direction, then
* returns a pointer to the USB_TARG_ENDPOINT_INFO structure for the
* endpoint.
*
* RETURNS: OK, or ERROR if unable to validate endpoint
*
* ERRNO:
*   S_usbTargLib_BAD_PARAM
*   S_usbTargLib_ENDPOINT_IN_USE
*/

LOCAL STATUS validateEndpoint
    (
    pTARG_TCD pTcd,
    UINT16 endpointId,
    UINT16 transferType,
    UINT16 direction,
    pUSB_TARG_ENDPOINT_INFO *ppEndpoint
    )

    {
    pUSB_TARG_ENDPOINT_INFO pEndpoint;


    /* Check if endpoint number is valid */

    if ((pEndpoint = findEndpointById (pTcd, endpointId)) == NULL)
	return ossStatus (S_usbTargLib_BAD_PARAM);


    /* See if the endpointId is in use. */

    if ((pEndpoint->flags & TCD_ENDPOINT_IN_USE) != 0)
	return ossStatus (S_usbTargLib_ENDPOINT_IN_USE);


    /* See if the endpoint supports the direction */

    if (direction == USB_DIR_IN)
	{
	if ((pEndpoint->flags & TCD_ENDPOINT_IN_OK) == 0)
	    return ossStatus (S_usbTargLib_BAD_PARAM);
	}
    else if (direction == USB_DIR_OUT)
	{
	if ((pEndpoint->flags & TCD_ENDPOINT_OUT_OK) == 0)
	    return ossStatus (S_usbTargLib_BAD_PARAM);
	}
    else
	return ossStatus (S_usbTargLib_BAD_PARAM);
    

    /* See if the endpoint supports the request transfer type */
    
    if (transferType == USB_XFRTYPE_CONTROL)
	{
	if ((pEndpoint->flags & TCD_ENDPOINT_CTRL_OK) == 0)
	    return ossStatus (S_usbTargLib_BAD_PARAM);
	}
    else if (transferType == USB_XFRTYPE_BULK)
	{
	if ((pEndpoint->flags & TCD_ENDPOINT_BULK_OK) == 0)
	    return ossStatus (S_usbTargLib_BAD_PARAM);
	}
    else if (transferType == USB_XFRTYPE_INTERRUPT)
	{
	if ((pEndpoint->flags & TCD_ENDPOINT_INT_OK) == 0)
	    return ossStatus (S_usbTargLib_BAD_PARAM);
	}
    else if (transferType == USB_XFRTYPE_ISOCH)
	{
	if ((pEndpoint->flags & TCD_ENDPOINT_ISOCH_OK) == 0)
	    return ossStatus (S_usbTargLib_BAD_PARAM);
	}
    else
	return ossStatus (S_usbTargLib_BAD_PARAM);


    *ppEndpoint = pEndpoint;

    return OK;
    }


/***************************************************************************
*
* resetDataToggle - reset data toggle on affected pipes
*
* This function is called when a "configuration event" is detected
* for a given node.  This function searches all pipes associated with
* the node for any that might be affected by the configuration event
* and resets their data toggles to DATA0.
*
* RETURNS: N/A
*/

LOCAL VOID resetDataToggle
    (
    pTARG_TCD pTcd,
    UINT16 configuration,
    UINT16 interface,
    UINT16 endpoint
    )

    {
    pTARG_PIPE pPipe;

    pPipe = usbListFirst (&pTcd->pipes);

    while (pPipe != NULL)
	{
	if (configuration == ANY_CONFIGURATION ||
	    configuration == pPipe->configuration)
	    {
	    if (interface == ANY_INTERFACE ||
		interface == pPipe->interface)
		{
		if (endpoint == ANY_ENDPOINT ||
		    endpoint == pPipe->pEndpoint->endpointNum)
		    {
		    /* Reset our DATA0/DATA1 indicator. */

		    pPipe->dataToggle = USB_DATA0;

		    /* Notify the TCD to reset any TCD/hardware indicators */

		    usbTcdEndpointStateSet (&pTcd->tcdNexus,
			pPipe->pEndpoint->endpointId, TCD_ENDPOINT_DATA0);

		    if (pPipe->pEndpoint2 != NULL)
			usbTcdEndpointStateSet (&pTcd->tcdNexus,
			    pPipe->pEndpoint2->endpointId, TCD_ENDPOINT_DATA0);
		    }
		}
	    }

	pPipe = usbListNext (&pPipe->pipeLink);
	}
    }


/***************************************************************************
*
* initErp - initializes ERP based on arguments passed by caller, then submits it
*
* RETURNS: OK, or ERROR if unable to initialize/submit ERP
*/

LOCAL STATUS initErp
    (
    pTARG_TCD pTcd,
    pBOOL erpInUse,
    pUSB_ERP pErp,
    ERP_CALLBACK callback,
    UINT16 dataToggle,
    UINT8 pid,
    pUINT8 pBfr,
    UINT16 bfrLen
    )

    {
    /* Check if the ERP is already in use. */

    if (*erpInUse)
	return ERROR;

    *erpInUse = TRUE;


    /* Initialize ERP */

    memset (pErp, 0, sizeof (*pErp));

    pErp->userPtr = pTcd;
    pErp->erpLen = sizeof (*pErp);
    pErp->userCallback = callback;
    pErp->dataToggle = dataToggle;

    pErp->bfrCount = 1;

    pErp->bfrList [0].pid = pid;
    pErp->bfrList [0].pBfr = pBfr;
    pErp->bfrList [0].bfrLen = bfrLen;


    /* Submit the ERP. */

    return usbTargTransfer (pTcd->controlPipe, pErp);
    }


/***************************************************************************
*
* initSetupErp - Initialize ERP to listen for control pipe requests
*
* RETURNS: OK, or ERROR if unable to initialize control ERP.
*/

LOCAL STATUS initSetupErp
    (
    pTARG_TCD pTcd
    )

    {
    /* Initialize the Setup phase ERP */

    return initErp (pTcd, &pTcd->setupErpPending, &pTcd->setupErp, 
	setupErpCallback, USB_DATA0, USB_PID_SETUP, 
	pTcd->setupBfr, sizeof (pTcd->setupBfr));
    }


/***************************************************************************
*
* initDataErpForResponse - Initialize ERP to send a control pipe response
*
* TARGET.dataBfr must already contain the data to be send to the host of
* length <bfrLen>.
*
* RETURNS: OK, or ERROR if unable to initialize/submit ERP
*/

LOCAL STATUS initDataErpForResponse
    (
    pTARG_TCD pTcd,
    UINT16 bfrLen
    )

    {
    /* Initialize the Data phase ERP */

    return initErp (pTcd, &pTcd->dataErpPending, &pTcd->dataErp, 
	responseErpCallback, USB_DATA1, USB_PID_IN, pTcd->dataBfr, bfrLen);
    }


/***************************************************************************
*
* initStatusErp - Initialize ERP to send/rcv status phase packet
*
* <pid> must specify the direction of the status phase packet as USB_PID_IN
* (from the device to the host) or USB_PID_OUT (from host to device).
*
* RETURNS: OK, or ERROR if unable to initialize/submit ERP
*/

LOCAL STATUS initStatusErp
    (
    pTARG_TCD pTcd,
    UINT8 pid
    )

    {
    /* Initialize the Status phase ERP */

    return initErp (pTcd, &pTcd->statusErpPending, &pTcd->statusErp, 
	statusErpCallback, USB_DATA1, pid, NULL, 0);
    }


/***************************************************************************
*
* requestClearFeature - processes a CLEAR_FEATURE setup packet
*
* RETURNS: OK if setup packet valid, else ERROR if invalid
*/

LOCAL STATUS requestClearFeature
    (
    pTARG_TCD pTcd,
    pUSB_SETUP pSetup
    )

    {
    UINT16 feature = FROM_LITTLEW (pSetup->value);
    UINT16 index = FROM_LITTLEW (pSetup->index);


    /* A CLEAR_FEATURE request may be configuration event.  Therefore, we
     * need to set the data toggle to DATA0 for any pipes which reference
     * this interface.
     */

    if (pSetup->requestType == (USB_RT_STANDARD | USB_RT_ENDPOINT) &&
	feature == USB_FSEL_DEV_ENDPOINT_HALT)
	{
	resetDataToggle (pTcd, ANY_CONFIGURATION, ANY_ENDPOINT, index);
	}


    /* The request itself isn't supported unless the target application
     * has provided a featureSet handler.
     */

    if (pTcd->pCallbacks->featureClear == NULL)
	return ERROR;

    if ((*pTcd->pCallbacks->featureClear)
	(pTcd->callbackParam, pTcd->targChannel, 
	pSetup->requestType, feature, index) != OK)
	return ERROR;


    /* Request terminates with status phase. */

    return initStatusErp (pTcd, USB_PID_IN);
    }


/***************************************************************************
*
* requestSetFeature - processes a SET_FEATURE setup packet
*
* RETURNS: OK if setup packet valid, else ERROR if invalid
*/

LOCAL STATUS requestSetFeature
    (
    pTARG_TCD pTcd,
    pUSB_SETUP pSetup
    )

    {
    UINT16 feature = FROM_LITTLEW (pSetup->value);
    UINT16 index = FROM_LITTLEW (pSetup->index);


    /* This request isn't supported unless the target application
     * has provided a featureSet handler.
     */

    if (pTcd->pCallbacks->featureSet == NULL)
	return ERROR;

    if ((*pTcd->pCallbacks->featureSet)
	(pTcd->callbackParam, pTcd->targChannel, 
	pSetup->requestType, feature, index) != OK)
	return ERROR;


    /* Request terminates with status phase. */

    return initStatusErp (pTcd, USB_PID_IN);
    }


/***************************************************************************
*
* requestGetConfiguration - processes a GET_CONFIGURATION setup packet
*
* RETURNS: OK if setup packet valid, else ERROR if invalid
*/

LOCAL STATUS requestGetConfiguration
    (
    pTARG_TCD pTcd,
    pUSB_SETUP pSetup
    )

    {
    /* This request isn't supported unless the target application has
     * provided a configurationGet handler.
     */

    if (pTcd->pCallbacks->configurationGet == NULL)
	return ERROR;


    /* Get the interface setting from the target application. */

    if ((*pTcd->pCallbacks->configurationGet)
	(pTcd->callbackParam, pTcd->targChannel, 
	(pUINT8) pTcd->dataBfr) != OK)
	return ERROR;


    /* Transmit the interface setting back to the host */

    return initDataErpForResponse (pTcd, sizeof (UINT8));
    }


/***************************************************************************
*
* requestSetConfiguration - processes a SET_CONFIGURATION setup packet
*
* RETURNS: OK if setup packet valid, else ERROR if invalid
*/

LOCAL STATUS requestSetConfiguration
    (
    pTARG_TCD pTcd,
    pUSB_SETUP pSetup
    )

    {
    UINT8 configuration = LSB (FROM_LITTLEW (pSetup->value));


    /* A SET_CONFIGURATION request is a configuration event.  Therefore, we
     * need to set the data toggle to DATA0 for any pipes which reference
     * this interface.
     */

    resetDataToggle (pTcd, configuration, ANY_INTERFACE, ANY_ENDPOINT);


    /* The request itself isn't supported unless the target application
     * has provided an interfaceSet handler.
     */

    if (pTcd->pCallbacks->configurationSet == NULL)
	return ERROR;

    if ((*pTcd->pCallbacks->configurationSet)
	(pTcd->callbackParam, pTcd->targChannel, configuration) != OK)
	return ERROR;


    /* Request terminates with status phase. */

    return initStatusErp (pTcd, USB_PID_IN);
    }


/***************************************************************************
*
* requestGetDescriptor - processes a GET_DESCRIPTOR setup packet
*
* RETURNS: OK if setup packet valid, else ERROR if invalid
*/

LOCAL STATUS requestGetDescriptor
    (
    pTARG_TCD pTcd,
    pUSB_SETUP pSetup
    )

    {
    UINT8 descriptorType = MSB (FROM_LITTLEW (pSetup->value));
    UINT8 descriptorIndex = LSB (FROM_LITTLEW (pSetup->value));
    UINT16 languageId = FROM_LITTLEW (pSetup->index);
    UINT16 length = FROM_LITTLEW (pSetup->length);
    UINT16 actLen;


    /* This request isn't supported unless the target application has
     * provided a descriptorGet handler.
     */

    if (pTcd->pCallbacks->descriptorGet == NULL)
	return ERROR;


    /* Get the descriptor from the target application. */

    if ((*pTcd->pCallbacks->descriptorGet)
	(pTcd->callbackParam, pTcd->targChannel, 
	pSetup->requestType, descriptorType, descriptorIndex,
	languageId, min (length, sizeof (pTcd->dataBfr)), pTcd->dataBfr, 
	&actLen) != OK)
	{
	return ERROR;
	}


    /* Transmit the descriptor back to the host */

    return initDataErpForResponse (pTcd, actLen);
    }


/***************************************************************************
*
* requestSetDescriptor - processes a SET_DESCRIPTOR setup packet
*
* RETURNS: OK if setup packet valid, else ERROR if invalid
*/

LOCAL STATUS requestSetDescriptor
    (
    pTARG_TCD pTcd,
    pUSB_SETUP pSetup
    )

    {
    UINT8 descriptorType = MSB (FROM_LITTLEW (pSetup->value));
    UINT8 descriptorIndex = LSB (FROM_LITTLEW (pSetup->value));
    UINT16 languageId = FROM_LITTLEW (pSetup->index);
    UINT16 length = FROM_LITTLEW (pSetup->length);


    /* This request isn't supported unless the target application has
     * provided a descriptorSet handler.
     */

    if (pTcd->pCallbacks->descriptorSet == NULL)
	return ERROR;


    /* Let the target application process the SET_DESCRIPTOR request. */

    return (*pTcd->pCallbacks->descriptorSet)
	(pTcd->callbackParam, pTcd->targChannel, 
	pSetup->requestType, descriptorType, descriptorIndex,
	languageId, length);
    }


/***************************************************************************
*
* requestGetInterface - processes a GET_INTERFACE setup packet
*
* RETURNS: OK if setup packet valid, else ERROR if invalid
*/

LOCAL STATUS requestGetInterface
    (
    pTARG_TCD pTcd,
    pUSB_SETUP pSetup
    )

    {
    UINT16 interface = FROM_LITTLEW (pSetup->index);


    /* This request isn't supported unless the target application has
     * provided an interfaceGet handler.
     */

    if (pTcd->pCallbacks->interfaceGet == NULL)
	return ERROR;


    /* Get the interface setting from the target application. */

    if ((*pTcd->pCallbacks->interfaceGet)
	(pTcd->callbackParam, pTcd->targChannel, 
	interface, (pUINT8) pTcd->dataBfr) != OK)
	return ERROR;


    /* Transmit the interface setting back to the host */

    return initDataErpForResponse (pTcd, sizeof (UINT8));
    }


/***************************************************************************
*
* requestSetInterface - processes a SET_INTERFACE setup packet
*
* RETURNS: OK if setup packet valid, else ERROR if invalid
*/

LOCAL STATUS requestSetInterface
    (
    pTARG_TCD pTcd,
    pUSB_SETUP pSetup
    )

    {
    UINT16 interfaceIndex = FROM_LITTLEW (pSetup->index);
    UINT8 alternateSetting = LSB (FROM_LITTLEW (pSetup->value));


    /* A SET_INTERFACE request is a configuration event.  Therefore, we
     * need to set the data toggle to DATA0 for any pipes which reference
     * this interface.
     */

    resetDataToggle (pTcd, ANY_CONFIGURATION, interfaceIndex, ANY_ENDPOINT);


    /* The request itself isn't supported unless the target application
     * has provided an interfaceSet handler.
     */

    if (pTcd->pCallbacks->interfaceSet == NULL)
	return ERROR;

    if ((*pTcd->pCallbacks->interfaceSet)
	(pTcd->callbackParam, pTcd->targChannel,
	interfaceIndex, alternateSetting) != OK)
	return ERROR;


    /* Request terminates with status phase. */

    return initStatusErp (pTcd, USB_PID_IN);
    }


/***************************************************************************
*
* requestGetStatus - processes a GET_STATUS setup packet
*
* RETURNS: OK if setup packet valid, else ERROR if invalid
*/

LOCAL STATUS requestGetStatus
    (
    pTARG_TCD pTcd,
    pUSB_SETUP pSetup
    )

    {
    UINT16 index = FROM_LITTLEW (pSetup->index);
    UINT16 length = FROM_LITTLEW (pSetup->length);
    UINT16 actLen;


    /* This request isn't supported unless the target application has
     * provided a statusGet handler.
     */

    if (pTcd->pCallbacks->statusGet == NULL)
	return ERROR;

    if ((*pTcd->pCallbacks->statusGet)
	(pTcd->callbackParam, pTcd->targChannel,
	pSetup->requestType, index, 
	min (length, sizeof (pTcd->dataBfr)), pTcd->dataBfr, 
	&actLen) != OK)
	{
	return ERROR;
	}


    /* Transmit the descriptor back to the host */

    return initDataErpForResponse (pTcd, actLen);
    }


/***************************************************************************
*
* requestSetAddress - processes a SET_ADDRESS setup packet
*
* RETURNS: OK if setup packet valid, else ERROR if invalid
*/

LOCAL STATUS requestSetAddress
    (
    pTARG_TCD pTcd,
    pUSB_SETUP pSetup
    )

    {
    UINT16 deviceAddress = FROM_LITTLEW (pSetup->value);


    /* If the target application has provided a SetAddress callback,
     * then invoke it now.
     */

    if (pTcd->pCallbacks->addressSet != NULL)
	(*pTcd->pCallbacks->addressSet)
	    (pTcd->callbackParam, pTcd->targChannel, deviceAddress);


    /* The new address goes into effect after the Status phase, if any. */

    pTcd->deviceAddress = deviceAddress;
    usbTcdAddressSet (&pTcd->tcdNexus, pTcd->deviceAddress);

    return initStatusErp (pTcd, USB_PID_IN);
    }


/***************************************************************************
*
* requestGetSynchFrame - processes a GET_SYNCH_FRAME setup packet
*
* RETURNS: OK if setup packet valid, else ERROR if invalid
*/

LOCAL STATUS requestGetSynchFrame
    (
    pTARG_TCD pTcd,
    pUSB_SETUP pSetup
    )

    {
    UINT16 endpoint = FROM_LITTLEW (pSetup->index);


    /* This request isn't supported unless the target application has
     * provided a synchFrameGet handler.
     */

    if (pTcd->pCallbacks->synchFrameGet == NULL)
	return ERROR;


    /* Get the synch frame from the target application. */

    if ((*pTcd->pCallbacks->synchFrameGet)
	(pTcd->callbackParam, pTcd->targChannel, 
	endpoint, (pUINT16) pTcd->dataBfr) != OK)
	return ERROR;


    /* Transmit the synch frame back to the host */

    return initDataErpForResponse (pTcd, sizeof (UINT16));
    }


/***************************************************************************
*
* requestVendorSpecific - processes a "vendor specific" setup packet
*
* RETURNS: OK if setup packet valid, else ERROR if invalid
*/

LOCAL STATUS requestVendorSpecific
    (
    pTARG_TCD pTcd,
    pUSB_SETUP pSetup
    )

    {
    /* This request isn't supported unless the target application has
     * provided a vendorSpecific handler.
     */

    if (pTcd->pCallbacks->vendorSpecific == NULL)
	return ERROR;

    return (*pTcd->pCallbacks->vendorSpecific) 
	(pTcd->callbackParam, pTcd->targChannel, 
	pSetup->requestType, pSetup->request, FROM_LITTLEW (pSetup->value),
	FROM_LITTLEW (pSetup->index), FROM_LITTLEW (pSetup->length));
    }


/***************************************************************************
*
* parseSetupPacket - parse/execute a control pipe request
*
* The TARGET.setupBfr should contain a setup packet.  Validate it.  If
* valid, parse and execute it.
*
* RETURNS: OK if setup packet valid, else ERROR if invalid
*/

LOCAL STATUS parseSetupPacket
    (
    pTARG_TCD pTcd
    )

    {
    pUSB_SETUP pSetup;


    /* Validate the setup packet */

    if (pTcd->setupErp.bfrList [0].actLen != sizeof (USB_SETUP))
	return ERROR;

    pSetup = (pUSB_SETUP) pTcd->setupBfr;


    /* Execute based on the type of request. */

    if ((pSetup->requestType & USB_RT_CATEGORY_MASK) == USB_RT_STANDARD)
	{
	switch (pSetup->request)
	    {
	    case USB_REQ_CLEAR_FEATURE:
		return requestClearFeature (pTcd, pSetup);

	    case USB_REQ_SET_FEATURE:
		return requestSetFeature (pTcd, pSetup);

	    case USB_REQ_GET_CONFIGURATION:
		return requestGetConfiguration (pTcd, pSetup);

	    case USB_REQ_SET_CONFIGURATION:
		return requestSetConfiguration (pTcd, pSetup);

	    case USB_REQ_GET_DESCRIPTOR:
		return requestGetDescriptor (pTcd, pSetup);

	    case USB_REQ_SET_DESCRIPTOR:
		return requestSetDescriptor (pTcd, pSetup);

	    case USB_REQ_GET_INTERFACE:
		return requestGetInterface (pTcd, pSetup);

	    case USB_REQ_SET_INTERFACE:
		return requestSetInterface (pTcd, pSetup);

	    case USB_REQ_GET_STATUS:
		return requestGetStatus (pTcd, pSetup);

	    case USB_REQ_SET_ADDRESS:
		return requestSetAddress (pTcd, pSetup);

	    case USB_REQ_GET_SYNCH_FRAME:
		return requestGetSynchFrame (pTcd, pSetup);

	    default:
		break;
	    }
	}


    return requestVendorSpecific (pTcd, pSetup);
    }


/***************************************************************************
*
* setupErpCallback - invoked when Setup ERP completes
*
* This function receives control when a Setup ERP completes.  Examines the
* reason for completion, possibly invoking additional control request
* handling.
*
* RETURNS: N/A
*/

LOCAL VOID setupErpCallback
    (
    pVOID p			    /* ptr to ERP */
    )

    {
    pUSB_ERP pErp = (pUSB_ERP) p;
    pTARG_TCD pTcd = (pTARG_TCD) pErp->userPtr;


    pTcd->setupErpPending = FALSE;


    /* Completion of a setup ERP, whether successful or not, implies that
     * any previous control pipe request must be terminated. */

    if (pTcd->dataErpPending)
	cancelErp ((pTARG_PIPE) pTcd->dataErp.targPtr, &pTcd->dataErp);

    if (pTcd->statusErpPending)
	cancelErp ((pTARG_PIPE) pTcd->dataErp.targPtr, &pTcd->statusErp);


    /* Was the ERP successful? */

    if (pErp->result == OK)
	{
	/* The ERP was successful.  Parse the setup packet. */

	if (parseSetupPacket (pTcd) == OK)
	    {
	    /* The packet can be parsed successfully.  Other code will
	     * take care of resubmitting the Setup ERP, so we do nothing.
	     */

	    return;
	    }
	else
	    {
	    /* We cannot process the indicated command.  Stall the
	     * control pipe.
	     */

	    usbTargPipeStatusSet (pTcd->targChannel, pTcd->controlPipe,
		TCD_ENDPOINT_STALL);
	    }
	}


    /* Re-submit the ERP unless it was canceled - which indicates that
     * the pipe is probably being torn down.
     */

    if (pErp->result != S_usbTcdLib_ERP_CANCELED)
	initSetupErp (pTcd);
    }


/***************************************************************************
*
* responseErpCallback - invoked when control pipe response ERP completes
*
* This function receives control after the data IN phase of a control pipe
* request has completed.
*
* RETURNS: N/A
*/

LOCAL VOID responseErpCallback
    (
    pVOID p			    /* ptr to ERP */
    )

    {
    pUSB_ERP pErp = (pUSB_ERP) p;
    pTARG_TCD pTcd = (pTARG_TCD) pErp->userPtr;


    pTcd->dataErpPending = FALSE;


    /* If the ERP was successful, then we prepare the status phase.  The
     * status phase is always in the oppposite direction to the data phase.
     * Since this callback handles only "responses" (from device to host),
     * then the status phase will always be an OUT (from host to device).
     *
     * If the data phase failed for some reason other than being canceled,
     * then we resubmit the Setup phase ERP to ensure that the target
     * continues to be responsive on the control pipe.
     */

    if (pErp->result == OK)
	{
	initStatusErp (pTcd, USB_PID_OUT);
	return;
	}

    if (pErp->result != S_usbTcdLib_ERP_CANCELED)
	initSetupErp (pTcd);
    }


/***************************************************************************
*
* statusErpCallback - invoked when control pipe status packet completes
*
* This function receives control after the status phase of a control pipe
* request has completed.
*
* RETURNS: N/A
*/

LOCAL VOID statusErpCallback
    (
    pVOID p			    /* ptr to ERP */
    )

    {
    pUSB_ERP pErp = (pUSB_ERP) p;
    pTARG_TCD pTcd = (pTARG_TCD) pErp->userPtr;


    pTcd->statusErpPending = FALSE;


    /* Re-submit the Setup ERP. */

    if (pErp->result != S_usbTcdLib_ERP_CANCELED)
	initSetupErp (pTcd);
    }


/***************************************************************************
*
* usbTargManagementCallback - invoked when TCD detects management event
*
* This function is invoked by a TCD when the TCD detects a "management"
* event on a target channel.  Examples of management events include
* bus resets, detection of a new or lost connection, etc.
*
* RETURNS: N/A
*/

LOCAL VOID usbTargManagementCallback
    (
    pVOID mngmtCallbackParam,	    /* caller-defined param */
    TCD_HANDLE handle,		    /* channel */
    UINT16 mngmtCode		    /* management code */
    )

    {
    pTARG_TCD pTcd = (pTARG_TCD) mngmtCallbackParam;

    switch (mngmtCode)
	{
	case TCD_MNGMT_VBUS_LOST:
	case TCD_MNGMT_BUS_RESET:

	    /* Reset the device configuration to the power-on state. */

	    pTcd->deviceAddress = 0;

	    /* code falls through into following case */

	case TCD_MNGMT_VBUS_DETECT:
	case TCD_MNGMT_SUSPEND:
	case TCD_MNGMT_RESUME:

	    /* reflect the management event to the target application */

	    mngmtFunc (pTcd, mngmtCode);
	    break;
	}
    }


/***************************************************************************
*
* usbTargErpCallback - invoked when ERP completes
*
* The TCD invokes this callback when an ERP completes.	This gives us the
* opportunity to monitor ERP execution.  We reflect the callback to the
* calling application after we finish our processing.
*
* NOTE: By convention, the targPtr field of the ERP has been initialized
* to point to the TARG_PIPE for this ERP.
*
* RETURNS: N/A
*/

LOCAL VOID usbTargErpCallback
    (
    pVOID p			    /* ptr to ERP */
    )

    {
    pUSB_ERP pErp = (pUSB_ERP) p;
    pTARG_PIPE pPipe = (pTARG_PIPE) pErp->targPtr;
    UINT16 packets;
    UINT16 i;


    OSS_MUTEX_TAKE (targMutex, OSS_BLOCK);


    /* Unlink the ERP */

    usbListUnlink (&pErp->targLink);


    /* Check if this ERP is being deleted.  If so, let the foreground
     * thread know the callback has been invoked.
     */

    if (pErp == pPipe->erpBeingDeleted)
	pPipe->erpDeleted = TRUE;


    /* Update data toggle for pipe.  Control and isochronous transfers
     * always begin with DATA0, which is set at pipe initialization -
     * so we don't change it here.  Bulk and interrupt pipes alternate
     * between DATA0 and DATA1, and we need to keep a running track of
     * the state across ERPs.
     */

    if (pErp->transferType == USB_XFRTYPE_INTERRUPT ||
	pErp->transferType == USB_XFRTYPE_BULK)
	{
	/* Calculate the number of packets exchanged to determine the
	 * next data toggle value.  If the count of packets is odd, then
	 * the data toggle needs to switch.
	 *
	 * NOTE: If the ERP is successful, then at least one packet MUST
	 * have been transferred.  However, it may have been a 0-length
	 * packet.  This case is handled after the following "for" loop.
	 */

	packets = 0;

	for (i = 0; i < pErp->bfrCount; i++)
	    {
	    packets += 
		(pErp->bfrList [i].actLen + pPipe->pEndpoint->maxPacketSize - 1) /
		pPipe->pEndpoint->maxPacketSize;
	    }

	if (pErp->result == OK)
	    packets = max (packets, 1);

	if ((packets & 1) != 0)
	    pPipe->dataToggle = (pPipe->dataToggle == USB_DATA0) ?
		USB_DATA1 : USB_DATA0;
	}


    /* Invoke the user's callback routine */

    if (pErp->userCallback != NULL)
	(*pErp->userCallback) (pErp);


    OSS_MUTEX_RELEASE (targMutex);
    }


/***************************************************************************
*
* usbTargVersionGet - Retrieve usbTargLib version
*
* This function returns the usbTargLib version.  If <pVersion> is not NULL, the 
* usbTargLib returns its version in BCD in <pVersion>.	For example, version 
* "1.02" would be coded as 01h in the high byte and 02h in the low byte.
*
* If <pMfg> is not NULL it must point to a buffer of at least USBT_NAME_LEN 
* bytes in length in which the USBD will store the NULL terminated name of
* the usbTargLib manufacturer (e.g., "Wind River Systems" + \0).
*
* RETURNS: OK
*/
 
STATUS usbTargVersionGet
    (
    pUINT16 pVersion,			/* usbTargLib version */
    pCHAR pMfg				/* usbTargLib manufacturer */
    )

    {
    /* return version info */

    if (pVersion != NULL)
	*pVersion = TARG_VERSION;

    if (pMfg != NULL)
	strncpy (pMfg, TARG_MFG, USBT_NAME_LEN);

    return OK;
    }


/***************************************************************************
*
* doShutdown - shut down usbTargLib
*
* RETURNS: N/A
*/

LOCAL VOID doShutdown (void)
    {
    pTARG_TCD pTcd;

    /* Shut down all target channels */

    while ((pTcd = usbListFirst (&tcdList)) != NULL)
	destroyTcd (pTcd);

    /* Release other resources. */

    if (targMutex != NULL)
	{
	OSS_MUTEX_DESTROY (targMutex);
	targMutex = NULL;
	}

    /* Shut down handle library */

    if (handleInitialized)
	{
	usbHandleShutdown ();
	handleInitialized = FALSE;
	}

    /* Shut down ossLib */

    if (ossInitialized)
	{
	ossShutdown ();
	ossInitialized = FALSE;
	}
    }


/***************************************************************************
*
* usbTargInitialize - Initializes usbTargLib
*
* usbTargInitialize() must be called at least once prior to calling other
* usbTargLib functions except for usbTargVersionGet().	usbTargLib maintains
* an internal initialization count, so calls to usbTargInitialize() may be 
* nested.  
*
* RETURNS: OK, or ERROR if unable to initialize usbTargLib.
*
* ERRNO:
*   S_usbTargLib_OUT_OF_RESOURCES
*   S_usbTargLib_GENERAL_FAULT;
*/

STATUS usbTargInitialize (void)
    {
    int status = OK;


    if (initCount == 0)
	{
	/* Initialize local data */

	memset (&tcdList, 0, sizeof (tcdList));
	ossInitialized = FALSE;
	handleInitialized = FALSE;

	/* Initialize ossLib */

	if (ossInitialize () != OK)
	    status = S_usbTargLib_GENERAL_FAULT;
	else
	    {
	    ossInitialized = TRUE;

	    /* Intialize usbHandleLib */

	    if (usbHandleInitialize (0) != OK)
		status = S_usbTargLib_GENERAL_FAULT;
	    else
		{
		handleInitialized = TRUE;

		if (OSS_MUTEX_CREATE (&targMutex) != OK)
		    status = S_usbTargLib_OUT_OF_RESOURCES;
		}
	    }
	}

    if (status == OK)
	initCount++;
    else
	doShutdown ();

    return ossStatus (status);
    }


/***************************************************************************
*
* usbTargShutdown - Shuts down usbTargLib
*
* usbTargShutdown() should be called once for every successful call to
* usbTargInitialize().	usbTargShutdown() closes any open target channels
* and releases resources allocated by the usbTargLib.
*
* RETURNS: OK, or ERROR if unable to shut down usbTargLib.
*
* ERRNO:
*   S_usbTargLib_NOT_INITIALIZED
*/

STATUS usbTargShutdown (void)
    {
    /* Are we initialized? */

    if (initCount == 0)
	return ossStatus (S_usbTargLib_NOT_INITIALIZED);

    if (--initCount == 0)
	doShutdown ();

    return OK;
    }


/***************************************************************************
*
* usbTargTcdAttach - Attaches and initializes a USB target controller driver
*
* This function attaches a USB TCD (Target Controller Driver) to usbTargLib.
* A TCD needs to be attached to usbTargLib before any other USB operations
* may be performed on the target channel it manages.
*
* <tcdExecFunc> is the primary entry point of the TCD being attached and
* <tcdParam> is a TCD-defined parameter which is passed to the TCD's attach
* function.  Generally, the <tcdParam> is points to a TCD-defined structure
* which identifies the characteristics of the target controller hardware
* (i.e., base I/O address, IRQ channel, etc.).	Each call to usbTargTcdAttach()
* enables one and only one target channel.
*
* <pCallbacks> points to a USB_TARG_CALLBACK_TABLE structure in which the
* caller has stored the addresses of callbacks for events in which it is
* interested in being notified.  <callbackParam> is a caller-defined parameter 
* which is passed to each callback function when the callbacks are invoked by
* usbTargLib.
*
* <pTargChannel> points to a USB_TARG_CHANNEL variable allocated by the caller
* which receives a handle to the target channel created by this call to
* usbTargTcdAttach().
*
* <pNumEndpoints> and <ppEndpoints> receive the count of target endpoints and
* a pointer to an array of USB_TARG_ENDPOINT_INFO structures, respectively.
* The caller can use this information to assign endpoints to pipes.
*
* RETURNS: OK, or ERROR if unable to attach/initialize TCD.
*
* ERRNO:
*   S_usbTargLib_BAD_PARAM
*   S_usbTargLib_OUT_OF_MEMORY
*   S_usbTargLib_OUT_OF_RESOURCES
*   S_usbTargLib_TCD_FAULT
*   S_usbTargLib_APP_FAULT
*/

STATUS usbTargTcdAttach
    (
    USB_TCD_EXEC_FUNC tcdExecFunc,	/* TCD entry point */
    pVOID tcdParam,			/* TCD parameter */
    pUSB_TARG_CALLBACK_TABLE pCallbacks,/* caller-supplied callbacks */
    pVOID callbackParam,		/* caller-defined callback param */
    pUSB_TARG_CHANNEL pTargChannel,	/* handle to target on return */
    pUINT16 pNumEndpoints,		/* bfr to receive nbr of endpoints */
    pUSB_TARG_ENDPOINT_INFO *ppEndpoints/* bfr to rcv ptr to endpt array */
    )

    {
    pTARG_TCD pTcd;
    int status;

    /* Validate parameters */

    if (tcdExecFunc == NULL || pCallbacks == NULL || pTargChannel == NULL ||
	pNumEndpoints == NULL || ppEndpoints == NULL)
	return ossStatus (S_usbTargLib_BAD_PARAM);

    if ((status = validateTarg (NULL, NULL)) != OK)
	return status;


    OSS_MUTEX_TAKE (targMutex, OSS_BLOCK);

    /* Allocate a TARG_TCD to manage this channel */

    if ((pTcd = OSS_CALLOC (sizeof (*pTcd))) == NULL)
	status = S_usbTargLib_OUT_OF_MEMORY;

    if (status == OK)
	{
	/* Initialize TCD */

	pTcd->pCallbacks = pCallbacks;
	pTcd->callbackParam = callbackParam;

	if (usbHandleCreate (TARG_TCD_SIG, pTcd, &pTcd->targChannel) != OK)
	    status = S_usbTargLib_OUT_OF_RESOURCES;
	}

    if (status == OK)
	{
	/* Try to initialize the TCD */

	if (usbTcdAttach (tcdExecFunc, tcdParam, &pTcd->tcdNexus,
	    usbTargManagementCallback, pTcd, &pTcd->speed, 
	    &pTcd->numEndpoints, &pTcd->pEndpoints) != OK)
	    {
	    status = S_usbTargLib_TCD_FAULT;
	    }
	else
	    {
	    /* Make sure the target exposed at least two endpoints, aka, the
	     * OUT and IN endpoints we need for the default control chanenl. */

	    if (pTcd->numEndpoints < 2 || pTcd->pEndpoints == NULL)
		status = S_usbTargLib_TCD_FAULT;
	    }
	}

    if (status == OK)
	{
	/* Create a default control channel for this target.
	 *
	 * NOTE: By convention, the target always places the endpoints which
	 * are best suited for the default control channel at the beginning
	 * of the endpoint array, with the USB_DIR_OUT (host->device) endpoint
	 * first, followed by the USB_DIR_IN endpoint.
	 */

	if ((status = usbTargPipeCreate (pTcd->targChannel, 
	    pTcd->pEndpoints [0].endpointId, pTcd->pEndpoints [1].endpointId,
	    USB_ENDPOINT_DEFAULT_CONTROL, NO_CONFIGURATION, NO_INTERFACE, 
	    USB_XFRTYPE_CONTROL, USB_DIR_INOUT, &pTcd->controlPipe)) == OK)
	    {
	    /* Start listening for requests on the default control pipe. */

	    status = initSetupErp (pTcd);	    
	    }
	}

    if (status == OK)
	{
	/* Link the TCD to the list of managed TCDs. */

	usbListLink (&tcdList, pTcd, &pTcd->tcdLink, LINK_TAIL);

	*pTargChannel = pTcd->targChannel;
	*pNumEndpoints = pTcd->numEndpoints;
	*ppEndpoints = pTcd->pEndpoints;
	}

    if (status == OK)
	{
	/* Invoke the target application's mngmtFunc to notify it that 
	 * the attach is complete. 
	 */

	if (mngmtFunc (pTcd, TCD_MNGMT_ATTACH) != OK)
	    status = S_usbTargLib_APP_FAULT;
	}

    /* If we failed to initialized, clean up the TCD. */

    if (status != OK)
	destroyTcd (pTcd);


    OSS_MUTEX_RELEASE (targMutex);

    return ossStatus (status);
    }


/***************************************************************************
*
* usbTargTcdDetach - Detaches a USB target controller driver
*
* This function detaches a USB TCD which was previously attached to the
* usbTargLib by calling usbTargTcdAttach().  <targChannel> is the handle
* of the target channel originally returned by usbTargTcdAttach().
*
* The usbTargLib automatically terminates any pending transactions on the
* target channel being detached and releases all internal usbTargLib
* resources allocated on behalf of the channel.  Once a target channel
* has been detached by calling this function, the <targChannel> is no
* longer valid.
*
* RETURNS: OK, or ERROR if unable to detach TCD.
*/

STATUS usbTargTcdDetach
    (
    USB_TARG_CHANNEL targChannel	/* target to detach */
    )

    {
    pTARG_TCD pTcd;
    STATUS status;


    OSS_MUTEX_TAKE (targMutex, OSS_BLOCK);

    /* Validate parameters */

    if ((status = validateTarg (targChannel, &pTcd)) == OK)
	{
	/* Destroy target channel */

	destroyTcd (pTcd);
	}

    OSS_MUTEX_RELEASE (targMutex);

    return status;
    }


/***************************************************************************
*
* usbTargEndpointInfoGet - Retrieves endpoint information for channel
*
* This function retrieves the number of endpoints on <targChannel> and
* returns a pointer to the base of the USB_TARG_ENDPOINT_INFO array.
*
* RETURNS: OK, or ERROR if unable to return target endpoint information
*/

STATUS usbTargEndpointInfoGet
    (
    USB_TARG_CHANNEL targChannel,	/* target channel */
    pUINT16 pNumEndpoints,		/* receives nbr of endpoints */
    pUSB_TARG_ENDPOINT_INFO *ppEndpoints/* receives ptr to array */
    )

    {
    pTARG_TCD pTcd;
    STATUS status;


    OSS_MUTEX_TAKE (targMutex, OSS_BLOCK);

    /* Validate parameters */

    if ((status = validateTarg (targChannel, &pTcd)) == OK)
	{
	if (pNumEndpoints != NULL)
	    *pNumEndpoints = pTcd->numEndpoints;

	if (ppEndpoints != NULL)
	    *ppEndpoints = pTcd->pEndpoints;
	}

    
    OSS_MUTEX_RELEASE (targMutex);

    return ossStatus (status);
    }


/***************************************************************************
*
* usbTargEnable - Enables target channel onto USB
*
* After attaching a TCD to usbTargLib and performing any other application-
* specific initialization that might be necessary, this function should be
* called to enable a target channel.  The USB target controlled by the TCD
* will not appear as a device on the USB until this function has been called.
*
* RETURNS: OK, or ERROR if unable to enable target channel.
*
* ERRNO:
*   S_usbTargLib_TCD_FAULT
*/

STATUS usbTargEnable
    (
    USB_TARG_CHANNEL targChannel	/* target to enable */
    )

    {
    pTARG_TCD pTcd;
    STATUS status;


    OSS_MUTEX_TAKE (targMutex, OSS_BLOCK);

    /* Validate parameters */

    if ((status = validateTarg (targChannel, &pTcd)) == OK)
	{
	/* Enable target channel */

	if (usbTcdEnable (&pTcd->tcdNexus) != OK)
	    status = S_usbTargLib_TCD_FAULT;
	}

    OSS_MUTEX_RELEASE (targMutex);

    return status;
    }


/***************************************************************************
*
* usbTargDisable - Disables a target channel
*
* This function is the counterpart to the usbTargEnable() function.  This
* function disables the indicated target channel.
*
* RETURNS: OK, or ERROR if unable to disable the target channel.
*
* ERRNO:
*   S_usbTargLib_TCD_FAULT
*/

STATUS usbTargDisable
    (
    USB_TARG_CHANNEL targChannel	/* target to disable */
    )

    {
    pTARG_TCD pTcd;
    STATUS status;


    OSS_MUTEX_TAKE (targMutex, OSS_BLOCK);

    /* Validate parameters */

    if ((status = validateTarg (targChannel, &pTcd)) == OK)
	{
	/* Enable target channel */

	if (usbTcdDisable (&pTcd->tcdNexus) != OK)
	    status = S_usbTargLib_TCD_FAULT;
	}

    OSS_MUTEX_RELEASE (targMutex);

    return status;
    }


/***************************************************************************
*
* usbTargPipeCreate - Creates a pipe for communication on an endpoint
*
* This function creates a pipe attached to a specific target endpoint.
* <endpointId> is the TCD-assigned ID of the target endpoint to be used
* for this pipe and <endpointNum> is the device endpoint number to which
* the endpoint will respond.  Some TCDs allow the flexible assignment of
* endpoints to specific endpoint numbers while others do not.  The
* endpointNumMask field in the USB_TARG_ENDPOINT_INFO structure is a bit
* mask that reveals which endpoint numbers are supported by a given
* endpoint.  (e.g., If bit 0 is '1', then the corresponding endpoint can
* be assigned to endpoint #0, and so forth).
*
* By convention, each of the endpoints exposed by the TCD is unidirectional.
* Control pipes are bidirectional, and therefore logically occupy two
* endpoints, one IN endpoint and one OUT endpoint, both with the same
* endpointNum.	When creating a control pipe, the caller must specify the
* second endpoint Id in <endpointId2>.	In this case, <endpointId> must
* specify the OUT endpoint, and <endpointId2> must specify the IN endpoint.
* <endpointId2> should be 0 for other types of pipes.
*
* <configuration> and <interface> specify the device configuration and
* interface with which the endpoint (and pipe) is associated.  The 
* usbTargLib uses these values to reset pipes as appropriate when
* USB "configuration events" are detected.  (In response to a USB
* configuration event, the data toggle for a given endpoint is always
* reset to DATA0.)
*
* <transferType> specifies the type of transfers to be performed as
* USB_XFRTYPE_xxxx.  <direction> specifies the direction of the pipe as
* USB_DIR_xxxx.  Control pipes specify direction as USB_DIR_INOUT.
*
* The caller must be aware that not all endpoints are capable of all types
* of transfers.  Prior to assigning an endpoint for a particular purpose,
* the caller should interrogate the USB_TARG_ENDPOINT_INFO structure for
* the endpoint to ensure that it supports the indicated type of transfer.
*
* RETURNS: OK, or ERROR if unable to create pipe
*
* ERRNO:
*   S_usbTargLib_BAD_PARAM
*   S_usbTargLib_ENDPOINT_IN_USE
*   S_usbTargLib_OUT_OF_MEMORY
*   S_usbTargLib_OUT_OF_RESOURCES
*   S_usbTargLib_TCD_FAULT
*/

STATUS usbTargPipeCreate
    (
    USB_TARG_CHANNEL targChannel,	/* target channel */
    UINT16 endpointId,			/* endpoint ID to use for pipe */
    UINT16 endpointId2, 		/* needed for control pipes only */
    UINT16 endpointNum, 		/* endpoint number to assign */
    UINT16 configuration,		/* associated configuration */
    UINT16 interface,			/* associated interface */
    UINT16 transferType,		/* USB_XFRTYPE_xxxx */
    UINT16 direction,			/* USB_DIR_xxxx */
    pUSB_TARG_PIPE pPipeHandle		/* returned pipe handle */
    )

    {
    pTARG_TCD pTcd;
    pUSB_TARG_ENDPOINT_INFO pEndpoint = NULL;
    pUSB_TARG_ENDPOINT_INFO pEndpoint2 = NULL;
    pTARG_PIPE pPipe = NULL;
    UINT16 direction2 = NULL;
    STATUS status;
    UINT16 i;


    OSS_MUTEX_TAKE (targMutex, OSS_BLOCK);


    /* Validate parameters */

    if (pPipeHandle == NULL)
	status = S_usbTargLib_BAD_PARAM;
    else
	status = validateTarg (targChannel, &pTcd);


    if (transferType == USB_XFRTYPE_CONTROL)
	{
	/* Must validate both endpoints.  pEndpointId must be OUT endpoint
	 * and pEndpointId2 must be IN endpoint.
	 */

	direction = USB_DIR_OUT;
	direction2 = USB_DIR_IN;

	if ((status = validateEndpoint (pTcd, endpointId, USB_XFRTYPE_CONTROL,
	    direction, &pEndpoint)) == OK)
	    {
	    status = validateEndpoint (pTcd, endpointId2, USB_XFRTYPE_CONTROL,
		direction2, &pEndpoint2);
	    }
	}
    else
	{
	/* Validate only the first endpointId. */

	status = validateEndpoint (pTcd, endpointId, transferType,
	    direction, &pEndpoint);
	}


    if (status == OK)
	{
	/* See if the requested endpoint number is valid */

	if (endpointNum > USB_MAX_ENDPOINT_NUM)
	    status = S_usbTargLib_BAD_PARAM;
	}


    if (status == OK)
	{
	/* See if the endpointNum is in use (for the same direction) */

	for (i = 0; i < pTcd->numEndpoints; i++)
	    {
	    if ((pTcd->pEndpoints [i].flags & TCD_ENDPOINT_IN_USE) != 0 &&
		pTcd->pEndpoints [i].direction == direction &&
		pTcd->pEndpoints [i].endpointNum == endpointNum)
		{
		status = S_usbTargLib_ENDPOINT_IN_USE;
		break;
		}
	    }
	}


    if (status == OK)
	{
	/* All of the parameters check out OK.	Create a pipe to manage
	 * this endpoint. */

	if ((pPipe = OSS_CALLOC (sizeof (*pPipe))) == NULL)
	    status = S_usbTargLib_OUT_OF_MEMORY;
	else
	    {
	    /* Initialize pipe */

	    pPipe->pTcd = pTcd;
	    pPipe->configuration = configuration;
	    pPipe->interface = interface;
	    pPipe->dataToggle = USB_DATA0;

	    if (usbHandleCreate (TARG_PIPE_SIG, pPipe, &pPipe->pipeHandle) != OK)
		status = S_usbTargLib_OUT_OF_RESOURCES;
	    }
	}

    if (status == OK)
	{
	/* Ask the TCD to assign endpointId to the pipe. */

	if (usbTcdEndpointAssign (&pTcd->tcdNexus, endpointId, endpointNum,
	    configuration, interface, transferType, direction) != OK)
	    status = S_usbTargLib_TCD_FAULT;
	else
	    pPipe->pEndpoint = pEndpoint;
	}


    if (status == OK && pEndpoint2 != NULL)
	{
	/* Ask the TCD to assign endpointId2 to the pipe. */

	if (usbTcdEndpointAssign (&pTcd->tcdNexus, endpointId2, endpointNum,
	    configuration, interface, transferType, direction2) != OK)
	    status = S_usbTargLib_TCD_FAULT;
	else
	    pPipe->pEndpoint2 = pEndpoint2;
	}


    /* If we failed to create the pipe, release any partially created pipe.
     * Otherwise, link the pipe to the list of pipes on this target channel. */

    if (status != OK)
	destroyPipe (pPipe);
    else
	{
	usbListLink (&pTcd->pipes, pPipe, &pPipe->pipeLink, LINK_TAIL);
	
	*pPipeHandle = pPipe->pipeHandle;
	}


    OSS_MUTEX_RELEASE (targMutex);

    return ossStatus (status);
    }


/***************************************************************************
*
* usbTargPipeDestroy - Destroys an endpoint pipe
*
* This function tears down a pipe previously created by calling
* usbTargPipeCreate().	Any pending transfers on the pipe are canceled
* and all resources allocated to the pipe are released.
*
* RETURNS: OK, or ERROR if unable to destroy pipe.
*/

STATUS usbTargPipeDestroy
    (
    USB_TARG_PIPE pipeHandle		/* pipe to be destroyed */
    )

    {
    pTARG_PIPE pPipe;
    STATUS status;


    OSS_MUTEX_TAKE (targMutex, OSS_BLOCK);

    /* Validate parameters */

    if ((status = validatePipe (pipeHandle, &pPipe)) == OK)
	{
	/* Destroy pipe */

	destroyPipe (pPipe);
	}

    OSS_MUTEX_RELEASE (targMutex);

    return status;
    }


/***************************************************************************
*
* usbTargTransfer - Submits a USB_ERP for transfer through a pipe
*
* A client uses this function to initiate an transfer on the pipe indicated 
* by <pipeHandle>.  The transfer is described by an ERP, or endpoint request 
* packet, which must be allocated and initialized by the caller prior to 
* invoking usbdTargTransfer().
*
* The USB_ERP structure is defined in usb.h as:
*
* .CS
* typedef struct usb_bfr_list
*     {
*     UINT16 pid;
*     pUINT8 pBfr;
*     UINT32 bfrLen;
*     UINT32 actLen;
*     } USB_BFR_LIST;
*
* typedef struct usb_erp
*     {
*     LINK targLink;		    // used by usbTargLib
*     pVOID targPtr;		    // used by usbTargLib
*     LINK tcdLink;		    // used by TCD
*     pVOID tcdPtr;		    // used by TCD
*     pVOID userPtr;	
*     UINT16 erpLen;		    
*     int result;		    // returned by usbTargLib/TCD
*     ERP_CALLBACK targCallback;    // used by usbTargLib
*     ERP_CALLBACK userCallback;
*     UINT16 endpointId;	    // filled in by usbTargLib
*     UINT16 transferType;	    // filled in by usbTargLib
*     UINT16 dataToggle;	    // filled in by usbTargLib
*     UINT16 bfrCount;	    
*     USB_BFR_LIST bfrList [1];
*     } USB_ERP, *pUSB_ERP;
* .CE
*
* The length of the USB_ERP structure must be stored in <erpLen> and varies 
* depending on the number of <bfrList> elements allocated at the end of the 
* structure.  By default, the default structure contains a single <bfrList>
* element, but clients may allocate a longer structure to accommodate a larger 
* number of <bfrList> elements.  
*
* <endpointId> and <transferType> are filled in automatically 
* by the usbTargLib using values recorded when the pipe was created.  
*
* <dataToggle> is filled in automatically except for control pipes.  For these
* pipes, the caller is required to store the next data toggle as USB_DATA0
* or USB_DATA1.  The Setup packet is always a DATA0.  The first packet of the
* data phase is always DATA1, with data packets alternating thereafter, and 
* the Status phase packet is always DATA1.  When using the functions
* usbTargLibResponseSend() and usbTargLibPayloadRcv(), usbTargLib handles the
* <dataToggle> value automatically.
*
* <bfrList> is an array of buffer descriptors which describe data buffers to 
* be associated with this ERP.	If more than the one <bfrList> element is 
* required then the caller must allocate the ERP by calculating the size as 
*
* .CS
* erpLen = sizeof (USB_ERP) + (sizeof (USB_BFR_DESCR) * (bfrCount - 1))
* .CE
*
* <pid> specifies the packet type to use for the indicated buffer and is
* specified as USB_PID_xxxx.  Note that packet types are specified from the
* perspective of the host.  For example, USB_PID_IN indicates a transfer from
* the device (target) to the host.
*
* The ERP <userCallback> routine must point to a client-supplied ERP_CALLBACK
* routine.  The usbdTargTransfer() function returns as soon as the ERP has been
* successfully enqueued.  If there is a failure in delivering the ERP to the
* TCD, then usbdTargTransfer() returns an error.  The actual result of the ERP
* should be checked after the <userCallback> routine has been invoked.
*
* RETURNS: OK, or ERROR if unable to submit USB_ERP for execution
*
* ERRNO:
*   S_usbTargLib_BAD_PARAM
*   S_usbTargLib_TCD_FAULT
*/

STATUS usbTargTransfer
    (
    USB_TARG_PIPE pipeHandle,		/* pipe for transfer */
    pUSB_ERP pErp			/* ERP describing transfer */
    )

    {
    pTARG_PIPE pPipe;
    STATUS status;


    OSS_MUTEX_TAKE (targMutex, OSS_BLOCK);

    /* Validate parameters */

    if (pErp == NULL)
	status = S_usbTargLib_BAD_PARAM;

    else if ((status = validatePipe (pipeHandle, &pPipe)) == OK)
	{
	/* Fill in fields in ERP. */

	pErp->targPtr = pPipe;
	pErp->targCallback = usbTargErpCallback;
	pErp->transferType = pPipe->pEndpoint->transferType;


	/* If this is a control pipe and with a direction of USB_PID_IN,
	 * then use the second endpoint for the pipe.
	 */

	if (pErp->transferType == USB_XFRTYPE_CONTROL &&
	    pErp->bfrList [0].pid == USB_PID_IN)
	    pErp->endpointId = pPipe->pEndpoint2->endpointId;
	else
	    pErp->endpointId = pPipe->pEndpoint->endpointId;


	/* The caller must specify DATA0/DATA1 for control pipes */

	if (pErp->transferType != USB_XFRTYPE_CONTROL)
	    pErp->dataToggle = pPipe->dataToggle;


	usbListLink (&pPipe->erps, pErp, &pErp->targLink, LINK_HEAD);

	/* Submit ERP to TCD */

	if (usbTcdErpSubmit (&pPipe->pTcd->tcdNexus, pErp) != OK)
	    {
	    status = S_usbTargLib_TCD_FAULT;
	    usbListUnlink (&pErp->targLink);
	    }
	}


    OSS_MUTEX_RELEASE (targMutex);

    return ossStatus (status);
    }


/***************************************************************************
*
* usbTargTransferAbort - Cancels a previously submitted USB_ERP
*
* This function aborts an ERP which was previously submitted through
* a call to usbdTargTransfer().  
*
* RETURNS: OK, or ERROR if unable to cancel USB_ERP
*
* ERRNO:
*   S_usbTargLib_BAD_PARAM
*/

STATUS usbTargTransferAbort
    (
    USB_TARG_PIPE pipeHandle,		/* pipe for transfer to abort */
    pUSB_ERP pErp			/* ERP to be aborted */
    )

    {
    pTARG_PIPE pPipe;
    STATUS status;


    OSS_MUTEX_TAKE (targMutex, OSS_BLOCK);

    /* Validate parameters */

    if (pErp == NULL)
	status = S_usbTargLib_BAD_PARAM;

    else if ((status = validatePipe (pipeHandle, &pPipe)) == OK)
	{
	/* cancel the ERP */

	status = cancelErp (pPipe, pErp);
	}

    OSS_MUTEX_RELEASE (targMutex);

    return ossStatus (status);
    }


/***************************************************************************
*
* usbTargControlResponseSend - Sends a response on the control pipe
*
* usbTargLib automatically creates a pipe to manage communication on the
* default control endpoint (#0) defined by the USB.  Certain application
* callbacks (e.g., the USB_TARG_VENDOR_SPECIFIC callback) may need to
* formulate a response and send it to the host.  This function allows a
* caller to respond to a host control pipe request.
*
* There are two kinds of responses, those that involve a data phase and
* those that do not.  This function may be used to handle both types.  If
* a data phase is required, the caller passes a non-NULL <pBfr> for the 
* usbTargTransfer() function.  The usbTargControlResponseSend() function
* sends the data described by the USB_ERP and then automatically
* accepts the "status phase" transfer sent the by the host to acknowledge
* the transfer.  If there is no data phase, the <pBfr> parameter should be
* NULL, in which case usbTargLib generates just the Status phase
* automatically.
*
* The contents of the <pBfr> passed by the caller are copied into a 
* private usbTargLib buffer.  <bfrLen> must not exceed USB_MAX_DESCR_LEN.
*
* This function returns as soon as the transfer is enqueued.  
*
* RETURNS: OK, or ERROR if unable to submit response to host.
*
* ERRNO:
*   S_usbTargLib_GENERAL_FAULT
*   S_usbTargLib_BAD_PARAM
*/

STATUS usbTargControlResponseSend
    (
    USB_TARG_CHANNEL targChannel,	/* target channel */
    UINT16 bfrLen,			/* length of response, or 0 */
    pUINT8 pBfr 			/* ptr to bfr or NULL */
    )

    {
    pTARG_TCD pTcd;
    STATUS status;


    OSS_MUTEX_TAKE (targMutex, OSS_BLOCK);

    /* Validate parameters */

    if ((status = validateTarg (targChannel, &pTcd)) == OK)
	{
	/* If <pErp> is NULL, then just do a Status packet.  Otherwise,
	 * submit the caller's ERP. 
	 */

	if (pBfr == NULL)
	    {
	    if (initStatusErp (pTcd, USB_PID_IN) != OK)
		status = S_usbTargLib_GENERAL_FAULT;
	    }
	else
	    {
	    /* Transfer the caller's data. */

	    if (bfrLen > sizeof (pTcd->dataBfr))
		status = S_usbTargLib_BAD_PARAM;
	    else
		{
		memcpy (pTcd->dataBfr, pBfr, bfrLen);

		if (initDataErpForResponse (pTcd, bfrLen) != OK)
		    status = S_usbTargLib_GENERAL_FAULT;
		}
	    }
	}

    
    OSS_MUTEX_RELEASE (targMutex);

    return ossStatus (status);
    }


/***************************************************************************
*
* usbTargControlPayloadRcv - Receives data on the default control pipe
*
* usbTargLib automatically creates a pipe to manage communication on the
* default control pipe (#0) defined by the USB.  Certain application
* callbacks (e.g., USB_TARG_VENDOR_SPECIFIC or USB_TARG_DESCRIPTOR_SET)
* may need to receive additional data on the control OUT endpoint in order
* to complete processing of the control pipe request.  This function allows
* a caller to receive data on a control pipe.
*
* The <pErp> parameter must point to an ERP which will receive the
* additional data. 
*
* This function returns as soon as the USB_ERP is enqueued.  Completion of
* the USB_ERP is indicated when the ERP's callback is invoked.	After the
* ERP completes, the application should terminate the USB request by
* invoking the usbTargControlResponseSend() function with a NULL <pBfr>
* parameter.  This will direct usbTargLib to generate a Status IN phase,
* signalling the end of the transaction.
*
* NOTE: The caller must ensure that the ERP remains valid until the ERP
* userCallback has been invoked - signalling completion of the ERP.
*
* RETURNS: OK, or ERROR if unable to submit ERP to receive additional data
*/

STATUS usbTargControlPayloadRcv
    (
    USB_TARG_CHANNEL targChannel,
    pUSB_ERP pErp
    )

    {
    pTARG_TCD pTcd;
    STATUS status;


    OSS_MUTEX_TAKE (targMutex, OSS_BLOCK);

    /* Validate parameters */

    if ((status = validateTarg (targChannel, &pTcd)) == OK)
	{
	status = usbTargTransfer (pTcd->controlPipe, pErp);
	}

    
    OSS_MUTEX_RELEASE (targMutex);

    return ossStatus (status);
    }


/***************************************************************************
*
* usbTargPipeStatusSet - sets pipe stalled/unstalled status
*
* If the target application detects an error while servicing a pipe,
* including the default control pipe, it may choose to stall the endpoint(s)
* associated with that pipe.  This function allows the caller to set the
* state of a pipe as "stalled" or "un-stalled".
*
* <pipeHandle> should be the handle of a pipe to be stalled.  A NULL
* <pipeHandle> indicates the default control pipe.  <state> indicates the
* endpoint state as TCD_ENDPOINT_STALL/UNSTALL/DATA0.  
*
* Note that the default control pipe will automatically "un-stall" upon 
* receipt of a new Setup packet.
*
* RETURNS: OK, or ERROR if unable to set indicated state
*
* ERRNO:
*   S_usbTargLib_TCD_FAULT
*/

STATUS usbTargPipeStatusSet
    (
    USB_TARG_CHANNEL targChannel,
    USB_TARG_PIPE pipeHandle,
    UINT16 state
    )

    {
    pTARG_TCD pTcd;
    pTARG_PIPE pPipe;
    STATUS status;


    OSS_MUTEX_TAKE (targMutex, OSS_BLOCK);

    /* Validate parameters */

    if ((status = validateTarg (targChannel, &pTcd)) == OK)
	{
	/* Validate pipe */

	if (pipeHandle == NULL)
	    pipeHandle = pTcd->controlPipe;

	status = validatePipe (pipeHandle, &pPipe);
	}

    if (status == OK)
	{
	/* Stall endpoint(s) associated with pipe. */

	if (pPipe->pEndpoint != NULL)
	    if (usbTcdEndpointStateSet (&pTcd->tcdNexus, 
		pPipe->pEndpoint->endpointId, state) != OK)
		status = S_usbTargLib_TCD_FAULT;

	if (status == OK &&
	    pPipe->pEndpoint2 != NULL)
	    if (usbTcdEndpointStateSet (&pTcd->tcdNexus,
		pPipe->pEndpoint2->endpointId, state) != OK)
		status = S_usbTargLib_TCD_FAULT;
	}

    
    OSS_MUTEX_RELEASE (targMutex);

    return ossStatus (status);
    }


/***************************************************************************
*
* usbTargCurrentFrameGet - Retrieves the current USB frame number
*
* It is sometimes necessary for callers to retrieve the current USB frame 
* number.  This function allows a caller to retrieve the current USB frame 
* number for the bus to which <targChannel> is connected.  Upon return, the 
* current frame number is stored in <pFrameNo>.
*
* RETURNS: OK, or ERROR if unable to retrieve USB frame number
*
* ERRNO:
*   S_usbTargLib_TCD_FAULT
*/

STATUS usbTargCurrentFrameGet
    (
    USB_TARG_CHANNEL targChannel,	/* target channel */
    pUINT16 pFrameNo			/* current frame number */
    )

    {
    pTARG_TCD pTcd;
    STATUS status;

    
    OSS_MUTEX_TAKE (targMutex, OSS_BLOCK);

    /* Validate parameters */

    if ((status = validateTarg (targChannel, &pTcd)) == OK)
	{
	/* Get current frame number from TCD */

	if (usbTcdCurrentFrameGet (&pTcd->tcdNexus, pFrameNo) != OK)
	    status = S_usbTargLib_TCD_FAULT;
	}


    OSS_MUTEX_RELEASE (targMutex);

    return ossStatus (status);
    }


/***************************************************************************
*
* usbTargSignalResume - Drives RESUME signalling on USB
*
* If a USB is in the SUSPENDed state, it is possible for a device (target)
* to request the bus to wake up (called remote wakeup).  This function allows
* the caller to drive USB resume signalling.  The function will return after
* resume signalling has completed.
*
* There is no guarantee that a host will honor RESUME signalling.  Therefore,
* the caller should make no assumptions about the state of the USB after
* calling this function.  Instead, the "management callback" for this 
* target channel will be invoked if the USB changes state in response to the
* RESUME signalling.
*
* RETURNS: OK, or ERROR if unable to drive RESUME signalling
*
* ERRNO:
*   S_usbTargLib_TCD_FAULT
*/

STATUS usbTargSignalResume
    (
    USB_TARG_CHANNEL targChannel	/* target channel */
    )

    {
    pTARG_TCD pTcd;
    STATUS status;


    OSS_MUTEX_TAKE (targMutex, OSS_BLOCK);

    /* Validate parameters */

    if ((status = validateTarg (targChannel, &pTcd)) == OK)
	{
	/* Have TCD drive resume signalling */

	if (usbTcdSignalResume (&pTcd->tcdNexus) != OK)
	    status = S_usbTargLib_TCD_FAULT;
	}


    OSS_MUTEX_RELEASE (targMutex);

    return ossStatus (status);
    }


/* End of file. */

