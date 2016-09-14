/* usbAcmLib.c - USB Communications Class - Abstract Control Model Driver */

/* Copyright 2000-2001 Wind River systems, Inc */

/*
modification history
--------------------
01b,08aug01,dat  Removing warnings
01a,19sep00,bri  Created
*/

/*

DESCRIPTION

This module implements the USB ACM Class Driver for the vxWorks operating
system.  This module presents an interface which is a superset of the vxWorks
SIO (serial IO) driver model.  That is, this driver presents the external APIs
which would be expected of a standard "multi-mode serial (SIO) driver" and
adds certain extensions which are needed to address adequately the requirements
of the hot-plugging USB environment.USB modems are described in the USB
communications class / acm sub class definition.  

The Client (user) interaction with the device is via the VxWorks IO system. 
This device appears as an SIO device and the driver exports the functions 
as required by VxWorks SIO driver Model. 

Unlike most SIO drivers, the number of channels supported by this driver is not
fixed.	Rather, USB modems may be added or removed from the system at any
time.  This creates a situation in which the number of modems is dynamic, and
clients of usbAcmLib need to be made aware of the appearance and 
disappearance of channels.  Therefore, this driver adds an additional set of
functions which allows clients to register for notification upon the insertion
and removal of USB modems, and hence the creation and deletion of channels.

This module itself is a client of the Universal Serial Bus Driver (USBD).  All
interaction with the USB buses and devices is handled through the USBD.


INITIALIZATION

This driver must be initialized by calling usbAcmLibInit(). usbAcmLibInit() in
turn initializes its connection to the USBD and other internal resources needed
for operation.

Prior to calling usbAcmLibInit(), the caller must ensure that the USBD
has been properly initialized by calling - at a minimum - usbdInitialize().
It is also the caller's responsibility to ensure that at least one USB HCD
(USB Host Controller Driver) is attached to the USBD - using the USBD function
usbdHcdAttach() - before modem operation can begin.  However, it is not 
necessary for usbdHcdAttach() to be called prior to initializating usbAcmLib.
usbAcmLib uses the USBD dynamic attach services and is capable of 
recognizing USB Modem attachment and removal on the fly.  Therefore, it is 
possible for USB HCDs to be attached to or detached from the USBD at run time
- as may be required, for example, in systems supporting hot swapping of
hardware.

usbAcmLib does not export entry points for transmit, receive, and error
interrupt entry points like traditional SIO drivers.  All "interrupt" driven
behavior is managed by the underlying USBD and USB HCD(s), so there is no
need for a caller (or BSP) to connect interrupts on behalf of usbAcmLib.
For the same reason, there is no post-interrupt-connect initialization code
and usbAcmLib.c therefore also omits the "devInit2" entry point.


OTHER FUNCTIONS

usbAcmLib also supports the SIO ioctl interface. All the set baudrate, 
get baud rate functions will work as per the SIO driver specification. 
Additional ioctl functions have been added to allow the caller to use 
the additional commands specified in the USB ACM class specification. 
However the modem can be treated as any serial modem and can be 
controlled.

Modems work on AT commands. The AT commands can be sent via the standard
way i.e., through the Bulk Out pipe. Here the driver doesn't differentiate 
between AT commands and any other data. 


DATA FLOW

For each USB modem connected to the system, usbAcmLib sets up  USB pipes
to transmit and receive data to the Modem. 

The USB Modem SIO driver supports only the SIO "interrupt" mode of operation
- SIO_MODE_INT. Any attempt to place the driver in the polled mode will return
an error.

INCLUDE FILES:
usbAcmLib.h

*/

/* includes */

#include "vxWorks.h"
#include "string.h"
#include "stdio.h"
#include "sioLib.h"
#include "errno.h"
#include "ctype.h"
#include "logLib.h"

#include "usb/usbPlatform.h"
#include "usb/ossLib.h"			/* operations system srvcs */
#include "usb/usb.h"			/* general USB definitions */
#include "usb/usbListLib.h"		/* linked list functions */
#include "usb/usbdLib.h"		/* USBD interface */
#include "usb/usbLib.h"			/* USB utility functions */

#include "usb/usbCommdevices.h"
#include "drv/usb/usbAcmLib.h"		/* our API */

/* defines */

#define ACM_CLIENT_NAME     "usbAcmLib" /* USBD client name */

/* for debugging */

#define USB_ACM_DEBUG

#ifdef	USB_ACM_DEBUG

#define USB_ACM_DEBUG_OFF		0x0000
#define USB_ACM_DEBUG_RX		0x0001
#define	USB_ACM_DEBUG_TX		0x0002
#define USB_ACM_DEBUG_BLOCK		0x0004
#define	USB_ACM_DEBUG_ATTACH		0X0008
#define	USB_ACM_DEBUG_INIT		0x0010
#define	USB_ACM_DEBUG_OPEN		0x0020
#define	USB_ACM_DEBUG_CLOSE		0x0040
#define	USB_ACM_DEBUG_IOCTL		0x0080
#define	USB_ACM_DEBUG_INT		0x0100

int	usbAcmDebug = (0x0);	

#define USB_ACM_LOG(FLG, X0, X1, X2, X3, X4, X5, X6)    \
	if (usbAcmDebug & FLG)                           \
            logMsg(X0, X1, X2, X3, X4, X5, X6);

#else /*USB_ACM_DEBUG*/

#define USB_ACM_LOG(DBG_SW, X0, X1, X2, X3, X4, X5, X6)

#endif /*USB_ACM_DEBUG*/

/*
 * ATTACH_REQUEST
 */

typedef struct attach_request
    {
    
    LINK reqLink;			/* linked list of requests  */
    USB_ACM_CALLBACK callback;		/* client callback routine  */
    pVOID callbackArg;			/* client callback argument */
    
    }ATTACH_REQUEST, *pATTACH_REQUEST;

/* forward static declarations */

int 	usbAcmTxStartup 	(SIO_CHAN * pSioChan);
LOCAL 	int usbAcmPollOutput 	(SIO_CHAN * pSioChan, char    outChar);
LOCAL 	int usbAcmPollInput  	(SIO_CHAN * pSioChan, char *thisChar);
int 	usbAcmIoctl 	   	(SIO_CHAN * pSioChan, int request, void *arg);

LOCAL VOID usbAcmTxIrpCallback 	(pVOID p);
LOCAL VOID usbAcmRxIrpCallback 	(pVOID p);

LOCAL VOID destroySioChan 	(pUSB_ACM_SIO_CHAN pSioChan);
LOCAL STATUS usbAcmCtrlCmdSend 	(pUSB_ACM_SIO_CHAN pSioChan, 
				 UINT16 request,             
	  	  		 UINT8 * pBuf,               
				 UINT16  count               
			        );
LOCAL STATUS usbAcmOpen		(SIO_CHAN * pSioChan);

/* locals */

LOCAL UINT16 initCount = 0;	 /* Count of init nesting */

LOCAL MUTEX_HANDLE acmMutex;	 /* mutex used to protect internal structs */

LOCAL LIST_HEAD sioList;	 /* linked list of USB_ACM_SIO_CHAN */
LOCAL LIST_HEAD reqList;	 /* Attach callback request list */

LOCAL USBD_CLIENT_HANDLE usbdHandle; /* our USBD client handle */

LOCAL SEM_HANDLE   acmIrpSem;    /* Semaphore for IRP Synchronisation */


/* Channel function table. */

LOCAL SIO_DRV_FUNCS usbAcmSioDrvFuncs =
    {
    usbAcmIoctl,
    usbAcmTxStartup,
    usbAcmCallbackRegister,
    usbAcmPollInput,
    usbAcmPollOutput
    };

/***************************************************************************
*
* notifyAttach - Notifies registered callers of attachment/removal events
*
* RETURNS: N/A
*/

