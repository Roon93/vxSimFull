/* usbNC1080End.c - NC1080 Turbo connect Network Interface driver */

/* Copyright 2000-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,15oct01,wef  fix SPR 70953 - fixes man page generation and 70716 - fixes
		 coding convention and some warnings
01f,08aug01,dat  Removing warnings
01e,08aug01,dat  Removing compiler warnings
01d,31jul01,wef  fixed comments for man page genereation
01c,03may01,wef  changed pUsbListDev to USB_NC1080_DEV, moved attach_request 
		 definition to .c file from .h file.
01b,09apr01,bri  modified for Dynamic Memory allocation, hot plugging 
		 and multiple devices.
01a,28nov00,bri  Created. Adapted from templateEnd.c.
*/

/*
DESCRIPTION

NC1080 Turbo connect device is a USB Host to Host communications device.
This device consists of two SIEs to interface two hosts simultaneously, 
bidirectional FIFOs and an EEPROM interface. EEPROM interface provides the 
necessary initialization details for the device. For each host, this device 
offers a Pair of BULK IN and BULK OUT end points having 256 byte fifo each, per 
direction. Additionally, it offers a Pair of mailbox end points and a status end 
point for each host. mail box end points are intended for additional data 
communication. Status endpoint (interrupt) informs the host of any data in its 
Fifo. It has the remote wakeup signalling capability, if any data becomes 
available for a host.

As such the device is a simple data transfer device. It communicates with any 
other device that understands the protocol used by the driver. The protocol is 
introduced  by the driver. The same protocol is being followed by all drivers in 
all OSs. The device will be viewed as a Network device and data will be 
packetized in Ethernet frames. In addition, the ethernet frames are packetized 
in another protocol and sent on the Bulk out pipe. This protocol is described 
underneath.

This driver uses only the Bulk End points for the communcation. The remaining 
end points are ignored. 

The driver is an VxWorks END driver for the device which also incorporates the 
NC1080 protocol. Then this driver uses the Bulk In and Bulk out end points for 
communication over USB. 

USB relies on the HC interrupt for all its activities. This means that there is 
no Polled mode for a USB device. So the NC1080 End driver doesn't support 
polled mode functions.

The NC1080 device is a back-to-back communication device. So the usbNC1080 end 
driver doesn't support broadcast/multicast functionality.

Since the device by itself is not a Network device, it doesn't have any MAC 
address. It is expected that the User passes the MAC address via the 
initialization string to endLoad(). The initilization string format is described 
in sysNC1080End.c.

INITIALIZATION

The driver is expected to be initialized via usbNC1080DrvInit() before the endLoad 
is called. It is required that the device be recognized as attached by the 
HCD/USBD and attach callback be exceuted, before the endLoad() for the usbNC1080 
driver is called as a part of the system wide network initialization. 

PROTOCOL

NC1080 protocol:  ethernet framing, and only use bulk endpoints (01/81);
not mailboxes 02/82 or status interrupt 83).  Expects Ethernet bridging.
Odd USB length == always short read. the data needs to be packetized before 
sending to the Netchip1080 device for transmission. Also the received data
has to be de-packetized before we send it up. The packetizing protocol is

  -------------------------------------
  |header|    data (payload)   |footer|
  -------------------------------------
	- netchip_header
	- payload, in Ethernet framing (14 byte header etc)
	- (optional padding byte, if needed so that length is odd)
	- netchip_footer
*/

/* includes */

#include "vxWorks.h"
#include "stdlib.h"
#include "cacheLib.h"
#include "intLib.h"
#include "end.h"		/* Common END structures. */
#include "endLib.h"
#include "lstLib.h"		/* Needed to maintain protocol list. */
#include "wdLib.h"
#include "iv.h"
#include "semLib.h"
#include "etherLib.h"
#include "logLib.h"
#include "netLib.h"
#include "stdio.h"
#include "sysLib.h"
#include "errno.h"
#include "errnoLib.h"
#include "memLib.h"
#include "iosLib.h"
#undef	ETHER_MAP_IP_MULTICAST
#include "etherMultiLib.h"		/* multicast stuff. */

#include "net/mbuf.h"
#include "net/unixLib.h"
#include "net/protosw.h"
#include "net/systm.h"
#include "net/if_subr.h"
#include "net/route.h"

#include "sys/socket.h"
#include "sys/ioctl.h"
#include "sys/times.h"

#include "usb/usbPlatform.h"
#include "usb/ossLib.h" 	/* operations system srvcs */
#include "usb/usb.h"		/* general USB definitions */
#include "usb/usbListLib.h"	/* linked list functions */
#include "usb/usbdLib.h"	/* USBD interface */
#include "usb/usbLib.h" 	/* USB utility functions */

#include "drv/usb/usbNC1080End.h"	/* device header file*/

#define	NETCHIP_PAD_BYTE		((UINT8)0xAC)
#define NETCHIP_MIN_HEADER		6

/*
 * This will only work if there is only a single unit, for multiple
 * unit device drivers these should be integrated into the END_DEVICE
 * structure.
 */

M_CL_CONFIG usbNC1080MclBlkConfig = 	/* network mbuf configuration table */
    {
    /*
    no. mBlks		no. clBlks	memArea		memSize
    -----------		----------	-------		-------
    */
    0, 			0, 		NULL, 		0
    };

CL_DESC usbNC1080ClDescTbl [] = 	/* network cluster pool configuration table */
    {
    /*
    clusterSize			num	memArea		memSize
    -----------			----	-------		-------
    */
    {NETCHIP_MTU + EH_SIZE + 2,	0,	NULL,		0}
    };

int usbNC1080ClDescTblNumEnt = (NELEMENTS(usbNC1080ClDescTbl));

NET_POOL usbNC1080CmpNetPool;

#define NETCHIP_MIN_FBUF	(NETCHIP_BUFSIZ) /* min first buffer size */

/* DEBUG MACROS */
#define DEBUG

#ifdef DEBUG
#   define LOGMSG(x,a,b,c,d,e,f) \
	if (endDebug) \
	    { \
	    logMsg (x,a,b,c,d,e,f); \
	    }
#else
#   define LOGMSG(x,a,b,c,d,e,f)
#endif /* ENDDEBUG */

#define DRV_DBG

#ifdef	DRV_DBG
#define DRV_DBG_OFF		0x0000
#define DRV_DBG_RX		0x0001
#define	DRV_DBG_TX		0x0002
#define DRV_DBG_ATTACH		0x0004
#define DRV_DBG_INIT		0x0008
#define DRV_DBG_REGISTER	0x1000
#define	DRV_DBG_LOAD		0x0020
#define	DRV_DBG_IOCTL		0x0040
#define	DRV_DBG_RESET		0x1000
#define DRV_DBG_START		0x0100
#define DRV_DBG_PARSE		0x20000
#define DRV_DBG_UNLOAD		0x40000

int	usbNC1080Debug = 0;	 /* no debug msgs, by default */

#define DRV_LOG(FLG, X0, X1, X2, X3, X4, X5, X6)                        \
	if (usbNC1080Debug & FLG)                                        \
            logMsg(X0, X1, X2, X3, X4, X5, X6);

void DRV_DESC_PRINT(int flg, char * str, UINT8 * ptr, UINT16 len)
    {
    if (usbNC1080Debug & flg)
        {
	int i = 0;
	printf("%s : %d bytes\n",str,len);
	for(i = 0; i < len; i++)
	    {
	    printf("%x ", *ptr++);
	    if ((i+1)%16 == 0)
		printf("\n");
	    }
	printf("\n");
        }
    }

#else /*DRV_DBG*/

#define DRV_LOG(DBG_SW, X0, X1, X2, X3, X4, X5, X6)
#define DRV_DESC_PRINT(DBG_SW,X,Y,Z)

#endif /*DRV_DBG*/

#define PKT_DBG

#ifdef PKT_DBG

#define PKT_DBG_TX	((UINT8)0x01)
#define PKT_DBG_RX	((UINT8)0x02)

UINT8 pktDebug = 0; /*(PKT_DBG_TX | PKT_DBG_RX); */

void PKT_PRINT(int flg, UINT8 *pkt, UINT16 len)
    {

    NC_HEADER * pHeader = (NC_HEADER *)pkt;
    NC_FOOTER * pFooter;

    int i =0;

    if (!(flg & pktDebug))
	return;

    if( flg == PKT_DBG_TX)
	printf(" Tx Packet : \n");
    else
	printf(" Rx Packet : \n");
 
    printf("Header : hdr_len : %d  pkt_len : %d pkt_id : %x \n",
		pHeader->hdr_len, pHeader->packet_len, pHeader->packet_id);
    
    printf(" Data :\n");

    for (i=0; i< pHeader->packet_len;i++)
	{
	printf("%x ",*(pkt+pHeader->hdr_len+i));
	if (((i+1) % 16) == 0)
	    printf("\n");
	}
        printf("\n");

    if (((pHeader->packet_len + sizeof (*pFooter)) & 0x01) == 0)
	{
	printf("Pad Byte : %x \n",*(pkt+pHeader->hdr_len+pHeader->packet_len));
	pFooter = (NC_FOOTER *)(pkt+pHeader->hdr_len+pHeader->packet_len + 1);
	}
    else
	{
	pFooter = (NC_FOOTER *)(pkt+pHeader->hdr_len+pHeader->packet_len);
	}
    
    printf("Footer : pkt_id : %x \n",pFooter->packet_id);

    }

#else

#define PKT_PRINT( X, Y, Z)

#endif
    
/* LOCALS */

LOCAL USBD_CLIENT_HANDLE usbNC1080Handle; /* our USBD client handle */

LOCAL UINT16 initCount = 0;	    	    /* Count of init nesting */

LOCAL MUTEX_HANDLE usbNC1080Mutex;
LOCAL MUTEX_HANDLE usbNC1080TxMutex;
LOCAL MUTEX_HANDLE usbNC1080RxMutex;

LOCAL SEM_HANDLE   usbNC1080IrpSem;       /* Semaphore for IRP Synchronisation */

LOCAL LIST_HEAD usbNC1080DevList;	    	/* linked list of Device Structs */
LOCAL LIST_HEAD    	reqList;            /* Attach callback request list */		

LOCAL const struct product
    {
    char	*name;
    UINT16	idVendor;
    UINT16	idProduct;
} products [] = {
	{ "NC1080 TurboCONNECT", 0x0525, 0x1080 },	/* NC1080 */
	{ 0, 0, 0 },					            /* END */
};

typedef struct attach_request
    {
    LINK reqLink;                          /* linked list of requests */
    USB_NETCHIP_ATTACH_CALLBACK callback;  /* client callback routine */
    pVOID callbackArg;                     /* client callback argument*/
    } ATTACH_REQUEST, *pATTACH_REQUEST;    /* added for multiple devices */


/* externally visible interfaces. */

END_OBJ * 	usbNC1080Load (char* initString);     /* the endLoad ( ) */
STATUS          usbNC1080DrvInit ( );             /* Initialization */
STATUS usbNC1080DynamicAttachRegister (USB_NETCHIP_ATTACH_CALLBACK callback,
										pVOID arg);
