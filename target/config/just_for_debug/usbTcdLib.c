/* usbTcdLib.c - TCD functional API */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01b,23nov99,rcb  Change #include ../xxx references to lower case.
01a,09aug99,rcb  First.
*/

/*
DESCRIPTION

This file defines a generic functional interface to a USB TCD (Target
Controller Driver).  USB TCDs export a single public entry point, their
USB_TCD_EXEC_FUNC.  To this entry point the caller passes a USB_TRB, or
TCD Request Block, which contains a function code identifying a task to
be performed by the TCD and which contains any parameters required for 
the specified function.

This library provides a functional API which to the TCD which hides the
underlying USB_TRBs from the caller.  This simplifies the caller's use
of the TCD. 

Normally, this library is used only by usbTargLib.  Other modules should
have no need to call this library directly.
*/


/* includes */

#include "usb/usbPlatform.h"

#include "string.h"

#include "usb/ossLib.h"
#include "drv/usb/target/usbTcd.h"
#include "usb/target/usbTcdLib.h"


/* functions */

/***************************************************************************
*
* trbInit - Initialize a TCD request block
*
* RETURNS: N/A
*/

LOCAL VOID trbInit 
    (
    pTRB_HEADER pTrb, 
    pTCD_NEXUS pNexus, 
    UINT16 function,
    UINT16 totalLen
    )

    {
    memset (pTrb, 0, totalLen);

    if (pNexus != NULL)
	pTrb->handle = pNexus->handle;

    pTrb->function = function;
    pTrb->trbLength = totalLen;
    }


/***************************************************************************
*
* usbTcdAttach - Attach/initialize a TCD
*
* RETURNS: OK, or ERROR if unable to attach TCD
*
* ERRNO:
*   S_usbTcdLib_BAD_PARAM
*/

STATUS usbTcdAttach
    (
    USB_TCD_EXEC_FUNC tcdExecFunc,  /* TCD's primary entry point */
    pVOID tcdParam,		    /* TCD-specific param */
    pTCD_NEXUS pNexus,		    /* nexus will be initialized on return */
    USB_TCD_MNGMT_CALLBACK mngmtCallback, /* caller's management callback */
    pVOID mngmtCallbackParam,	    /* caller-defined mngmt callback param */
    pUINT16 pSpeed,		    /* bfr to receive target's speed */
    pUINT16 pNumEndpoints,	    /* bfr to receive nbr of endpoints */
    pUSB_TARG_ENDPOINT_INFO *ppEndpoints /* bfr to receive ptr to endpt tbl */
    )

    {
    TRB_ATTACH trb;
    STATUS status;

    /* validate parameters */

    if (tcdExecFunc == NULL || pNexus == NULL || mngmtCallback == NULL ||
	pSpeed == NULL || pNumEndpoints == NULL || ppEndpoints == NULL)
	return ossStatus (S_usbTcdLib_BAD_PARAM);

    /* Initialize/execute TRB */

    trbInit (&trb.header, NULL, TCD_FNC_ATTACH, sizeof (trb));
    trb.tcdParam = tcdParam;
    trb.mngmtCallback = mngmtCallback;
    trb.mngmtCallbackParam = mngmtCallbackParam;

    status = (*tcdExecFunc) (&trb);

    /* return results */

    pNexus->tcdExecFunc = tcdExecFunc;
    pNexus->handle = trb.header.handle;

    *pSpeed = trb.speed;
    *pNumEndpoints = trb.numEndpoints;
    *ppEndpoints = trb.pEndpoints;

    return status;
    }


/***************************************************************************
*
* usbTcdDetach - Detaches/shuts down TCD
*
* RETURNS: OK, or ERROR if unable to detach TCD
*/

STATUS usbTcdDetach
    (
    pTCD_NEXUS pNexus		    /* client's nexus */
    )

    {
    TRB_DETACH trb;

    /* initialize/execute trb */

    trbInit (&trb.header, pNexus, TCD_FNC_DETACH, sizeof (trb));

    return (*pNexus->tcdExecFunc) (&trb);
    }


/***************************************************************************
*
* usbTcdEnable - Enables TCD
*
* RETURNS: OK, or ERROR if unable to enable TCD
*/

STATUS usbTcdEnable
    (
    pTCD_NEXUS pNexus		    /* client's nexus */
    )

    {
    TRB_ENABLE_DISABLE trb;

    /* initialize/execute trb */

    trbInit (&trb.header, pNexus, TCD_FNC_ENABLE, sizeof (trb));

    return (*pNexus->tcdExecFunc) (&trb);
    }


/***************************************************************************
*
* usbTcdDisable - Disables TCD
*
* RETURNS: OK, or ERROR if unable to disable TCD
*/

STATUS usbTcdDisable
    (
    pTCD_NEXUS pNexus		    /* client's nexus */
    )

    {
    TRB_ENABLE_DISABLE trb;

    /* initialize/execute trb */

    trbInit (&trb.header, pNexus, TCD_FNC_DISABLE, sizeof (trb));

    return (*pNexus->tcdExecFunc) (&trb);
    }


/***************************************************************************
*
* usbTcdAddressSet - Sets device address on the USB
*
* RETURNS: OK, or ERROR if unable to set USB address
*/

STATUS usbTcdAddressSet
    (
    pTCD_NEXUS pNexus,		    /* client's nexus */
    UINT16 deviceAddress	    /* new address for target */
    )

    {
    TRB_ADDRESS_SET trb;

    /* initialize/execute trb */

    trbInit (&trb.header, pNexus, TCD_FNC_ADDRESS_SET, sizeof (trb));
    trb.deviceAddress = deviceAddress;

    return (*pNexus->tcdExecFunc) (&trb);
    }