LOCAL VOID notifyAttach
    (
    pUSB_ACM_SIO_CHAN pSioChan,
    UINT16 attachCode
    )

    {
    pATTACH_REQUEST pRequest = usbListFirst (&reqList);

    while (pRequest != NULL)
	{
        /* Invoke the Callback registered */

        (*pRequest->callback) (pRequest->callbackArg, 
	    (SIO_CHAN *) pSioChan, attachCode, NULL, NULL);
        
        /* Move to the next client */

	pRequest = usbListNext (&pRequest->reqLink);
	}
    }

/***************************************************************************
*
* findEndpoint - Searches for a <type> endpoint of the <direction> direction.
*
* RETURNS: pointer to matching endpoint descriptor or NULL if not found
*/

LOCAL pUSB_ENDPOINT_DESCR findEndpoint
    (
    pUINT8 pBfr,
    UINT16 bfrLen,
    UINT16 direction,
    UINT16 attributes
    )

    {
    pUSB_ENDPOINT_DESCR pEp;

    while ((pEp = usbDescrParseSkip (&pBfr, &bfrLen, USB_DESCR_ENDPOINT)) 
		  != NULL)
	{
	if ((pEp->attributes & USB_ATTR_EPTYPE_MASK) == attributes &&
	    (pEp->endpointAddress & USB_ENDPOINT_DIR_MASK) == direction)
	    break;
	}

    return pEp;
    }

/***************************************************************************
*
* configureSioChan - configure USB Modem for operation.
*
* Selects the configuration/interfaces specified in the <pSioChan>
* structure.  These values come from the USBD dynamic attach callback,
* which in turn are retrieved from the configuration/interface
* descriptors which reported the device to be a modem.
*
* ACM Specification requires that the device supporting two interfaces.
* 1. Communication Interface class
* 2. Data Interface class.
*
* We've registered (with usbd) for getting notified when a ACM device 
* is attached.  While registering, we supplied the Communications Class 
* information and Communications interface class information.  The 
* configuration with the communication interface class also should support 
* the data interface class.  Also the communication interface class shall 
* support the common AT command set protocol.
*
* RETURNS: OK if successful, else ERROR if failed to configure channel
*/

LOCAL STATUS configureSioChan
    (
    pUSB_ACM_SIO_CHAN pSioChan,
    UINT16 configuration,
    UINT16 interface
    )

    {
    pUSB_CONFIG_DESCR pCfgDescr;
    pUSB_INTERFACE_DESCR pIfDescr;
    pUSB_ENDPOINT_DESCR pOutEp = NULL;
    pUSB_ENDPOINT_DESCR pInEp = NULL;
    pUSB_ENDPOINT_DESCR pIntrEp = NULL;
    UINT8 bfr [USB_MAX_DESCR_LEN];
    pUINT8 pBfr;
    UINT16 actLen;
    UINT16 ifNo;

    /* 
     * Read the configuration descriptor to get the configuration selection
     * value and to determine the device's power requirements.
     */

    if (usbdDescriptorGet (usbdHandle, 
			   pSioChan->nodeId,
			   USB_RT_STANDARD | USB_RT_DEVICE, USB_DESCR_CONFIGURATION, 
			   (configuration - 1), 
			   0, 
			   sizeof (bfr), 
			   bfr, 
			   &actLen) != OK)
	return ERROR;

    if ((pCfgDescr = usbDescrParse (bfr, actLen, USB_DESCR_CONFIGURATION)) 
	== NULL)
	return ERROR;

    pSioChan->configuration = configuration;

    /*
     * Now look for the Data Interface Class  and Communication interface 
     * Class. Then find the Corresponding End points
     */
    
    ifNo = 0;
    pBfr = bfr;

    while ((pIfDescr = usbDescrParseSkip (&pBfr, &actLen, USB_DESCR_INTERFACE)) 
					 != NULL)
	{
        
        if (pIfDescr->interfaceClass == USB_CLASS_COMMINTERFACE)
            {
            
            if (pIfDescr->interfaceProtocol == USB_COMM_PROTOCOL_COMMONAT)
                {
                
                pSioChan->ifaceCommClass = ifNo;
                pSioChan->ifaceCommAltSetting = pIfDescr->alternateSetting;
                pSioChan->protocol = USB_COMM_PROTOCOL_COMMONAT;

                /* It should have an interrupt end point. Find out */

                if ((pIntrEp = findEndpoint (pBfr, actLen, 
                                            USB_ENDPOINT_IN, 
                                            USB_ATTR_INTERRUPT)) == NULL)
                    {

                    USB_ACM_LOG (USB_ACM_DEBUG_ATTACH, 
                                " No Interrupt End point \n",0, 0, 0, 0, 0, 0);
                    return ERROR;
                    
                    }

                usbdInterfaceSet (usbdHandle, pSioChan->nodeId,
            	                pSioChan->ifaceCommClass, 
                                pSioChan->ifaceCommAltSetting);
                }
            }
        else
            {
            if (pIfDescr->interfaceClass == USB_CLASS_DATAINTERFACE)
                {
                  pSioChan->ifaceDataClass = ifNo;
                  pSioChan->ifaceDataAltSetting = pIfDescr->alternateSetting;

                  /* It should have a Bulk I/P and a Bulk O/P end points */
                  
                  if ((pInEp = findEndpoint (pBfr, 
					     actLen, 
                                             USB_ENDPOINT_IN,
                                             USB_ATTR_BULK)) == NULL)
                      {
                    
                      USB_ACM_LOG (USB_ACM_DEBUG_ATTACH, 
                                   " No Input End point \n",0, 0, 0, 0, 0, 0);
                      return ERROR;
                      
                      }
                  
                  if ((pOutEp = findEndpoint (pBfr, actLen, 
                                            USB_ENDPOINT_OUT, 
                                            USB_ATTR_BULK)) == NULL)
                      {
                      
                      USB_ACM_LOG (USB_ACM_DEBUG_ATTACH, 
                                   " No Output End point \n",0, 0, 0, 0, 0, 0);
                      return ERROR;
                      
                      }

                  usbdInterfaceSet (usbdHandle, pSioChan->nodeId,
            	                    pSioChan->ifaceDataClass, 
                                    pSioChan->ifaceDataAltSetting);
                }
            }

        ifNo++;

        if (pIntrEp != NULL)
            if (pOutEp != NULL)
                if (pInEp != NULL)
                    break;
        }

    if (pIfDescr == NULL)
        {
        USB_ACM_LOG (USB_ACM_DEBUG_ATTACH, " No appropriate interface \
            descriptor ",0, 0, 0, 0, 0, 0);
	return ERROR;
        }

    if ((pInEp == NULL) || (pOutEp == NULL) || (pIntrEp == NULL))
	{
	USB_ACM_LOG (USB_ACM_DEBUG_ATTACH, " No End points \n",0, 0, 0, 0, 0, 0);
	}

    USB_ACM_LOG (USB_ACM_DEBUG_ATTACH, "Intr EP : 0x%x :: \
        In EP : 0x%x  :: Out EP : 0x%x \n", (UINT16)pIntrEp->endpointAddress, 
                  (UINT16)(pInEp->endpointAddress), 
                  (UINT16)(pOutEp->endpointAddress), 0, 0, 0);

    /* Fill in the maximum packet sizes */

    pSioChan->outBfrLen = pOutEp->maxPacketSize;
    pSioChan->inBfrLen = pInEp->maxPacketSize;

    /* Select the configuration. */

    if (usbdConfigurationSet (usbdHandle, 
			      pSioChan->nodeId,
			      pCfgDescr->configurationValue, 
			      pCfgDescr->maxPower * USB_POWER_MA_PER_UNIT) 
			    != OK)
	return ERROR;

    /* Create a pipe for output to the modem. */

    if (usbdPipeCreate (usbdHandle, 
			pSioChan->nodeId, 
			pOutEp->endpointAddress, 
			pCfgDescr->configurationValue,
			pSioChan->ifaceDataClass, 
			USB_XFRTYPE_BULK, 
			USB_DIR_OUT,	
			FROM_LITTLEW (pOutEp->maxPacketSize), 
			0, 
			0, 
			&pSioChan->outPipeHandle) 
		       != OK)
	{
	return ERROR;
	}

    /* 
     * Create a pipe to listen for input from the device 
     */

    if (usbdPipeCreate (usbdHandle, 
			pSioChan->nodeId, 
			pInEp->endpointAddress, 
			pCfgDescr->configurationValue,
			pSioChan->ifaceDataClass, 
			USB_XFRTYPE_BULK, 
			USB_DIR_IN,	
			FROM_LITTLEW (pInEp->maxPacketSize), 
			0, 
			0, 
			&pSioChan->inPipeHandle) 
		      != OK)
	{
	return ERROR;
	}

    /* set the default settings for the modem */

    pSioChan->lineCode.baudRate = 9600;    /* Bits/sec */
    pSioChan->lineCode.noOfStopBits = USB_ACM_STOPBITS_1; /* 1 stop bit */
    pSioChan->lineCode.parityType = USB_ACM_PARITY_NONE; /* No parity */
    pSioChan->lineCode.noOfDataBits = 8;   /* 8 data bits */

    if (usbAcmCtrlCmdSend (pSioChan, 
			   USB_ACM_REQ_LINE_CODING_SET, 
			   (UINT8 *)(&pSioChan->lineCode), 
			   sizeof(pSioChan->lineCode)) 
			 == ERROR)
	{
        USB_ACM_LOG (USB_ACM_DEBUG_ATTACH, "LineCode could not be set\n",
	    0, 0, 0, 0, 0, 0);
	}

    return OK;
    }