STATUS usbNC1080DynamicAttachRegister (USB_NETCHIP_ATTACH_CALLBACK callback,	
										pVOID arg);
STATUS usbNC1080DevLock (USBD_NODE_ID nodeId);
STATUS usbNC1080DevUnlock (USBD_NODE_ID nodeId);



/* END Specific interfaces. */

IMPORT	int     endMultiLstCnt (END_OBJ* pEnd);

/* forward static functions */

LOCAL STATUS	usbNC1080Start	(NC1080_END_DEV * pDrvCtrl);
LOCAL STATUS	usbNC1080Stop	    (NC1080_END_DEV * pDrvCtrl);
LOCAL int	    usbNC1080Ioctl    (NC1080_END_DEV * pDrvCtrl, int cmd, caddr_t data);
LOCAL STATUS	usbNC1080Unload	(NC1080_END_DEV * pDrvCtrl);
LOCAL STATUS	usbNC1080Send	    (NC1080_END_DEV * pDrvCtrl, M_BLK_ID pBuf);

LOCAL STATUS	usbNC1080MCastAdd (NC1080_END_DEV * pDrvCtrl, char * pAddress);
LOCAL STATUS	usbNC1080MCastDel (NC1080_END_DEV * pDrvCtrl, char * pAddress);
LOCAL STATUS	usbNC1080MCastGet (NC1080_END_DEV * pDrvCtrl, MULTI_TABLE * pTable);

LOCAL STATUS	usbNC1080PollSend (NC1080_END_DEV * pDrvCtrl, M_BLK_ID pBuf);
LOCAL STATUS	usbNC1080PollRcv  (NC1080_END_DEV * pDrvCtrl, M_BLK_ID pBuf);

LOCAL STATUS	usbNC1080Recv	(NC1080_END_DEV * pDrvCtrl, UINT8 * pData, UINT16 len);

LOCAL STATUS 	usbNC1080Shutdown (int errCode);
LOCAL STATUS 	usbNC1080Reset 	(NC1080_END_DEV * pDrvCtrl);

LOCAL STATUS 	usbNC1080DevInit 	(NC1080_END_DEV * pDrvCtrl,
                                 UINT16 vendorId,
                                 UINT16 productId);

LOCAL void 	usbNC1080TxCallback(pVOID p);
LOCAL void 	usbNC1080RxCallback(pVOID p);

LOCAL STATUS 	usbNC1080ListenForInput   (NC1080_END_DEV * pDrvCtrl);
LOCAL USB_NC1080_DEV * usbNC1080FindUsbNode	    (USBD_NODE_ID nodeId);
LOCAL USB_NC1080_DEV * usbNC1080FindUsbDevice 	(UINT16 vendorId, UINT16 productId);

LOCAL VOID 	usbNC1080DestroyDevice 	    (NC1080_END_DEV * pDrvCtrl);

LOCAL STATUS 	usbNC1080AttachCallback
			 	(USBD_NODE_ID nodeId,
   				UINT16 attachAction,
    				UINT16 configuration,
    				UINT16 interface,
    				UINT16 deviceClass,
    				UINT16 deviceSubClass,
    				UINT16 deviceProtocol
				);

LOCAL STATUS usbNC1080ReadRegister(NC1080_END_DEV * pDrv, UINT8 reg, UINT16 * retVal);
LOCAL STATUS usbNC1080WriteRegister(NC1080_END_DEV * pDrv, UINT8 reg, UINT16 retVal);
LOCAL VOID notifyAttach ( USB_NC1080_DEV* pDev, UINT16 attachCode);




/*
 * Declare our function table.  This is static across all driver
 * instances.
 */

LOCAL NET_FUNCS usbNC1080FuncTable =
    {
    (FUNCPTR) usbNC1080Start,	/* Function to start the device. */
    (FUNCPTR) usbNC1080Stop,	/* Function to stop the device. */
    (FUNCPTR) usbNC1080Unload,	/* Unloading function for the driver. */
    (FUNCPTR) usbNC1080Ioctl,	/* Ioctl function for the driver. */
    (FUNCPTR) usbNC1080Send,	/* Send function for the driver. */
    (FUNCPTR) usbNC1080MCastAdd,	/* Multicast add function for the */
				/* driver. */
    (FUNCPTR) usbNC1080MCastDel,	/* Multicast delete function for */
				/* the driver. */
    (FUNCPTR) usbNC1080MCastGet,	/* Multicast retrieve function for */
				/* the driver. */
    (FUNCPTR) usbNC1080PollSend,	/* Polling send function */
    (FUNCPTR) usbNC1080PollRcv,	/* Polling receive function */
    endEtherAddressForm,	/* put address info into a NET_BUFFER */
    endEtherPacketDataGet, 	/* get pointer to data in NET_BUFFER */
    endEtherPacketAddrGet 	/* Get packet addresses. */
    };


/*
 * functions to do a short (UINT16) access to read / write the registers
 * from the net1080 chip.
 */

LOCAL STATUS usbNC1080ReadRegister
    (
    NC1080_END_DEV * pDrv, 
    UINT8 reg, 
    UINT16 * retVal
    )
    {
    UINT16 actLen;
    UINT16 data;

    if (usbdVendorSpecific (usbNC1080Handle, pDrv->pDev->nodeId,
	USB_RT_VENDOR | USB_RT_DEV_TO_HOST,	/* bmRequest */ 
	NETCHIP_REQ_REGISTER_READ, 		/* bRequest */
	0, 					/* wValue */
	TO_LITTLEW(reg),			/* wIndex */
	TO_LITTLEW(2),				/* wLength must be 2 */
	(UINT8 *)retVal, &actLen) != OK)
	{
	DRV_LOG (DRV_DBG_REGISTER, "Read Register Failed reading from "
		"%d regsiter..\n", reg, 0, 0, 0, 0, 0);
	return ERROR;
	}
  
    data = FROM_LITTLEW(*retVal);
    *retVal = data;

    return OK;
    }


LOCAL STATUS usbNC1080WriteRegister 
    (
    NC1080_END_DEV * pDrv, 
    UINT8 reg, 
    UINT16 data
    )
    {

    if (usbdVendorSpecific (usbNC1080Handle, pDrv->pDev->nodeId,
	USB_RT_VENDOR | USB_RT_HOST_TO_DEV,	/* bmRequest */
	NETCHIP_REQ_REGISTER_READ, 		/* bRequest */
	TO_LITTLEW(data), 			/* wValue */
	TO_LITTLEW(reg),			/* wIndex */
	0,					/* wLength */
	0, NULL) != OK)
	{
	DRV_LOG (DRV_DBG_REGISTER, "Write Register Failed while writing "
		"%d to %d register..\n", data, reg, 0, 0, 0, 0);
	return ERROR;
	}

     return OK;
}

/* Register display routines */

/**************************************************************************
*
* usbNC1080ShowStatus - Displays the NC1080 status register
* 
* Displays, in human readable way, the status register of the NC1080 
* Turboconnect device
*
* RETURNS: N/A 
*/

LOCAL void usbNC1080ShowStatus 
    (
    UINT16 status
    )
    {

    DRV_LOG (DRV_DBG_REGISTER, "Status Register : %x \n"
                               "Reading from Port : %c \n",
                                status,
                                (status & 0x8000)?'A':'B',
                                 0, 0, 0, 0);

    DRV_LOG (DRV_DBG_REGISTER, "Peer port : \n"
                               "   %sconnected : %s: "
                               "MBOX data %savailable : "
                               "data %savailable \n",
                                (UINT)((status & 0x4000)?"":"dis"),
                                (UINT)((status & 0x2000)?"suspended":"live"),
                                (UINT)((status & 0x1000)?"":"Not"),
                                (UINT)((status & 0x0300)?"":"Not"),
                                0, 0);

    DRV_LOG (DRV_DBG_REGISTER, "This port : \n"
                               "\t%sconnected : %s: "
                               "MBOX data %savailable : "
                               "data %savailable \n",
                                (UINT)((status & 0x0040)?"":"dis"),
                                (UINT)((status & 0x0020)?"suspended":"live"),
                                (UINT)((status & 0x0010)?"":"Not"),
                                (UINT)((status & 0x0003)?"":"Not"),
                                0, 0);

    }


/**************************************************************************
*
* usbNC1080ShowUsbctl -  Displays NC1080 USBCTL register
*
* Displays, in human readable way, the USBCTL register of the NC1080 
* Turboconnect device
*
* RETURNS: N/A
*/

LOCAL void usbNC1080ShowUsbctl
    (
    UINT16 usbctl
    )
    {
    DRV_LOG (DRV_DBG_REGISTER, "USBCTL Register : %x \n", usbctl,
	    2, 3, 4, 5, 6);
    }


/**************************************************************************
*
* usbNC1080ShowTtl - Display NC1080 TTL register
*
* Displays, in human readable way, the TTL register of the NC1080 
* Turboconnect device
*
* RETURNS: N/A
*/

LOCAL void usbNC1080ShowTtl
    (
    UINT16 ttl
    )
    {

    DRV_LOG (DRV_DBG_REGISTER, "TTL Register : %x \n"
                               "\tTTL for this Port : %d ms "
                               "other port : %d ms\n",
                                ttl, (ttl&0x00ff), ((ttl>>8)&0x00ff),
                                 0, 0, 0);
    }


/**************************************************************************
*
* usbNC1080Reset - Resets the NC1080 Turboconnect device
*
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS usbNC1080Reset 
    (
    NC1080_END_DEV* pDrvCtrl
    )
    {

    UINT16 reg_status, reg_usbctl, reg_ttl, temp_ttl;

    /* Read the status register */

    if (usbNC1080ReadRegister(pDrvCtrl, NETCHIP_STATUS, &reg_status) != OK)
        {
        DRV_LOG (DRV_DBG_RESET, "Failed to read status register",
                0, 0, 0, 0, 0, 0);
        return ERROR;
        }

    usbNC1080ShowStatus (reg_status);

    if (usbNC1080ReadRegister(pDrvCtrl, NETCHIP_USBCTL, &reg_usbctl) != OK)
        {
        DRV_LOG (DRV_DBG_RESET, "Failed to read usbctl register",
                0, 0, 0, 0, 0, 0);
        return ERROR;
        }

    usbNC1080ShowUsbctl (reg_usbctl);

    /* Flush Fifo of both ports */

    if (usbNC1080WriteRegister(pDrvCtrl, NETCHIP_USBCTL,
                USBCTL_FLUSH_THIS | USBCTL_FLUSH_OTHER) == ERROR)
        {
        return ERROR;
        }

    /* Some TTL funda */

    if (usbNC1080ReadRegister(pDrvCtrl, NETCHIP_TTL, &reg_ttl) != OK)
        {
        DRV_LOG (DRV_DBG_RESET, "Failed to read ttl register",
                0, 0, 0, 0, 0, 0);
        return ERROR;
        }

    usbNC1080ShowTtl(reg_ttl);

    /* set some timeout for our read fifo. it shall time out and flush data */

    temp_ttl = (reg_ttl) & 0xff00;		/* on the other side */

    reg_ttl = (NETCHIP_TTLVAL & 0x00ff);        /* our ttl */

    reg_ttl |= temp_ttl;			/* TTL register value */

    if (usbNC1080WriteRegister(pDrvCtrl, NETCHIP_TTL,
                reg_ttl) == ERROR)
        {
        DRV_LOG (DRV_DBG_RESET, "Failed to set ttl register",
                0, 0, 0, 0, 0, 0);
        return ERROR;
        }

    return OK;

    }

