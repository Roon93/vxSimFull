/* usbBulkDevLib.c - USB Bulk Only Mass Storage class driver */

/* Copyright 2000-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01h,06nov01,wef  remove automatic buffer creations and repalce with OSS_xALLOC
		 remove more warnings
01g,08aug01,dat  Removing warnings
01f,25jul01,wef  fixed spr #69285
01e,01may01,wef  fixed incorrect declaration of usbBulkDevDestroy
01d,12apr01,wef  added logic to do READ/WRITE 6 or 10 based on configlette
		 parameter - added a paramter to usbBulBlkDevCreate () 
		 for this purpose.  added support for drives with partition 
		 tables.
01c,02sep00,bri  added support for multiple devices.
01b,04aug00,bri  updated as per review.
01a,22may00,bri  written.
*/

/*
DESCRIPTION

This module implements the USB Mass Storage class driver for the vxWorks 
operating system.  This module presents an interface which is a superset 
of the vxWorks Block Device driver model.  This driver implements external 
APIs which would be expected of a standard block device driver. 

This class driver restricts to Mass Storage class devices that follow bulk-only
transport.  For bulk-only devices transport of command, data and status occurs 
solely via bulk endpoints.  The default control pipe is only used to set 
configuration, clear STALL condition on endpoints and to issue class-specific 
requests. 

The class driver is a client of the Universal Serial Bus Driver (USBD).  All
interaction with the USB buses and devices is handled through the USBD.

INITIALIZATION

The class driver must be initialized with usbBulkDevInit().  It is assumed that
USBD is already initialized and attached to atleast one USB Host Controller.  
usbBulkDevInit() registers the class driver as a client module with USBD.  It 
also registers a callback routine to get notified whenever a USB MSC/SCSI/
BULK-ONLY device is attached or removed from the system.  The callback routine
creates a USB_BULK_DEV structure to represent the USB device attached.  It 
also sets device configuration, interface settings and creates pipes for 
BULK_IN and BULK_OUT transfers. 

OTHER FUNCTIONS

usbBulkBlkDevCreate() is the entry point to define a logical block device.
This routine initializes the fields with in the vxWorks block device structure 
BLK_DEV.  This BLK_DEV structure is part of the USB_BULK_DEV structure.  
Memory is allocated for USB_BULK_DEV by the dynamic attach notification 
callback routine.  So, this create routine just initializes the BLK_DEV 
structure and returns a pointer to it, which is used during file system 
initializtion call.

usbBulkDevIoctl() implements functions which are beyond basic file handling. 
Class-specific requests, Descriptor show, are some of the functions.  Function
code parameter identifies the IO operation requested. 

DATA FLOW

For each USB MSC/SCSI/BULK-ONLY device detected, usbBulkPhysDevCreate() will
create pipes to BULK_IN and a BULK_OUT endpoints of the device.  A pipe is a
channel between USBD client i,e usbBulkDevLib and a specific endpoint.  All 
SCSI commands are encapsulated with in a Command Block Wrapper (CBW) and 
transferred across the BULK_OUT endpoint through the out pipe created.  
This is followed by a data transfer phase.  Depending on the SCSI command 
sent to the device, the direction bit in the CBW will indicate whether data 
is transferred to or from the device.  This bit has no significance if no
data transfer is expected.  Data is transferred to the device through BULK_OUT
endpoint and if the device is required to transfer data, it does through
the BULK_IN endpoint.  The device shall send Command Status Wrapper (CSW) via 
BULK_IN endpoint.  This will indicate the success or failure of the CBW.  The
data to be transferred to device will be pointed by the file system launched 
on the device. 


INCLUDE FILES: usbBulkDevLib.h, blkIo.h

SEE ALSO:
.I "USB Mass Storage Class - Bulk Only Transport Specification Version 1.0,"
.I "SCSI-2 Standard specification 10L - Direct Access device commands"

*/

/* includes */

#include "vxWorks.h"
#include "string.h"
#include "errno.h"
#include "errnoLib.h"
#include "ioLib.h"
#include "blkIo.h"
#include "stdio.h"
#include "logLib.h"
#include "dosFsLib.h"


#include "usb/usbPlatform.h"
#include "usb/ossLib.h"	            /* operations system srvcs */
#include "usb/usb.h"	            /* general USB definitions */
#include "usb/usbListLib.h"         /* linked list functions   */
#include "usb/usbdLib.h"            /* USBD interface          */
#include "usb/usbLib.h"	            /* USB utility functions   */

#include "drv/usb/usbBulkDevLib.h"


/* defines */

#define USB_DEBUG_MSG        0x01
#define USB_DEBUG_ERR        0x02

#define USB_BULK_DEBUG                  \
        if (usbBulkDebug & USB_DEBUG_MSG)   \
            logMsg

#define USB_BULK_ERR                    \
        if (usbBulkDebug & USB_DEBUG_ERR)   \
            logMsg

#define USB_BULK_OFFS  1000

/* typedefs */

/* USB_BULK_DEV Structure - used to describe USB MSC/SCSI/BULK-ONLY device */

typedef struct usbBulkDev
    {
    BLK_DEV           blkDev;         /* Vxworks block device structure */
                                      /* Must be the first one          */
    USBD_NODE_ID      bulkDevId;      /* USBD node ID of the device     */	 
    UINT16            configuration;  /* Configuration value        	*/    
    UINT16            interface;      /* Interface number               */
    UINT16            altSetting;     /* Alternate setting of interface */ 
    UINT16            outEpAddress;   /* Bulk out EP address            */   
    UINT16            inEpAddress;    /* Bulk in EP address             */
    USBD_PIPE_HANDLE  outPipeHandle;  /* Pipe handle for Bulk out EP    */
    USBD_PIPE_HANDLE  inPipeHandle;   /* Pipe handle for Bulk in EP     */
    USB_IRP           inIrp;          /* IRP used for bulk-in data      */
    USB_IRP           outIrp;         /* IRP used for bulk-out data     */
    USB_IRP           statusIrp;      /* IRP used for reading status    */ 
    UINT8             maxLun;         /* Max. number of LUN supported   */
    USB_BULK_CBW      bulkCbw;        /* Structure for Command block    */
    USB_BULK_CSW      bulkCsw;        /* Structure for Command status   */
    UINT8 *           bulkInData;     /* Pointer for bulk-in data       */
    UINT8 *           bulkOutData;    /* Pointer for bulk-out data      */   
    UINT32            numBlks;	      /* Number of blocks on device     */	 
    UINT32            blkOffset;      /* Offset of the starting block   */
    UINT16            lockCount;      /* Count of times structure locked*/
    BOOL              connected;      /* TRUE if USB_BULK device connected */    
    LINK              bulkDevLink;    /* Link to other USB_BULK devices    */  
    BOOL              read10Able;     /* Which read/write command the device  */
				      /* supports.  If TRUE, the device uses  */
				      /* READ10/WRITE10, if FALSE uses READ6 / */
				      /* WRITE6 */
    } USB_BULK_DEV, *pUSB_BULK_DEV;    

/* Attach request for user callback */

typedef struct attach_request
    {
    LINK reqLink;                       /* linked list of requests */
    USB_BULK_ATTACH_CALLBACK callback;  /* client callback routine */
    pVOID callbackArg;                  /* client callback argument*/
    } ATTACH_REQUEST, *pATTACH_REQUEST;

/* globals */

BOOL usbBulkDebug =  0;

/* locals */

LOCAL UINT16 initCount = 0;           /* Count for Bulk device initialisation */

LOCAL USBD_CLIENT_HANDLE usbdHandle;  /* Handle for this class driver */

LOCAL LIST_HEAD bulkDevList;          /* linked list of USB_BULK_DEV */

LOCAL LIST_HEAD    reqList;           /* Attach callback request list */

 MUTEX_HANDLE bulkDevMutex;      /* mutex used to protect internal structs */

SEM_HANDLE   bulkIrpSem;        /* Semaphore for IRP Synchronisation */

LOCAL UINT32 usbBulkIrpTimeOut = USB_BULK_IRP_TIME_OUT; /* Time out for IRP */

/* forward declarations */

LOCAL  STATUS usbBulkDescShow (USBD_NODE_ID nodeId);
LOCAL  STATUS usbBulkConfigDescShow  (USBD_NODE_ID nodeId, UINT8 index);
LOCAL  STATUS usbBulkPhysDevCreate (USBD_NODE_ID nodeId, UINT16 config, 
                                    UINT16 interface);
LOCAL  pUSB_BULK_DEV usbBulkDevFind (USBD_NODE_ID nodeId);
LOCAL  STATUS usbBulkDevBlkRd (BLK_DEV *blkDev, int offset, int num, 
			       char * buf);
LOCAL  STATUS usbBulkDevBlkWrt (BLK_DEV *blkDev, int offset, int num, 
				char * buf);
LOCAL  STATUS usbBulkDevStatusChk (BLK_DEV *blkDev);
LOCAL  STATUS usbBulkDevReset (BLK_DEV *blkDev);
LOCAL  void   usbBulkIrpCallback (pVOID p);
LOCAL  STATUS usbBulkFormScsiCmd (pUSB_BULK_DEV pBulkDev, 
				  UINT16 scsiCmd, 
				  UINT32 cmdParam1, 
				  UINT32 cmdParam2);
                                   
LOCAL  USB_COMMAND_STATUS usbBulkCmdExecute (pUSB_BULK_DEV pBulkDev);
LOCAL  void   usbBulkDevDestroy (pUSB_BULK_DEV pBulkDev);
LOCAL  STATUS usbBulkDevResetRecovery (pUSB_BULK_DEV  pBulkDev);
LOCAL  VOID notifyAttach (USBD_NODE_ID nodeId, UINT16 attachCode);


/***************************************************************************
*
* usbBulkDevAttachCallback - called when BULK-ONLY/SCSI device is attached/
* removed
*
* This routine is called by USBD when a mass storage device with SCSI command 
* set as interface sub-class and with bulk-only as interface protocol, is 
* attached/detached.
*
* <nodeId> is the USBD_NODE_ID of the node being attached or removed.	
* <attachAction> is USBD_DYNA_ATTACH or USBD_DYNA_REMOVE.
* <configuration> and <interface> indicate the configuration/interface
* that reports itself as a MSC/SCSI/BULK-ONLY device.  
* <deviceClass>, <deviceSubClass>, and <deviceProtocol> will identify a 
* MSC/SCSI/BULK-ONLY device.
*
* NOTE: The USBD will invoke this function once for each configuration/
* interface which reports itself as a MSC/SCSI/BULK-ONLY.  So, it is possible 
* that a single device insertion/removal may trigger multiple callbacks. 
*
*
* RETURNS: N/A
*/

LOCAL VOID usbBulkDevAttachCallback
    (
    USBD_NODE_ID nodeId,         /* USBD Node ID of the device attached */
    UINT16       attachAction,   /* Whether device attached / detached */
    UINT16       configuration,  /* Configur'n value for  MSC/SCSI/BULK-ONLY */
    UINT16       interface,      /* Interface number for  MSC/SCSI/BULK-ONLY */
    UINT16       deviceClass,    /* Interface class   - 0x8  for MSC */
    UINT16       deviceSubClass, /* Device sub-class  - 0x6  for SCSI 
				  * command 
				  */ 
    UINT16       deviceProtocol  /* Interfaceprotocol - 0x50 for Bulk only */ 
    )
    {
    pUSB_BULK_DEV  pBulkDev;     /* Pointer to bulk device,in case of 
				  * removal 
				  */

    OSS_MUTEX_TAKE (bulkDevMutex, OSS_BLOCK);
 
    switch (attachAction)
        { 
        case USBD_DYNA_ATTACH: 

            /* MSC/SCSI/BULK-ONLY Device attached */

            /* Check out whether we already have a structure for this device */

            if (usbBulkDevFind (nodeId) != NULL)
                break;

            USB_BULK_DEBUG ("usbBulkDevAttachCallback : New Bulk-only device "\
                            "attached\n", 0, 0, 0, 0, 0, 0);

            USB_BULK_DEBUG ("usbBulkDevAttachCallback: Configuration = %d, " \
                            "Interface = %d, Node Id = %d \n", configuration,
                            interface, (UINT)nodeId, 0, 0, 0); 

            /* create a USB_BULK_DEV structure for the device attached */
           
            if ( usbBulkPhysDevCreate (nodeId, configuration,interface) != OK )
                {
                USB_BULK_ERR ("usbBulkDevAttachCallback : Error creating Bulk"\
                              "device\n", 0, 0, 0, 0, 0, 0);
                break; 
                } 

            /* Notify registered callers that a USB_BULK_DEV has been added */

	    notifyAttach (nodeId, USB_BULK_ATTACH); 
 
            break;

        case USBD_DYNA_REMOVE:

            /* MSC/SCSI/BULK-ONLY Device detached */

            if ((pBulkDev = usbBulkDevFind (nodeId)) == NULL)
                break;

            /* Check the connected flag  */

            if (pBulkDev->connected == FALSE)
                break;
            
            pBulkDev->connected = FALSE;

	    /* Notify registered callers that the SCSI/BULK-ONLY device has 
             * been removed 
	     *
	     * NOTE: We temporarily increment the device's lock count
	     * to prevent usbBulkDevUnlock() from destroying the
	     * structure while we're still using it.
	     */

            pBulkDev->lockCount++; 

            notifyAttach (pBulkDev->bulkDevId, USB_BULK_REMOVE); 

            pBulkDev->lockCount--; 
           
            if (pBulkDev->lockCount == 0) 
                usbBulkDevDestroy (pBulkDev); 

            USB_BULK_DEBUG ("usbBulkDevAttachCallback : Bulk only Mass \
			     storage device detached\n", 0, 0, 0, 0, 0, 0);

            break;

        default :
            break; 
        }

    OSS_MUTEX_RELEASE (bulkDevMutex);  
    }