/***************************************************************************
*
* createSioChan - creates a new USB_ACM_SIO_CHAN structure
*
* Creates a new USB_ACM_SIO_CHAN structure for the indicated <nodeId>.
* If successful, the new structure is linked into the sioList upon 
* return.
*
* <configuration> and <interface> identify the configuration/interface
* that first reported itself as a modem for this device.
*
* RETURNS: pointer to newly created structure, or NULL if failure
*/

LOCAL pUSB_ACM_SIO_CHAN createSioChan
    (
    USBD_NODE_ID nodeId,
    UINT16 configuration,
    UINT16 interface,
    UINT16 deviceProtocol
    )

    {
    pUSB_ACM_SIO_CHAN pSioChan;

    /* Try to allocate space for a new modem struct and its buffers */

    if ((pSioChan = OSS_CALLOC (sizeof (*pSioChan))) == NULL )
        {
        USB_ACM_LOG (USB_ACM_DEBUG_ATTACH, "Out of Memory \n", 
	    0, 0, 0, 0, 0, 0);
        return NULL;
        }

    if((pSioChan->outBfr = OSS_MALLOC (ACM_OUT_BFR_SIZE)) == NULL)
	{
	destroySioChan (pSioChan);
        USB_ACM_LOG (USB_ACM_DEBUG_ATTACH, "Out of Memory \n",
            0, 0, 0, 0, 0, 0);
       	return NULL;
	}

    if ((pSioChan->inBfr = OSS_MALLOC (ACM_IN_BFR_SIZE)) == NULL)
        {
	destroySioChan (pSioChan);
        USB_ACM_LOG (USB_ACM_DEBUG_ATTACH, "Out of Memory \n",
            0, 0, 0, 0, 0, 0);
	return NULL;
	}
    
    pSioChan->sioChan.pDrvFuncs = &usbAcmSioDrvFuncs;

    pSioChan->nodeId = nodeId;
    pSioChan->mode = SIO_MODE_INT;
    pSioChan->connected = TRUE;

    pSioChan->callbackStatus = 0x0;     /* No callbacks installed */
    
    /* Configure the Modem. */

    if (configureSioChan (pSioChan, configuration, interface) != OK)
	{
	destroySioChan (pSioChan);
	return NULL;
	}

    /* Link the newly created structure. */

    usbListLink (&sioList, pSioChan, &pSioChan->sioLink, LINK_TAIL);

    USB_ACM_LOG (USB_ACM_DEBUG_ATTACH, "SIOCHAN created \n",
            0, 0, 0, 0, 0, 0);

    return pSioChan;
    }

/***************************************************************************
*
* findSioChan - Searches for a USB_ACM_SIO_CHAN for indicated node ID
*
* RETURNS: pointer to matching USB_ACM_SIO_CHAN or NULL if not found
*/

LOCAL pUSB_ACM_SIO_CHAN findSioChan
    (
    USBD_NODE_ID nodeId
    )

    {
    pUSB_ACM_SIO_CHAN pSioChan = usbListFirst (&sioList);

    while (pSioChan != NULL)
	{
	if (pSioChan->nodeId == nodeId)
	    break;

	pSioChan = usbListNext (&pSioChan->sioLink);
	}

    return pSioChan;
    }

/***************************************************************************
*
* usbAcmAttachCallback - called by USBD when a modem is attached/removed
*
* The USBD will invoke this callback when a USB modem is attached to or
* removed from the system.  <nodeId> is the USBD_NODE_ID of the node being
* attached or removed.	<attachAction> is USBD_DYNA_ATTACH or USBD_DYNA_REMOVE.
* modems report their class information at the interface level,
* so <configuration> and <interface> will indicate the configuratin/interface
* that reports itself as a modem.  <deviceClass> and <deviceSubClass> 
* will match the class/subclass for which we registered.  <deviceProtocol>
* shall be USB_COMM_PROTOCOL_COMMONAT.
*
* NOTE: The USBD will invoke this function once for each configuration/
* interface which reports itself as a modem.  So, it is possible that
* a single device insertion/removal may trigger multiple callbacks.  We
* ignore all callbacks except the first for a given device.
*
* RETURNS: N/A
*/

LOCAL VOID usbAcmAttachCallback
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
    pUSB_ACM_SIO_CHAN pSioChan;

    OSS_MUTEX_TAKE (acmMutex, OSS_BLOCK);

    /* 
     * Depending on the attach code, add a new modem or remove one
     * that's already been created.
     */

    switch (attachAction)
	{
	case USBD_DYNA_ATTACH:

	    /* 
             * A new device is being attached.	Check if we already 
	     * have a structure for this device.
	     */

	    if (findSioChan (nodeId) != NULL)
		break;

            USB_ACM_LOG (USB_ACM_DEBUG_ATTACH, "New ACM device found \n",
                0, 0, 0, 0, 0, 0);

            /* 
             * Create a new structure to manage this device.  If there's
	     * an error, there's nothing we can do about it, so skip the
	     * device and return immediately. 
	     */

	    if ((pSioChan = createSioChan (nodeId, configuration, interface,
		deviceProtocol)) == NULL)
                {
                USB_ACM_LOG (USB_ACM_DEBUG_ATTACH, "Could not create \
                    SIO_CHAN \n", 0, 0, 0, 0, 0, 0);
		break;
                }

	    /* 
             * Notify registered callers that a new modem has been
	     * added and a new channel created.
	     */

	    notifyAttach (pSioChan, USB_ACM_CALLBACK_ATTACH);

            usbAcmOpen ((SIO_CHAN *)pSioChan);

	    break;


	case USBD_DYNA_REMOVE:

	    /* 
             * A device is being detached.  Check if we have any
	     * structures to manage this device.
	     */

	    if ((pSioChan = findSioChan (nodeId)) == NULL)
		break;

	    /* The device has been disconnected. */

	    pSioChan->connected = FALSE;

	    /* 
             * Notify registered callers that the modem has been
	     * removed and the channel disabled. 
	     *
	     * NOTE: We temporarily increment the channel's lock count
	     * to prevent usbAcmSioChanUnlock() from destroying the
	     * structure while we're still using it.
	     */

	    pSioChan->lockCount++;

	    notifyAttach (pSioChan, USB_ACM_CALLBACK_DETACH);

	    pSioChan->lockCount--;

	    /* 
             * If no callers have the channel structure locked, destroy
	     * it now.	If it is locked, it will be destroyed later during
	     * a call to usbAcmUnlock().
	     */

	    if (pSioChan->lockCount == 0)
		destroySioChan (pSioChan);

	    break;
	}

    OSS_MUTEX_RELEASE (acmMutex);
    }

