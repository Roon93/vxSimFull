/* usbTargLib.h - Defines interface to usbTargLib.c */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01b,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce
01d,07may01,wef  changed module number to be (module sub num << 8) | 
                 M_usbPeriphLib
01c,02may01,wef  changed module number to be M_<module> + M_usbPeriphLib
01b,05dec00,wef  moved Module number defs to vwModNum.h - add this
                 to #includes
01a,04aug99,rcb  First.
*/

/*
DESCRIPTION

Defines the interface to usbTargLib.c.	usbTargLib.c implements a hardware-
independent interface to USB target controller drivers.
*/

#ifndef __INCusbTargLibh
#define __INCusbTargLibh

#ifdef	__cplusplus
extern "C" {
#endif


/* includes */

#include "usb/usbHandleLib.h"
#include "usb/usb.h"
#include "drv/usb/target/usbTcd.h"
#include "vwModNum.h"           /* USB Module number def's */


/* defines */

#define USBT_NAME_LEN		    32	    /* max usbTargLib string len */


/* usbTargLib error codes */

/* 
 * USB errnos are defined as being part of the USB peripheral Module, as are 
 * all vxWorks module numbers, but the USB Module number is further divided 
 * into sub-modules.  Each sub-module has upto 255 values for its own error 
 * codes
 */
 
#define USB_TARG_SUB_MODULE  1

#define M_usbTargLib 	( (USB_TARG_SUB_MODULE  << 8) | M_usbPeriphLib )

#define usbTargErr(x)	(M_usbTargLib | (x))

#define S_usbTargLib_BAD_PARAM		usbTargErr(1)
#define S_usbTargLib_BAD_HANDLE 	usbTargErr(2)
#define S_usbTargLib_OUT_OF_MEMORY	usbTargErr(3)
#define S_usbTargLib_OUT_OF_RESOURCES	usbTargErr(4) 
#define S_usbTargLib_NOT_IMPLEMENTED	usbTargErr(5)
#define S_usbTargLib_GENERAL_FAULT	usbTargErr(6)
#define S_usbTargLib_NOT_INITIALIZED	usbTargErr(7)
#define S_usbTargLib_CANNOT_CANCEL	usbTargErr(8)
#define S_usbTargLib_TCD_FAULT		usbTargErr(9)
#define S_usbTargLib_ENDPOINT_IN_USE	usbTargErr(10)
#define S_usbTargLib_APP_FAULT		usbTargErr(11)


/* typedefs */

typedef GENERIC_HANDLE USB_TARG_CHANNEL, *pUSB_TARG_CHANNEL;
typedef GENERIC_HANDLE USB_TARG_DESCR, *pUSB_TARG_DESCR;
typedef GENERIC_HANDLE USB_TARG_PIPE, *pUSB_TARG_PIPE;


/* USB_TARG_INFO */

typedef struct usb_targ_info
    {
    UINT16 speed;			    /* target speed: USB_SPEED_xxxx */

    UINT16 numEndpoints;		    /* count of endpoints to follow */
    pUSB_TARG_ENDPOINT_INFO pEndpoints;     /* pointer to array of endpoints */

    } USB_TARG_INFO, *pUSB_TARG_INFO;


/* caller-supplied target callbacks */

/* target management callback */

typedef STATUS (*USB_TARG_MANAGEMENT_FUNC)
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT16 mngmtCode			    /* management code */
    );


/* control pipe callbacks */

typedef STATUS (*USB_TARG_FEATURE_CLEAR_FUNC)
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT8 requestType,
    UINT16 feature,
    UINT16 index
    );

typedef STATUS (*USB_TARG_FEATURE_SET_FUNC)
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT8 requestType,
    UINT16 feature,
    UINT16 index
    );

typedef STATUS (*USB_TARG_CONFIGURATION_GET_FUNC)
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    pUINT8 pConfiguration
    );

typedef STATUS (*USB_TARG_CONFIGURATION_SET_FUNC)
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT8 configuration
    );

typedef STATUS (*USB_TARG_DESCRIPTOR_GET_FUNC)
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

typedef STATUS (*USB_TARG_DESCRIPTOR_SET_FUNC)
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT8 requestType,
    UINT8 descriptorType,
    UINT8 descriptorIndex,
    UINT16 languageId,
    UINT16 length
    );

typedef STATUS (*USB_TARG_INTERFACE_GET_FUNC)
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT16 interfaceIndex,
    pUINT8 pAlternateSetting
    );

typedef STATUS (*USB_TARG_INTERFACE_SET_FUNC)
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT16 interfaceIndex,
    UINT8 alternateSetting
    );

typedef STATUS (*USB_TARG_STATUS_GET_FUNC)
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT16 requestType,
    UINT16 index,
    UINT16 length,
    pUINT8 pBfr,
    pUINT16 pActLen
    );

typedef STATUS (*USB_TARG_ADDRESS_SET_FUNC)
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT16 deviceAddress
    );