/***************************************************************************
*
* usbBulkDevShutDown - shuts down the USB bulk-only class driver.
*
* This routine unregisters the driver from USBD and releases any resources 
* allocated for the devices.
*
* RETURNS: OK or ERROR.
*/

STATUS usbBulkDevShutDown 
    (
    int   errCode                /* Error code - reason for shutdown */
    )
    {

    pUSB_BULK_DEV pBulkDev;      /* Pointer to bulk device */
    pATTACH_REQUEST  pRequest;

    if (initCount == 0)
        return ossStatus (S_usbBulkDevLib_NOT_INITIALIZED);

    /* release any bulk devices */

    while ((pBulkDev = usbListFirst (&bulkDevList)) != NULL)
        usbBulkDevDestroy (pBulkDev); 

    /* Dispose of any outstanding notification requests */

    while ((pRequest = usbListFirst (&reqList)) != NULL)
        {
      	usbListUnlink (&pRequest->reqLink);
        OSS_FREE (pRequest); 
        }

    /* 
     * Unregister with the USBD. USBD will automatically release any pending
     * IRPs or attach requests.
     */

    if (usbdHandle != NULL)
        {
        usbdClientUnregister (usbdHandle);
        usbdHandle = NULL;
        USB_BULK_DEBUG ("usbBulkDevShutDown : Bulk Only class driver "\
                        "unregistered \n", 0, 0, 0, 0, 0, 0);
        }

    /* release resources */

    if (bulkDevMutex != NULL)
        {
        OSS_MUTEX_DESTROY (bulkDevMutex);
        bulkDevMutex = NULL;
        }

    if (bulkIrpSem != NULL)
        {
        OSS_SEM_DESTROY (bulkIrpSem);
        bulkIrpSem = NULL;
        }

    initCount--; 
   
    return ossStatus (errCode); 
    }



/***************************************************************************
*
* usbBulkDevInit - registers USB Bulk only mass storage class driver.
*
* This routine registers the mass storage class driver with USB driver.  It
* also registers attach callback routine to get notified of the USB/MSC/BULK
* ONLY devices.  
*
* RETURNS: OK, or ERROR if unable to register with USBD.
*
* ERRNO:
*   S_usbbulkDevLib_OUT_OF_RESOURCES
*   S_usbbulkDevLib_USBD_FAULT
*/

STATUS usbBulkDevInit (void)
    {

    /* 
     * Check whether already initilized. If not, then initialise the required
     * structures and register the class driver with USBD.
     */

    if (initCount == 0)
        {

        memset (&bulkDevList, 0, sizeof (bulkDevList));
        memset (&reqList, 0, sizeof (reqList));
        bulkDevMutex = NULL;
        bulkIrpSem   = NULL;
        usbdHandle   = NULL;


        if (OSS_MUTEX_CREATE (&bulkDevMutex) != OK)
            return (usbBulkDevShutDown (S_usbBulkDevLib_OUT_OF_RESOURCES));

        if (OSS_SEM_CREATE (1, 1 , &bulkIrpSem) != OK)
            return (usbBulkDevShutDown (S_usbBulkDevLib_OUT_OF_RESOURCES));

        /* Establish connection to USBD and register for attach callback */

        if (usbdClientRegister ("BULK_CLASS", &usbdHandle) != OK ||
            usbdDynamicAttachRegister (usbdHandle, USB_CLASS_MASS_STORAGE,
                                       USB_SUBCLASS_SCSI_COMMAND_SET, 
                                       USB_INTERFACE_PROTOCOL_BULK_ONLY,
                                       usbBulkDevAttachCallback) != OK)
            {
            USB_BULK_ERR ("usbBulkDevInit: Client Registration Failed \n",
                          0, 0, 0, 0, 0, 0); 
            return usbBulkDevShutDown (S_usbBulkDevLib_USBD_FAULT);
            }
        }

    initCount++;

    return (OK);
    }

/***************************************************************************
*
* usbBulkDevIoctl - perform a device-specific control.
*
* Typically called to invoke device-specific functions which are not needed
* by a file system.  
*
* RETURNS: The status of the request, or ERROR if the request is unsupported.
*/

STATUS usbBulkDevIoctl
    (
    BLK_DEV * pBlkDev,           /* pointer to bulk device       */
    int request,                 /* request type                 */
    int someArg                  /* arguments related to request */
    )
    {
    UINT16 actLen= 0xffff;       

    /* get a pointer to the bulk device */

    pUSB_BULK_DEV  pBulkDev = (USB_BULK_DEV *)pBlkDev;   

    if ( pBulkDev == (pUSB_BULK_DEV)NULL )
        return (ERROR);

    /* Check whether the device exists or not */

    if (usbBulkDevFind (pBulkDev->bulkDevId) != pBulkDev)
        {
        USB_BULK_ERR ("usbBulkDevIoctl: Bulk Device not found\n", 
                        0, 0, 0, 0, 0, 0);
        return (ERROR);
        }

    switch (request)
        {

        case FIODISKFORMAT: 

            /*  
             * This is the IO control function supported by file system,
             * but supplied by the device driver. Other IO control functions 
             * are directly handled by file system with out the use of this 
	     * routine.
             */

            if ( usbBulkFormScsiCmd (pBulkDev, 
				     USB_SCSI_FORMAT_UNIT, 
				     0, 
				     0) 
				   != OK )
                return (ERROR); 

            if ( usbBulkCmdExecute (pBulkDev) != USB_COMMAND_SUCCESS )
                {
                USB_BULK_ERR ("usbBulkDevIoctl: FORMAT UNIT Command failed\n", 
                              0, 0, 0, 0, 0, 0);  
                return (ERROR); 
                }   
                
            return (OK);

        case USB_BULK_DESCRIPTOR_GET:

            /* invoke routine to display all descriptors */

            return (usbBulkDescShow (pBulkDev->bulkDevId));

        case USB_BULK_DEV_RESET:

            /* send a class-specific mass storage reset command */
             
            return (usbBulkDevResetRecovery (pBulkDev));

        case USB_BULK_EJECT_MEDIA:

            /* Only applicable if media is removable */

            if ( pBulkDev->blkDev.bd_removable  != TRUE )
                return (ERROR);  

            else
                {
                if ( usbBulkFormScsiCmd (pBulkDev, 
					 USB_SCSI_START_STOP_START, 
					 USB_SCSI_START_STOP_LOEJ, 
					 0) 
					!= OK )
                    return (ERROR); 

                if ( usbBulkCmdExecute (pBulkDev) != USB_COMMAND_SUCCESS )
                    {
                    USB_BULK_ERR ("usbBulkDevIoctl: EJECT Command " \
                                  "failed\n", 0, 0, 0, 0, 0, 0);  
                    return (ERROR); 
                    }
                }

            return (OK); 
	        
        case USB_BULK_MAX_LUN:

            /* May not be supported by devices having single LUN */
        
            if (usbdVendorSpecific (usbdHandle, pBulkDev->bulkDevId,
                USB_RT_DEV_TO_HOST | USB_RT_CLASS | USB_RT_INTERFACE,
                USB_BULK_GET_MAX_LUN, 0, pBulkDev->interface, 1, 
                &(pBulkDev->maxLun), &actLen) != OK )
                {
                USB_BULK_ERR ("usbBulkDevIoctl: Failed to acquire max lun \n", 
                              0, 0, 0, 0, 0, 0);  
                }
            else 
                { 
                 USB_BULK_DEBUG ("usbBulkDevIoctl: Max Lun = %c \n", 
                                 pBulkDev->maxLun, 0, 0, 0, 0, 0);
                }   
      
            return (OK); 
                 
        default:
            errnoSet (S_ioLib_UNKNOWN_REQUEST);
            USB_BULK_DEBUG ("usbBulkDevIoctl: Unknown Request\n", 
                            0, 0, 0, 0, 0, 0);
            return (ERROR);

        }

    }

/***************************************************************************
*
* usbBulkDevResetRecovery - Performs reset recovery on the specified device.
*
* Reset recovery shall be done when device returns a phase error status 
* while executing a CBW.  It is also done, when a stall condition is detected
* on bulk-out endpoint during CBW transport.  The following sequence is 
* performed on the device 
*
* - BulkOnly Mass Storage Reset 
* - Clear HALT feature on the Bulk-in Endpoint
* - Clear HALT feature on the Bulk-out Endpoint
*
* RETURNS: OK, if reset sequence successful otherwise ERROR.
*/

LOCAL STATUS usbBulkDevResetRecovery
    (
    pUSB_BULK_DEV  pBulkDev           /* pointer to bulk device */
    )
    {
    OSS_MUTEX_TAKE (bulkDevMutex, OSS_BLOCK);
      
    /* issue bulk-only mass storage reset request on control End point */


    if ((usbdVendorSpecific (usbdHandle, 
			     pBulkDev->bulkDevId,
			     USB_RT_HOST_TO_DEV | USB_RT_CLASS | USB_RT_INTERFACE,
			     USB_BULK_RESET, 
			     0, 
			     pBulkDev->interface, 
			     0, 
			     NULL, 
			     NULL)) 
			   != OK )
        {
        USB_BULK_ERR ("usbBulkDevResetRecovery: Failed to reset %x\n", 
                      0, 0, 0, 0, 0, 0);  
        
        OSS_MUTEX_RELEASE (bulkDevMutex);
        return (ERROR); 
        }
    else 
       { 
       USB_BULK_DEBUG ("usbBulkDevResetRecovery: Reset...done\n", 
                       0, 0, 0, 0, 0, 0);
       }

    /* clear HALT feature on bulk in and bulk out end points */

    if ((usbdFeatureClear (usbdHandle, 
			   pBulkDev->bulkDevId, 
			   USB_RT_ENDPOINT, 
			   USB_FSEL_DEV_ENDPOINT_HALT, 
			   (pBulkDev->inEpAddress & 0x0F))) 
			 != OK)
        {
        USB_BULK_ERR ("usbBulkDevResetRecovery: Failed to clear HALT feauture"\
                      " on bulk in Endpoint %x\n", 0, 0, 0, 0, 0, 0);
        }  

    if ((usbdFeatureClear (usbdHandle, 
			   pBulkDev->bulkDevId, 
			   USB_RT_ENDPOINT,                            
			   USB_FSEL_DEV_ENDPOINT_HALT, 
			   (pBulkDev->outEpAddress & 0x0F))) 
			 != OK)
        {
        USB_BULK_ERR ("usbBulkDevResetRecovery: Failed to clear HALT feauture"\
                      " on bulk out Endpoint %x\n", 0, 0, 0, 0, 0, 0);
        } 

    OSS_MUTEX_RELEASE (bulkDevMutex);
    return (OK);
  
    }

/***************************************************************************
*
* usbBulkFormScsiCmd - forms command block wrapper(CBW) for requested SCSI command
*
* This routine forms SCSI command blocks as per the Bulk-only command   
* specifications and SCSI protocol.
*
* The following are the input parameters 
*
* .IP <pBulkDev>
* pointer to USB/BULK-ONLY/SCSI device for which the command has to be formed.
*
* .IP <scsiCmd>
* identifies the SCSI command to be formed.
* 
* .IP <cmdParam1>
* parameter required to form a SCSI comamnd, if any.
*
* .IP <cmdParam2>
* parameter required to form a SCSI comamnd, if any.
*
* RETURNS: OK, or ERROR if the command is unsupported.
*/