/***************************************************************************
*
* destroyAttachRequest - disposes of an ATTACH_REQUEST structure
*
* RETURNS: N/A
* NOMANUAL
*/

LOCAL VOID destroyAttachRequest
    (
    pATTACH_REQUEST pRequest
    )
    {
    /* Unlink request */

    usbListUnlink (&pRequest->reqLink);

    /* Dispose of structure */

    OSS_FREE (pRequest);
    }

/***************************************************************************
*
* destroySioChan - disposes of a USB_PRN_ACM_CHAN structure
*
* Unlinks the indicated USB_ACM_SIO_CHAN structure and de-allocates
* resources associated with the channel.
*
* RETURNS: N/A
*/

LOCAL VOID destroySioChan
    (
    pUSB_ACM_SIO_CHAN pSioChan
    )
    {
    if (pSioChan != NULL)
	{
	/* Unlink the structure. */

	usbListUnlink (&pSioChan->sioLink);

	/* Release pipes and wait for IRPs to be cancelled if necessary. */

	if (pSioChan->outPipeHandle != NULL)
	    usbdPipeDestroy (usbdHandle, pSioChan->outPipeHandle);

	if (pSioChan->inPipeHandle != NULL)
	    usbdPipeDestroy (usbdHandle, pSioChan->inPipeHandle);

	if (pSioChan->intrPipeHandle != NULL)
	    usbdPipeDestroy (usbdHandle, pSioChan->intrPipeHandle);

	while (pSioChan->outIrpInUse || pSioChan->inIrpInUse 
                || pSioChan->intrIrpInUse)
            {
	    OSS_THREAD_SLEEP (1);
            }

	/* release buffers */

	if (pSioChan->outBfr != NULL)
	    OSS_FREE (pSioChan->outBfr);

	if (pSioChan->inBfr != NULL)
	    OSS_FREE (pSioChan->inBfr);

	/* Release structure. */

	OSS_FREE (pSioChan);
	}
    }
    
/***************************************************************************
*
* doShutdown - shuts down USB ACM SIO driver
*
* <errCode> should be OK or S_usbAcmLib_xxxx.  This value will be
* passed to ossStatus() and the return value from ossStatus() is the
* return value of this function.
*
* RETURNS: OK, or ERROR per value of <errCode> passed by caller
*/

LOCAL STATUS doShutdown
    (
    int errCode
    )

    {
    pATTACH_REQUEST pRequest;
    USB_ACM_SIO_CHAN * pSioChan;


    /* Dispose of any outstanding notification requests */

    while ((pRequest = usbListFirst (&reqList)) != NULL)
	destroyAttachRequest (pRequest);

    /* Dispose of any open connections. */

    while ((pSioChan = usbListFirst (&sioList)) != NULL)
	destroySioChan (pSioChan);
	
    /* 
     * Release our connection to the USBD.  The USBD automatically 
     * releases any outstanding dynamic attach requests when a client
     * unregisters.
     */

    if (usbdHandle != NULL)
	{
	usbdClientUnregister (usbdHandle);
	usbdHandle = NULL;
	}

    /* Release resources. */

    if (acmMutex != NULL)
	{
	OSS_MUTEX_DESTROY (acmMutex);
	acmMutex = NULL;
	}

    if (acmIrpSem != NULL)
        {
        OSS_SEM_DESTROY (acmIrpSem);
        acmIrpSem = NULL;
        }

    return ossStatus (errCode);
    }

/**************************************************************************
*
* usbAcmLibInit - initialize USB ACM SIO driver.
*
* Initializes the USB ACM SIO driver.  The USB acm SIO driver
* maintains an initialization count, so calls to this function may be
* nested. This function will regsiter the driver as a client for the USBD. It
* also registers itself for a notification (by usbd) when a ACM device
* is dynamically attached / removed from the USB.
*
* RETURNS: OK, or ERROR if unable to initialize.
*
* ERRNO:
*
*   S_usbAcmLib_OUT_OF_RESOURCES
*   S_usbAcmLib_USBD_FAULT
*/

STATUS usbAcmLibInit (void)
    {
    /* 
     * If not already initialized, then initialize internal structures
     * and connection to USBD.
     */

    initCount++;
    if (initCount > 1)
        return OK;
           
    /* Initialize lists, structures, resources. */

    memset (&sioList, 0, sizeof (sioList));
    memset (&reqList, 0, sizeof (reqList));
    acmMutex = NULL;
    usbdHandle = NULL;
    acmIrpSem = NULL;

    if (OSS_MUTEX_CREATE (&acmMutex) != OK)
        return doShutdown (S_usbAcmLib_OUT_OF_RESOURCES);

    if (OSS_SEM_CREATE (1, 0 , &acmIrpSem) != OK)
        return (doShutdown (S_usbAcmLib_OUT_OF_RESOURCES));

    /* Establish connection to USBD */

    if (usbdClientRegister (ACM_CLIENT_NAME, &usbdHandle) != OK 
	||
        usbdDynamicAttachRegister (usbdHandle, 
				   USB_CLASS_COMMDEVICE,
				   USB_SUBCLASS_ACM, 
				   USB_COMM_PROTOCOL_COMMONAT,	
				   usbAcmAttachCallback) 
				  != OK)
            {
	    return doShutdown (S_usbAcmLib_USBD_FAULT);
	    }
    
    return OK;
    }

/**************************************************************************
*
* usbAcmCallbackRegister - Installs a callback for an event.
*
* This function installs a callback for a <callbackType> event. If the event
* occurs, the client will be notified via the installed <callback> function.
* <arg> is a client specific argument, used as a parameter to the <callback>
* routine.
*
* The macro USB_ACM_CALLBACK defines a callback routine which will be invoked
* by usbAcmLib when any of these events happen, provided that the user
* registered a callback for such an event. This macro is compatible with the
* SIO TxCallback and RxCallback definitions and provides additional 
* functionality for registering for attachment/removal.
* 
* Note that all these fields are not required for all of the events.
* They will be filled with NULL or ZERO (0) when the callback is executed.
*
*
* .CS
* 
* typedef STATUS (*USB_ACM_CALLBACK)
    (
    pVOID       arg,	            /@ caller-defined argument @/
    SIO_CHAN * pChan,		    /@ pointer to affected SIO_CHAN @/
    UINT16 callbackType,	    /@ defined as USB_ACM_CALLBACK_xxxx @/
    UINT8 * pBuf,                   /@ pointer to data buffer, if any data @/
                                    /@ transfer is involved. Otherwise NULL @/
    UINT16 count                    /@ No of bytes of data transferred @/
                                    /@ if a data transfer is involved. @/
                                    /@ 0 otherwise. @/
    );
*
* .CE
*
* Note that the <pChan> paramter shall be NULL for the Dynamic attachment
* notification requests, indicating that the event is not for any particular
* device.
*
* Care should be taken not to install multiple callbacks for the same event
* for the same <pChan>. How-ever this is not applicable for the dynamic 
* attachment event notification requests.
*
* Different <callbackType> events suppoted by this driver are
*
* .IP "USB_ACM_CALLBACK_ATTACH"
*  Used to get notified of dynamic attachment / removal of usb modems. 
* .IP "USB_ACM_CALLBACK_SIO_TX"
*  Used for transmitting characters. This is equivalent to SIO_CALLBACK_GET_TX_CHAR.
* .IP "USB_ACM_CALLBACK_SIO_RX"
*  Used for receiving characters. This is equivalent to SIO_CALLBACK_PUT_RCV_CHAR.
*
* RETURNS: OK, or ERROR if unable to install.
*
* ERRNO : S_usbAcmLib_OUT_OF_MEMORY,  S_usbAcmLib_BAD_PARAM
*
*/