/**************************************************************************
*
* usbNC1080DrvInit - Initializes the usbNC1080 Library
*
* Initializes the usbNC1080 driver. The driver maintains an initialization
* count so that the calls to this function might be nested.
*
* This function initializes the system resources required for the driver
* initializes the linked list for the usbNC1080 devices found.
* This function reegisters the library as a client for the usbd calls and 
* registers for dynamic attachment notification of usb devices. Since the
* usbNC1080 device is a "vendor specific" device, this function registers
* for attachment / removal notification for all devices.
*
* This function is to be called AFTER the usbd initialization and before 
* the endLoad gets called. Otherwise the driver can't perform.
*
* RETURNS : OK if successful, ERROR if failure
*
* ERRNO :
*
* S_usbNC1080Drv_OUT_OF_RESOURCES
* S_usbNC1080Drv_USBD_FAULT
*/

STATUS usbNC1080DrvInit(void)
    {
    
    /* see if already initialized. if not, initialize the library */

    initCount++;

    if(initCount != 1)	/* if its the first time */
	return OK;

    /* Initialize usbd */

    usbdInitialize();

    memset (&usbNC1080DevList, 0, sizeof (usbNC1080DevList));
 
    usbNC1080Mutex = NULL;
    usbNC1080TxMutex = NULL;
    usbNC1080RxMutex = NULL;
    usbNC1080IrpSem = NULL;

    usbNC1080Handle = NULL;

    /* create the mutex */

    if (OSS_MUTEX_CREATE (&usbNC1080Mutex) != OK)
	return usbNC1080Shutdown (S_usbNC1080Lib_OUT_OF_RESOURCES);

    if (OSS_MUTEX_CREATE (&usbNC1080TxMutex) != OK)
	return usbNC1080Shutdown (S_usbNC1080Lib_OUT_OF_RESOURCES);

    if (OSS_MUTEX_CREATE (&usbNC1080RxMutex) != OK)
	return usbNC1080Shutdown (S_usbNC1080Lib_OUT_OF_RESOURCES);

    if (OSS_SEM_CREATE (1, 0 , &usbNC1080IrpSem) != OK)
        return usbNC1080Shutdown (S_usbNC1080Lib_OUT_OF_RESOURCES);

    /*
     * Register the Library as a Client and register for
     * dynamic attachment callback.
     */
     if((usbdClientRegister (CLIENT_NAME, &usbNC1080Handle) != OK) ||
	(usbdDynamicAttachRegister (usbNC1080Handle, 
				    USBD_NOTIFY_ALL, 
				    USBD_NOTIFY_ALL, 
				    USBD_NOTIFY_ALL, 
				(USBD_ATTACH_CALLBACK) usbNC1080AttachCallback)
			 != OK))
	{
    	return usbNC1080Shutdown (S_usbNC1080Lib_USBD_FAULT);
	}
    return OK;
    }


/**************************************************************************
*
* usbNC1080AttachCallback - Gets called for attachment/detachment of devices
*
* The USBD will invoke this callback when a USB device is attached to or 
* removed from the system (usb bus).  
* <nodeId> is the USBD_NODE_ID of the node being attached or removed.	
* <attachAction> is USBD_DYNA_ATTACH or USBD_DYNA_REMOVE.
* The registeration has been for notification of any usb device attachment /
* removal. This means that the USBD will invoke this function once for each
* configuration / interface for any device attached. <configuration> and
* <interface> will indicate the configuration / interface information of
* the device. How-ever, usbNC1080 device has only one configuration which has
* only one interface.
* <deviceClass> and <deviceSubClass> will match the class/subclass for
* which we registered.  In this case, since usbNC1080 is a vendor specific
* device, these fields donot matter for this device.
* <deviceProtocol> doesn't have meaning for the vendor devices so we 
* ignore this field.
*
* NOTE: The USBD will invoke this function once for each configuration/
* interface.  So, it is possible that a single device insertion/removal may 
* trigger multiple callbacks.  This function ignores all callbacks except the
* first for a given device.
*
* RETURNS: N/A
*
* NOMANUAL
*/

STATUS  usbNC1080AttachCallback
    (
    USBD_NODE_ID nodeId, 
    UINT16 attachAction, 
    UINT16 configuration,
    UINT16 interface,
    UINT16 deviceClass, 
    UINT16 deviceSubClass, 
    UINT16 deviceProtocol
    )
    {

    USB_NC1080_DEV * pNewDev;

    UINT8 bfr[USB_MAX_DESCR_LEN];
    UINT16 actLen;

    UINT16 vendorId;
    UINT16 productId;

    int index = 0;

    OSS_MUTEX_TAKE (usbNC1080Mutex, OSS_BLOCK);

    switch (attachAction)
	{
	case USBD_DYNA_ATTACH :
	
	    /* a new device is found */

	    DRV_LOG (DRV_DBG_ATTACH, "New Usb device on Bus..\n",
	    	    0, 0, 0, 0, 0, 0);

	    /* First Ensure that this device is not already on the list */
	
	    if (usbNC1080FindUsbNode (nodeId) != NULL)
	        break;

	    /* Now, we have to ensure that its a NETCHIP device */

            if (usbdDescriptorGet (usbNC1080Handle, 
				   nodeId, 
				   USB_RT_STANDARD | USB_RT_DEVICE, 
				   USB_DESCR_DEVICE, 
				   0, 
				   0, 
				   sizeof (bfr), 
				   bfr, 
				   &actLen) 
				!= OK)
	        {
		DRV_LOG (DRV_DBG_ATTACH, "DescriporGet Failed for device "
			"descriptor.\n", 0, 0, 0, 0, 0, 0);
		break;
            	}

	    DRV_DESC_PRINT (DRV_DBG_ATTACH, "Device Descriptor", bfr, actLen);

            vendorId = ((pUSB_DEVICE_DESCR)bfr)->vendor;
	    productId = ((pUSB_DEVICE_DESCR)bfr)->product;		

	    DRV_LOG (DRV_DBG_ATTACH, "VendorID : 0x%x \t ProductID : 0x%x.\n",
		     vendorId, productId, 0, 0, 0, 0);

	    /* Check if the device is a NC1080 Turboconnect */

	    for (index = 0; products [index].idVendor != 0; index++) 
		{
		if (products [index].idVendor != vendorId)
			continue;
		if (products [index].idProduct != productId)
			continue;
		break;
		}
	
	    if (products [index].idVendor == 0)	/* device not a usbNC1080 device */
		{
		DRV_LOG (DRV_DBG_ATTACH, " unsupported device ... \n",
			0, 0, 0, 0, 0, 0);
		break;
		}

  	    DRV_LOG (DRV_DBG_ATTACH, "that was a NC1080 device!!!  \n",
		    0, 0, 0, 0, 0, 0);

	    /* 
	     * Now create a structure for the newly found device and add 
	     * it to the linked list
	     */

         /* Try to allocate space for a new device struct */

	    if ((pNewDev = OSS_CALLOC (sizeof (*pNewDev))) == NULL)
 	    	{
		DRV_LOG (DRV_DBG_ATTACH, "Could not allocate memory "
			"for new device \n", 0, 0, 0, 0, 0, 0);
		break;	    	
		}

	    /* Fill in the device structure */
	
	    pNewDev->nodeId = nodeId;
	    pNewDev->configuration = configuration;
	    pNewDev->interface = interface;

	    pNewDev->vendorId = vendorId;
	    pNewDev->productId = productId;

	    /* Add this device to the linked list */
	
	    usbListLink (&usbNC1080DevList, pNewDev, &pNewDev->devLink, 
					LINK_TAIL);
	    /* Notify registered callers that a NETCHIP device has been added */

        notifyAttach(pNewDev, USB_NETCHIP_ATTACH);

	    break;

	case USBD_DYNA_REMOVE:

	    /* First Ensure that this device is on the list */

	    if ((pNewDev = usbNC1080FindUsbNode (nodeId)) == NULL)
	        break;


		if (pNewDev->connected == FALSE)
			{
				printf (" Device not found %x \n",(int)pNewDev->nodeId);
                break;
			}

	    DRV_LOG (DRV_DBG_ATTACH, "NC1080 device Removed from Bus..\n",
	    	    0, 0, 0, 0, 0, 0);

	    pNewDev->connected = FALSE;
	    /*
	     * we need not check for the vendor/product ids as if the device is
	     * on the list, then it is a usbNC1080 device only.
	     */

    	    pNewDev->lockCount++; 

            notifyAttach (pNewDev, USB_NETCHIP_REMOVE); 

            pNewDev->lockCount--; 

	    if (pNewDev->lockCount == 0)
			usbNC1080DestroyDevice((NC1080_END_DEV *)pNewDev->pDevStructure);
	    break;

	}

    OSS_MUTEX_RELEASE (usbNC1080Mutex);

    return OK;

    }

/**************************************************************************
*
* findEndpoint - Searches for a BULK endpoint of the indicated direction.
*
* RETURNS: pointer to matching endpoint descriptor or NULL if not found
* NOMANUAL
*/

LOCAL pUSB_ENDPOINT_DESCR findEndpoint
    (
    pUINT8 pBfr,			/* buffer to search for */
    UINT16 bfrLen,			/* buffer length */
    UINT16 direction		/* end point direction */
    )
    {
    pUSB_ENDPOINT_DESCR pEp;

    while ((pEp = (pUSB_ENDPOINT_DESCR) 
	    usbDescrParseSkip (&pBfr, &bfrLen, USB_DESCR_ENDPOINT)) 
		!= NULL)
	{
	if ((pEp->attributes & USB_ATTR_EPTYPE_MASK) == USB_ATTR_BULK &&
	    (pEp->endpointAddress & USB_ENDPOINT_DIR_MASK) == direction)
	    break;
	}

    return pEp;
    }

/**************************************************************************
*
* usbNC1080FindUsbNode - Searches for a usbNC1080 device of indicated <nodeId>
* in the usbNC1080 device linked list
*
* RETURNS: pointer to matching dev struct or NULL if not found
* NOMANUAL
*/

USB_NC1080_DEV* usbNC1080FindUsbNode
    (
    USBD_NODE_ID nodeId		/* Node Id to find */
    )
    {
    USB_NC1080_DEV * pDev = usbListFirst (&usbNC1080DevList);
    while (pDev != NULL)
	{
	if (pDev->nodeId == nodeId)
	    break;
	pDev = usbListNext (&pDev->devLink);
	}
    return pDev;
    }

/**************************************************************************
*
* usbNC1080FindUsbDevice - Searches for a usbNC1080 device of a given <productId>
* and <vendorId> in the usbNC1080 device linked list.
*
* RETURNS: pointer to matching dev struct or NULL if not found
* NOMANUAL
*/