LOCAL STATUS usbBulkFormScsiCmd 
    (
    pUSB_BULK_DEV pBulkDev,      /* pointer to bulk device     */
    UINT16 scsiCmd,                /* SCSI command               */ 
    UINT32 cmdParam1,              /* command parameter          */
    UINT32 cmdParam2               /* command parameter          */
    )
    {

    BOOL status = OK;

    /* TODO : how to support a LUN other than 0. Also support for more than
     * one scsi device on the usb-scsi bus. how can we get SCSI ID
     */ 

    pBulkDev->bulkCbw.signature = USB_BULK_SWAP_32 (USB_CBW_SIGNATURE);  
    pBulkDev->bulkCbw.tag       = USB_BULK_SWAP_32 (USB_CBW_TAG);
    pBulkDev->bulkCbw.lun       = 0x00;       

    switch (scsiCmd)
        {
        case USB_SCSI_WRITE10:    /* 6-byte SCSI WRITE command  */

             cmdParam1 += pBulkDev->blkOffset;  /* add offset to blk number */

            /* 
             * For WRITE10 command, cmdParam1 refers to the block number from
             * which cmdParam2 number of blocks are to be written. Calculate
             * the transfer length based on the block size and num. of blocks
             */

            pBulkDev->bulkCbw.dataXferLength = USB_BULK_SWAP_32(((cmdParam2) * 
                                               (pBulkDev->blkDev.bd_bytesPerBlk))); 

            pBulkDev->bulkCbw.direction  = USB_CBW_DIR_OUT;

            pBulkDev->bulkCbw.length = 0x0A;

	    /* op code */
            pBulkDev->bulkCbw.CBD[0] = 0x2A;
	    
	    /* DPU, FOA,  and RELADR bits*/
            pBulkDev->bulkCbw.CBD[1] = 0x0;

	    /* lba is bytes 2-5  MSB first, LSB .last */
	    pBulkDev->bulkCbw.CBD[2] = 0x0;
            pBulkDev->bulkCbw.CBD[3] = 0x0;
	    pBulkDev->bulkCbw.CBD[4] = (UINT8)((cmdParam1 & 0xFF00) >>8);
            pBulkDev->bulkCbw.CBD[5] = (UINT8)(cmdParam1 & 0xFF);

	    /* Reserved */
            pBulkDev->bulkCbw.CBD[6] = 0x0;

	    /* transfer length */
            pBulkDev->bulkCbw.CBD[7] = (UINT8)((cmdParam2 & 0xFF00) >>8);
            pBulkDev->bulkCbw.CBD[8] = (UINT8)(cmdParam2 & 0xFF);

	    /* Control */
            pBulkDev->bulkCbw.CBD[9] = 0x0;	


            break;

        case USB_SCSI_WRITE6:    /* 6-byte SCSI WRITE command  */

             cmdParam1 += pBulkDev->blkOffset;  /* add offset to blk number */

            /* 
             * For WRITE6 command, cmdParam1 refers to the block number from
             * which cmdParam2 number of blocks are to be written. Calculate
             * the transfer length based on the block size and num. of blocks
             */

            pBulkDev->bulkCbw.dataXferLength = USB_BULK_SWAP_32(((cmdParam2) * 
                                               (pBulkDev->blkDev.bd_bytesPerBlk))); 

            pBulkDev->bulkCbw.direction  = USB_CBW_DIR_OUT;
            
            pBulkDev->bulkCbw.length = 0x06;
            pBulkDev->bulkCbw.CBD[0] = USB_SCSI_WRITE6;
            pBulkDev->bulkCbw.CBD[1] = 0x0;
            pBulkDev->bulkCbw.CBD[2] = (UINT8)((cmdParam1 & 0xFF00) >>8) ;
            pBulkDev->bulkCbw.CBD[3] = (UINT8)(cmdParam1 & 0xFF);
            pBulkDev->bulkCbw.CBD[4] = (UINT8)cmdParam2;
            pBulkDev->bulkCbw.CBD[5] = 0x0; 
    
            break;

        case USB_SCSI_READ10:     /* 10-byte SCSI READ command */

	    cmdParam1 += pBulkDev->blkOffset; 

            /* 
             * For READ10 command, cmdParam1 refers to the block number from
             * which cmdParam2 number of blocks are to be read. Calculate
             * the transfer length based on the block size and num. of blocks
             */ 
  
            pBulkDev->bulkCbw.dataXferLength = USB_BULK_SWAP_32(((cmdParam2) * 
                                              (pBulkDev->blkDev.bd_bytesPerBlk))); 

            pBulkDev->bulkCbw.direction      = USB_CBW_DIR_IN;
            pBulkDev->bulkCbw.length = 0x0A;
            pBulkDev->bulkCbw.CBD[0] = 0x28;	/* op code */
            pBulkDev->bulkCbw.CBD[1] = 0x0;	/* DPU, FOA,  and RELADR bits*/
            pBulkDev->bulkCbw.CBD[2] = 0x0;	/* lba is bytes 2-5 MSB */
            pBulkDev->bulkCbw.CBD[3] = 0x0;	/* lba is bytes 2-5 */
            pBulkDev->bulkCbw.CBD[4] = (UINT8)((cmdParam1 & 0xFF00) >>8);	/* lba is bytes 2-5 */
            pBulkDev->bulkCbw.CBD[5] = (UINT8)(cmdParam1 & 0xFF);	/* lba is bytes 2-5  LSB*/
            pBulkDev->bulkCbw.CBD[6] = 0x0;	/* Reserved */
            pBulkDev->bulkCbw.CBD[7] = (UINT8)((cmdParam2 & 0xFF00) >>8);	/* transfer length MSB */
            pBulkDev->bulkCbw.CBD[8] = (UINT8)(cmdParam2 & 0xFF);	/* transfer length LSB */
            pBulkDev->bulkCbw.CBD[9] = 0x0;	/* Control */

            break;

        case USB_SCSI_READ6:     /* 6-byte SCSI READ command */

	    cmdParam1 += pBulkDev->blkOffset; 

            /* 
             * For READ6 command, cmdParam1 refers to the block number from
             * which cmdParam2 number of blocks are to be read. Calculate
             * the transfer length based on the block size and num. of blocks
             */ 
  
            pBulkDev->bulkCbw.dataXferLength = USB_BULK_SWAP_32(((cmdParam2) * 
                                              (pBulkDev->blkDev.bd_bytesPerBlk))); 

            pBulkDev->bulkCbw.direction      = USB_CBW_DIR_IN;
            pBulkDev->bulkCbw.length = 0x06;
            pBulkDev->bulkCbw.CBD[0] = USB_SCSI_READ6;
            pBulkDev->bulkCbw.CBD[1] = 0x0;
            pBulkDev->bulkCbw.CBD[2] = (UINT8)((cmdParam1 & 0xFF00) >>8) ;
            pBulkDev->bulkCbw.CBD[3] = (UINT8)(cmdParam1 & 0xFF);
            pBulkDev->bulkCbw.CBD[4] = (UINT8)cmdParam2;
            pBulkDev->bulkCbw.CBD[5] = 0x0; 

            break;

	case USB_SCSI_INQUIRY:   /* Standard 36-byte INQUIRY command */

            pBulkDev->bulkCbw.dataXferLength = USB_BULK_SWAP_32 (USB_SCSI_STD_INQUIRY_LEN); 
            pBulkDev->bulkCbw.direction      = USB_CBW_DIR_IN;

            pBulkDev->bulkCbw.length  = 0x06;
            pBulkDev->bulkCbw.CBD[0]  = USB_SCSI_INQUIRY;
            pBulkDev->bulkCbw.CBD[1]  = 0x0;
            pBulkDev->bulkCbw.CBD[2]  = 0x0;
            pBulkDev->bulkCbw.CBD[3]  = 0x0;
            pBulkDev->bulkCbw.CBD[4]  = USB_SCSI_STD_INQUIRY_LEN;
            pBulkDev->bulkCbw.CBD[5]  = 0x0; 

            break;

        case USB_SCSI_START_STOP_UNIT:      /* for Eject/start media command */

            /* Eject applicable only for removable media */

             if ((cmdParam1 == USB_SCSI_START_STOP_LOEJ) &
                (pBulkDev->blkDev.bd_removable != TRUE)) 
                {
                status = ERROR;
                break;
                }

            pBulkDev->bulkCbw.dataXferLength = 0; 
            pBulkDev->bulkCbw.direction  = USB_CBW_DIR_NONE;

            pBulkDev->bulkCbw.length = 0x06;
            pBulkDev->bulkCbw.CBD[0] = USB_SCSI_START_STOP_UNIT;
            pBulkDev->bulkCbw.CBD[1] = 0x0;
            pBulkDev->bulkCbw.CBD[2] = 0x0;
            pBulkDev->bulkCbw.CBD[3] = 0x0;
            pBulkDev->bulkCbw.CBD[4] = (UINT8)cmdParam1;
            pBulkDev->bulkCbw.CBD[5] = 0x0; 

            break;

        case USB_SCSI_TEST_UNIT_READY: /* Test unit ready command */ 

            pBulkDev->bulkCbw.dataXferLength = 0; 
            pBulkDev->bulkCbw.direction  = USB_CBW_DIR_NONE;

            pBulkDev->bulkCbw.length = 0x06;
            pBulkDev->bulkCbw.CBD[0] = USB_SCSI_TEST_UNIT_READY;
            pBulkDev->bulkCbw.CBD[1] = 0x0;
            pBulkDev->bulkCbw.CBD[2] = 0x0;
            pBulkDev->bulkCbw.CBD[3] = 0x0;
            pBulkDev->bulkCbw.CBD[4] = 0x0;
            pBulkDev->bulkCbw.CBD[5] = 0x0; 

            break;
 
        case USB_SCSI_REQ_SENSE:      /* Request sense command */ 

            pBulkDev->bulkCbw.dataXferLength = 
                                    USB_BULK_SWAP_32(USB_SCSI_REQ_SENSE_LEN); 
            pBulkDev->bulkCbw.direction  = USB_CBW_DIR_IN;

            pBulkDev->bulkCbw.length = 0x06;
            pBulkDev->bulkCbw.CBD[0] = USB_SCSI_REQ_SENSE;
            pBulkDev->bulkCbw.CBD[1] = 0x0;
            pBulkDev->bulkCbw.CBD[2] = 0x0;
            pBulkDev->bulkCbw.CBD[3] = 0x0;
            pBulkDev->bulkCbw.CBD[4] = USB_SCSI_REQ_SENSE_LEN;
            pBulkDev->bulkCbw.CBD[5] = 0x0; 

            break;

        case USB_SCSI_FORMAT_UNIT:    /* Format Unit command */ 

            /* issue a FORMAT UNIT command with default parameters */

            pBulkDev->bulkCbw.dataXferLength = 0; 
            pBulkDev->bulkCbw.direction  = USB_CBW_DIR_NONE;

            pBulkDev->bulkCbw.length = 0x06;
            pBulkDev->bulkCbw.CBD[0] = USB_SCSI_FORMAT_UNIT;
            pBulkDev->bulkCbw.CBD[1] = 0x0;
            pBulkDev->bulkCbw.CBD[2] = 0x0;
            pBulkDev->bulkCbw.CBD[3] = 0x0;
            pBulkDev->bulkCbw.CBD[4] = 0x0;
            pBulkDev->bulkCbw.CBD[5] = 0x0; 

            break;
    
        case USB_SCSI_READ_CAPACITY:  /* Read capacity command */

            /* 
             * Provides a means to request information regarding the capacity 
             * of the logical unit. The response will consists of number of 
             * logical blocks present and the size of the block.
             */

            pBulkDev->bulkCbw.dataXferLength = 
                                      USB_BULK_SWAP_32(USB_SCSI_READ_CAP_LEN); 
            pBulkDev->bulkCbw.direction  = USB_CBW_DIR_IN;

            pBulkDev->bulkCbw.length = 0x0A;
            pBulkDev->bulkCbw.CBD[0] = USB_SCSI_READ_CAPACITY;
            pBulkDev->bulkCbw.CBD[1] = 0x0;
            pBulkDev->bulkCbw.CBD[2] = 0x0;
            pBulkDev->bulkCbw.CBD[3] = 0x0;
            pBulkDev->bulkCbw.CBD[4] = 0x0;
            pBulkDev->bulkCbw.CBD[5] = 0x0; 
            pBulkDev->bulkCbw.CBD[6] = 0x0; 
            pBulkDev->bulkCbw.CBD[7] = 0x0; 
            pBulkDev->bulkCbw.CBD[8] = 0x0; 
            pBulkDev->bulkCbw.CBD[9] = 0x0; 

            break;

        case USB_SCSI_PREVENT_REMOVAL: /* Prevent media removal command */

            /* Applicable only for removable media. Disables removal of media */    

            if (pBulkDev->blkDev.bd_removable  != TRUE) 
                {
                status = ERROR;
                break;
                }

            pBulkDev->bulkCbw.dataXferLength = 0; 
            pBulkDev->bulkCbw.direction  = USB_CBW_DIR_NONE;

            pBulkDev->bulkCbw.length = 0x06;
            pBulkDev->bulkCbw.CBD[0] = USB_SCSI_PREVENT_REMOVAL;
            pBulkDev->bulkCbw.CBD[1] = 0x0;
            pBulkDev->bulkCbw.CBD[2] = 0x0;
            pBulkDev->bulkCbw.CBD[3] = 0x0;
            pBulkDev->bulkCbw.CBD[4] = 0x0;
            pBulkDev->bulkCbw.CBD[5] = 0x0; 

            break; 
         
        default: 
            status= ERROR;
        }
 
    return (status);
 
    } 