int usbAcmCallbackRegister 
    (
    SIO_CHAN * pChan,               /* Channel for the callback */
    int      callbackType,          /* Callback type. see above */
    FUNCPTR  callback,      	    /* the callback routine */
    pVOID   arg                     /* User defined argument */
    )
    {

    pUSB_ACM_SIO_CHAN pSioChan = (pUSB_ACM_SIO_CHAN)pChan;
    pATTACH_REQUEST pRequest;
    int status = OK;
    
    if (callback == NULL)
	{
        return ossStatus (S_usbAcmLib_BAD_PARAM);
	}

    if ((callbackType != USB_ACM_CALLBACK_ATTACH) && 
        (callbackType != USB_ACM_CALLBACK_DETACH))
        {
        if (pSioChan == NULL)
	    {
	    return ossStatus (S_usbAcmLib_BAD_PARAM);
	    }
        }

    switch (callbackType)
        {

        case USB_ACM_CALLBACK_ATTACH :
        case USB_ACM_CALLBACK_DETACH :

            /* Client wants to get notified of attachment/removal of modems */
            /* Create a new request structure to track this callback request */

            OSS_MUTEX_TAKE (acmMutex, OSS_BLOCK);

            if ((pRequest = OSS_CALLOC (sizeof (*pRequest))) == NULL)
		{
	        status = S_usbAcmLib_OUT_OF_MEMORY;
		}
            else
	        {
	        pRequest->callback = (USB_ACM_CALLBACK)callback;
	        pRequest->callbackArg = arg;

	        usbListLink (&reqList, pRequest, &pRequest->reqLink, LINK_TAIL);

                /*
                 * Perform an initial notification of all currrently 
                 * attached modems.
                 */
	 	pSioChan = usbListFirst (&sioList);

	        while (pSioChan != NULL)
	            {
	            if (pSioChan->connected)
			(*callback) (arg, 
				     (SIO_CHAN *) pSioChan, 
				     USB_ACM_CALLBACK_ATTACH, 
				     NULL, 
				     0);

	            pSioChan = usbListNext (&pSioChan->sioLink);
	            }
	        }

            OSS_MUTEX_RELEASE (acmMutex);
            return ossStatus (status);

        case USB_ACM_CALLBACK_SIO_TX    :
        case SIO_CALLBACK_GET_TX_CHAR   :

            /* Callback for getting Tx Chars : Normal VxWorks SIO Model */

 	    USB_ACM_LOG (USB_ACM_DEBUG_ATTACH, "Tx callback installed \n",
                0, 0, 0, 0, 0, 0);
            pSioChan->callbackStatus |= USB_ACM_CALLBACK_SIO_TX;

            pSioChan->getTxCharCallback = (FUNCPTR)callback;
	    pSioChan->getTxCharArg = arg;
	    return OK;
        
        case USB_ACM_CALLBACK_SIO_RX    :
        case SIO_CALLBACK_PUT_RCV_CHAR  :

            /* Callback for returning rx Chars : Normal VxWorks SIO Model */

 	    USB_ACM_LOG (USB_ACM_DEBUG_ATTACH, "Rx callback installed \n",
                0, 0, 0, 0, 0, 0);

            pSioChan->callbackStatus |= USB_ACM_CALLBACK_SIO_RX;

            pSioChan->putRxCharCallback = (FUNCPTR)callback;
	    pSioChan->putRxCharArg = arg;
	    return OK;

        default :

            return ossStatus (S_usbAcmLib_BAD_PARAM);

        }

    return OK;

    }

/**************************************************************************
*
* usbAcmCallbackRemove - De-Registers a callback for an event.
*
* This function removess a (previously installed) <callback> callback routine 
* for a <callbackType> event. 
*
* Though this function provides a way to remove the callbacks even for the
* transmit and receive functionality, it is adviced that these callbacks be
* not removed. Note that the <pChan> paramter shall be NULL for the Dynamic 
* attachment notification requests, indicating that the event is not for any 
* particular device. Similary, <callback> can be NULL for all the other 
* events other than the dynamic attachment / removal events.
*
* Different <callbackType> events suppoted by this driver are mentioned in 
* usbAcmCallbackRegister. Please refer to the corresponding entry.
*
* RETURNS: OK, or ERROR if unable to remove a callback.
*
* ERRNO : 
*  S_usbAcmLib_BAD_PARAM
*  S_usbAcmLib_NOT_REGISTERED
*
*/

STATUS usbAcmCallbackRemove
    (
    SIO_CHAN * pChan,           /* Channel for de-registering callbacks */
    UINT callbackType,          /* Event the call back is to be removed */
    USB_ACM_CALLBACK callback	/* callback to be unregistered */
    )
    {
    
    pUSB_ACM_SIO_CHAN pSioChan = (pUSB_ACM_SIO_CHAN)pChan;

    pATTACH_REQUEST pRequest;
    int status = OK;

    /* Check for the validity of the input paramaters */
    
    if ((callbackType != USB_ACM_CALLBACK_ATTACH) && 
        (callbackType != USB_ACM_CALLBACK_DETACH))
        {

        /* 
         * Except for attachment/removal requests, pChan should never 
         * be Null 
         */

        if (pSioChan == NULL)
            return ossStatus (S_usbAcmLib_BAD_PARAM);
        
        /* Check if there is a callback for this event. */
        
        if ((pSioChan->callbackStatus & callbackType) == 0)
            return ossStatus (S_usbAcmLib_NOT_REGISTERED);
        
        /* De-Register the callback.*/

        pSioChan->callbackStatus &= ~callbackType;

        /* Thats all required , return now.*/

        return OK;
        }

    if (callback == NULL)
        return ossStatus (S_usbAcmLib_BAD_PARAM);

    status = S_usbAcmLib_NOT_REGISTERED;

    OSS_MUTEX_TAKE (acmMutex, OSS_BLOCK);

    pRequest = usbListFirst (&reqList);

    while (pRequest != NULL)
        {
	if (callback == pRequest->callback)
            {
	    
            /* We found a matching notification request. */

            destroyAttachRequest (pRequest);
	    status = OK;
	    break;
            }
        
	    pRequest = usbListNext (&pRequest->reqLink);
        }

    OSS_MUTEX_RELEASE (acmMutex);

    return ossStatus (status);
        
    }

/***************************************************************************
*
* usbAcmSioChanLock - Marks SIO_CHAN structure as in use
*
* A caller uses usbAcmSioChanLock() to notify usbAcmLib that
* it is using the indicated SIO_CHAN structure.	 usbAcmLib maintains
* a count of users using a particular SIO_CHAN structure so that it 
* knows when it is safe to dispose of a structure when the underlying
* USB Modem is removed from the system.  As long as the "lock count"
* is greater than zero, usbAcmLib will not dispose of an SIO_CHAN
* structure.
*
* RETURNS: OK, or ERROR if unable to mark SIO_CHAN structure in use.
*/

STATUS usbAcmSioChanLock
    (
    SIO_CHAN *pChan		/* SIO_CHAN to be marked as in use */
    )

    {
    pUSB_ACM_SIO_CHAN pSioChan = (pUSB_ACM_SIO_CHAN) pChan;
    
    pSioChan->lockCount++;

    return OK;
    }


