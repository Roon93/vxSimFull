/* usrUsbNC1080EndInit.c - Initialization of the NC1080 End driver */

/* Copyright 1999-2001 Wind River Systems, Inc. */

/*
Modification history
--------------------
01b,10jun01,wef  moved end attach functionality to src/test/usb/usbNC1080Test.c
01a,20feb01,wef	 Created
*/

/*
DESCRIPTION

This configlette initializes the USB NC1080 driver.  This assumes the
USB host stack has already been initialized and has a host controller
driver attached.   

*/


/* includes */

#include "vxWorks.h"
#include "end.h"
#include "pingLib.h"
#include "usb/usbQueueLib.h"
#include "drv/usb/usbNC1080End.h"

/* defines */

#define VX_UNBREAKABLE 0x0002

#define INCLUDE_NETCHIP_TEST 0

/* externals */

/* locals */

LOCAL QUEUE_HANDLE 	callbackQueue;
LOCAL THREAD_HANDLE	callbackHandle;
LOCAL USB_MESSAGE  	msg;

/*****************************************************************************
*
* usbNC1080AttachCallback - 
*
*
*
* RETURNS: Nothing 
*/

VOID usbNC1080AttachCallback
    (
    pVOID arg,			    /* caller-defined argument */
    USB_NC1080_DEV* pDev,	            /* pointer to NETCHIP Device  */
    UINT16 attachCode		    /* attach code */
    )

    {

         usbQueuePut (callbackQueue, 
		      (UINT16)pDev->nodeId, 
		      attachCode, 
		      (UINT32)pDev, 
		      5000); 

    }



/***************************************************************************
*
* netChipClientThread- TODO
*
* This function initiates transmission on the ethernet.
*
* RETURNS: OK or ERROR
*/

void netChipClientThread(void)
    {
    char *    pAddrString;     	/* enet address */
    char *    pNetRoute;  	/* netroute   */	   
    int       unitNum ;		         	/* unit number */
    int       netmask=0xff000000;          	/* netmask */
    static int index;	

    USB_NC1080_DEV* pDev;	

    while (1)
        {
        usbQueueGet (callbackQueue, &msg, OSS_BLOCK);

#if INCLUDE_NETCHIP_TEST		 
	pNetRoute = "90.0.0.3";	
	netmask = 0xff000000;
	unitNum = 0;	
	pAddrString = "90.0.0.53";
#endif
	logMsg ("Brinco Flash-Link adapter....\n", 0, 0, 0, 0, 0, 0);	

        if (msg.wParam == USB_NETCHIP_ATTACH)
	    {

            logMsg ("Wait...Loading Netchip Device!\n", 0, 0, 0, 0, 0, 0); 

	    if (usbNC1080DevLock ((USBD_NODE_ID)msg.msg) != OK)
	        logMsg ("usbNetChipDevLock() returned ERROR\n", 
							0, 0, 0, 0, 0, 0);

	    else
		{

		/* 
		* This is where the stack has become aware of the device 
		* being plugged in.  Here you may call code to further act 
		* on a device.  
                * The sample code called here can be found in:
                * target/src/test/usb/usbNC1080Test.c
		*/

#if INCLUDE_NETCHIP_TEST		 

		 if(loadNetChip(unitNum,pDev) == OK)
                   {

                    taskDelay(sysClkRateGet() * 1);

	            /* Attach IP address*/

		    test(unitNum,pAddrString,netmask);
		    } 	
#endif

		 }
	    }
        else if (msg.wParam == USB_NETCHIP_REMOVE)
	    {

   	    logMsg (" Wait... NetChip Device Unload in progress!\n", 
							0, 0, 0, 0, 0, 0);

	    if (usbNC1080DevUnlock ((USBD_NODE_ID)msg.msg) != OK)
               logMsg ("usbNetChipDevUnlock() returned ERROR!\n",
							0, 0, 0, 0, 0, 0);

	    /* 
	    * This is where the stack has become aware of the device 
	    * being removed in.  Here you may call code to further handle
	    * a device being removed from the system
	    */

#if INCLUDE_NETCHIP_TEST		 

	    if (routeDelete(pNetRoute,pAddrString)!=OK)

	       logMsg (" RouteDeletion failed!  \n", 0, 0, 0, 0, 0, 0);	

	    if (muxDevUnload("netChip",unitNum)!=OK)

	       logMsg ("  muxDevUnload failed!\n", 0, 0, 0, 0, 0, 0);
#endif

  	    logMsg (" NetChip Device Unloaded sucessfully!!!\n",
							0, 0, 0, 0, 0, 0);		
	    }
	}

    }

/*****************************************************************************
*
* usrUsbNC1080EndInit - initialize the USB Netchip 1080 driver
*
* This function initializes the USB Netchip 1080 driver
*
* RETURNS: Nothing 
*/

STATUS usrUsbNC1080EndInit (void) 
    {
    int taskId;

    if (usbNC1080DrvInit () == OK)

        logMsg ("usbNC1080DrvInit () returned OK\n", 0, 0, 0, 0, 0, 0);

    else
	{
        logMsg ("usbNC1080DrvInit () returned ERROR\n", 0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    if (usbQueueCreate (128, &callbackQueue)!=OK)

	logMsg ("NetChip callbackqueue creation error\n ", 0, 0, 0, 0, 0, 0);

    if((taskId = taskSpawn ( "tNetchipClnt", 
			     5, 
			     VX_UNBREAKABLE, 
			     20000, 
			     (FUNCPTR) netChipClientThread, 
			     0, 0, 0, 0, 0, 0, 0, 0, 0, 0 )) 
			 ==ERROR)

	logMsg (" TaskSpawn Error...!\n", 0, 0, 0, 0, 0, 0);

    if (usbNC1080DynamicAttachRegister (usbNC1080AttachCallback, NULL) != OK)
	
	logMsg ("usbNetChipDynamicAttachRegister() returned ERROR\n", 
							0, 0, 0, 0, 0, 0);
	

    
    }