/***************************************************************************
*
* usbBulkCmdExecute - Executes a previously formed command block. 
*
* This routine transports the CBW across the BULK_OUT endpoint followed by
* a data transfer phase(if required) to/from the device depending on the 
* direction bit.  After data transfer, CSW is received from the device.
* 
* All transactions to the device are done by forming IRP's initilized with  
* command dependent values.
*
* RETURNS: OK on success, or ERROR if failed to execute
*/

LOCAL USB_COMMAND_STATUS usbBulkCmdExecute
    ( 
    pUSB_BULK_DEV pBulkDev       /* pointer to bulk device */ 
    )
    {
    pUSB_BULK_CSW pCsw ;


if ( pBulkDev->connected == FALSE )
	return (USB_INTERNAL_ERROR);

    if ( OSS_SEM_TAKE (bulkIrpSem, usbBulkIrpTimeOut + 1000) == ERROR )
        {
        USB_BULK_DEBUG ("usbBulkCmdExecute: Irp in Use \n", 0, 0, 0, 0, 0, 0);
        return (USB_INTERNAL_ERROR);
        }

    /* Form an IRP to submit the command block on BULK OUT pipe */

    memset (&(pBulkDev->outIrp), 0, sizeof (USB_IRP));
    
    pBulkDev->outIrp.irpLen            = sizeof (USB_IRP);
    pBulkDev->outIrp.userCallback      = usbBulkIrpCallback; 
    pBulkDev->outIrp.timeout           = usbBulkIrpTimeOut;
    pBulkDev->outIrp.transferLen       = USB_CBW_LENGTH;
    pBulkDev->outIrp.bfrCount          = 0x01;  
    pBulkDev->outIrp.bfrList[0].pid    = USB_PID_OUT;
    pBulkDev->outIrp.bfrList[0].pBfr   = (UINT8 *)&(pBulkDev->bulkCbw);
    pBulkDev->outIrp.bfrList[0].bfrLen = USB_CBW_LENGTH;
    pBulkDev->outIrp.userPtr           = pBulkDev;            

    /* Submit IRP */
 
    if (usbdTransfer (usbdHandle, 
		      pBulkDev->outPipeHandle, 
		      &(pBulkDev->outIrp)) 
		    != OK)
        {
        USB_BULK_ERR ("usbBulkCmdExecute: Unable to submit CBW IRP for transfer\n",
                      0, 0, 0, 0, 0, 0);
        return (USB_INTERNAL_ERROR);
        }

    /* Wait till the transfer of IRP completes or time out */
 
    if ( OSS_SEM_TAKE (bulkIrpSem, 
                       usbBulkIrpTimeOut + USB_BULK_OFFS) 
		     == ERROR )
        {
        USB_BULK_DEBUG ("usbBulkCmdExecute: Irp Time out \n", 0, 0, 0, 0, 0, 0);
        OSS_SEM_GIVE (bulkIrpSem);
        return (USB_INTERNAL_ERROR);
        }

    if (pBulkDev->outIrp.result == S_usbHcdLib_STALLED)
        {
        usbBulkDevResetRecovery (pBulkDev); 
        OSS_SEM_GIVE (bulkIrpSem);
        return (USB_BULK_IO_ERROR); 
        }

    /* 
     * Check whether any data is to be transferred to/from the device.
     * If yes, form an IRP and submit it.
     */

    if (pBulkDev->bulkCbw.dataXferLength > 0)
        {
        if (pBulkDev->bulkCbw.direction == USB_CBW_DIR_IN)
            {
         
            /* data is expected from the device. read from BULK_IN pipe */

            memset (&(pBulkDev->inIrp), 0, sizeof (USB_IRP));   

            /* form an IRP to read from BULK_IN pipe */ 

            pBulkDev->inIrp.irpLen            = sizeof(USB_IRP);
            pBulkDev->inIrp.userCallback      = usbBulkIrpCallback; 
            pBulkDev->inIrp.timeout           = usbBulkIrpTimeOut;
            pBulkDev->inIrp.transferLen       = USB_BULK_SWAP_32(
                                                pBulkDev->bulkCbw.dataXferLength);
            pBulkDev->inIrp.bfrCount          = 0x01;  
            pBulkDev->inIrp.bfrList[0].pid    = USB_PID_IN;
            pBulkDev->inIrp.bfrList[0].pBfr   = pBulkDev->bulkInData;
            pBulkDev->inIrp.bfrList[0].bfrLen = USB_BULK_SWAP_32( 
                                                pBulkDev->bulkCbw.dataXferLength);
            pBulkDev->inIrp.userPtr           = pBulkDev;            
 
            /* Submit IRP */
 
            if (usbdTransfer (usbdHandle, 
			      pBulkDev->inPipeHandle, 
			      &(pBulkDev->inIrp)) 
			    != OK)
                {
                USB_BULK_ERR ("usbBulkCmdExecute: Unable to submit IRP for "\
                              "BULK_IN data transfer\n", 0, 0, 0, 0, 0, 0);
                OSS_SEM_GIVE (bulkIrpSem); 
                return (USB_INTERNAL_ERROR);
                }

            /* 
             * wait till the data transfer ends on the bulk in pipe, before 
             * reading the command status
             */
           
            if ( OSS_SEM_TAKE (bulkIrpSem, 
                               usbBulkIrpTimeOut + USB_BULK_OFFS)  == ERROR )
                {
                USB_BULK_DEBUG ("usbBulkCmdExecute: Irp time out \n", 
                                0, 0, 0, 0, 0, 0);
                OSS_SEM_GIVE (bulkIrpSem);
                return (USB_INTERNAL_ERROR);
                }


            }
        else
            {  

            /* device is expecting data over BULK_OUT pipe. send it */
        
            memset (&pBulkDev->outIrp, 0, sizeof (USB_IRP));

            /* form an IRP to write to BULK_OUT pipe */ 

            pBulkDev->outIrp.irpLen            = sizeof(USB_IRP);
            pBulkDev->outIrp.userCallback      = usbBulkIrpCallback; 
            pBulkDev->outIrp.timeout           = usbBulkIrpTimeOut;
            pBulkDev->outIrp.transferLen       = USB_BULK_SWAP_32(
                                                 pBulkDev->bulkCbw.dataXferLength);
            pBulkDev->outIrp.bfrCount          = 0x01;  
            pBulkDev->outIrp.bfrList[0].pid    = USB_PID_OUT;
            pBulkDev->outIrp.bfrList[0].pBfr   = pBulkDev->bulkOutData;
            pBulkDev->outIrp.bfrList[0].bfrLen = USB_BULK_SWAP_32(
                                                 pBulkDev->bulkCbw.dataXferLength);
            pBulkDev->outIrp.userPtr           = pBulkDev;


            /* Submit IRP */
 
            if (usbdTransfer (usbdHandle, pBulkDev->outPipeHandle, 
                              &(pBulkDev->outIrp)) != OK)
                {
                USB_BULK_ERR ("usbBulkCmdExecute: Unable to submit IRP for "\
                              "BULK_OUT data transfer\n", 0, 0, 0, 0, 0, 0);
                OSS_SEM_GIVE (bulkIrpSem);  
                return (USB_INTERNAL_ERROR);
                }
       
            /* 
             * wait till the data transfer ends on bulk out pipe before reading
             * the command status 
             */

            if ( OSS_SEM_TAKE (bulkIrpSem, 
                               usbBulkIrpTimeOut + USB_BULK_OFFS) == ERROR )
                {
                USB_BULK_DEBUG ("usbBulkCmdExecute: Irp time out \n", 
                                0, 0, 0, 0, 0, 0);
                OSS_SEM_GIVE (bulkIrpSem);
                return (USB_INTERNAL_ERROR);
                }
            }  
        }

    /* read the command status from the device. */

    memset (&pBulkDev->statusIrp, 0, sizeof (USB_IRP));

    /* Form an IRP to read the CSW from the BULK_IN pipe */

    pBulkDev->statusIrp.irpLen            = sizeof( USB_IRP );
    pBulkDev->statusIrp.userCallback      = usbBulkIrpCallback;
    pBulkDev->statusIrp.timeout           = usbBulkIrpTimeOut;
    pBulkDev->statusIrp.transferLen       = USB_CSW_LENGTH;
    pBulkDev->statusIrp.bfrCount          = 0x01;  
    pBulkDev->statusIrp.bfrList[0].pid    = USB_PID_IN;
    pBulkDev->statusIrp.bfrList[0].pBfr   = (UINT8 *)(&pBulkDev->bulkCsw);
    pBulkDev->statusIrp.bfrList[0].bfrLen = USB_CSW_LENGTH;
    pBulkDev->statusIrp.userPtr           = pBulkDev;

    /* Submit IRP */

    if (usbdTransfer (usbdHandle, 
		      pBulkDev->inPipeHandle, 
		      &pBulkDev->statusIrp) 
		    != OK)
        {
        USB_BULK_ERR ("usbBulkCmdExecute: Unable to submit Status IRP\n",
                      0, 0, 0, 0, 0, 0);
        OSS_SEM_GIVE (bulkIrpSem);
        return (USB_INTERNAL_ERROR);
        }

    if ( OSS_SEM_TAKE (bulkIrpSem, 
                       usbBulkIrpTimeOut + USB_BULK_OFFS) 
		     == ERROR )
        {
        USB_BULK_DEBUG ("usbBulkCmdExecute: Irp time out \n", 
                        0, 0, 0, 0, 0, 0);
        OSS_SEM_GIVE (bulkIrpSem);
        return (USB_INTERNAL_ERROR);
        }

    /* Check whether status IRP was transferred */
        
    if (pBulkDev->statusIrp.result == S_usbHcdLib_STALLED)  /* if stalled */
        { 
 
        /* Clear STALL on the BULK IN endpoint */

        if ((usbdFeatureClear (usbdHandle, 
			       pBulkDev->bulkDevId, 
			       USB_RT_ENDPOINT, 
			       USB_FSEL_DEV_ENDPOINT_HALT, 
			       (pBulkDev->inEpAddress & 0x7F))) 
			      != OK)
            {
            USB_BULK_ERR ("usbBulkCmdExecute: Failed to clear HALT feauture "\
                          "on bulk in Endpoint %x\n", 0, 0, 0, 0, 0, 0);
            }  

        /* Try to read the CSW once again */

        if (usbdTransfer (usbdHandle, 
			  pBulkDev->inPipeHandle, 
			  &pBulkDev->statusIrp) 
			!= OK)
            {
            USB_BULK_ERR ("usbBulkCmdExecute: Unable to submit Status IRP\n",
                          0, 0, 0, 0, 0, 0);
            OSS_SEM_GIVE (bulkIrpSem);
            return (USB_INTERNAL_ERROR);
            } 

        if ( OSS_SEM_TAKE (bulkIrpSem, 
                           usbBulkIrpTimeOut + USB_BULK_OFFS) 
			 == ERROR )
            {
            USB_BULK_DEBUG ("usbBulkCmdExecute: Irp time out \n", 
                            0, 0, 0, 0, 0, 0);
            OSS_SEM_GIVE (bulkIrpSem);
            return (USB_INTERNAL_ERROR);
            }

        /* how about the status this time */

        if (pBulkDev->statusIrp.result == S_usbHcdLib_STALLED)
            {
            /* Failed to read CSW again. Do reset recovery */
            
            OSS_SEM_GIVE (bulkIrpSem);
            usbBulkDevResetRecovery (pBulkDev); 
            return (USB_BULK_IO_ERROR);
            }
        }

    OSS_SEM_GIVE (bulkIrpSem);    

    /* If any error other than STALL on Endpoint. */ 

    if ( pBulkDev->statusIrp.result != OK )
        {
        return (USB_IRP_FAILED);
        }

    /* If successful in transferring the CSW IRP, check the buffer */

    pCsw = (pUSB_BULK_CSW)(pBulkDev->statusIrp.bfrList[0].pBfr);

    /* Check the length of CSW received. If not USB_CSW_LENGTH, invalid CSW. */


    if ( pBulkDev->statusIrp.bfrList[0].actLen != USB_CSW_LENGTH)
        {
        USB_BULK_ERR ("usbBulkCmdExecute: Invalid CSW\n", 0, 0, 0, 0, 0, 0);
        usbBulkDevResetRecovery (pBulkDev);   
        return (USB_INVALID_CSW);
        }

    /* check the signature/tag/status in command status block */

    if ( (pCsw->signature != USB_BULK_SWAP_32 (USB_CSW_SIGNATURE)) 
         || (pCsw->tag != USB_BULK_SWAP_32 (USB_CBW_TAG))
         || (pCsw->status > USB_CSW_PHASE_ERROR))
        {
        USB_BULK_ERR ("usbBulkCmdExecute: Logical Error in status block\n", 
                      0, 0, 0, 0, 0, 0);   
        usbBulkDevResetRecovery (pBulkDev);
        return (USB_INVALID_CSW);
        }
 
    /* check for Data residue. */ 
        
    if (pCsw->dataResidue > 0) 
        { 
        USB_BULK_ERR ("usbBulkCmdExecute: Data transfer incomplete\n", 
                      0, 0, 0, 0, 0, 0);   
        return (USB_DATA_INCOMPLETE); 
        }

    /* It is a valid CSW. Check for the status of the CBW executed */

    if ( pCsw->status == USB_CSW_STATUS_FAIL )  /* Command failed */
        {
        USB_BULK_ERR ("usbBulkCmdExecute: CBW Failed \n", 0, 0, 0, 0, 0, 0);
        return (USB_COMMAND_FAILED); 
        } 
    else if (pCsw->status == USB_CSW_PHASE_ERROR) 
        {
        /* Phase error while executing the command in CBW. Reset recovery */ 

        USB_BULK_ERR ("usbBulkCmdExecute: Phase Error\n", 0, 0, 0, 0, 0, 0);
           
        /* fatal error. do a reset recovery */        

        usbBulkDevResetRecovery (pBulkDev);
        return (USB_PHASE_ERROR);
        }    

    return (USB_COMMAND_SUCCESS);
    }