/***************************************************************************
*
* usbAcmSioChanUnlock - Marks SIO_CHAN structure as unused
*
* This function releases a lock placed on an SIO_CHAN structure.  When a
* caller no longer needs an SIO_CHAN structure for which it has previously
* called usbAcmSioChanLock(), then it should call this function to
* release the lock.
*
* NOTE: If the underlying USB Modem device has already been removed
* from the system, then this function will automatically dispose of the
* SIO_CHAN structure if this call removes the last lock on the structure.
* Therefore, a caller must not reference the SIO_CHAN again structure after
* making this call.
*
* RETURNS: OK, or ERROR if unable to mark SIO_CHAN structure unused
*
* ERRNO:  S_usbAcmLib_NOT_LOCKED
*
*/

STATUS usbAcmSioChanUnlock
    (
    SIO_CHAN *pChan		/* SIO_CHAN to be marked as unused */
    )

    {
    pUSB_ACM_SIO_CHAN pSioChan = (pUSB_ACM_SIO_CHAN) pChan;
    int status = OK;


    OSS_MUTEX_TAKE (acmMutex, OSS_BLOCK);

    if (pSioChan->lockCount == 0)
	status = S_usbAcmLib_NOT_LOCKED;
    else
	{
	
        /* 
         * If this is the last lock and the underlying USB modem is
	 * no longer connected, then dispose of the acm struct.
	 */

	if (--pSioChan->lockCount == 0 && !pSioChan->connected)
	    destroySioChan (pSioChan);
	}

    OSS_MUTEX_RELEASE (acmMutex);

    return ossStatus (status);
    }

/***************************************************************************
*
* initiateOutput - initiates SIO model data transmission to modem.
*
* If the output IRP is not already in use, this function fills the output
* buffer by invoking the "tx char callback".  If at least one character is
* available for output, the function then initiates the output IRP.
*
* RETURNS: OK, or ERROR if unable to initiate transmission
*
* NOMANUAL
*/

STATUS initiateOutput
    (
    pUSB_ACM_SIO_CHAN pSioChan
    )

    {
    pUSB_IRP pIrp = &pSioChan->outIrp;
    UINT16 count;

    /* Return immediately if the output IRP is already in use. */

    if (pSioChan->outIrpInUse)
	{
	logMsg(" OutIrp in Use..\n",0,0,0,0,0,0);
	return ERROR; 
	}

    /* If there is no tx callback, return an error */

    if (pSioChan->getTxCharCallback == NULL)
	{
	logMsg("No TxCallback \n",0,0,0,0,0,0);
	return ERROR;
	}

    /* 
     * Fill the output buffer until it is full or until the tx callback
     * has no more data.  Return if there is no data available.
     */

    count = 0;

    USB_ACM_LOG(USB_ACM_DEBUG_TX," Preparing data \n",0,0,0,0,0,0);
    while (count < pSioChan->outBfrLen &&
	(*pSioChan->getTxCharCallback) (pSioChan->getTxCharArg, 
					&pSioChan->outBfr [count]) 
				      == OK)
	{
	count++;
	}

    if (count == 0)
	return OK;

    /* Initialize IRP */

    memset (pIrp, 0, sizeof (*pIrp));

    pIrp->userPtr = pSioChan;
    pIrp->irpLen = sizeof (*pIrp);
    pIrp->userCallback = usbAcmTxIrpCallback;
    pIrp->timeout = 2000;
    pIrp->transferLen = count;

    pIrp->bfrCount = 1;
    pIrp->bfrList [0].pid = USB_PID_OUT;
    pIrp->bfrList [0].pBfr = pSioChan->outBfr;
    pIrp->bfrList [0].bfrLen = count;

    USB_ACM_LOG(USB_ACM_DEBUG_TX," Irp framed %d bytes\n",count,0,0,0,0,0);

    pSioChan->outIrpInUse = TRUE;   /* This is a MUST */
 
    /* Submit IRP */

    if (usbdTransfer (usbdHandle, pSioChan->outPipeHandle, pIrp) != OK)
	return ERROR;

    USB_ACM_LOG(USB_ACM_DEBUG_TX," Data.submitted for tx \n",0,0,0,0,0,0);

    /* Wait for IRP to complete or cancel by timeout */

    if ( OSS_SEM_TAKE (acmIrpSem, 2000 + 1000) == ERROR )
        {
        printf ("ACM:initiateOutput: Fatal Error \n");
        }

    USB_ACM_LOG(USB_ACM_DEBUG_TX," returning \n",0,0,0,0,0,0);

    return OK;
    }


/***************************************************************************
*
* listenForInput - Initialize IRP to listen for input from Modem
*
* RETURNS: OK, or ERROR if unable to submit IRP to listen for input
*/

LOCAL STATUS listenForInput
    (
    pUSB_ACM_SIO_CHAN pSioChan
    )

    {
    pUSB_IRP pIrp = &pSioChan->inIrp;

    /* Initialize IRP */

    memset (pIrp, 0, sizeof (*pIrp));

    pIrp->userPtr = pSioChan;
    pIrp->irpLen = sizeof (*pIrp);
    pIrp->userCallback = usbAcmRxIrpCallback;
    pIrp->timeout = USB_TIMEOUT_NONE;
    pIrp->transferLen = pSioChan->inBfrLen;

    pIrp->bfrCount = 1;
    pIrp->bfrList [0].pid = USB_PID_IN;
    pIrp->bfrList [0].pBfr = (pUINT8) &pSioChan->inBfr;
    pIrp->bfrList [0].bfrLen = pSioChan->inBfrLen;

    pSioChan->inIrpInUse = TRUE;

    /* Submit IRP */

    if (usbdTransfer (usbdHandle, pSioChan->inPipeHandle, pIrp) != OK)
	return ERROR;

    return OK;
    }

/***************************************************************************
*
* usbAcmTxIrpCallback - Invoked upon IRP completion/cancellation
*
* Examines the cause of the IRP completion and re-submits the IRP.
*
* RETURNS: N/A
*/

LOCAL VOID usbAcmTxIrpCallback
    (
    pVOID p			/* completed IRP */
    )

    {
    pUSB_IRP pIrp = (pUSB_IRP) p;
    pUSB_ACM_SIO_CHAN pSioChan = pIrp->userPtr;

    pSioChan->outIrpInUse = FALSE;

    if (pIrp->result != OK)
        pSioChan->outErrors++;

    /*
     * If a Block transmission occured, we just return. Otherwise
     * we'll see if some more data is to be transmitted.
     */

    if (pSioChan->callbackStatus | USB_ACM_CALLBACK_BLK_TX)
        {
        pSioChan->callbackStatus &= ~USB_ACM_CALLBACK_BLK_TX;
        }

    OSS_SEM_GIVE (acmIrpSem);
    }

/***************************************************************************
*
* usbAcmRxIrpCallback - Invoked upon IRP completion/cancellation
*
* Examines the cause of the IRP completion and re-submits the IRP.
*
* RETURNS: N/A
*/