USB_NC1080_DEV* usbNC1080FindUsbDevice
    (
    UINT16 vendorId,		/* Vendor Id to search for */
    UINT16 productId		/* Product Id to search for */
    )

    {
    USB_NC1080_DEV * pDev = usbListFirst (&usbNC1080DevList);
    while (pDev != NULL)
	{
	if ((pDev->vendorId == vendorId) && (pDev->productId == productId))
	    break;
	pDev = usbListNext (&pDev->devLink);
	}
    return pDev;
    }

/**************************************************************************
*
* usbNC1080DevInit - Initializes the usbNC1080 Device structure.
*
* This function initializes the usb ethernet device. It is called by 
* usbNC1080Load() as part of the end driver initialzation. usbNC1080Load()
* expects this routine to perform all the device and usb specific
* initialization and fill in the corresponding member fields in the 
* NC1080_END_DEV structure.
*
* This function first checks to see if the device corresponding to
* <vendorId> and <productId> exits in the linkedlist usbNC1080DevList.
* It allocates memory for the input and output buffers. The device
* descriptors will be retrieved and are used to findout the IN and OUT
* bulk end points. Once the end points are found, the corresponding
* pipes are constructed. The Pipe handles are stored in the device 
* structure <pDrvCtrl>. 
*
* RETURNS : OK if every thing succeds and ERROR if any thing fails.
*
*/