/***************************************************************************
*
* usbBulkDevDestroy - release USB_BULK_DEV structure and its links
*
* Unlinks the indicated USB_BULK_DEV structure and de-allocates
* resources associated with the device.
*
* RETURNS: N/A
*/

LOCAL void usbBulkDevDestroy
    (
    pUSB_BULK_DEV pBlkDev       /* pointer to bulk device   */
    )

    {
    pUSB_BULK_DEV pBulkDev = (pUSB_BULK_DEV) pBlkDev;

    if (pBulkDev != NULL)
        {

        /* Unlink the structure. */

        usbListUnlink (&pBulkDev->bulkDevLink);

        /* Release pipes and wait for IRPs. */

        if (pBulkDev->outPipeHandle != NULL)
            usbdPipeDestroy (usbdHandle, pBulkDev->outPipeHandle);

        if (pBulkDev->inPipeHandle != NULL)
            usbdPipeDestroy (usbdHandle, pBulkDev->inPipeHandle);

        /* wait for any IRP to complete */

        OSS_SEM_TAKE (bulkIrpSem, OSS_BLOCK); 

        OSS_SEM_GIVE (bulkIrpSem);

        /* Release structure. */

        OSS_FREE (pBulkDev);
        }
    }

/***************************************************************************
*
* usbBulkDevFind - Searches for a USB_BULK_DEV for indicated node ID
*
* RETURNS: pointer to matching USB_BULK_DEV or NULL if not found
*/

LOCAL pUSB_BULK_DEV usbBulkDevFind
    (
    USBD_NODE_ID nodeId          /* node ID to be looked for */
    )

    {
    pUSB_BULK_DEV pBulkDev = usbListFirst (&bulkDevList);

    /* browse through the list */

    while (pBulkDev != NULL)
        {
        if (pBulkDev->bulkDevId == nodeId)
        break;

        pBulkDev = usbListNext (&pBulkDev->bulkDevLink);
        }

    return (pBulkDev);
    }

/***************************************************************************
*
* findEndpoint - Searches for a BULK endpoint of the indicated direction.
*
* RETURNS: pointer to matching endpoint descriptor or NULL if not found
*/

LOCAL pUSB_ENDPOINT_DESCR findEndpoint
    (
    pUINT8 pBfr,
    UINT16 bfrLen,
    UINT16 direction
    )

    {
    pUSB_ENDPOINT_DESCR pEp;

    while ((pEp = usbDescrParseSkip (&pBfr, &bfrLen, USB_DESCR_ENDPOINT)) 
          != NULL)
        {
        if ((pEp->attributes & USB_ATTR_EPTYPE_MASK) == USB_ATTR_BULK &&
            (pEp->endpointAddress & USB_ENDPOINT_DIR_MASK) == direction)
            break;
        }

    return pEp;
    }


/***************************************************************************
*
* usbBulkPhysDevCreate - create a USB_BULK_DEV Structure for the device attached
*
* This function is invoked from the dynamic attach callback routine whenever 
* a USB_BULK_DEV device is attached.  It allocates memory for the structure,
* sets device configuration, and creates pipe for bulk-in and bulk-out endpoints.
* 
* RETURNS: OK on success, ERROR if failed to create pipes, set configuration
*/

LOCAL STATUS usbBulkPhysDevCreate
    (
    USBD_NODE_ID nodeId,         /* USBD Node Id ofthe device */     
    UINT16       configuration,  /* Configuration value       */ 
    UINT16       interface       /* Interface Number          */ 
    )
    {
    UINT16   actLength;
    UINT8  *  pBfr;           /* pointer to descriptor store */ 
    UINT8  * pScratchBfr;       /* another pointer to the above store */ 
    UINT     ifNo;
    UINT16   maxPacketSize;  
    USB_BULK_DEV * pBulkDev;
    USB_CONFIG_DESCR * pCfgDescr;
    USB_INTERFACE_DESCR * pIfDescr;
    USB_ENDPOINT_DESCR * pOutEp;
    USB_ENDPOINT_DESCR * pInEp;


    /*
     * A new device is being attached.	Check if we already 
     * have a structure for this device.
     */

    if (usbBulkDevFind (nodeId) != NULL)
        return (OK);

    /* Allocate a non-cacheable buffer for the descriptor's */

    if ((pBfr = OSS_MALLOC (USB_MAX_DESCR_LEN)) == NULL)
	{        
	USB_BULK_ERR ("usbBulkPhysDevCreate: Unable to allocate memory:pBfr\n",
                        0, 0, 0, 0, 0, 0);
        return ERROR;
        }

    /* Allocate memory for a new structure to represent this device */

    if ((pBulkDev = OSS_CALLOC (sizeof (*pBulkDev))) == NULL)
        {
        USB_BULK_ERR ("usbBulkPhysDevCreate: Unable to allocate \
			memory:pBulkDev\n", 0, 0, 0, 0, 0, 0);
        goto errorExit;
        }


    pBulkDev->bulkDevId     = nodeId; 
    pBulkDev->configuration = configuration;
    pBulkDev->interface     = interface;
    pBulkDev->connected     = TRUE;

    /* Check out the device configuration */

    /* Configuration index is assumed to be one less than config'n value */

    if (usbdDescriptorGet (usbdHandle, 
			   pBulkDev->bulkDevId, 
			   USB_RT_STANDARD | USB_RT_DEVICE, 
			   USB_DESCR_CONFIGURATION, 
			   (configuration - 1), 
			   0, 
			   USB_MAX_DESCR_LEN, 
			   pBfr, 
			   &actLength) 
		  != OK)  
        {
        USB_BULK_ERR ("usbBulkPhysDevCreate: Unable to read configuration "\
                      "descriptor\n", 0, 0, 0, 0, 0, 0);
        goto errorExit;
        }


    if ((pCfgDescr = usbDescrParse (pBfr, 
				    actLength, 
				    USB_DESCR_CONFIGURATION)) 
				 == NULL)
        {
        USB_BULK_ERR ("usbBulkPhysDevCreate: Unable to find configuration "\
                      "descriptor\n", 0, 0, 0, 0, 0, 0);
        goto errorExit;
        }

    /* Look for the interface representing the MSC/SCSI/BULK_ONLY. */

    ifNo = 0;

    /*
     * usbDescrParseSkip() modifies the value of the pointer it recieves
     * so we pass it a copy of our buffer pointer
     */

    pScratchBfr = pBfr;

    while ((pIfDescr = usbDescrParseSkip (&pScratchBfr, 
					  &actLength, 
					  USB_DESCR_INTERFACE)) 
				!= NULL)
        {
        if (ifNo == pBulkDev->interface)
            break;
        ifNo++;
        }

    if (pIfDescr == NULL)
        goto errorExit;

    pBulkDev->altSetting = pIfDescr->alternateSetting;

    /* 
     * Retrieve the endpoint descriptor(s) following the identified interface
     * descriptor.
     */

    if ((pOutEp = findEndpoint (pScratchBfr, 
				actLength, 
				USB_ENDPOINT_OUT)) 
			   == NULL)
        goto errorExit;

    if ((pInEp = findEndpoint (pScratchBfr, 
			       actLength, 
			       USB_ENDPOINT_IN)) 
			     == NULL)
        goto errorExit;

    pBulkDev->outEpAddress = pOutEp->endpointAddress;
    pBulkDev->inEpAddress  = pInEp->endpointAddress;

    /* Set the device configuration corresponding to MSC/SCSI/BULK-ONLY */

    if ((usbdConfigurationSet (usbdHandle, 
			       pBulkDev->bulkDevId, 
			       pBulkDev->configuration, 
			       pCfgDescr->maxPower * USB_POWER_MA_PER_UNIT)) 
			     != OK )
        {
        USB_BULK_ERR ("usbBulkPhysDevCreate: Unable to set device "\
                      "configuration \n", 0, 0, 0, 0, 0, 0);
        goto errorExit;
        }
    else
        {
        USB_BULK_DEBUG ("usbBulkPhysDevCreate: Configuration set to 0x%x \n",
                        pBulkDev->configuration, 0, 0, 0, 0, 0);
        }
    
   /* Select interface 
    * 
    * NOTE: Some devices may reject this command, and this does not represent
    * a fatal error.  Therefore, we ignore the return status.
    */

    usbdInterfaceSet (usbdHandle, 
		      pBulkDev->bulkDevId,
		      pBulkDev->interface, 
		      pBulkDev->altSetting);

    maxPacketSize = *((pUINT8) &pOutEp->maxPacketSize) |
                    (*(((pUINT8) &pOutEp->maxPacketSize) + 1) << 8);

    /* Create a Bulk-out pipe for the MSC/SCSI/BULK-ONLY device */

    if (usbdPipeCreate (usbdHandle, 
			pBulkDev->bulkDevId, 
			pOutEp->endpointAddress, 
			pBulkDev->configuration, 
			pBulkDev->interface, 
			USB_XFRTYPE_BULK, 
			USB_DIR_OUT, 
			maxPacketSize, 
			0, 
			0, 
			&(pBulkDev->outPipeHandle)) 
		      != OK)
        {
        USB_BULK_ERR ("usbBulkPhysDevCreate: Error creating bulk out pipe\n",
                        0, 0, 0, 0, 0, 0);
        goto errorExit;
        } 

    maxPacketSize = *((pUINT8) &pInEp->maxPacketSize) |
	    (*(((pUINT8) &pInEp->maxPacketSize) + 1) << 8);

    /* Create a Bulk-in pipe for the MSC/SCSI/BULK-ONLY device */
       
    if (usbdPipeCreate (usbdHandle, 
			pBulkDev->bulkDevId, 
			pInEp->endpointAddress, 
			pBulkDev->configuration, 
			pBulkDev->interface, 
			USB_XFRTYPE_BULK, 
			USB_DIR_IN, 
			maxPacketSize, 
			0, 
			0, 
			&(pBulkDev->inPipeHandle)) 
		      != OK)
        {
        USB_BULK_ERR ("usbBulkPhysDevCreate: Error creating bulk in pipe\n",
                        0, 0, 0, 0, 0, 0);
        goto errorExit;
        } 

    /* Clear HALT feauture on the endpoints */

    if ((usbdFeatureClear (usbdHandle, 
			   pBulkDev->bulkDevId, 
			   USB_RT_ENDPOINT, 
			   USB_FSEL_DEV_ENDPOINT_HALT, 
			   (pOutEp->endpointAddress & 0x0F))) 
			 != OK)
        {
        USB_BULK_ERR ("usbBulkPhysDevCreate: Failed to clear HALT feauture "\
                      "on bulk out Endpoint %x\n", 0, 0, 0, 0, 0, 0);
        }  

    if ((usbdFeatureClear (usbdHandle, 
			   pBulkDev->bulkDevId, 
			   USB_RT_ENDPOINT, 
			   USB_FSEL_DEV_ENDPOINT_HALT, 
			   (pOutEp->endpointAddress & 0x0F))) 
			 != OK)
        {
        USB_BULK_ERR ("usbBulkPhysDevCreate: Failed to clear HALT feauture "\
                      "on bulk in Endpoint %x\n", 0, 0, 0, 0, 0, 0);
        } 

    /* Link the newly created structure. */

    usbListLink (&bulkDevList, pBulkDev, &pBulkDev->bulkDevLink, LINK_TAIL);
    
    OSS_FREE (pBfr);

    return (OK);

errorExit:

    /* Error in creating a bulk device ..destroy */

    OSS_FREE (pBfr);

    usbBulkDevDestroy (pBulkDev);

    return (ERROR);
    }