LOCAL VOID usbAcmRxIrpCallback
    (
    pVOID p			/* completed IRP */
    )
    {
    pUSB_IRP pIrp = (pUSB_IRP) p;
    pUSB_ACM_SIO_CHAN pSioChan = pIrp->userPtr;
    UINT16 count;

    USB_ACM_LOG(USB_ACM_DEBUG_RX," RxCallBack \n",0,0,0,0,0,0);

    /* Input IRP completed */

    pSioChan->inIrpInUse = FALSE;

    if (pIrp->result != OK)
        pSioChan->inErrors++;

    /*
     * If the IRP was successful then pass the data back to the client.
     */

    if (pIrp->result == OK)
        {
        
        /* 
         * How to pass the data to clients? If a BlockRxCallback is installed,
         * then that callback shall be used to send back the data. Otherwise
         * it shall be the Vxworks SIO model callback 
         */

            /* Its the normal VxWorks SIO model now */
          
	USB_ACM_LOG(USB_ACM_DEBUG_RX," Passing data up \n",0,0,0,0,0,0);
        
        if (pSioChan->putRxCharCallback == NULL)
	    {
	    logMsg("ACM: Rx Callback Null!!!\n",0,0,0,0,0,0);
            ossStatus (S_usbAcmLib_NOT_REGISTERED);
            return;
            }

        for (count = 0; count < pIrp->bfrList [0].actLen; count++)
	    {
	    (*pSioChan->putRxCharCallback) (pSioChan->putRxCharArg,
		pIrp->bfrList [0].pBfr [count]);
	    }
	}

	/* 
         * Unless the IRP was cancelled - implying the channel is being
	 * shutdown, re-initiate the "in" IRP to listen for more data from
	 * the modem.
	 */

        USB_ACM_LOG(USB_ACM_DEBUG_RX," Passing data up ..done \n",0,0,0,0,0,0);
	if (pIrp->result != S_usbHcdLib_IRP_CANCELED)
	    listenForInput (pSioChan);
	
   }
 
/***************************************************************************
*
* usbAcmCtrlCmdSend - Sends a Control command to the Modem.
*
* This function sends an AT command to the modem. The response to the command
* will be passed to the client via a previously installed callback for the
* Modem response.
*
* RETURNS: OK or ERROR if the command could not be sent.
*
*/

LOCAL STATUS usbAcmCtrlCmdSend
    (
    pUSB_ACM_SIO_CHAN pSioChan, /* Modem reference */
    UINT16 request,             /* Command to send */
    UINT8 * pBuf,               /* Modem command buffer */
    UINT16  count               /* no of bytes in command */
    )
    {

    UINT16 actLen;

    if (pSioChan == NULL)
        return ERROR;

    if (usbdVendorSpecific (usbdHandle, 
		pSioChan->nodeId,
		USB_RT_HOST_TO_DEV | USB_RT_CLASS | USB_RT_INTERFACE,
		request, 
		pSioChan->configuration,
		(pSioChan->ifaceCommClass << 8 | pSioChan->ifaceCommAltSetting),
		count, 
		pBuf, 
		&actLen) 
	      != OK)
	{
	return ERROR;
	}

    return OK;
    }

/***************************************************************************
*
* usbAcmHwOptsSet - set hardware options
*
* This routine sets up the modem according to the specified option
* argument.  If the hardware cannot support a particular option value, then
* it should ignore that portion of the request.
*
* USB modems supports more features than what are provided by the SIO model.
* It is suggested that to take full advantage of these features, user shall
* make use of the additional ioctl requests provided.
*
* RETURNS: OK upon success, or EIO for invalid arguments.
*/

LOCAL int usbAcmHwOptsSet
    (
    USB_ACM_SIO_CHAN * pChan,		/* channel */
    uint_t	    newOpts          	/* new options */
    )
    {
    BOOL hdweFlowCtrl= TRUE;
    BOOL rcvrEnable = TRUE;


    LINE_CODE oldLineCode;

    if (pChan == NULL || newOpts & 0xffffff00)
	return EIO;

    /* do nothing if options already set */

    if ((uint_t)pChan->options == newOpts)
	return OK;

    oldLineCode = pChan->lineCode;

    /* decode individual request elements */

    switch (newOpts & CSIZE)
	{
	case CS5:
	    pChan->lineCode.noOfDataBits = 5; 
	    break;

	case CS6:
	    pChan->lineCode.noOfDataBits = 6; 
	    break;

	case CS7:
	    pChan->lineCode.noOfDataBits = 7; 
	    break;

	default:
	case CS8:
	    pChan->lineCode.noOfDataBits = 8; 
	    break;

	}

    if (newOpts & STOPB)
	pChan->lineCode.noOfStopBits = USB_ACM_STOPBITS_2;
    else
	pChan->lineCode.noOfStopBits = USB_ACM_STOPBITS_1;

    switch (newOpts & (PARENB|PARODD))
	{
	case PARENB|PARODD:

	    pChan->lineCode.parityType = USB_ACM_PARITY_ODD;
	    break;

	case PARENB:

	    pChan->lineCode.parityType = USB_ACM_PARITY_EVEN;
	    break;

	case PARODD:

	    /* invalid mode, not normally used. */
	    break;

	default:
	case 0:

	    pChan->lineCode.parityType = USB_ACM_PARITY_NONE;
            break;
	}

    /* TODO : set the H/W Flow control some how */

    if (newOpts & CLOCAL)
	{

        /* clocal disables hardware flow control */
	
        hdweFlowCtrl = FALSE;
	}

    if ((newOpts & CREAD) == 0)
	rcvrEnable = FALSE;

    USB_ACM_LOG (USB_ACM_DEBUG_IOCTL, "Setting HW options \n", 
        0, 0, 0, 0, 0, 0);
    
    if (usbAcmCtrlCmdSend (pChan, 
			   USB_ACM_REQ_LINE_CODING_SET,
			   (UINT8 *)(&pChan->lineCode), 
			   sizeof(pChan->lineCode)) 
			  == ERROR)
        {
        pChan->lineCode = oldLineCode;
        return ERROR;
        }

    pChan->options = newOpts;

    return (OK);
    }

/***************************************************************************
*
* usbAcmIoctl - special device control
*
* This routine handles the IOCTL messages from the user. It supports commands 
* to get/set baud rate, mode( only INT mode), hardware options(parity, 
* number of data bits). 
*
* This driver works only in the interrupt mode. Attempts to place the driver
* in polled mode result in ERROR.
*
* SIO_HUP is not implemented. Instead, user can issue a "ATH" AT command
* to the modem to hangup the line.
*
* RETURNS: OK on success, ENOSYS on unsupported request, EIO on failed
* request.
*/

