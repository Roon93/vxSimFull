/* usbTcdLib.h - TCD functional API */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01a,09aug99,rcb  First.
*/

/*
DESCRIPTION

This file defines a functional interface to the TCD.
*/

#ifndef __INCusbTcdLibh
#define __INCusbTcdLibh

#ifdef	__cplusplus
extern "C" {
#endif


/* includes */

#include "drv/usb/target/usbTcd.h"


/* defines */


/* typedefs */

/*
 * TCD_NEXUS
 *
 * TCD_NEXUS contains the entry point and TCD_CLIENT_HANDLE needed by a
 * TCD caller to invoke a TCD.
 */

typedef struct tcd_nexus
    {
    USB_TCD_EXEC_FUNC tcdExecFunc;  /* TCD primary entry point */
    TCD_HANDLE handle;		    /* client's handle with TCD */
    } TCD_NEXUS, *pTCD_NEXUS;


/* functions */

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
    );


STATUS usbTcdDetach
    (
    pTCD_NEXUS pNexus		    /* client's nexus */
    );


STATUS usbTcdEnable
    (
    pTCD_NEXUS pNexus		    /* client's nexus */
    );


STATUS usbTcdDisable
    (
    pTCD_NEXUS pNexus		    /* client's nexus */
    );


STATUS usbTcdAddressSet
    (
    pTCD_NEXUS pNexus,		    /* client's nexus */
    UINT16 deviceAddress	    /* new address for target */
    );


STATUS usbTcdSignalResume
    (
    pTCD_NEXUS pNexus		    /* client's nexus */
    );


STATUS usbTcdEndpointAssign
    (
    pTCD_NEXUS pNexus,		    /* client's nexus */
    UINT16 endpointId,		    /* TCD-assigned endpoint ID */
    UINT16 endpointNum, 	    /* endpoint number to be assigned */
    UINT16 configuration,	    /* configuration associated with endpoint */
    UINT16 interface,		    /* interface associated with endpoint */
    UINT16 transferType,	    /* transfer type for endpoint */
    UINT16 direction		    /* direction for endpoint */
    );


STATUS usbTcdEndpointRelease
    (
    pTCD_NEXUS pNexus,		    /* client's nexus */
    UINT16 endpointId		    /* endpointId to release */
    );


STATUS usbTcdEndpointStateSet
    (
    pTCD_NEXUS pNexus,		    /* client's nexus */
    UINT16 endpointId,		    /* endpointId */
    UINT16 state		    /* TCD_ENDPOINT_STALL/UNSTALL */
    );


STATUS usbTcdCurrentFrameGet
    (
    pTCD_NEXUS pNexus,		    /* client's nexus */
    pUINT16 pFrameNo		    /* current frame number */
    );


STATUS usbTcdErpSubmit
    (
    pTCD_NEXUS pNexus,		    /* client's nexus */
    pUSB_ERP pErp		    /* ERP to be executed */
    );


STATUS usbTcdErpCancel
    (
    pTCD_NEXUS pNexus,		    /* client's nexus */
    pUSB_ERP pErp		    /* ERP to be canceled */
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbTcdLibh */


/* End of file. */