/***************************************************************************
*
* usbBulkBlkDevCreate - create a block device.
*
* This routine  initializes a BLK_DEV structure, which describes a 
* logical partition on a USB_BULK_DEV device.  A logical partition is an array 
* of contiguously addressed blocks; it can be completely described by the number
* of blocks and the address of the first block in the partition.  
*
* NOTE:
* If `numBlocks' is 0, the rest of device is used.
* 
* This routine supplies an additional parameter called <flags>.  This bitfield 
* currently only uses bit 1.  This bit determines whether the driver will use a
* SCSI READ6 or SCSI READ10 for read access.
*
* RETURNS: A pointer to the BLK_DEV, or NULL if parameters exceed
* physical device boundaries, or if no bulk device exists.
*/

BLK_DEV * usbBulkBlkDevCreate
    (
    USBD_NODE_ID nodeId,        /* nodeId of the bulk-only device     */ 
    UINT32       numBlks,       /* number of logical blocks on device */
    UINT32       blkOffset,     /* offset of the starting block       */ 
    UINT32       flags		/* optional flags		      */ 
    )
    {
    UINT8 * pInquiry;         /* store for INQUIRY data  */

    pUSB_BULK_DEV pBulkDev  = usbBulkDevFind (nodeId);

    OSS_MUTEX_TAKE (bulkDevMutex, OSS_BLOCK); 

    if ((pInquiry = OSS_MALLOC (USB_SCSI_STD_INQUIRY_LEN)) == NULL)
	{
        USB_BULK_ERR ("usbBulkBlkDevCreate: Error allocating memory\n",
                        0, 0, 0, 0, 0, 0);

        OSS_MUTEX_RELEASE (bulkDevMutex);

        return (NULL);
        }

    if (pBulkDev == NULL)
        {
        USB_BULK_ERR ("usbBulkBlkDevCreate: No MSC/SCSI/BULK-ONLY found\n",
                        0, 0, 0, 0, 0, 0);

	OSS_FREE (pInquiry);

        OSS_MUTEX_RELEASE (bulkDevMutex);

        return (NULL);
        }

    /* 
     * Initialize the standard block device structure for use with file 
     * systems.
     */

    pBulkDev->blkDev.bd_blkRd        = (FUNCPTR) usbBulkDevBlkRd;
    pBulkDev->blkDev.bd_blkWrt       = (FUNCPTR) usbBulkDevBlkWrt;
    pBulkDev->blkDev.bd_ioctl        = (FUNCPTR) usbBulkDevIoctl;
    pBulkDev->blkDev.bd_reset        = (FUNCPTR) usbBulkDevReset; 
    pBulkDev->blkDev.bd_statusChk    = (FUNCPTR) usbBulkDevStatusChk;
    pBulkDev->blkDev.bd_retry        = 1;
    pBulkDev->blkDev.bd_mode         = O_RDWR;
    pBulkDev->blkDev.bd_readyChanged = TRUE;

    /* 
     * Read out the standard INQUIRY information from the device, mainly to
     * check whether the media is removable or not.
     */

    pBulkDev->bulkInData  = pInquiry;
 
    if ( usbBulkFormScsiCmd (pBulkDev, USB_SCSI_INQUIRY, 0, 0) != OK )
        {
        USB_BULK_ERR ("usbBulkBlkDevCreate: Error forming command\n",
                        0, 0, 0, 0, 0, 0);

	OSS_FREE (pInquiry);

        OSS_MUTEX_RELEASE (bulkDevMutex);

        return (NULL); 
        } 

    if ( usbBulkCmdExecute (pBulkDev) != USB_COMMAND_SUCCESS )
        {
        USB_BULK_ERR ("usbBulkBlkDevCreate: Error executing USB_SCSI_INQUIRY "\
                      "command\n", 0, 0, 0, 0, 0, 0);        

	OSS_FREE (pInquiry);

        OSS_MUTEX_RELEASE (bulkDevMutex);

        return (NULL);  
        }
 
    /* Check the media type bit  */

    if (*(pInquiry+1) & USB_SCSI_INQUIRY_RMB_BIT)
        {
        pBulkDev->blkDev.bd_removable = TRUE;
        }
    else
        {
        pBulkDev->blkDev.bd_removable = FALSE;      
        }

    /* read the block size and capacity of device (in terms of blocks) */

    if ( usbBulkFormScsiCmd (pBulkDev, 
			     USB_SCSI_READ_CAPACITY, 
			     0, 
			     0) 
			   != OK )
        {
	OSS_FREE (pInquiry);

        OSS_MUTEX_RELEASE (bulkDevMutex); 

        return (NULL); 
        } 

    /* 
     * Read Capacity command will usually return CHECK CONDITION status
     * very first time, just try again.
     */

    if ( usbBulkCmdExecute (pBulkDev) != USB_COMMAND_SUCCESS )
        {
        if ( usbBulkCmdExecute (pBulkDev) != USB_COMMAND_SUCCESS )  
            {
            USB_BULK_ERR ("usbBulkBlkDevCreate: Read Capacity command failed\n",
                          0, 0, 0, 0, 0, 0);

	    OSS_FREE (pInquiry);

            OSS_MUTEX_RELEASE (bulkDevMutex);

            return (NULL);  
            } 
        } 
    
    /*
     * Response is in BIG endian format. Swap it to get correct value.
     * Also, READ_CAPACITY returns the address of the last logical block,
     * NOT the number of blocks.  Since the blkDev structure wants the
     * number of blocks, we add 1 (numBlocks = last address + 1).
     */

    pBulkDev->numBlks   = USB_SCSI_SWAP_32(*((UINT32 *) pInquiry)) + 1;

    pBulkDev->blkOffset = blkOffset;

    if ( numBlks == 0 )
        pBulkDev->blkDev.bd_nBlocks = (pBulkDev->numBlks - blkOffset); 
    else 
        pBulkDev->blkDev.bd_nBlocks = numBlks;
 
    pBulkDev->blkDev.bd_bytesPerBlk = \
			USB_SCSI_SWAP_32(*((UINT32 *)(pInquiry+4))); 

    /* determine which type of SCSI read command is implemnted */

    if ( (flags & USB_SCSI_FLAG_READ_WRITE10) == 1 )
	pBulkDev->read10Able = TRUE;
    else
	pBulkDev->read10Able = FALSE;

    OSS_FREE (pInquiry);

    OSS_MUTEX_RELEASE (bulkDevMutex);

    return (&pBulkDev->blkDev);
       
    }        

/***************************************************************************
*
* usbBulkDevBlkRd - routine to read one or more blocks from the device.
*
* This routine reads the specified physical sector(s) from a specified
* physical device.  Typically called by file system when data is to be
* read from a particular device.
*
* RETURNS: OK on success, or ERROR if failed to read from device
*/

LOCAL STATUS usbBulkDevBlkRd
    (
    BLK_DEV * pBlkDev,           /* pointer to bulk device   */ 
    int       blkNum,            /* logical block number     */
    int       numBlks,           /* number of blocks to read */
    char *    pBuf               /* store for data           */ 
    )
    {

    pUSB_BULK_DEV  pBulkDev = (USB_BULK_DEV *)pBlkDev;   
    UINT readType;

    /*  Ensure that the device has not been removed during a transfer */

    if ( pBulkDev->connected == FALSE ) 
	return ERROR;

    USB_BULK_DEBUG ("usbBulkDevBlkRd: Number of blocks = %d, Starting blk = %d\n",
                    numBlks, blkNum, 0, 0, 0, 0); 

    OSS_MUTEX_TAKE (bulkDevMutex, OSS_BLOCK);

    /* intialise the pointer to store bulk in data */ 

    pBulkDev->bulkInData = (UINT8 *)pBuf ; 

    if (pBulkDev->read10Able)
	readType = USB_SCSI_READ10;
    else 
	readType = USB_SCSI_READ6;

    if ( usbBulkFormScsiCmd (pBulkDev, 
			     readType, 
			     blkNum, 
			     numBlks) 
			   != OK )
        {
        OSS_MUTEX_RELEASE (bulkDevMutex);
        return (ERROR);  
        }

    if ( usbBulkCmdExecute (pBulkDev) != USB_COMMAND_SUCCESS )
        {
        OSS_MUTEX_RELEASE (bulkDevMutex);
        return (ERROR); 
        }

    OSS_MUTEX_RELEASE (bulkDevMutex);
    return (OK);
  
    }

/***************************************************************************
*
* usbBulkDevBlkWrt - routine to write one or more blocks to the device.
*
* This routine writes the specified physical sector(s) to a specified physical
* device.
*
* RETURNS: OK on success, or ERROR if failed to write to device
*/

LOCAL STATUS usbBulkDevBlkWrt
    (
    BLK_DEV * pBlkDev,           /* pointer to bulk device    */  
    int       blkNum,            /* logical block number      */
    int       numBlks,           /* number of blocks to write */ 
    char *    pBuf               /* data to be written        */ 
    )
    {

    pUSB_BULK_DEV  pBulkDev = (USB_BULK_DEV *)pBlkDev;   
    UINT writeType;

    /*  Ensure that the device has not been removed during a transfer */

    if ( pBulkDev->connected == FALSE ) 
	return ERROR;

    USB_BULK_DEBUG ("usbBulkDevBlkWrt: Number of blocks = %d, Starting blk = %d\n",
                    numBlks, blkNum, 0, 0, 0, 0);

    OSS_MUTEX_TAKE (bulkDevMutex, OSS_BLOCK); 

    /* initialise the pointer to fetch bulk out data */

    pBulkDev->bulkOutData = (UINT8 *)pBuf;

    if (pBulkDev->read10Able)
	writeType = USB_SCSI_WRITE10;
    else 
	writeType = USB_SCSI_WRITE6;


    if ( usbBulkFormScsiCmd (pBulkDev, 
			     writeType, 
			     blkNum, 
                             numBlks) 
			   != OK )
        {
        OSS_MUTEX_RELEASE (bulkDevMutex);
        return (ERROR); 
        }

    if ( usbBulkCmdExecute (pBulkDev) != USB_COMMAND_SUCCESS )
        {
        OSS_MUTEX_RELEASE (bulkDevMutex);
        return (ERROR); 
        }

    OSS_MUTEX_RELEASE (bulkDevMutex);
    return (OK);
    }


/***************************************************************************
*
* usbBulkIrpCallback - Invoked upon IRP completion
*
* Examines the status of the IRP submitted.  
* 
*
* RETURNS: N/A
*/

LOCAL void usbBulkIrpCallback
    (
    pVOID p                      /* pointer to the IRP submitted */
    )
    {
    pUSB_IRP      pIrp     = (pUSB_IRP) p;
    pUSB_BULK_DEV pBulkDev = pIrp->userPtr;


    /* check whether the IRP was for bulk out/ bulk in / status transport */

    if (pIrp == &(pBulkDev->outIrp))
        {  
        if (pIrp->result == OK)     /* check the result of IRP */
            {
            USB_BULK_DEBUG ("usbBulkIrpCallback: Num of Bytes transferred on "\
                            "out pipe %d\n", pIrp->bfrList[0].actLen, 
                            0, 0, 0, 0, 0); 
            }
        else
            {

            USB_BULK_ERR ("usbBulkIrpCallback: Irp failed on Bulk Out %x \n",
                            pIrp->result, 0, 0, 0, 0, 0); 

            /* Clear HALT Feature on Bulk out Endpoint */ 

            if ((usbdFeatureClear (usbdHandle, 
				   pBulkDev->bulkDevId, 
				   USB_RT_ENDPOINT, 
				   USB_FSEL_DEV_ENDPOINT_HALT, 
				   (pBulkDev->outEpAddress & 0x0F))) 
				 != OK)
                {
                USB_BULK_ERR ("usbBulkIrpCallback: Failed to clear HALT "\
                              "feauture on bulk out Endpoint %x\n", 0, 0, 0, 
                              0, 0, 0);
                }
            }
      
        } 
    else if (pIrp == &(pBulkDev->statusIrp))    /* if status block IRP */
        {
        if (pIrp->result == OK)     /* check the result of the IRP */
            {
            USB_BULK_DEBUG ("usbBulkIrpCallback : Num of Status Bytes \
                            read  =%d \n", pIrp->bfrList[0].actLen, 0, 0, 0, 0, 0);
            }
        else     /* status IRP failed */
            {

            USB_BULK_ERR ("usbBulkIrpCallback: Status Irp failed on Bulk in "\
                          "%x\n", pIrp->result, 0, 0, 0, 0, 0);
            } 

        }
    else     /* IRP for bulk_in data */
        {
        if (pIrp->result == OK)
            {
            USB_BULK_DEBUG ("usbBulkIrpCallback: Num of Bytes read from Bulk "\
                            "In =%d\n", pIrp->bfrList[0].actLen, 0, 0, 0, 0, 0); 
            }
        else   /* IRP on BULK IN failed */
            {

            USB_BULK_ERR ("usbBulkIrpCallback : Irp failed on Bulk in ,%x\n", 
                            pIrp->result, 0, 0, 0, 0, 0);

            /* Clear HALT Feature on Bulk in Endpoint */

            if ((usbdFeatureClear (usbdHandle, pBulkDev->bulkDevId, USB_RT_ENDPOINT, 
                                   USB_FSEL_DEV_ENDPOINT_HALT, 
                                   (pBulkDev->inEpAddress & 0x0F))) != OK)
                {
                USB_BULK_ERR ("usbBulkIrpCallback: Failed to clear HALT "\
                              "feature on bulk in Endpoint %x\n", 0, 0, 0, 0, 0, 0);
                }
            }

        }

    OSS_SEM_GIVE (bulkIrpSem);
   
    }   
  