STATUS usbNC1080DevInit
    (
    NC1080_END_DEV * pDrvCtrl,	/* the device structure to be updated */
    UINT16 	  vendorId,	/* manufacturer id of the device */
    UINT16 	  productId    	/* product id of the device */
    )
    {

    USB_NC1080_DEV * pNewDev;

    pUSB_CONFIG_DESCR pCfgDescr;	

    pUSB_INTERFACE_DESCR pIfDescr;

    pUSB_ENDPOINT_DESCR pOutEp;
    pUSB_ENDPOINT_DESCR pInEp;

    UINT8 bfr [USB_MAX_DESCR_LEN];

    pUINT8 pBfr;
    UINT8** ppIrpBfrs;
    UINT8** ppInBfr;
    UINT16 actLen;
    
    int index = 0;

    if(pDrvCtrl == NULL)
	{
	DRV_LOG (DRV_DBG_INIT,"Null Device \n", 0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    /* Find if the device is in the found devices list */

    if ((pNewDev = usbNC1080FindUsbDevice (vendorId,productId)) == NULL)
	{
	DRV_LOG (DRV_DBG_INIT,"Could not find device in the attached "
		"usbNC1080 devices..\n", 0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    /* Link the End Structure and the device that is found */

    pDrvCtrl->pDev = pNewDev;

    /* Allocate memory for the output buffers..*/

    if ((ppIrpBfrs = (UINT8**) memalign (sizeof(ULONG), 
					pDrvCtrl->noOfIrps * sizeof (char *))) 
				== NULL)
	{
	DRV_LOG (DRV_DBG_INIT,"Could not allocate memory for IRPs.\n",
		     0, 0, 0, 0, 0, 0);		
	return ERROR;
	}

    for (index=0;index<pDrvCtrl->noOfIrps;index++)
	{
	if ((ppIrpBfrs[index] = (pUINT8)memalign(sizeof(ULONG),
				   NETCHIP_OUT_BFR_SIZE+8)) == NULL)
	    {
	    DRV_LOG (DRV_DBG_INIT,"Could not allocate memory for buffer"
				" for input Irp %x\n", 
				index, 0, 0, 0, 0, 0);	   
	    return ERROR;
	    }
	}

    /* Allocate memory for input buffers */

    if ((ppInBfr = (UINT8**)memalign(sizeof(ULONG),
		pDrvCtrl->noOfInBfrs * sizeof (char *)))==NULL)
	{
	DRV_LOG (DRV_DBG_INIT,"Could Not align Memory for"
		    " InBfrs pointer array...\n",
		     0, 0, 0, 0, 0, 0);			
        return ERROR;
        }


   for (index=0;index<pDrvCtrl->noOfInBfrs;index++)
	{
	if ((ppInBfr[index] = (pUINT8)memalign(sizeof(ULONG),
				   NETCHIP_IN_BFR_SIZE+8)) == NULL)
	    {
	    DRV_LOG (DRV_DBG_INIT,"Could not allocate memory for"
				" buffer for input Irp %x\n", 
				index, 0, 0, 0, 0, 0);	   
	    return ERROR;
	    }
	}
    
    pDrvCtrl->pOutBfrArray= ppIrpBfrs;
    pDrvCtrl->pInBfrArray = ppInBfr;

    pDrvCtrl->txBufIndex=0;		
    pDrvCtrl->rxBufIndex=0;


    pDrvCtrl->inBfrLen = NETCHIP_IN_BFR_SIZE;

    /*
     * Now we decifer the descriptors provided by the device ..
     * we try finding out what end point is what..
     */

    /* To start with, get the configuration descriptor ..*/

    if (usbdDescriptorGet (usbNC1080Handle, 
			   pNewDev->nodeId, 
			   USB_RT_STANDARD | USB_RT_DEVICE, 
			   USB_DESCR_CONFIGURATION, 
			   0, 
			   0, 
			   sizeof (bfr), 
			   bfr, 
			   &actLen) 
			!= OK)
	{
	DRV_LOG (DRV_DBG_INIT, "DescriptorGet failed for CFG...\n",
		     0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    if ((pCfgDescr = (pUSB_CONFIG_DESCR)
	usbDescrParse (bfr, actLen, USB_DESCR_CONFIGURATION)) == NULL)
	{
	DRV_LOG (DRV_DBG_INIT, "Could not find Config. Desc...\n",
		     0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    /* DRV_DESC_PRINT (DRV_DBG_INIT, "Configuration Descriptor",
                   (UINT8 *)pCfgDescr, actLen); */

    pBfr = bfr;

    /*
     * Since we registered for NOTIFY_ALL for attachment of devices,
     * the configuration no and interface number as reported by the
     * call back function doesn't have any meanning.
     * we refer to the DRV document and it says, it has only one
     * interface with number 0. So the first (only) interface we find
     * is the interface we want.
     * If there are more interfaces and one of them meet our requirement
     * (as reported by callback funtction), then we need to parse
     * until we find our one..
     */

    if ((pIfDescr = (pUSB_INTERFACE_DESCR)
	usbDescrParseSkip (&pBfr, &actLen, USB_DESCR_INTERFACE)) == NULL)
	{
	DRV_LOG (DRV_DBG_INIT, "Could not find Interface Desc.\n",
		     0, 0, 0, 0, 0, 0);
    	return ERROR;
	}

    /*
     * DRV_DESC_PRINT (DRV_DBG_INIT, "Interface Descriptor", 
     * (UINT8 *)pIfDescr, actLen); 
     */

    /* Find out the output and input end points ..*/

    if ((pOutEp = findEndpoint (pBfr, actLen, USB_ENDPOINT_OUT)) == NULL)
	{
	DRV_LOG (DRV_DBG_INIT, "No Output End Point \n",
		     0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    DRV_DESC_PRINT (DRV_DBG_INIT, "Out EP", (UINT8 *)pOutEp, 5);

    if ((pInEp = findEndpoint (pBfr, actLen, USB_ENDPOINT_IN)) == NULL)
	{
	DRV_LOG (DRV_DBG_INIT, "No Input End Point \n",
		     0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    DRV_DESC_PRINT (DRV_DBG_INIT, "In EP", (UINT8 *)pInEp, 5);

    pNewDev->maxPower = pCfgDescr->maxPower;

    /* Now, set the configuration. */

    if (usbdConfigurationSet (usbNC1080Handle, 
			      pNewDev->nodeId, 
			      pCfgDescr->configurationValue, 
			      pCfgDescr->maxPower * USB_POWER_MA_PER_UNIT) 
			   != OK)
	return ERROR;

    /*  reset the device */

    usbNC1080Reset(pDrvCtrl);

    /* Now, Create the Pipes.. */

    if (usbdPipeCreate (usbNC1080Handle, 
			pNewDev->nodeId, 
			pOutEp->endpointAddress, 
			pCfgDescr->configurationValue, 
			pNewDev->interface, 
			USB_XFRTYPE_BULK, 
			USB_DIR_OUT, 
			FROM_LITTLEW (pOutEp->maxPacketSize), 
			0, 
			0, 
			&pDrvCtrl->outPipeHandle) 
		!= OK)
	{
	DRV_LOG (DRV_DBG_INIT, "Pipe O/P coud not be created \n",
		     0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    if (usbdPipeCreate (usbNC1080Handle, 
			pNewDev->nodeId, 
			pInEp->endpointAddress, 
			pCfgDescr->configurationValue, 
			pNewDev->interface, 
			USB_XFRTYPE_BULK, 
			USB_DIR_IN, 
			FROM_LITTLEW (pInEp->maxPacketSize), 
			0, 
			0, 
			&pDrvCtrl->inPipeHandle) 
		!= OK)
	{
	DRV_LOG (DRV_DBG_INIT, "Pipe I/P coud not be created \n",
	     	     0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    /* Reset counters and indices */

    pDrvCtrl->inIrpInUse = FALSE;
    pDrvCtrl->outIrpInUse = FALSE;

    pDrvCtrl->txBufIndex = 0;
    pDrvCtrl->rxBufIndex = 0;

    pDrvCtrl->txPkts = 0;


    /* Now device is ready for communication : return back to the caller */

    return OK;

    }

/**************************************************************************
*
* usbNC1080Shutdown - shuts down usbNC1080Lib
*
* <errCode> should be OK or S_usbNC1080Lib_xxxx.  This value will be
* passed to ossStatus() and the return value from ossStatus() is the
* return value of this function.
*
* RETURNS: OK, or ERROR per value of <errCode> passed by caller
*/

LOCAL STATUS usbNC1080Shutdown
    (
    int errCode
    )

    {

    NC1080_END_DEV * pDev;

    /* Dispose of any open connections. */

    while ((pDev = usbListFirst (&usbNC1080DevList)) != NULL)
	usbNC1080DestroyDevice (pDev);

    /*
     * Release our connection to the USBD.  The USBD automatically
     * releases any outstanding dynamic attach requests when a client
     * unregisters.
     */

    if (usbNC1080Handle != NULL)
	{
	usbdClientUnregister (usbNC1080Handle);
	usbNC1080Handle = NULL;
	}

    /* Release resources. */

    if (usbNC1080Mutex != NULL)
	{
	OSS_MUTEX_DESTROY (usbNC1080Mutex);
	usbNC1080Mutex = NULL;
	}

    if (usbNC1080TxMutex != NULL)
	{
	OSS_MUTEX_DESTROY (usbNC1080TxMutex);
	usbNC1080TxMutex = NULL;
	}

    if (usbNC1080RxMutex != NULL)
	{
	OSS_MUTEX_DESTROY (usbNC1080RxMutex);
	usbNC1080RxMutex = NULL;
	}

    if (usbNC1080IrpSem != NULL)
        {
        OSS_SEM_DESTROY (usbNC1080IrpSem);
        usbNC1080IrpSem = NULL;
        }


    usbdShutdown();

    return ossStatus (errCode);
    }

/**************************************************************************
*
* usbNC1080DestroyDevice - disposes of a NC1080_END_DEV structure
*
* Unlinks the indicated NC1080_END_DEV structure and de-allocates
* resources associated with the channel.
*
* RETURNS: N/A
*/

VOID usbNC1080DestroyDevice
    (
    NC1080_END_DEV * pDrvCtrl
    )
    {
    USB_NC1080_DEV * pDev;

    if (pDrvCtrl != NULL)
	{
	pDev = pDrvCtrl->pDev;

	/* Unlink the structure. */

	usbListUnlink (&pDev->devLink);
  
	/* Release pipes and wait for IRPs to be cancelled if necessary. */

	if (pDrvCtrl->outPipeHandle != NULL)
	    usbdPipeDestroy (usbNC1080Handle, pDrvCtrl->outPipeHandle);

	if (pDrvCtrl->inPipeHandle != NULL)
	    usbdPipeDestroy (usbNC1080Handle, pDrvCtrl->inPipeHandle);

	while (pDrvCtrl->outIrpInUse || pDrvCtrl->inIrpInUse) 
	    OSS_THREAD_SLEEP (1);

	 	/*  Release Input buffers*/

	if ( pDrvCtrl->pInBfrArray !=NULL)
	    {	
            OSS_FREE(pDrvCtrl->pInBfrArray);
	    taskDelay(sysClkRateGet()*1);
		
	    DRV_LOG (DRV_DBG_ATTACH, "Destroy device InBfrArray destroyed...\n",
	    	    0, 0, 0, 0, 0, 0);
	    }	

	/*  Release outputIrp buffers*/
	if ( pDrvCtrl->pOutBfrArray != NULL)
	    {	
            OSS_FREE(pDrvCtrl->pOutBfrArray);
	    taskDelay(sysClkRateGet()*1);
	   
	    DRV_LOG (DRV_DBG_ATTACH, "Destroy device pOutBfrArray destroyed...\n",
	    	    0, 0, 0, 0, 0, 0);
            }

	/* Release structure. */
        if (pDev!=NULL)
            {
	    OSS_FREE (pDev);
		
	    DRV_LOG (DRV_DBG_ATTACH, "destroy device pDev destroyed...\n",
	    	    0, 0, 0, 0, 0, 0);
    	    }
	}
    }

/**************************************************************************
*
* usbNC1080EndMemInit - initialize memory for the device.
*
* We setup the END's Network memory pool. This code is highly generic and
* very simple. We just follow the technique described in netBufLib.
* We don't even need to make the memory cache DMA coherent. This is because
* unlike other END drivers, which act on the top of the hardware, we are
* layers above the hardware.
*
* RETURNS: OK or ERROR.
* NOMANUAL
*/

STATUS usbNC1080EndMemInit
    (
    NC1080_END_DEV * pDrvCtrl	/* device to be initialized */
    )
    {

    /*
     * This is how we would set up and END netPool using netBufLib(1).
     * This code is pretty generic.
     */

    if ((pDrvCtrl->endObj.pNetPool = malloc (sizeof (NET_POOL))) == NULL)
        return (ERROR);

    usbNC1080MclBlkConfig.mBlkNum = 512;
    usbNC1080ClDescTbl[0].clNum = 256;
    usbNC1080MclBlkConfig.clBlkNum = usbNC1080ClDescTbl[0].clNum;

    /* Calculate the total memory for all the M-Blks and CL-Blks. */

    usbNC1080MclBlkConfig.memSize = (usbNC1080MclBlkConfig.mBlkNum *
				    (MSIZE + sizeof (long))) +
			      (usbNC1080MclBlkConfig.clBlkNum *
				    (CL_BLK_SZ + sizeof(long)));

    if ((usbNC1080MclBlkConfig.memArea = (char *) memalign (sizeof(long),
         usbNC1080MclBlkConfig.memSize)) == NULL)
        return (ERROR);

    /* Calculate the memory size of all the clusters. */

    usbNC1080ClDescTbl[0].memSize = (usbNC1080ClDescTbl[0].clNum *
				    (NETCHIP_BUFSIZ + 8)) + sizeof(int);

    /* Allocate the memory for the clusters from cache safe memory. */

    usbNC1080ClDescTbl[0].memArea =
        (char *) cacheDmaMalloc (usbNC1080ClDescTbl[0].memSize);

    if (usbNC1080ClDescTbl[0].memArea == NULL)
        {
        DRV_LOG (DRV_DBG_LOAD,"usbNC1080EndMemInit:system memory "
		    "unavailable\n", 1, 2, 3, 4, 5, 6);
        return (ERROR);
        }

    /* Initialize the memory pool. */

    if (netPoolInit(pDrvCtrl->endObj.pNetPool, &usbNC1080MclBlkConfig,
                    &usbNC1080ClDescTbl[0], usbNC1080ClDescTblNumEnt,
		    NULL) == ERROR)
        {
        DRV_LOG (DRV_DBG_LOAD, "Could not init buffering\n",
		1, 2, 3, 4, 5, 6);
        return (ERROR);
        }


    if ((pDrvCtrl->pClPoolId = netClPoolIdGet (pDrvCtrl->endObj.pNetPool,
	NETCHIP_MTU, FALSE))         
        == NULL)
	{
        DRV_LOG (DRV_DBG_LOAD, "netClPoolIdGet() not successfull \n",
  		    1, 2, 3, 4, 5, 6);
	return (ERROR);

        }

    DRV_LOG (DRV_DBG_LOAD, "Memory setup complete\n",
		1, 2, 3, 4, 5, 6);

    return OK;
    }



/**************************************************************************
*
* usbNC1080EndParse - parse the init string
*
* Parse the input string.  Fill in values in the driver control structure.
*
* The muxLib.o module automatically prepends the unit number to the user's
* initialization string from the BSP (configNet.h).
*
* This function parses the input string and fills in the places pointed
* to by <pVendorId> and <pProductId>. Unit Number of the string will be
* be stored in the device structure pointed to by <pDrvCtrl>.
* The MAC Address of the card will be passed onto us by this initString.
*
* .IP <pDrvCtrl>
* Pointer to the device structure.
* .IP <initString>
* Initialization string for the device. It will be of the following format :
* "unit:vendorId:productId:no of input irp buffers: no of output IRP buffers:
* <6 byte mac address, delimited by :>"
* Device unit number, a small integer.
* .IP <pVendorId>
* Pointer to the place holder of the device vendor id.
* .IP <pProductId>
* Pointer to the place holder of the device product id.
* .IP <noOfIrps>
* No of Input Irp buffers 
* .IP <noOfOutIrps>
* No of output Irp buffers
* .IP <macAddress>
* 6 byte Hardware address of the device.
*
* RETURNS: OK or ERROR for invalid arguments.
* NOMANUAL
*/

STATUS usbNC1080EndParse
    (
    NC1080_END_DEV * pDrvCtrl,	/* device pointer */
    char * initString,		/* information string */
    UINT16 * pVendorId,
    UINT16 * pProductId
    )
    {

    char *	tok;
    char *	pHolder = NULL;

    int i;

    /* Parse the initString */

    /* Unit number. (from muxLib.o) */

    tok = strtok_r (initString, ":", &pHolder);

    if (tok == NULL)
	return ERROR;

    pDrvCtrl->unit = atoi (tok);

    DRV_LOG (DRV_DBG_PARSE, "Parse: Unit : %d..\n",
		pDrvCtrl->unit, 2, 3, 4, 5, 6);

    /* Vendor Id. */

    tok = strtok_r (NULL, ":", &pHolder);

    if (tok == NULL)
	return ERROR;

    *pVendorId = atoi (tok);

    DRV_LOG (DRV_DBG_PARSE, "Parse: VendorId : 0x%x..\n",
		*pVendorId, 2, 3, 4, 5, 6);

    /* Product Id. */

    tok = strtok_r (NULL, ":", &pHolder);

    if (tok == NULL)
	return ERROR;

    *pProductId = atoi (tok);

    DRV_LOG (DRV_DBG_PARSE, "Parse: ProductId : 0x%x..\n",
		*pProductId, 2, 3, 4, 5, 6);

	/* no of in buffers */

    tok = strtok_r (NULL, ":", &pHolder);

    if (tok == NULL)
	return ERROR;


    pDrvCtrl->noOfInBfrs  = atoi (tok);
  
    DRV_LOG (DRV_DBG_PARSE, "Parse: noOfInBfrs : 0x%x..\n",
		pDrvCtrl->noOfInBfrs, 2, 3, 4, 5, 6);	

    /* no of out IRPs */

    tok = strtok_r (NULL, ":", &pHolder);

    if (tok == NULL)
	return ERROR;

    pDrvCtrl->noOfIrps = atoi (tok);
  
    DRV_LOG (DRV_DBG_PARSE, "Parse: noOfIrps : 0x%x..\n",
		pDrvCtrl->noOfIrps, 2, 3, 4, 5, 6);	


    /* Hardware address */

    for (i=0; i<6; i++)
        {

        tok = strtok_r (NULL, ":", &pHolder);

        if (tok == NULL)
	    return ERROR;

        pDrvCtrl->enetAddr[i] = atoi (tok);
        }

     DRV_LOG (DRV_DBG_PARSE, "Parse: Mac Address : "
                                "%x: %x: %x : %x: %x: %x..\n",
				pDrvCtrl->enetAddr[0],
                                pDrvCtrl->enetAddr[1],
                                pDrvCtrl->enetAddr[2],
                                pDrvCtrl->enetAddr[3],
                                pDrvCtrl->enetAddr[4],
                                pDrvCtrl->enetAddr[5]);

    DRV_LOG (DRV_DBG_PARSE, "Parse: Processed all arugments\n",
		1, 2, 3, 4, 5, 6);

    return OK;
    }


/* End Interface routines as required by VxWorks */

/**************************************************************************
*
* usbNC1080Load - initialize the driver and device
*
* This routine initializes the driver and the device to the operational state.
* All of the device specific parameters are passed in the initString.
*
* This function first extracts the vendorId, productId and mac address of
* the device from the initialization string using the usbNC1080EndParse()
* function. It then passes these parameters and its control strcuture to
* the usbNC1080DevInit() function.
*
* usbNC1080DevInit() does most of the device specific initialization
* and brings the device to the operational state. This driver will be attached
* to MUX.
*
* This function doesn't do any thing device specific. Instead, it delegates
* such initialization to usbNC1080DevInit(). This routine handles the other part
* of the driver initialization as required by MUX.
*
* muxDevLoad calls this function twice. First time this function is called,
* initialization string will be NULL . We are required to fill in the device
* name ("usbNC1080") in the string and return. The next time this function is 
* called the intilization string will be proper.
*
* <initString> will be in the following format :
* "unit:vendorId:productId:<6bytes of mac address, delimited by :>"
*
* PARAMETERS
*
*.IP <initString>
*The device initialization string.
*
* RETURNS: An END object pointer or NULL on error.
*/

END_OBJ * usbNC1080Load
    (
    char * initString	                           /* initialization string */
    )
    {

    NC1080_END_DEV * pDrvCtrl;                        /* driver structure */

    UINT16 vendorId;                               /* vendor information */
    UINT16 productId;                              /* product information */

    DRV_LOG (DRV_DBG_LOAD, "Loading usbNC1080 ...\n", 1, 2, 3, 4, 5, 6);

    if (initString == NULL)
	return (NULL);

    if (initString[0] == EOS)
	{

        /* Fill in the device name and return */

	bcopy ((char *)NETCHIP_NAME, (void *)initString, NETCHIP_NAME_LEN);
	return (0);
	}

    /* allocate the device structure */

    pDrvCtrl = (NC1080_END_DEV *)calloc (sizeof (NC1080_END_DEV), 1);

    if (pDrvCtrl == NULL)
	{
	DRV_LOG (DRV_DBG_LOAD, "No Memory!!...\n", 1, 2, 3, 4, 5, 6);
	goto errorExit;
	}

    /* parse the init string, filling in the device structure */

    if (usbNC1080EndParse (pDrvCtrl, initString, &vendorId, &productId) == ERROR)
	{
	DRV_LOG (DRV_DBG_LOAD, "Parse Failed.\n", 1, 2, 3, 4, 5, 6);
	goto errorExit;
	}

    /* Ask the usbNC1080Lib to do the necessary initilization. */

    if (usbNC1080DevInit(pDrvCtrl,vendorId,productId) == ERROR)
	{
	DRV_LOG (DRV_DBG_LOAD, "EnetDevInitFailed.\n",
		    1, 2, 3, 4, 5, 6);
	goto errorExit;
	}

    /* initialize the END and MIB2 parts of the structure */

    if (END_OBJ_INIT (&pDrvCtrl->endObj, 
		      (DEV_OBJ *)pDrvCtrl, 
		      "usbNC1080", 
		      pDrvCtrl->unit, 
		      &usbNC1080FuncTable, 
		      "usbNC1080") 
		    == ERROR
     || END_MIB_INIT (&pDrvCtrl->endObj, 
		      M2_ifType_ethernet_csmacd, 
		      &pDrvCtrl->enetAddr[0], 
		      6, 
		      NETCHIP_BUFSIZ, 
		      NETCHIP_SPEED) 
		     == ERROR)
	{
	DRV_LOG (DRV_DBG_LOAD, "END MACROS FAILED...\n",
		1, 2, 3, 4, 5, 6);
	goto errorExit;
	}

    /* Perform memory allocation/distribution */

    if (usbNC1080EndMemInit (pDrvCtrl) == ERROR)
	{
	DRV_LOG (DRV_DBG_LOAD, "endMemInit() Failed...\n",
		    1, 2, 3, 4, 5, 6);
	goto errorExit;
	}

    /* set the flags to indicate readiness */

    END_OBJ_READY (&pDrvCtrl->endObj,
		    IFF_UP | IFF_RUNNING | IFF_NOTRAILERS );

    DRV_LOG (DRV_DBG_LOAD, "Done loading Netchip ...\n",
	1, 2, 3, 4, 5, 6);

	pDrvCtrl->pDev->connected = TRUE;

    return (&pDrvCtrl->endObj);

errorExit:

    if (pDrvCtrl != NULL)
	{

	free ((char *)pDrvCtrl);

	}

    return NULL;
    }


/**************************************************************************
*
* usbNC1080Unload - unload a driver from the system
*
* This function first brings down the device, and then frees any
* stuff that was allocated by the driver in the load function.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS usbNC1080Unload
    (
    NC1080_END_DEV * pDrvCtrl	/* device to be unloaded */
    )
	
    {
    DRV_LOG (DRV_DBG_UNLOAD, "In netchipUnload... \n",
	    	    0, 0, 0, 0, 0, 0);

    END_OBJECT_UNLOAD (&pDrvCtrl->endObj);

    usbNC1080DestroyDevice (pDrvCtrl);

    DRV_LOG(DRV_DBG_UNLOAD,"netchipdestroy device ..done\n" 
							, 0, 0, 0, 0, 0, 0);


    netPoolDelete (pDrvCtrl->endObj.pNetPool);

	DRV_LOG(DRV_DBG_UNLOAD,"netpoolDelete ..done\n" 
							, 0, 0, 0, 0, 0, 0);


    free(pDrvCtrl);
	DRV_LOG(DRV_DBG_UNLOAD,"pDrvCtrl free...done\n" 
							, 0, 0, 0, 0, 0, 0);


    return (OK);

    }


/**************************************************************************
*
* usbNC1080Start - start the device
*
* This function just starts communication with the device.
*
* RETURNS: OK or ERROR
*
*/

STATUS usbNC1080Start
    (
    NC1080_END_DEV  * pDrvCtrl	/* device ID */
    )
    {

    UINT16 status;
    int temp;

    /* Check if the other side is connected, FIXME */

    if (usbNC1080ReadRegister(pDrvCtrl, NETCHIP_STATUS, &status) != OK)
        {
        DRV_LOG (DRV_DBG_START, "usbNC1080Start: Failed to read status",
                0, 0, 0, 0, 0, 0);
        return ERROR;
        }

    if (!(status & 0x4000))
        {
        DRV_LOG (DRV_DBG_START, "usbNC1080Start: Peer not connected",
                0, 0, 0, 0, 0, 0);

        temp = usbNC1080Debug;
        usbNC1080Debug |= DRV_DBG_REGISTER;

        usbNC1080ShowStatus(status);

        usbNC1080Debug = temp;

        return ERROR;
        }

    /*
     * Listem for any ethernet packet coming in.
     * This is just simulating "connecting the interrupt" in End Model.
     */

     if (usbNC1080ListenForInput (pDrvCtrl) != OK)
	{
        DRV_LOG (DRV_DBG_START, "usbNC1080Start: Failed\n",0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    return OK;
    }

/**************************************************************************
*
* usbNC1080Stop - stop the device
*
* This function stops communication with the device.
*
* RETURNS: OK or ERROR
*
*/

STATUS usbNC1080Stop
    (
    NC1080_END_DEV * pDrvCtrl	/* device ID */
    )
    {
    /* Abort any pending transfers */
	
    DRV_LOG(DRV_DBG_UNLOAD,"In usbNC1080Stop..\n",
		0, 0, 0, 0, 0, 0);

    pDrvCtrl->pDev->connected = FALSE;

    usbNC1080DestroyDevice(pDrvCtrl);

    taskDelay(sysClkRateGet()*5);

    return OK;

    }

/************************************************************************
* usbNC1080ListenForInput - Listens for data on the Bulk In Pipe
*
* Input IRP will be initialized to listen on the BULK input pipe and will
* be submitted to the usbd.
*
* RETURNS : OK or ERROR
*
* NOMANUAL
*/

LOCAL STATUS usbNC1080ListenForInput
    (
    NC1080_END_DEV * pDrvCtrl		/* device to receive from */
    )
    {

    pUSB_IRP pIrp = &pDrvCtrl->inIrp;

    if (pDrvCtrl == NULL)
	return ERROR;

    /* Initialize IRP */

    memset (pIrp, 0, sizeof (*pIrp));

    pIrp->userPtr = pDrvCtrl;
    pIrp->irpLen = sizeof (*pIrp);
    pIrp->userCallback = usbNC1080RxCallback;
    pIrp->timeout = USB_TIMEOUT_DEFAULT;
    pIrp->transferLen = NETCHIP_IN_BFR_SIZE;	/* for the max pkt size */
    pIrp->flags = USB_FLAG_SHORT_OK;

    pIrp->bfrCount = 1;

    pIrp->bfrList[0].pid = USB_PID_IN;
    pIrp->bfrList[0].bfrLen = NETCHIP_IN_BFR_SIZE;
    pIrp->bfrList[0].pBfr = (pUINT8)pDrvCtrl->pInBfrArray[pDrvCtrl->rxBufIndex];
					/* for dynamic memory allocation */
    /* Submit IRP */

    if (usbdTransfer (usbNC1080Handle, pDrvCtrl->inPipeHandle, pIrp) != OK)
	return ERROR;

    pDrvCtrl->inIrpInUse = TRUE;

    return OK;
    }


/**************************************************************************
*
* usbNC1080RxCallback - Invoked when a Packet is received.
*
* NOMANUAL
*/

LOCAL VOID usbNC1080RxCallback
    (
    pVOID p			/* completed IRP */
    )

    {
    pUSB_IRP pIrp = (pUSB_IRP) p;

    NC1080_END_DEV * pDrvCtrl = pIrp->userPtr;

    /* Input IRP completed */

    pDrvCtrl->inIrpInUse = FALSE;

    /*
     * If the IRP was successful then pass the data back to the client.
     */

    if (pIrp->result != OK)
	{
        pDrvCtrl->inErrors++;	/* FIXME : Should also update MIB */

	if(pIrp->result == S_usbHcdLib_STALLED)
	    {
            DRV_LOG (DRV_DBG_RX, "BULK_IN End point stalled",
                0, 0, 0, 0, 0, 0);
	    usbdFeatureClear (usbNC1080Handle,
			      pDrvCtrl->pDev->nodeId, 
			      USB_RT_STANDARD | USB_RT_ENDPOINT, 
			      0, 
			      0);
	    }
	}
    else
	{
	if( pIrp->bfrList[0].actLen >= 2)
	    {
	    PKT_PRINT(0x2,pIrp->bfrList[0].pBfr,pIrp->bfrList[0].actLen);
  	    usbNC1080Recv (pDrvCtrl,pIrp->bfrList[0].pBfr, pIrp->bfrList[0].actLen);
	    pDrvCtrl->rxBufIndex++;
	    pDrvCtrl->rxBufIndex %= NETCHIP_NUM_IN_BFRS;
	    }
	}

    /*
     * Unless the IRP was cancelled - implying the channel is being
     * torn down, re-initiate the "in" IRP to listen for more data from
     * the device.
     */

    if (pIrp->result != S_usbHcdLib_IRP_CANCELED)
	usbNC1080ListenForInput (pDrvCtrl);

    }

/**************************************************************************
*
* usbNC1080Recv - process the incoming packet
*
* usbNC1080Recv is called by the usbNC1080RxCallBack() upon successful execution
* of an input IRP. This means we got some proper data. This function will be
* called with the pointer to be buffer and the length of data.
* What we do here is to construct an MBlk strcuture with the data received
* and pass it onto the upper layer.
*
* RETURNS: N/A.
* NOMANUAL
*/

LOCAL STATUS usbNC1080Recv
    (
    NC1080_END_DEV * pDrvCtrl,	/* device structure */
    UINT8 *  pBuf,              /* pointer to data buffer */
    UINT16  len                 /* length of data */
    )
    {

    char *      pNewCluster;    /* Clsuter to store the data */
    CL_BLK_ID	pClBlk;         /* Control block to "control" the cluster */
    M_BLK_ID 	pMblk;          /* and an MBlk to complete a MBlk contruct */

    NC_HEADER * pHeader;        /* usbNC1080 Protocol */

    /* Add one to our unicast data. */

    END_ERR_ADD (&pDrvCtrl->endObj, MIB2_IN_UCAST, +1);

    /* NC1080 Protocol verifications */

    pHeader = (NC_HEADER *) pBuf;

    if (pHeader->hdr_len < NETCHIP_MIN_HEADER)
        {
        DRV_LOG (DRV_DBG_RX, "Rx : Header Length mismatch! Actual : %d \n",
		    pHeader->hdr_len, 2, 3, 4, 5, 6);
        return ERROR;
        }

    if (pHeader->hdr_len != sizeof (NC_HEADER))
        {
        DRV_LOG (DRV_DBG_RX, "Rx : Out of Band Header : %d bytes\n",
		    pHeader->hdr_len, 2, 3, 4, 5, 6);
        }

     /* All protocol checks complete */

    pNewCluster = netClusterGet (pDrvCtrl->endObj.pNetPool, pDrvCtrl->pClPoolId);

    if (pNewCluster == NULL)
        {
	DRV_LOG (DRV_DBG_RX, "Recv: Cannot loan!\n", 1, 2, 3, 4, 5, 6);
	END_ERR_ADD (&pDrvCtrl->endObj, MIB2_IN_ERRS, +1);
	goto cleanRXD;
        }

    memcpy( pNewCluster, pBuf + pHeader->hdr_len, pHeader->packet_len);

	/* pNewCluster = pBuf + pHeader->hdr_len; */

    /* Grab a cluster block to marry to the cluster we received. */

    if ((pClBlk = netClBlkGet (pDrvCtrl->endObj.pNetPool, M_DONTWAIT)) == NULL)
        {
        netClFree (pDrvCtrl->endObj.pNetPool, (UCHAR *)pNewCluster);
	DRV_LOG (DRV_DBG_RX, "Out of Cluster Blocks!\n",
		    1, 2, 3, 4, 5, 6);
	END_ERR_ADD (&pDrvCtrl->endObj, MIB2_IN_ERRS, +1);
	goto cleanRXD;
        }

    /*
     * Let's get an M_BLK_ID and marry it to the one in the ring.
     */

    if ((pMblk = mBlkGet (pDrvCtrl->endObj.pNetPool, M_DONTWAIT, MT_DATA))
		== NULL)
        {
        netClBlkFree (pDrvCtrl->endObj.pNetPool, pClBlk);
		netClFree (pDrvCtrl->endObj.pNetPool, (UCHAR *)pNewCluster);
		DRV_LOG (DRV_DBG_RX, "Out of M Blocks!\n",
		    1, 2, 3, 4, 5, 6);
		END_ERR_ADD (&pDrvCtrl->endObj, MIB2_IN_ERRS, +1);
		goto cleanRXD;
        }

    END_ERR_ADD (&pDrvCtrl->endObj, MIB2_IN_UCAST, +1);

    /* Join the cluster to the MBlock */

    netClBlkJoin (pClBlk, pNewCluster, pHeader->packet_len, NULL, 0, 0, 0);
    netMblkClJoin (pMblk, pClBlk);

    pMblk->mBlkHdr.mLen = pHeader->packet_len;
    pMblk->mBlkHdr.mFlags |= M_PKTHDR;
    pMblk->mBlkPktHdr.len = pHeader->packet_len;

    /* Call the upper layer's receive routine. */

    END_RCV_RTN_CALL(&pDrvCtrl->endObj, pMblk);

    cleanRXD:

    return OK;
    }


/**************************************************************************
*
* usbNC1080Send - the driver send routine
*
* This routine takes a M_BLK_ID sends off the data in the M_BLK_ID. We copy
* the data contained in the MBlks to a character buffer, forms an IRP and
* transmits the data.
* We put the usbNC1080 turboconnect protocol information in the buffer here.
* The data copied from MBlks, must already have the addressing information
* properly installed in it.  This is done by a higher layer.
*
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS usbNC1080Send
    (
    NC1080_END_DEV * pDrvCtrl,	/* device ptr */
    M_BLK_ID      pMblk		/* data to send */
    )
    {

    UINT8 *     pBuf; 		/* buffer to hold the data */
    UINT32	noOfBytes;	/* noOfBytes to be transmitted */

    NC_HEADER * pHeader;        /* protcol header */
    NC_FOOTER * pFooter;        /* protocol footer */

    int         len;            /* length of packet */

    pUSB_IRP pIrp = &pDrvCtrl->outIrp;

    if ((pDrvCtrl == NULL) || (pMblk == NULL))
	return ERROR;

    OSS_MUTEX_TAKE (usbNC1080TxMutex, OSS_BLOCK);

    DRV_LOG (DRV_DBG_TX, "usbNC1080Send: Entered.\n", 0, 0, 0, 0, 0, 0);

	pBuf = 	pDrvCtrl->pOutBfrArray[pDrvCtrl->txBufIndex];


    pDrvCtrl->txBufIndex++;
    pDrvCtrl->txBufIndex %= NETCHIP_NUM_OUT_BFRS;

    /* copy the MBlk chain to a buffer */

    noOfBytes = netMblkToBufCopy (pMblk, (char *)pBuf + sizeof(NC_HEADER),NULL);

    if (noOfBytes == 0)
	return ERROR;

    DRV_LOG (DRV_DBG_TX, "usbNC1080Send: %d bytes to be sent.\n",
		noOfBytes, 0, 0, 0, 0, 0);

    /* Set the netchip protocol header/footer */

    pHeader = (NC_HEADER *)pBuf;

    pHeader->hdr_len = sizeof (NC_HEADER);
    pHeader->packet_len = noOfBytes;
    pHeader->packet_id = ++(pDrvCtrl->txPkts);

    /* Maintain odd length. Pad a byte if necessary */

    len = noOfBytes + sizeof (NC_HEADER)+ sizeof (NC_FOOTER);

    if (!(len & 0x01))
        {
        *(pBuf + noOfBytes + sizeof (NC_HEADER)) = NETCHIP_PAD_BYTE;
        pFooter = (NC_FOOTER *)(pBuf + noOfBytes + sizeof (NC_HEADER) + 1);
        len ++;
        }
    else
        {
        pFooter = (NC_FOOTER *)(pBuf + noOfBytes + sizeof (NC_HEADER));
        }

    pFooter->packet_id = pDrvCtrl->txPkts;

    PKT_PRINT(PKT_DBG_TX,pBuf,len);

    /* Transmit data */

    pDrvCtrl->outIrpInUse = TRUE;

    /* Initialize IRP */

    memset (pIrp, 0, sizeof (*pIrp));

    pIrp->userPtr = pDrvCtrl;
    pIrp->irpLen = sizeof (*pIrp);
    pIrp->userCallback = usbNC1080TxCallback;
    pIrp->timeout = 2000;
    pIrp->transferLen = len;

    pIrp->bfrCount = 1;
    pIrp->bfrList [0].pid = USB_PID_OUT;
    pIrp->bfrList [0].pBfr = pBuf;
    pIrp->bfrList [0].bfrLen = len;

    /* Submit IRP */

    if (usbdTransfer (usbNC1080Handle, pDrvCtrl->outPipeHandle, pIrp) != OK)
	return ERROR;

    DRV_LOG (DRV_DBG_TX, "usbNC1080Send: Pkt submitted for tx.\n",
		0, 0, 0, 0, 0, 0);

    /* Wait for IRP to complete or cancel by timeout */

    if (OSS_SEM_TAKE (usbNC1080IrpSem, 2000 + 1000) == ERROR )
        {
        DRV_LOG (DRV_DBG_TX, "usbNC1080Send: Fatal error. "
                "pId : %x  len : %x \n",
		pHeader->packet_id, len, 0, 0, 0, 0);
        OSS_SEM_GIVE (usbNC1080IrpSem);
        }
    else
        {
        DRV_LOG (DRV_DBG_TX, "usbNC1080Send: Tx Successful!!. "
                "pId : %x  len : %d \n",
		pHeader->packet_id, len, 0, 0, 0, 0);
        }

    OSS_MUTEX_RELEASE (usbNC1080TxMutex);

    /* Bump the statistic counter. */

    END_ERR_ADD (&pDrvCtrl->endObj, MIB2_OUT_UCAST, +1);

    /* Cleanup.  The driver frees the packet now. */

    netMblkClChainFree (pMblk);

    return (OK);
    }

/**************************************************************************
*
* usbNC1080TxCallback - Invoked upon Transmit IRP completion/cancellation
*
* NOMANUAL
*/

LOCAL VOID usbNC1080TxCallback
    (
    pVOID p			/* completed IRP */
    )

    {
    pUSB_IRP pIrp = (pUSB_IRP) p;

    NC1080_END_DEV * pDrvCtrl = pIrp->userPtr;

    /* Output IRP completed */

    pDrvCtrl->outIrpInUse = FALSE;

    DRV_LOG (DRV_DBG_TX, "Tx Callback \n", 0, 0, 0, 0, 0, 0);

    if (pIrp->result != OK)
	{
        DRV_LOG (DRV_DBG_TX, "Tx error %x.\n",pIrp->result, 0, 0, 0, 0, 0);

	if (pIrp->result == S_usbHcdLib_STALLED)
	    {
  	    if (usbdFeatureClear (usbNC1080Handle, 
				  pDrvCtrl->pDev->nodeId, 
				  USB_RT_STANDARD | USB_RT_ENDPOINT, 
				  0, 
				  1) 
			      == ERROR)
		{
	        DRV_LOG (DRV_DBG_TX, "Could not clear STALL.\n",
	  	  pIrp->result, 0, 0, 0, 0, 0);
		}
	    }

        pDrvCtrl->outErrors++;	/* Should also Update MIB */
	}

    /* Release the sync semaphore */

    OSS_SEM_GIVE (usbNC1080IrpSem);
    }

/**************************************************************************
*
* usbNC1080MCastAdd - add a multicast address for the device
*
* This function is not supported.
* RETURNS: ERROR 
*/

LOCAL STATUS usbNC1080MCastAdd
    (
    NC1080_END_DEV * pDrvCtrl,		/* device pointer */
    char* pAddress				/* new address to add */
    )
    {
    return (ERROR);
    }

/**************************************************************************
*
* usbNC1080MCastDel - delete a multicast address for the device
*
* This function is not supported.
* RETURNS: ERROR.
*/

LOCAL STATUS usbNC1080MCastDel
    (
    NC1080_END_DEV * pDrvCtrl,		/* device pointer */
    char* pAddress				/* new address to add */
    )
    {
    return (ERROR);
    }


/**************************************************************************
*
* usbNC1080MCastGet - get the multicast address list for the device
*
* This routine is not supported.
* RETURNS: ERROR.
*/

LOCAL STATUS usbNC1080MCastGet
    (
    NC1080_END_DEV * pDrvCtrl,		/* device pointer */
    MULTI_TABLE * pTable		/* address table to be filled in */
    )
    {
    return (ERROR);
    }

/**************************************************************************
*
* usbNC1080Ioctl - the driver I/O control routine
*
* Process an ioctl request.
*
* RETURNS: A command specific response, usually OK or ERROR.
*/

LOCAL int usbNC1080Ioctl
    (
    NC1080_END_DEV * pDrvCtrl,		/* device receiving command */
    int cmd,					/* ioctl command code */
    caddr_t data				/* command argument */
    )
    {

    int error = 0;
    long value;

    switch (cmd)
        {
        case EIOCSADDR	:		/* Set Device Address */

	    return ERROR;

        case EIOCGADDR	:		/* Get Device Address */

	    if (data == NULL)
		return (EINVAL);

            bcopy ((char *)NETCHIP_HADDR (&pDrvCtrl->endObj), 
		   (char *)data, 
		   NETCHIP_HADDR_LEN (&pDrvCtrl->endObj));

            break;

        case EIOCSFLAGS	:		/* Set Device Flags */
	    
	    value = (long)data;
	    if (value < 0)
	        {
		value = -(--value);
		END_FLAGS_CLR (&pDrvCtrl->endObj, value);
		}
	    else
		{
		END_FLAGS_SET (&pDrvCtrl->endObj, value);
		}
            break;


        case EIOCGFLAGS:		/* Get Device Flags */
	
	    *(int *)data = END_FLAGS_GET(&pDrvCtrl->endObj);
	    
            break;


	case EIOCPOLLSTART :		/* Begin polled operation */

	    return EINVAL;		/* Not supported */


	case EIOCPOLLSTOP :		/* End polled operation */

	    return EINVAL;		/* Not supported */


        case EIOCGMIB2	:		/* return MIB information */

            if (data == NULL)
                return (EINVAL);

            bcopy ((char *)&pDrvCtrl->endObj.mib2Tbl, 
		   (char *)data,
                   sizeof(pDrvCtrl->endObj.mib2Tbl));

            break;

        case EIOCGFBUF	:		/* return minimum First Buffer for chaining */

            if (data == NULL)
                return (EINVAL);

            *(int *)data = NETCHIP_MIN_FBUF;

            break;

	case EIOCMULTIADD : 		/* Add a Multicast Address */

	    return EINVAL;		/* Not supported */


	case EIOCMULTIDEL : 		/* Delete a Multicast Address */

	    return EINVAL;		/* Not supported */
	    break;


	case EIOCMULTIGET : 		/* Get the Multicast List */

	    return EINVAL;		/* Not supported */
	    break;

       default:
            error = EINVAL;
	
        }

    return (error);
    }

/**************************************************************************
*
* usbNC1080PollRcv - routine to receive a packet in polled mode.
*
* This routine is NOT supported
*
* RETURNS: ERROR Always
*/

LOCAL STATUS usbNC1080PollRcv
    (
    NC1080_END_DEV * pDrvCtrl,		/* device to be polled */
    M_BLK_ID      pMblk			/* ptr to buffer */
    )
    {
    return (ERROR);
    }

/**************************************************************************
*
* usbNC1080PollSend - routine to send a packet in polled mode.
*
* This routine is NOT SUPPORTED
*
* RETURNS: ERROR always
*/

LOCAL STATUS usbNC1080PollSend
    (
    NC1080_END_DEV * 	pDrvCtrl,	/* device to be polled */
    M_BLK_ID    	pMblk		/* packet to send */
    )
    {
    return (ERROR);
    }

/***************************************************************************
*
* notifyAttach - Notifies registered callers of attachment/removal
*
* RETURNS: N/A
*/

LOCAL VOID notifyAttach
    (
    USB_NC1080_DEV* pDev,
    UINT16 attachCode
    )

    {

    pATTACH_REQUEST pRequest = usbListFirst (&reqList);

    DRV_LOG (DRV_DBG_ATTACH, "We are in notify attach\n",
		     0, 0, 0, 0, 0, 0);

    while (pRequest != NULL)
    {
    (*pRequest->callback) (pRequest->callbackArg, 
	                   pDev, attachCode);
    pRequest = usbListNext (&pRequest->reqLink);

    DRV_LOG (DRV_DBG_ATTACH, "Attach call back executed\n",
		     0, 0, 0, 0, 0, 0);
	
    }
    }

/***************************************************************************
*
* usbNC1080DynamicAttachRegister - Register NETCHIP device attach callback.
*
* <callback> is a caller-supplied function of the form:
*
* .CS
* typedef (*USB_NETCHIP_ATTACH_CALLBACK) 
*     (
*     pVOID arg,
*     USB_NC1080_DEV * pDev,
*     UINT16 attachCode
*     );
* .CE
*
* usbNC1080End will invoke <callback> each time a NETCHIP device
* is attached to or removed from the system.  <arg> is a caller-defined
* parameter which will be passed to the <callback> each time it is
* invoked.  The <callback> will also be passed the nodeID of the device 
* being created/destroyed and an attach code of USB_NETCHIP_ATTACH or 
* USB_NETCHIP_REMOVE.
*
* NOTE: The user callback routine should not invoke any driver function that
* submits IRPs.  Further processing must be done from a different task context.
* As the driver routines wait for IRP completion, they cannot be invoked from
* USBD client task's context created for this driver.
*
*
* RETURNS: OK, or ERROR if unable to register callback
*
* ERRNO:
*   S_usbNC1080Lib_BAD_PARAM
*   S_usbNC1080Lib_OUT_OF_MEMORY
*/

STATUS usbNC1080DynamicAttachRegister
    (
    USB_NETCHIP_ATTACH_CALLBACK callback,	/* new callback to be registered */
    pVOID arg								/* user-defined arg to callback */
    )

    {
    pATTACH_REQUEST   pRequest;
    USB_NC1080_DEV  *	      pNC1080Dev;
    int status = OK;


    /* Validate parameters */

    if (callback == NULL)
        return (ossStatus (S_usbNC1080Lib_BAD_PARAM));

    OSS_MUTEX_TAKE (usbNC1080Mutex, OSS_BLOCK);

    /* Create a new request structure to track this callback request. */

    if ((pRequest = OSS_CALLOC (sizeof (*pRequest))) == NULL)
        {
        status = ossStatus (S_usbNC1080Lib_OUT_OF_MEMORY);
        }
    else
        {
        pRequest->callback    = callback;
        pRequest->callbackArg = arg;

        usbListLink (&reqList, pRequest, &pRequest->reqLink, LINK_TAIL);
    
       /* 
        * Perform an initial notification of all currrently attached
        * NETCHIP devices.
        */

        pNC1080Dev = usbListFirst (&usbNC1080DevList);

        while (pNC1080Dev != NULL)
	    {
            if (pNC1080Dev->connected)
                (*callback) (arg, pNC1080Dev, USB_NETCHIP_ATTACH);

	    pNC1080Dev = usbListNext (&pNC1080Dev->devLink);
	    }

        }

    OSS_MUTEX_RELEASE (usbNC1080Mutex);

    return (ossStatus (status));
    }


/***************************************************************************
*
* usbNC1080DynamicAttachUnregister - Unregisters NETCHIP attach callback.
*
* This function cancels a previous request to be dynamically notified for
* NETCHIP device attachment and removal.  The <callback> and <arg> paramters 
* must exactly match those passed in a previous call to 
* usbNC1080DynamicAttachRegister().
*
* RETURNS: OK, or ERROR if unable to unregister callback
*
* ERRNO:
*   S_usbNC1080Lib_NOT_REGISTERED
*/

STATUS usbNC1080DynamicAttachUnregister
    (
    USB_NETCHIP_ATTACH_CALLBACK callback, /* callback to be unregistered */
    pVOID arg				  /* user-defined arg to callback */
    )

    {
    pATTACH_REQUEST pRequest;
    int status = S_usbNC1080Lib_NOT_REGISTERED;

    OSS_MUTEX_TAKE (usbNC1080Mutex, OSS_BLOCK);

    pRequest = usbListFirst (&reqList);

    while (pRequest != NULL)
        {
        if ((callback == pRequest->callback) && (arg == pRequest->callbackArg))
	    {
	    /* We found a matching notification request. */

	    usbListUnlink (&pRequest->reqLink);

        /* Dispose of structure */

        OSS_FREE (pRequest);
	    status = OK;

	    break;
	    }
        pRequest = usbListNext (&pRequest->reqLink);
	}

    OSS_MUTEX_RELEASE (usbNC1080Mutex);

    return (ossStatus (status));
    }


/***************************************************************************
*
* usbNC1080DevLock - Marks USB_NC1080_DEV structure as in use.
*
* A caller uses usbNC1080DevLock() to notify usbNC1080End that
* it is using the indicated USB_NC1080_DEV structure.  usbNC1080End maintains
* a count of callers using a particular USB_NC1080_DEV structure so that it 
* knows when it is safe to dispose of a structure when the underlying
* USB_NC1080_DEV is removed from the system.  So long as the "lock count"
* is greater than zero, usbNC1080End will not dispose of an USB_NC1080_DEV
* structure.
*
* RETURNS: OK, or ERROR if unable to mark USB_NC1080_DEV structure in use.
*/

STATUS usbNC1080DevLock
    (
    USBD_NODE_ID nodeId    /* NodeId of the USB_NC1080_DEV to be marked as in use */
    )

    {
    USB_NC1080_DEV* pNC1080Dev = usbNC1080FindUsbNode (nodeId);

    if ( pNC1080Dev == NULL)
        return (ERROR);

    pNC1080Dev->lockCount++;

    return (OK);
    }
 

/***************************************************************************
*
* usbNC1080DevUnlock - Marks USB_NC1080_DEV structure as unused.
*
* This function releases a lock placed on an USB_NC1080_DEV structure.  When a
* caller no longer needs an USB_NC1080_DEV structure for which it has previously
* called usbNC1080DevLock(), then it should call this function to
* release the lock.
*
* RETURNS: OK, or ERROR if unable to mark USB_NC1080_DEV structure unused
*
* ERRNO:
*   S_usbNC1080Lib_NOT_LOCKED
*/

STATUS usbNC1080DevUnlock
    (
    USBD_NODE_ID nodeId    /* NodeId of the BLK_DEV to be marked as unused */
    )

    {
    int status = OK;

    USB_NC1080_DEV *      pNC1080Dev =  usbNC1080FindUsbNode (nodeId);
 
    if ( pNC1080Dev == NULL)
        return (ERROR);

    OSS_MUTEX_TAKE (usbNC1080Mutex, OSS_BLOCK);

    if (pNC1080Dev->lockCount == 0)
        {
        status = S_usbNC1080Lib_NOT_LOCKED;
        }
    else
    	{

    	/* 
     	* If this is the last lock and the underlying NETCHIP device is
     	* no longer connected, then dispose of the device.
     	*/

     	if ((--pNC1080Dev->lockCount == 0) && (!pNC1080Dev->connected))
	   usbNC1080DestroyDevice ((NC1080_END_DEV *)pNC1080Dev->pDevStructure);
    	}

    OSS_MUTEX_RELEASE (usbNC1080Mutex);

    return (ossStatus (status));
    }