/***************************************************************************
*
* usbTcdSignalResume - Drives USB RESUME signalling
*
* RETURNS: OK, or ERROR if unable to drive RESUME signalling
*/

STATUS usbTcdSignalResume
    (
    pTCD_NEXUS pNexus		    /* client's nexus */
    )

    {
    TRB_SIGNAL_RESUME trb;

    /* initialize/execute trb */

    trbInit (&trb.header, pNexus, TCD_FNC_SIGNAL_RESUME, sizeof (trb));

    return (*pNexus->tcdExecFunc) (&trb);
    }


/***************************************************************************
*
* usbTcdEndpointAssign - Assigns an endpoint for subsequent data transfer
*
* RETURNS: OK, or ERROR if unable to assign endpoint.
*/

STATUS usbTcdEndpointAssign
    (
    pTCD_NEXUS pNexus,		    /* client's nexus */
    UINT16 endpointId,		    /* TCD-assigned endpoint ID */
    UINT16 endpointNum, 	    /* endpoint number to be assigned */
    UINT16 configuration,	    /* configuration associated with endpoint */
    UINT16 interface,		    /* interface associated with endpoint */
    UINT16 transferType,	    /* transfer type for endpoint */
    UINT16 direction		    /* direction for endpoint */
    )

    {
    TRB_ENDPOINT_ASSIGN trb;

    /* initialize/execute TRB */

    trbInit (&trb.header, pNexus, TCD_FNC_ENDPOINT_ASSIGN, sizeof (trb));
    trb.endpointId = endpointId;
    trb.endpointNum = endpointNum;
    trb.configuration = configuration;
    trb.interface = interface;
    trb.transferType = transferType;
    trb.direction = direction;

    return (*pNexus->tcdExecFunc) (&trb);
    }


/***************************************************************************
*
* usbTcdEndpointRelease - Release a previously assigned endpoint
*
* RETURNS: OK, or ERROR if unable to release endpoint
*/

STATUS usbTcdEndpointRelease
    (
    pTCD_NEXUS pNexus,		    /* client's nexus */
    UINT16 endpointId		    /* endpointId to release */
    )

    {
    TRB_ENDPOINT_RELEASE trb;

    /* initialize/execute TRB */

    trbInit (&trb.header, pNexus, TCD_FNC_ENDPOINT_RELEASE, sizeof (trb));
    trb.endpointId = endpointId;

    return (*pNexus->tcdExecFunc) (&trb);
    }


/***************************************************************************
*
* usbTcdEndpointStateSet - Sets endpoint stall/unstall state
*
* RETURNS: OK, or ERROR if unable to set endpoint state
*/

STATUS usbTcdEndpointStateSet
    (
    pTCD_NEXUS pNexus,		    /* client's nexus */
    UINT16 endpointId,		    /* endpointId */
    UINT16 state		    /* TCD_ENDPOINT_STALL/UNSTALL */
    )

    {
    TRB_ENDPOINT_STATE_SET trb;

    /* initialize/execute TRB */

    trbInit (&trb.header, pNexus, TCD_FNC_ENDPOINT_STATE_SET, sizeof (trb));
    trb.endpointId = endpointId;
    trb.state = state;

    return (*pNexus->tcdExecFunc) (&trb);
    }


/***************************************************************************
*
* usbTcdCurrentFrameGet - Gets current USB frame number
*
* RETURNS: OK, or ERROR if unable to get USB frame number
*/

STATUS usbTcdCurrentFrameGet
    (
    pTCD_NEXUS pNexus,		    /* client's nexus */
    pUINT16 pFrameNo		    /* current frame number */
    )

    {
    TRB_CURRENT_FRAME_GET trb;
    STATUS status;

    /* initialize/execute TRB */

    trbInit (&trb.header, pNexus, TCD_FNC_CURRENT_FRAME_GET, sizeof (trb));

    status = (*pNexus->tcdExecFunc) (&trb);

    /* return results */

    if (pFrameNo != NULL)
	*pFrameNo = trb.frameNo;

    return status;
    }


/***************************************************************************
*
* usbTcdErpSubmit - Submits a USB_ERP for execution
*
* RETURNS: OK, or ERROR if unable to submit ERP
*/

STATUS usbTcdErpSubmit
    (
    pTCD_NEXUS pNexus,		    /* client's nexus */
    pUSB_ERP pErp		    /* ERP to be executed */
    )

    {
    TRB_ERP_SUBMIT trb;

    /* initialize/execute TRB */

    trbInit (&trb.header, pNexus, TCD_FNC_ERP_SUBMIT, sizeof (trb));
    trb.pErp = pErp;

    return (*pNexus->tcdExecFunc) (&trb);
    }


/***************************************************************************
*
* usbTcdErpCancel - Cancels a USB_ERP
*
* RETURNS: OK, or ERROR if unable to cancel ERP
*/

STATUS usbTcdErpCancel
    (
    pTCD_NEXUS pNexus,		    /* client's nexus */
    pUSB_ERP pErp		    /* ERP to be canceled */
    )

    {
    TRB_ERP_CANCEL trb;

    /* initialize/execute TRB */

    trbInit (&trb.header, pNexus, TCD_FNC_ERP_CANCEL, sizeof (trb));
    trb.pErp = pErp;

    return (*pNexus->tcdExecFunc) (&trb);
    }


/* End of file. */