/***************************************************************************
*
* usbBulkDevStatusChk - routine to check device status
*
* Typically called by the file system before doing a read/write to the device.
* The routine issues a TEST_UNIT_READY command.  Also, if the device is not
* ready, then the sense data is read from the device to check for presence/
* change of media.  For removable devices, the ready change flag is set to 
* indicate media change.  File system checks this flag, and remounts the device
*
* RETURNS: OK if the device is ready for IO, or ERROR if device is busy. 
*/

LOCAL STATUS usbBulkDevStatusChk
    (
    BLK_DEV *pBlkDev             /* pointer to bulk device */
    )
    {
     
    USB_COMMAND_STATUS status;
    UINT8   requestSense[20];         /* store for REQUEST SENSE data  */                 
    /* 
     * requestSense can be a automatic buffer because it is not 
     * passed to the hardware.  Instead it is a place holder for 
     * data read from the hardware.
     */

    pUSB_BULK_DEV  pBulkDev = (USB_BULK_DEV *)pBlkDev;    
 
    OSS_MUTEX_TAKE (bulkDevMutex, OSS_BLOCK);

    /* 
     * The device might take long time to respond when the disk is
     * changed. So the time out for this IRP is increased.
     */

    usbBulkIrpTimeOut = (USB_BULK_IRP_TIME_OUT * 4);

    /* Form a SCSI Test Unit Ready command and send it */ 
 
    if ( usbBulkFormScsiCmd (pBulkDev, 
			     USB_SCSI_TEST_UNIT_READY, 
			     0, 
			     0) 
			   != OK )
        {
        OSS_MUTEX_RELEASE (bulkDevMutex);
        return (ERROR); 
        } 

    status = usbBulkCmdExecute (pBulkDev);

    usbBulkIrpTimeOut = USB_BULK_IRP_TIME_OUT;

    if (status == USB_COMMAND_FAILED )
        {

        /* TEST_UNIT_READY command failed.  Get request sense data.*/

        pBulkDev->bulkInData = requestSense;
        if ( usbBulkFormScsiCmd (pBulkDev, 
				 USB_SCSI_REQ_SENSE, 
				 0, 
				 0) 
				!= OK )
            {
            USB_BULK_ERR ("usbBulkDevStatusChk: Error forming command\n",
                          0, 0, 0, 0, 0, 0);
            OSS_MUTEX_RELEASE (bulkDevMutex);
            return (ERROR); 
            } 

        if ( usbBulkCmdExecute (pBulkDev) != USB_COMMAND_SUCCESS )
            {
            USB_BULK_ERR ("usbBulkDevStatusChk: Error executing USB_SCSI_REQ_SENSE" \
                          "command\n", 0, 0, 0, 0, 0, 0);        
            OSS_MUTEX_RELEASE (bulkDevMutex);
            return (ERROR);  
            }

        /* Check the sense data for possible reasons of CHECK_CONDITION */

        /* This is really needed for removable media to declare media change or
         * or media not present. Once request sense is read, the device shall be
         * ready for further commands.
         */       

        /* check for media change */

        if (((requestSense [USB_SCSI_SENSE_KEY_OFFSET]) & 
            (USB_SCSI_SENSE_KEY_MASK)) == USB_SCSI_KEY_UNIT_ATTN)
            {
            if (requestSense [USB_SCSI_SENSE_ASC] == USB_SCSI_ASC_RESET) 
                {
                USB_BULK_ERR ("usbBulkDevStatusChk: Bus reset or media "\
                              "change \n", 0, 0, 0, 0, 0, 0);

                /* declare that device has to be mounted on next IO operation */

                if (pBulkDev->blkDev.bd_removable == TRUE)
                    pBulkDev->blkDev.bd_readyChanged = TRUE;

                OSS_MUTEX_RELEASE (bulkDevMutex);
                return (OK);
                }
            }

        /* check for media present */

        if (((requestSense [USB_SCSI_SENSE_KEY_OFFSET]) & 
            (USB_SCSI_SENSE_KEY_MASK)) == USB_SCSI_KEY_NOT_READY)
            {
            if (requestSense [USB_SCSI_SENSE_ASC] == USB_SCSI_ASC_NO_MEDIA) 
                {
                USB_BULK_ERR ("usbBulkDevStatusChk: Media not present\n",
                              0, 0, 0, 0, 0, 0);

                /* declare that device has to be mounted on next IO operation */

                if (pBulkDev->blkDev.bd_removable == TRUE)
                    pBulkDev->blkDev.bd_readyChanged = TRUE;
                }
                /* return ERROR */
            }  

        /* For all other conditions, return error */ 

        OSS_MUTEX_RELEASE (bulkDevMutex);  
        return (ERROR);
        
        } 
    else if (status != USB_COMMAND_SUCCESS) /* other than COMMAND_FAILED error */
        {
        OSS_MUTEX_RELEASE (bulkDevMutex);
        return (ERROR);
        }   

    /* if we are here, then TEST UNIT READY command returned device is READY */

    OSS_MUTEX_RELEASE (bulkDevMutex);
    return (OK);
    }


/***************************************************************************
*
* usbBulkDevReset -  routine to reset the MSC/SCSI/BULK-only device.
*
* Typically called by file system when it mounts the device first time or when a 
* read/write fails even after specified number of retries.  It issues a mass 
* storage reset class-specific request, which prepares the device ready to 
* receive the next CBW from the host. 
* 
* RETURNS: OK if reset succcssful, or ERROR
*/

LOCAL STATUS usbBulkDevReset
    (
    BLK_DEV * pBlkDev            /* pointer to bulk device */
    )
    {
    STATUS s;
    pUSB_BULK_DEV  pBulkDev = (USB_BULK_DEV *)pBlkDev;    


    /*  Ensure that the device has not been removed during a transfer */

    if ( pBulkDev->connected == FALSE ) 
	return ERROR;

    OSS_MUTEX_TAKE (bulkDevMutex, OSS_BLOCK);
      
    /* issue bulk-only mass storage reset request on control End point */

    if ((s = usbdVendorSpecific (usbdHandle, 
				 pBulkDev->bulkDevId,
				 USB_RT_HOST_TO_DEV | USB_RT_CLASS | USB_RT_INTERFACE,
				 USB_BULK_RESET, 
				 0, 
				 pBulkDev->interface, 
				 0, 
				 NULL, 
				 NULL)) 
				!= OK )
        {
        USB_BULK_ERR ("usbBulkDevReset: Failed to reset %x\n", 
                        0, 0, 0, 0, 0, 0);  
        }
    else 
       { 
       USB_BULK_DEBUG ("usbBulkDevReset: Reset...done\n", 0, 0, 0, 0, 0, 0);
       }


    OSS_MUTEX_RELEASE (bulkDevMutex);
    return (s);
    }


/***************************************************************************
*
* notifyAttach - Notifies registered callers of attachment/removal
*
* RETURNS: N/A
*/

LOCAL VOID notifyAttach
    (
    USBD_NODE_ID bulkDevId,
    UINT16 attachCode
    )

    {
    pATTACH_REQUEST pRequest = usbListFirst (&reqList);

    while (pRequest != NULL)
    {
    (*pRequest->callback) (pRequest->callbackArg, 
			   bulkDevId, attachCode);
    pRequest = usbListNext (&pRequest->reqLink);
    }
    }

/***************************************************************************
*
* usbBulkDynamicAttachRegister - Register SCSI/BULK-ONLY device attach callback.
*
* <callback> is a caller-supplied function of the form:
*
* .CS
* typedef (*USB_BULK_ATTACH_CALLBACK) 
*     (
*     pVOID arg,
*     USBD_NODE_ID bulkDevId,
*     UINT16 attachCode
*     );
* .CE
*
* usbBulkDevLib will invoke <callback> each time a  MSC/SCSI/BULK-ONLY device
* is attached to or removed from the system.  <arg> is a caller-defined
* parameter which will be passed to the <callback> each time it is
* invoked.  The <callback> will also be passed the nodeID of the device 
* being created/destroyed and an attach code of USB_BULK_ATTACH or 
* USB_BULK_REMOVE.
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
*   S_usbBulkDevLib_BAD_PARAM
*   S_usbBulkDevLib_OUT_OF_MEMORY
*/

STATUS usbBulkDynamicAttachRegister
    (
    USB_BULK_ATTACH_CALLBACK callback,	/* new callback to be registered */
    pVOID arg                           /* user-defined arg to callback  */
    )

    {
    pATTACH_REQUEST   pRequest;
    pUSB_BULK_DEV     pBulkDev;
    int status = OK;

    /* Validate parameters */

    if (callback == NULL)
        return (ossStatus (S_usbBulkDevLib_BAD_PARAM));

    OSS_MUTEX_TAKE (bulkDevMutex, OSS_BLOCK);

    /* Create a new request structure to track this callback request. */

    if ((pRequest = OSS_CALLOC (sizeof (*pRequest))) == NULL)
        {
        status = S_usbBulkDevLib_OUT_OF_MEMORY;
        }
    else
        {
        pRequest->callback    = callback;
        pRequest->callbackArg = arg;

        usbListLink (&reqList, pRequest, &pRequest->reqLink, LINK_TAIL) ;
    
       /* 
        * Perform an initial notification of all currrently attached
        * SCSI/BULK-ONLY devices.
        */

        pBulkDev = usbListFirst (&bulkDevList);

        while (pBulkDev != NULL)
	    {
            if (pBulkDev->connected)
                (*callback) (arg, pBulkDev->bulkDevId, USB_BULK_ATTACH);

	    pBulkDev = usbListNext (&pBulkDev->bulkDevLink);
	    }
        }

    OSS_MUTEX_RELEASE (bulkDevMutex);

    return (ossStatus (status));
    }


/***************************************************************************
*
* usbBulkDynamicAttachUnregister - Unregisters SCSI/BULK-ONLY attach callback.
*
* This function cancels a previous request to be dynamically notified for
* SCSI/BULK-ONLY device attachment and removal.  The <callback> and <arg> 
* paramters must exactly match those passed in a previous call to 
* usbBulkDynamicAttachRegister().
*
* RETURNS: OK, or ERROR if unable to unregister callback
*
* ERRNO:
*   S_usbBulkDevLib_NOT_REGISTERED
*/

STATUS usbBulkDynamicAttachUnregister
    (
    USB_BULK_ATTACH_CALLBACK callback,  /* callback to be unregistered  */
    pVOID arg                           /* user-defined arg to callback */
    )

    {
    pATTACH_REQUEST pRequest;
    int status = S_usbBulkDevLib_NOT_REGISTERED;


    OSS_MUTEX_TAKE (bulkDevMutex, OSS_BLOCK);

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

    OSS_MUTEX_RELEASE (bulkDevMutex);

    return (ossStatus (status));
    }


/***************************************************************************
*
* usbBulkDevLock - Marks USB_BULK_DEV structure as in use.
*
* A caller uses usbBulkDevLock() to notify usbBulkDevLib that it is using 
* the indicated USB_BULK_DEV structure.  usbBulkDevLib maintains
* a count of callers using a particular USB_BULK_DEV structure so that it 
* knows when it is safe to dispose of a structure when the underlying
* USB_BULK_DEV is removed from the system.  So long as the "lock count"
* is greater than zero, usbBulkDevLib will not dispose of an USB_BULK_DEV
* structure.
*
* RETURNS: OK, or ERROR if unable to mark USB_BULK_DEV structure in use.
*/

STATUS usbBulkDevLock
    (
    USBD_NODE_ID nodeId    /* NodeId of the BLK_DEV to be marked as in use */
    )

    {
    pUSB_BULK_DEV pBulkDev = usbBulkDevFind (nodeId);

    if ( pBulkDev == NULL)
        return (ERROR);

    pBulkDev->lockCount++;

    return (OK);
    }


