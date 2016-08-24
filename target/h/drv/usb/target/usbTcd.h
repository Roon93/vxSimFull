/* usbTcd.h - Defines generic interface to USB target controller drivers */

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

Defines the generic interface to a USB target controller driver (TCD).
*/


#ifndef __INCusbTcdh
#define __INCusbTcdh

#ifdef	__cplusplus
extern "C" {
#endif


/* includes */

#include "usb/usbHandleLib.h"
#include "usb/usb.h"
#include "vwModNum.h"           /* USB Module Number Def's */


/* defines */

/* TRB function codes */

#define TCD_FNC_ATTACH		    0x0000  /* attach/init */
#define TCD_FNC_DETACH		    0x0001  /* detach/shutdown */
#define TCD_FNC_ENABLE		    0x0002  /* enable target channel */
#define TCD_FNC_DISABLE 	    0x0003  /* disable target channel */

#define TCD_FNC_ADDRESS_SET	    0x0100  /* set device address */
#define TCD_FNC_ENDPOINT_ASSIGN     0x0101  /* assigns an endpoint */
#define TCD_FNC_ENDPOINT_RELEASE    0x0102  /* releases an endpoint */
#define TCD_FNC_SIGNAL_RESUME	    0x0103  /* drive resume signalling */
#define TCD_FNC_ENDPOINT_STATE_SET  0x0104  /* set endpoint state */

#define TCD_FNC_CURRENT_FRAME_GET   0x0200  /* get current frame number */
#define TCD_FNC_ERP_SUBMIT	    0x0201  /* submit an ERP for exection */
#define TCD_FNC_ERP_CANCEL	    0x0202  /* cancel a pending ERP */


/* TRB result codes */

/* 
 * USB errnos are defined as being part of the USB peripheral Module, as are 
 * all vxWorks module numbers, but the USB Module number is further divided 
 * into sub-modules.  Each sub-module has upto 255 values for its own error 
 * codes
 */
 
#define USB_TCD_SUB_MODULE  2

#define M_usbTcdLib  	( (USB_TCD_SUB_MODULE  << 8) | M_usbPeriphLib )

#define tcdErr(x)	(M_usbTcdLib | (x))

#define S_usbTcdLib_BAD_PARAM		tcdErr(1)
#define S_usbTcdLib_BAD_HANDLE		tcdErr(2)
#define S_usbTcdLib_OUT_OF_MEMORY	tcdErr(3)
#define S_usbTcdLib_OUT_OF_RESOURCES	tcdErr(4) 
#define S_usbTcdLib_NOT_IMPLEMENTED	tcdErr(5)
#define S_usbTcdLib_GENERAL_FAULT	tcdErr(6)
#define S_usbTcdLib_NOT_INITIALIZED	tcdErr(7)
#define S_usbTcdLib_INT_HOOK_FAILED	tcdErr(8)
#define S_usbTcdLib_HW_NOT_READY	tcdErr(9)
#define S_usbTcdLib_NOT_SUPPORTED	tcdErr(10)
#define S_usbTcdLib_ERP_CANCELED	tcdErr(11)
#define S_usbTcdLib_CANNOT_CANCEL	tcdErr(12)
#define S_usbTcdLib_SHUTDOWN		tcdErr(13)
#define S_usbTcdLib_DATA_TOGGLE_FAULT	tcdErr(14)
#define S_usbTcdLib_PID_MISMATCH	tcdErr(15)
#define S_usbTcdLib_COMM_FAULT		tcdErr(16)
#define S_usbTcdLib_NEW_SETUP_PACKET	tcdErr(17)


/* endpoint characteristics */

#define TCD_ENDPOINT_IN_OK	0x0001	    /* host->device capable */
#define TCD_ENDPOINT_OUT_OK	0x0002	    /* device->host capable */
#define TCD_ENDPOINT_CTRL_OK	0x0010	    /* capable of control xfr */
#define TCD_ENDPOINT_BULK_OK	0x0020	    /* capable of bulk xfr */
#define TCD_ENDPOINT_INT_OK	0x0040	    /* capable of interrupt xfr */
#define TCD_ENDPOINT_ISOCH_OK	0x0080	    /* capable of isoch xfr */
#define TCD_ENDPOINT_IN_USE	0x0100	    /* endpoint has pipe */


/* managment codes passed to USB_TCD_MANAGEMENT_CALLBACK (below) */

#define TCD_MNGMT_ATTACH	1	    /* initial TCD attachment */
#define TCD_MNGMT_DETACH	2	    /* TCD detach */
#define TCD_MNGMT_VBUS_DETECT	3	    /* Vbus detected */
#define TCD_MNGMT_VBUS_LOST	4	    /* Vbus lost */
#define TCD_MNGMT_BUS_RESET	5	    /* bus reset */
#define TCD_MNGMT_SUSPEND	6	    /* suspend signalling detected */
#define TCD_MNGMT_RESUME	7	    /* resume signalling detected */


/* endpoint states */

#define TCD_ENDPOINT_STALL	0x01
#define TCD_ENDPOINT_UNSTALL	0x02
#define TCD_ENDPOINT_DATA0	0x04


/* typedefs */

typedef GENERIC_HANDLE TCD_HANDLE, *pTCD_HANDLE;


/* primary TCD entry point */

typedef STATUS (*USB_TCD_EXEC_FUNC) (pVOID pTrb);


/* management notification callback function */

typedef VOID (*USB_TCD_MNGMT_CALLBACK)
    (
    pVOID mngmtCallbackParam,		    /* caller-defined param */
    TCD_HANDLE handle,			    /* channel */
    UINT16 mngmtCode			    /* management code */
    );


/* USB_TARG_ENDPOINT_INFO 
 *
 * NOTE: The xxxMaxPacketSize fields indicate the maximum packet size which the
 * endpoint can support.  Since control pipes are always bidirectional, only a 
 * single value is given.  However, for a given endpoint in bulk, interrupt, or
 * isochronous mode, there may be additional limitations depending on how other
 * endpoints are configured, so separate IN, OUT, and INOUT figures are provided.
 * For example, if two endpoints are configured for isochronous operation at 
 * endpoint number 2 (ie., one IN and one OUT), the isochInOutMaxPacketSize figure
 * would apply.  When configuring endpoints in this way, the caller should
 * specify the direction as USB_DIR_INOUT.  The actual direction for an endpoint
 * will be determined based on its capability as indicated in the flags field.
 */

typedef struct usb_targ_endpoint_info
    {
    UINT16 endpointId;			    /* TCD-assigned endpoint ID */
    UINT16 flags;			    /* endpoint characteristics */

    UINT16 endpointNumMask;		    /* mask of allowed endpoint numbers */

    UINT32 ctlMaxPacketSize;		    /* max packet size in control mode */

    UINT32 bulkInMaxPacketSize; 	    /* max packet size in bulk IN mode */
    UINT32 bulkOutMaxPacketSize;	    /* max packet size in bulk OUT mode */
    UINT32 bulkInOutMaxPacketSize;	    /* max packet size in bulk I/O mode */

    UINT32 intInMaxPacketSize;		    /* max packet size in intrpt IN mode */
    UINT32 intOutMaxPacketSize; 	    /* max packet size in intrpt OUT mode */
    UINT32 intInOutMaxPacketSize;	    /* max packet size in intrpt I/O mode */

    UINT32 isochInMaxPacketSize;	    /* max packet size in isoch IN mode */
    UINT32 isochOutMaxPacketSize;	    /* max packet size in isoch OUT mode */
    UINT32 isochInOutMaxPacketSize;	    /* max packet size in isoch I/O mode */

    UINT16 endpointNum; 		    /* currently assigned endpoint num */
    UINT16 configuration;		    /* current configuration */
    UINT16 interface;			    /* current interface */
    UINT16 transferType;		    /* current transfer type */
    UINT16 direction;			    /* current direction */

    UINT16 maxPacketSize;		    /* max packet size as configured */

    } USB_TARG_ENDPOINT_INFO, *pUSB_TARG_ENDPOINT_INFO;


/*
 * TRB_HEADER
 *
 * All requests to a TCD begin with a TRB (TCD Request Block) header.
 */

typedef struct trb_header
    {
    TCD_HANDLE handle;		/* I/O	caller's TCD client handle */
    UINT16 function;		/* IN	TCD function code */
    UINT16 trbLength;		/* IN	Length of the total TRB */
    } TRB_HEADER, *pTRB_HEADER;


/*
 * TRB_ATTACH
 */

typedef struct trb_attach
    {
    TRB_HEADER header;		/*	TRB header */
    pVOID tcdParam;		/* IN	TCD-specific parameter */

    USB_TCD_MNGMT_CALLBACK mngmtCallback;
				/* IN	caller's management callback */
    pVOID mngmtCallbackParam;	/* IN	param to mngmt callback */

    UINT16 speed;		/* OUT	target speed: USB_SPEED_xxxx */
    UINT16 numEndpoints;	/* OUT	number of endpoints supported */
    pUSB_TARG_ENDPOINT_INFO pEndpoints;
				/* OUT	ptr to array of endpoints */
    } TRB_ATTACH, *pTRB_ATTACH;


/*
 * TRB_DETACH
 */

typedef struct trb_detach
    {
    TRB_HEADER header;		/*	TRB header */
    } TRB_DETACH, *pTRB_DETACH;


/*
 * TRB_ENABLE_DISABLE
 */

typedef struct trb_enable_disable
    {
    TRB_HEADER header;		/*	TRB header */
    } TRB_ENABLE_DISABLE, *pTRB_ENABLE_DISABLE;


/*
 * TRB_ADDRESS_SET
 */

typedef struct trb_address_set
    {
    TRB_HEADER header;		/*	TRB header */
    UINT16 deviceAddress;	/* IN	new device address */
    } TRB_ADDRESS_SET, *pTRB_ADDRESS_SET;


/*
 * TRB_SIGNAL_RESUME
 */

typedef struct trb_signal_resume
    {
    TRB_HEADER header;		/*	TRB header */
    } TRB_SIGNAL_RESUME, *pTRB_SIGNAL_RESUME;


/*
 * TRB_ENDPOINT_ASSIGN
 */

typedef struct trb_endpoint_assign
    {
    TRB_HEADER header;		/*	TRB header */
    UINT16 endpointId;		/* IN	TCD-assigned endpoint ID */
    UINT16 endpointNum; 	/* IN	newly assigned endpoint num */
    UINT16 configuration;	/* IN	new configuration */
    UINT16 interface;		/* IN	new interface */
    UINT16 transferType;	/* IN	new transfer type */
    UINT16 direction;		/* IN	new direction */
    } TRB_ENDPOINT_ASSIGN, *pTRB_ENDPOINT_ASSIGN;


/*
 * TRB_ENDPOINT_RELEASE
 */

typedef struct trb_endpoint_release
    {
    TRB_HEADER header;		/*	TRB header */
    UINT16 endpointId;		/* IN	TCD-assigned endpoint Id */
    } TRB_ENDPOINT_RELEASE, *pTRB_ENDPOINT_RELEASE;


/*
 * TRB_ENDPOINT_STATE_SET
 */

typedef struct trb_endpoint_state_set
    {
    TRB_HEADER header;		/*	TRB header */
    UINT16 endpointId;		/* IN	endpoint Id */
    UINT16 state;		/* IN	TCD_ENDPOINT_xxxx bitmask */
    } TRB_ENDPOINT_STATE_SET, *pTRB_ENDPOINT_STATE_SET;


/*
 * TRB_CURRENT_FRAME_GET
 */

typedef struct trb_current_frame_get
    {
    TRB_HEADER header;		/*	TRB header */
    UINT16 frameNo;		/* OUT	current frame number */
    } TRB_CURRENT_FRAME_GET, *pTRB_CURRENT_FRAME_GET;


/* 
 * TRB_ERP_SUBMIT
 */

typedef struct trb_erp_submit
    {
    TRB_HEADER header;		/*	TRB header */
    pUSB_ERP pErp;		/* IN	ptr to IRP */
    } TRB_ERP_SUBMIT, *pTRB_ERP_SUBMIT;


/*
 * TRB_ERP_CANCEL
 */

typedef struct trb_erp_cancel
    {
    TRB_HEADER header;		/*	TRB header */
    pUSB_ERP pErp;		/* IN	ptr to IPR to be canceled */
    } TRB_ERP_CANCEL, *pTRB_ERP_CANCEL;



#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbTcdh */


/* End of file. */