int usbAcmIoctl
    (
    SIO_CHAN * pChan,       /* Channel to control */
    int     request,        /* IOCTL request */
    void *  pArg            /* pointer to an argument  */
    )
    {

    pUSB_ACM_SIO_CHAN pSioChan = (pUSB_ACM_SIO_CHAN)pChan;

    int arg = (int)pArg;

    UINT16 tempValue = 0;

    if (pSioChan == NULL)
        return ERROR;

    USB_ACM_LOG (USB_ACM_DEBUG_IOCTL, "Ioctl : 0x%0x request \n",
        request, 0, 0, 0, 0, 0);

    switch (request)
	{

        case USB_ACM_SIO_MODE_SET:

	    /* 
	     * Set driver operating mode: interrupt or polled.
	     * NOTE: This driver supports only SIO_MODE_INT.
	     */

	    USB_ACM_LOG (USB_ACM_DEBUG_IOCTL, 
                         "Ioctl : MODE_SET request for %s mode \n",
        	         (UINT)((arg == SIO_MODE_INT)?"Interrupt":"Polled"), 
                         0, 0, 0, 0, 0);

	    if (arg != SIO_MODE_INT)
		return EIO;

	    pSioChan->mode = arg;
	    return OK;


	case USB_ACM_SIO_MODE_GET:

	    /* Return current driver operating mode for channel */

	    USB_ACM_LOG (USB_ACM_DEBUG_IOCTL, "Ioctl : MODE_GET request \n",
        	0, 0, 0, 0, 0, 0);

	    *((int *) arg) = pSioChan->mode;
	    return OK;


	case USB_ACM_SIO_AVAIL_MODES_GET:

	    /* Return modes supported by driver. */

	    USB_ACM_LOG (USB_ACM_DEBUG_IOCTL, 
                         "Ioctl : GET_ALL_MODES request \n",
        	         0, 0, 0, 0, 0, 0);

	    *((int *) arg) = SIO_MODE_INT;
	    return OK;


	case USB_ACM_SIO_OPEN:

	    /* Channel is always open. */

	    USB_ACM_LOG (USB_ACM_DEBUG_IOCTL, "Ioctl : OPEN request \n",
        	0, 0, 0, 0, 0, 0);

	    return OK;

        case USB_ACM_SIO_BAUD_SET:

            /*
	     * like unix, a baud request for 0 is really a request to
	     * hangup. In usb modems, hangup is achieved by sending an
             * AT command "ATH" than this way. We return ERROR here.
	     */

	    USB_ACM_LOG (USB_ACM_DEBUG_IOCTL, 
                         "Ioctl : SET_BAUD of %d  request \n",
        	         arg, 0, 0, 0, 0, 0);

	    if (arg == 0)
		/*return usbAcmHup (pSioChan);*/
                return EIO;

	    /*
	     * Set the baud rate. Return EIO for an invalid baud rate, or
	     * OK on success.
	     */

	    if (arg < USB_ACM_BAUD_MIN || arg > USB_ACM_BAUD_MAX)
	        {
 	        USB_ACM_LOG (USB_ACM_DEBUG_IOCTL, 
                             "Ioctl : %dbaud exceeds range \n",
        	             arg, 0, 0, 0, 0, 0);

		return (EIO);
	        }

            tempValue = pSioChan->lineCode.baudRate;

            pSioChan->lineCode.baudRate = arg;

            if (usbAcmCtrlCmdSend (pSioChan, 
				   USB_ACM_REQ_LINE_CODING_SET, 
				   (UINT8 *)(&pSioChan->lineCode), 
				   sizeof(pSioChan->lineCode)) 
				 == ERROR)
                {
                pSioChan->lineCode.baudRate = tempValue;
 	        USB_ACM_LOG (USB_ACM_DEBUG_IOCTL, 
                             "Ioctl : SET_BAUD of %d  request failed \n",
        	             arg, 0, 0, 0, 0, 0);
                return EIO;
                }
            return OK;

        case USB_ACM_SIO_BAUD_GET:

	    USB_ACM_LOG (USB_ACM_DEBUG_IOCTL, 
                         "Ioctl : GET_BAUD of %d  request \n",
        	         0, 0, 0, 0, 0, 0);

            *(UINT *)arg = pSioChan->lineCode.baudRate;
            return OK;

        case USB_ACM_SIO_HW_OPTIONS_SET:

            /*
	     * Optional command to set the hardware options (as defined
	     * in sioLib.h).
	     * Return OK, or ENOSYS if this command is not implemented.
	     * Note: several hardware options are specified at once.
	     * This routine should set as many as it can and then return
	     * OK. The SIO_HW_OPTS_GET is used to find out which options
	     * were actually set.
	     */

	    USB_ACM_LOG (USB_ACM_DEBUG_IOCTL, "Ioctl : SET_HW_OPTS request \n",
        	0, 0, 0, 0, 0, 0);

            return  usbAcmHwOptsSet (pSioChan, arg);

        case USB_ACM_SIO_HW_OPTIONS_GET:
            
            /*
	     * Optional command to get the hardware options (as defined
	     * in sioLib.h). Return OK or ENOSYS if this command is not
	     * implemented.  Note: if this command is unimplemented, it
	     * will be assumed that the driver options are CREAD | CS8
	     * (e.g., eight data bits, one stop bit, no parity, ints enabled).
	     */

	    USB_ACM_LOG (USB_ACM_DEBUG_IOCTL, "Ioctl : GET_HW_OPTS request \n",
        	0, 0, 0, 0, 0, 0);

            *(UINT *) arg = pSioChan->options;

            return OK;

        case USB_ACM_SIO_HUP:

            /* 
             * As mentioned before, hang-up is achieved by sending an AT 
             * command. 
             */

	    USB_ACM_LOG (USB_ACM_DEBUG_IOCTL, "Ioctl : hangUp request \n",
        	0, 0, 0, 0, 0, 0);

            return ENOSYS;

        case USB_ACM_SIO_FEATURE_SET:
	
		return ENOSYS;

        case USB_ACM_SIO_FEATURE_GET:
		
		return ENOSYS;

        case USB_ACM_SIO_FEATURE_CLEAR:
		
		return ENOSYS;

        case USB_ACM_SIO_LINE_CODING_SET:

            /*
             * The USB way of setting line parameters 
             */

            if (usbAcmCtrlCmdSend (pSioChan, 
				   USB_ACM_REQ_LINE_CODING_SET, 
				   (UINT8 *)(arg), 
				   sizeof(pSioChan->lineCode)) 
				  == ERROR)
                {
                return ERROR;
                }
            pSioChan->lineCode =  *(LINE_CODE *)arg;

            return OK;

        case USB_ACM_SIO_LINE_CODING_GET:

            /*
             * USB way of retreving parameters 
             */

            memcpy ((UINT8 *)arg, 
		    (UINT8 *) &pSioChan->lineCode, 
		    sizeof (pSioChan->lineCode));

        case USB_ACM_SIO_CTRL_LINE_STATE_SET:   
		
		return ENOSYS;

        case USB_ACM_SIO_SEND_BREAK:   
	
		return ENOSYS;

        case USB_ACM_SIO_MAX_BUF_SIZE_GET:

            *(UINT *)arg = pSioChan->outBfrLen;
            return OK;

        default:

	    USB_ACM_LOG (USB_ACM_DEBUG_IOCTL, 
                         "Ioctl : 0x%0x UnKnown request \n",
        	         request, 0, 0, 0, 0, 0);
            return ENOSYS;

        }

        return OK;

    }

/***************************************************************************
*
* usbAcmOpen - Start the Modem communications.
*
* RETURNS: OK on success, or ERROR if listening fails
*/

LOCAL STATUS usbAcmOpen
    (
    SIO_CHAN * pChan 	/* pointer to channel */
    )
    {
    
    pUSB_ACM_SIO_CHAN pSioChan = (pUSB_ACM_SIO_CHAN)pChan;

    if (pSioChan == NULL)
        return ERROR;

    /* Start listening for normal data */

    if (listenForInput(pSioChan) == ERROR)
        {
        USB_ACM_LOG (USB_ACM_DEBUG_OPEN, "listenForInput() Failed....",
            0, 0, 0, 0, 0, 0);

        return ERROR;
        }

    return OK;

    }


/***************************************************************************
*
* usbAcmTxStartup - start the transmition.
*
* This function uses the initiateOutput for the transmission.
*
* RETURNS: OK on success, or EIO if output initiation fails
*
* NOMANUAL
*/

int usbAcmTxStartup
    (
    SIO_CHAN * pChan                 /* channel to start */
    )
    {
    pUSB_ACM_SIO_CHAN pSioChan = (pUSB_ACM_SIO_CHAN)pChan;

    int status = OK;

    if (initiateOutput (pSioChan) != OK)
	{
	logMsg("InitiateOutput failed \n",0,0,0,0,0,0);
	status = EIO;
	}

    return status;
    }

/***************************************************************************
*
* usbAcmPollOutput - output a character in polled mode
*
* The USB modem driver supports only interrupt-mode operation.  Therefore,
* this function always returns the error ENOSYS.
*
* RETURNS: ENOSYS
*/

LOCAL int usbAcmPollOutput
    (
    SIO_CHAN *pChan,
    char outChar
    )

    {
    return ENOSYS;
    }


/***************************************************************************
*
* usbAcmPollInput - poll the device for input
*
* The USB modem driver supports only interrupt-mode operation.  Therefore,
* this function always returns the error ENOSYS.
*
* RETURNS: ENOSYS
*/

LOCAL int usbAcmPollInput
    (
    SIO_CHAN *pChan,
    char *thisChar
    )

    {
    return ENOSYS;
    }