typedef STATUS (*USB_TARG_SYNCH_FRAME_GET_FUNC)
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT16 endpoint,
    pUINT16 pFrameNo
    );

typedef STATUS (*USB_TARG_VENDOR_SPECIFIC_FUNC)
    (
    pVOID param,
    USB_TARG_CHANNEL targChannel,
    UINT8 requestType,
    UINT8 request,
    UINT16 value,
    UINT16 index,
    UINT16 length
    );


typedef struct usb_targ_callback_table
    {
    /* device management callbacks */

    USB_TARG_MANAGEMENT_FUNC mngmtFunc;

    /* Control pipe callbacks */

    USB_TARG_FEATURE_CLEAR_FUNC featureClear;
    USB_TARG_FEATURE_SET_FUNC featureSet;
    USB_TARG_CONFIGURATION_GET_FUNC configurationGet;
    USB_TARG_CONFIGURATION_SET_FUNC configurationSet;
    USB_TARG_DESCRIPTOR_GET_FUNC descriptorGet;
    USB_TARG_DESCRIPTOR_SET_FUNC descriptorSet;
    USB_TARG_INTERFACE_GET_FUNC interfaceGet;
    USB_TARG_INTERFACE_SET_FUNC interfaceSet;
    USB_TARG_STATUS_GET_FUNC statusGet;
    USB_TARG_ADDRESS_SET_FUNC addressSet;
    USB_TARG_SYNCH_FRAME_GET_FUNC synchFrameGet;
    USB_TARG_VENDOR_SPECIFIC_FUNC vendorSpecific;

    } USB_TARG_CALLBACK_TABLE, *pUSB_TARG_CALLBACK_TABLE;


/* function prototypes */

STATUS usbTargVersionGet
    (
    pUINT16 pVersion,			/* usbTargLib version */
    pCHAR pMfg				/* usbTargLib manufacturer */
    );


STATUS usbTargInitialize (void);


STATUS usbTargShutdown (void);


STATUS usbTargTcdAttach
    (
    USB_TCD_EXEC_FUNC tcdExecFunc,	/* TCD entry point */
    pVOID tcdParam,			/* TCD parameter */
    pUSB_TARG_CALLBACK_TABLE pCallbacks,/* caller-supplied callbacks */
    pVOID callbackParam,		/* caller-defined callback param */
    pUSB_TARG_CHANNEL pTargChannel,	/* handle to target on return */
    pUINT16 pNumEndpoints,		/* bfr to receive nbr of endpoints */
    pUSB_TARG_ENDPOINT_INFO *ppEndpoints/* bfr to rcv ptr to endpt array */
    );


STATUS usbTargTcdDetach
    (
    USB_TARG_CHANNEL targChannel	/* target to detach */
    );


STATUS usbTargEndpointInfoGet
    (
    USB_TARG_CHANNEL targChannel,	/* target channel */
    pUINT16 pNumEndpoints,		/* receives nbr of endpoints */
    pUSB_TARG_ENDPOINT_INFO *ppEndpoints/* receives ptr to array */
    );


STATUS usbTargEnable
    (
    USB_TARG_CHANNEL targChannel	/* target to enable */
    );


STATUS usbTargDisable
    (
    USB_TARG_CHANNEL targChannel	/* target to disable */
    );


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
    );


STATUS usbTargPipeDestroy
    (
    USB_TARG_PIPE pipeHandle		/* pipe to be destroyed */
    );


STATUS usbTargTransfer
    (
    USB_TARG_PIPE pipeHandle,		/* pipe for transfer */
    pUSB_ERP pErp			/* ERP describing transfer */
    );


STATUS usbTargTransferAbort
    (
    USB_TARG_PIPE pipeHandle,		/* pipe for transfer to abort */
    pUSB_ERP pErp			/* ERP to be aborted */
    );


STATUS usbTargControlResponseSend
    (
    USB_TARG_CHANNEL targChannel,	/* target channel */
    UINT16 bfrLen,			/* length of response, or 0 */
    pUINT8 pBfr 			/* ptr to bfr or NULL */
    );


STATUS usbTargControlPayloadRcv
    (
    USB_TARG_CHANNEL targChannel,	/* target channel */
    pUSB_ERP pErp			/* ERP to receive control OUT data */
    );


STATUS usbTargPipeStatusSet
    (
    USB_TARG_CHANNEL targChannel,	/* target channel */
    USB_TARG_PIPE pipeHandle,		/* pipe handle or NULL for ctrl pipe */
    UINT16 state			/* USBT_PIPE_STALLED/UNSTALLED */
    );


STATUS usbTargCurrentFrameGet
    (
    USB_TARG_CHANNEL targChannel,	/* target channel */
    pUINT16 pFrameNo			/* current frame number */
    );	


STATUS usbTargSignalResume
    (
    USB_TARG_CHANNEL targChannel	/* target channel */
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbTargLibh */


/* End of file. */