/***************************************************************************
*
* usbBulkDevUnlock - Marks USB_BULK_DEV structure as unused.
*
* This function releases a lock placed on an USB_BULK_DEV structure.  When a
* caller no longer needs an USB_BULK_DEV structure for which it has previously
* called usbBulkDevLock(), then it should call this function to
* release the lock.
*
* NOTE: If the underlying SCSI/BULK-ONLY device has already been removed
* from the system, then this function will automatically dispose of the
* USB_BULK_DEV structure if this call removes the last lock on the structure.
* Therefore, a caller must not reference the USB_BULK_DEV structure after
* making this call.
*
* RETURNS: OK, or ERROR if unable to mark USB_BULK_DEV structure unused
*
* ERRNO:
*   S_usbBulkDevLib_NOT_LOCKED
*/

STATUS usbBulkDevUnlock
    (
    USBD_NODE_ID nodeId    /* NodeId of the BLK_DEV to be marked as unused */
    )

    {
    int status = OK;
    pUSB_BULK_DEV pBulkDev = usbBulkDevFind (nodeId);
 
    if ( pBulkDev == NULL)
        return (ERROR);

    OSS_MUTEX_TAKE (bulkDevMutex, OSS_BLOCK);

    if (pBulkDev->lockCount == 0)
        {
        status = S_usbBulkDevLib_NOT_LOCKED;
        }
    else
    {
    /* If this is the last lock and the underlying USB_BULK device is
     * no longer connected, then dispose of the device.
     */

    if ((--pBulkDev->lockCount == 0) && (!pBulkDev->connected))
	usbBulkDevDestroy (pBulkDev);
    }

    OSS_MUTEX_RELEASE (bulkDevMutex);

    return (ossStatus (status));
    }



/***************************************************************************
*
* usbBulkDescShow - Show routine for displaying all descriptors
*
* RETURNS: OK on success or ERROR if unable to read descriptors.
*/

LOCAL STATUS usbBulkDescShow 
    (
    USBD_NODE_ID nodeId           /* nodeId of bulk device         */
    )
    {

    UINT8    bfr[255];            /* store for descriptors         */
    UINT8    index;               
    UINT8    numCfg;              /* number of configuration       */
    UINT8    mfcIndex;            /* index for manufacturer string */
    UINT8    productIndex;        /* index for product string      */
    UINT16   actLength;           /* actual length of descriptor   */
    pUSB_DEVICE_DESCR  pDevDescr; /* pointer to device descriptor  */
    pUSB_STRING_DESCR  pStrDescr; /* pointer to string descriptor  */   

    /* Get the Device descriptor first */

    if (usbdDescriptorGet (usbdHandle, nodeId, 
                           USB_RT_STANDARD | USB_RT_DEVICE,
                           USB_DESCR_DEVICE, 0, 0, 20, bfr, &actLength) != OK)
        {
        USB_BULK_ERR ("usbBulkDevDescShow: Failed to read Device descriptor\n",
                        0, 0, 0, 0, 0, 0);
        return (ERROR);
        }

    if ((pDevDescr = usbDescrParse (bfr, actLength, USB_DESCR_DEVICE)) == NULL)
        {
        USB_BULK_ERR ("usbBulkDevDescShow: No Device descriptor was found "\
                       "in the buffer \n", 0, 0, 0, 0, 0, 0);
        return (ERROR);
        }

    /* store the num. of configurations, string indices locally */

    numCfg       = pDevDescr->numConfigurations;
    mfcIndex     = pDevDescr->manufacturerIndex;
    productIndex = pDevDescr->productIndex;

    printf ("DEVICE DESCRIPTOR:\n");
    printf ("------------------\n");
    printf (" Length                    = 0x%x\n", pDevDescr->length);
    printf (" Descriptor Type           = 0x%x\n", pDevDescr->descriptorType);
    printf (" USB release in BCD        = 0x%x\n", pDevDescr->bcdUsb);
    printf (" Device Class              = 0x%x\n", pDevDescr->deviceClass);
    printf (" Device Sub-Class          = 0x%x\n", pDevDescr->deviceSubClass);
    printf (" Device Protocol           = 0x%x\n", pDevDescr->deviceProtocol);
    printf (" Max Packet Size           = 0x%x\n", pDevDescr->maxPacketSize0);
    printf (" Vendor ID                 = 0x%x\n", pDevDescr->vendor);
    printf (" Product ID                = 0x%x\n", pDevDescr->product);
    printf (" Dev release in BCD        = 0x%x\n", pDevDescr->bcdDevice);
    printf (" Manufacturer              = 0x%x\n", pDevDescr->manufacturerIndex);
    printf (" Product                   = 0x%x\n", pDevDescr->productIndex);
    printf (" Serial Number             = 0x%x\n", 
            pDevDescr->serialNumberIndex);
    printf (" Number of Configurations  = 0x%x\n\n", 
            pDevDescr->numConfigurations);

    /* get the manufacturer string descriptor  */     

    if (usbdDescriptorGet (usbdHandle, nodeId, 
                           USB_RT_STANDARD | USB_RT_DEVICE,
                           USB_DESCR_STRING , mfcIndex,
                           0x0409, 100, bfr, &actLength) != OK)
        {
        USB_BULK_ERR ("usbBulkDevDescShow: Failed to read String descriptor\n",
                        0, 0, 0, 0, 0, 0);
        }
    else
        {

        if ((pStrDescr = usbDescrParse (bfr, actLength, 
                                        USB_DESCR_STRING)) == NULL)
            {
            USB_BULK_ERR ("usbBulkDevDescShow: No String descriptor was "\
                          "found in the buffer \n", 0, 0, 0, 0, 0, 0);
            }
        else
            {
            printf ("STRING DESCRIPTOR : %d\n",1);
            printf ("----------------------\n");
            printf (" Length              = 0x%x\n", pStrDescr->length);
            printf (" Descriptor Type     = 0x%x\n", pStrDescr->descriptorType); 
            printf (" Manufacturer String = ");  
            for (index = 0; index < (pStrDescr->length -2) ; index++)
                printf("%c", pStrDescr->string [index]);
            printf("\n\n");
            }
        }  

    /* get the product string descriptor  */

    if (usbdDescriptorGet (usbdHandle, nodeId, 
                           USB_RT_STANDARD | USB_RT_DEVICE,
                           USB_DESCR_STRING , productIndex,
                           0x0409, 100, bfr, &actLength) != OK)
        {
        USB_BULK_ERR ("usbBulkDevDescShow: Failed to read String descriptor\n", 
                        0, 0, 0, 0, 0, 0);
        }
    else
        {

        if ((pStrDescr = usbDescrParse (bfr, actLength, 
                                        USB_DESCR_STRING)) == NULL)
            {
            USB_BULK_ERR ("usbBulkDevDescShow: No String descriptor was "\
                          "found in the buffer \n", 0, 0, 0, 0, 0, 0);
            }
        else
            {
            printf ("STRING DESCRIPTOR : %d\n",2);
            printf ("----------------------\n");
            printf (" Length              = 0x%x\n", pStrDescr->length);
            printf (" Descriptor Type     = 0x%x\n", pStrDescr->descriptorType); 
            printf (" Product String      = ");  

            for (index = 0; index < (pStrDescr->length -2) ; index++)
                printf("%c", pStrDescr->string [index]);

            printf("\n\n");
            }
        }

    /* get the configuration descriptors one by one for each configuration */

    for (index = 0; index < numCfg; index++) 
       if (usbBulkConfigDescShow ( nodeId, index ) == ERROR)
          return (ERROR);

    return  (OK);

    }


/***************************************************************************
*
* usbBulkConfigDescShow - shows routine for displaying configuration descriptors.
*
* RETURNS: OK on success or ERROR if unable to read descriptors. 
*/

LOCAL STATUS usbBulkConfigDescShow 
    (
    USBD_NODE_ID nodeId,         /* nodeId of bulk device       */
    UINT8 cfgIndex               /* index of configuration      */
    )
    {

    UINT8    bfr[255];           /* store for config descriptor */
    UINT8  * pBfr = bfr;         /* pointer to the above store  */
    UINT8    ifIndex;            /* index for interfaces        */
    UINT8    epIndex;            /* index for endpoints         */
    UINT16   actLength;          /* actual length of descriptor */

    pUSB_CONFIG_DESCR pCfgDescr;    /* pointer to config'n descriptor   */
    pUSB_INTERFACE_DESCR pIntDescr; /* pointer to interface descriptor  */ 
    pUSB_ENDPOINT_DESCR pEndptDescr;/* pointer to endpointer descriptor */ 
 
    /* get the configuration descriptor */

    if (usbdDescriptorGet (usbdHandle, nodeId, 
                           USB_RT_STANDARD | USB_RT_DEVICE,
                           USB_DESCR_CONFIGURATION, cfgIndex, 0, 
                           255, bfr, &actLength) != OK)
        {
        USB_BULK_ERR ("usbBulkConfigDescShow: Failed to read Config'n "\
                      "descriptor\n", 0, 0, 0, 0, 0, 0);
        return (ERROR);
        }

        
    if ((pCfgDescr = usbDescrParseSkip (&pBfr, &actLength, 
                                        USB_DESCR_CONFIGURATION)) == NULL)
        {
        USB_BULK_ERR ("usbBulkConfigDescShow: No Config'n descriptor was found "\
                      "in the buffer \n", 0, 0, 0, 0, 0, 0);
        return (ERROR);
        }

    printf ("CONFIGURATION DESCRIPTOR: %d\n", cfgIndex);
    printf ("----------------------------\n");
    printf (" Length                    = 0x%x\n", pCfgDescr->length);
    printf (" Descriptor Type           = 0x%x\n", pCfgDescr->descriptorType);
    printf (" Total Length              = 0x%x\n", pCfgDescr->totalLength);
    printf (" Number of Interfaces      = 0x%x\n", pCfgDescr->numInterfaces);
    printf (" Configuration Value       = 0x%x\n", pCfgDescr->configurationValue);
    printf (" Configuration Index       = 0x%x\n", pCfgDescr->configurationIndex);
    printf (" Attributes                = 0x%x\n", pCfgDescr->attributes);
    printf (" Maximum Power             = 0x%x\n\n", pCfgDescr->maxPower);

    /* Parse for the interface and its related endpoint descriptors */   

    for (ifIndex = 0; ifIndex < pCfgDescr->numInterfaces; ifIndex++)
        {

        if ((pIntDescr = usbDescrParseSkip (&pBfr, &actLength, 
                                            USB_DESCR_INTERFACE)) == NULL)
            {
            USB_BULK_ERR ("usbBulkInterfaceDescShow: No Interface descriptor "\
                          "was found \n", 0, 0, 0, 0, 0, 0);
            return (ERROR);
            }

        printf ("INTERFACE DESCRIPTOR: %d\n", ifIndex);
        printf ("-----------------------\n");
        printf (" Length                    = 0x%x\n", pIntDescr->length);
        printf (" Descriptor Type           = 0x%x\n", pIntDescr->descriptorType);
        printf (" Interface Number          = 0x%x\n", pIntDescr->interfaceNumber);
        printf (" Alternate setting         = 0x%x\n", 
                pIntDescr->alternateSetting);
        printf (" Number of Endpoints       = 0x%x\n", pIntDescr->numEndpoints) ;
        printf (" Interface Class           = 0x%x\n", 
                pIntDescr->interfaceClass);
        printf (" Interface Sub-Class       = 0x%x\n", 
                pIntDescr->interfaceSubClass);
        printf (" Interface Protocol        = 0x%x\n", 
                pIntDescr->interfaceProtocol);
        printf (" Interface Index           = 0x%x\n\n", pIntDescr->interfaceIndex); 
        
        for ( epIndex = 0; epIndex < pIntDescr->numEndpoints; epIndex++)
            { 
            if ((pEndptDescr = usbDescrParseSkip (&pBfr, &actLength, 
                                                  USB_DESCR_ENDPOINT)) == NULL)
                {
                USB_BULK_ERR ("usbBulkInterfaceDescShow: No Endpoint descriptor" \
                              "was found \n", 0, 0, 0, 0, 0, 0);
                return (ERROR);
                }

            printf ("ENDPOINT DESCRIPTOR: %d\n", epIndex);
            printf ("-----------------------\n");
            printf (" Length                    = 0x%x\n", pEndptDescr->length);
            printf (" Descriptor Type           = 0x%x\n", 
                    pEndptDescr->descriptorType);
            printf (" Endpoint Address          = 0x%x\n", 
                    pEndptDescr->endpointAddress);
            printf (" Attributes                = 0x%x\n", 
                    pEndptDescr->attributes);
            printf (" Max Packet Size           = 0x%x\n", 
                    pEndptDescr->maxPacketSize);
            printf (" Interval                  = 0x%x\n\n", 
                    pEndptDescr->interval);
            }

        }
  
    return (OK);
    }
