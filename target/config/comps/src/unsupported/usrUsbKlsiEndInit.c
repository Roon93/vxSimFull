/* usrUsbKlsiEndInit.c - Initialization of the USB END driver */

/* Copyright 1999-2001 Wind River Systems, Inc. */

/*
Modification history
--------------------
01c,10jun01,wef	 moved end attach functionality to src/test/usb/usbKlsiTest.c
01b,21mar01,wef	 removed straggling #endif.
01a,23aug00,wef	 Created
*/

/*
DESCRIPTION

This configlette initializes the USB END klsi driver.  This assumes the
USB host stack has already been initialized and has a host controller
driver attached.   

*/

#include "usb/ossLib.h"
#include "usb/usbQueueLib.h"
#include "drv/usb/usbKlsiEnd.h"



/* defines */

#define VX_UNBREAKABLE 0x0002

#define INCLUDE_KLSI_TEST	0

extern int ipAttach ();

LOCAL QUEUE_HANDLE 	klsiCallbackQueue;
LOCAL USB_MESSAGE  	klsiDeviceStatus;



/***************************************************************************
*
* usbKlsiAttachCallback- TODO
*
* This function initiates transmission on the ethernet.
*
* RETURNS: OK or ERROR
*/

VOID usbKlsiAttachCallback
    (
    pVOID arg,			    /* caller-defined argument */
    USBD_NODE_ID nodeId,            /* pointer to KLSI Device  */
    UINT16 attachCode		    /* attach code */
    )

    {

    usbQueuePut (klsiCallbackQueue, 
	         (UINT16) nodeId,		/* msg */   
		 attachCode,		/* wParam */
		 (UINT32) NULL,		/* lParam */
		 5000); 

    }


/***************************************************************************
*
* klsiClientThread- TODO
*
* This function initiates transmission on the ethernet.
*
* RETURNS: OK or ERROR
*/

void klsiClientThread(void)
    {
    char *    pAddrString;     			/* enet address */
    char *    pNetRoute;  			/* netroute   */
    int       unitNum ;				/* unit number */
    int       netmask=0xffff0000;		/* netmask */
    static int index;	

    USB_KLSI_DEV* pDev;	

    while (1)
        {
        usbQueueGet (klsiCallbackQueue, &klsiDeviceStatus, OSS_BLOCK);

	pDev = (USB_KLSI_DEV *)klsiDeviceStatus.lParam;

#if INCLUDE_KLSI_TEST 

	pNetRoute = "90.0.0.3";
	netmask = 0xffff0000;		 
	unitNum = 0;	
	pAddrString = "90.0.0.53";
#endif

	printf(" NETGEAR adapter found.\n");				


        if (klsiDeviceStatus.wParam == USB_KLSI_ATTACH)
	    {
            printf("Loading Klsi Device...\n"); 

	    if (usbKlsiDevLock ((USBD_NODE_ID)klsiDeviceStatus.msg) != OK)
	        printf ("usbKlsiDevLock() returned ERROR\n");
	    else
		{
#if INCLUDE_KLSI_TEST 

		/* 
		* This is where the stack has become aware of the device 
		* being plugged in.  Here you may call code to further act 
		* on a device.  
		* The sample code called here can be found in:
		* target/src/test/usb/usbKlsiTest.c
		*/

		 if(loadKlsi (unitNum,pDev) == OK)
                   {
                   taskDelay(sysClkRateGet() * 5);

	            /* Attach IP address*/

		    usbKlsiEndStart (unitNum,
				     pAddrString,
				     netmask);
		    } 	
		else
	            printf ("sysUsbKlsiEndLoad() returned ERROR\n");
#endif
		
		}
            printf("Done.\n"); 
	    }
        else /*if (klsiDeviceStatus.wParam == USB_KLSI_REMOVE)*/

	    {
	   		
   	    printf("Klsi Device Unload in progress!\n");

	    if (usbKlsiDevUnlock ((USBD_NODE_ID)klsiDeviceStatus.msg) != OK)

               printf ("usbKlsiDevUnlock() returned ERROR!\n");

	    /* 
	    * This is where the stack has become aware of the device 
	    * being removed in.  Here you may call code to further handle
	    * a device being removed from the system
	    */

#if INCLUDE_KLSI_TEST		 


           if (routeDelete(pNetRoute,pAddrString)!=OK)
               printf (" RouteDeletion failed!  \n");	

            if (muxDevUnload("usb",unitNum)!=OK)
	       printf ("  muxDevUnload failed!\n");
#endif
  	    printf(" Klsi Device unloaded.\n");		
	    }
	}

    }


/*****************************************************************************
*
* usrUsbKlsiEndInit - initialize the USB END Kawasaki driver
*
* This function initializes the USB END Kawasaki driver
*
* RETURNS: Nothing 
*/

void usrUsbKlsiEndInit (void)
    {
    int taskId;


    if (usbKlsiEndInit () == OK)
        printf ("usbKlsiEndInit () returned OK\n");
    else
        printf ("usbKlsiEndInit () returned ERROR\n");

    if (usbQueueCreate (128, &klsiCallbackQueue)!=OK)
	printf("klsiCallbackQueue creation error\n ");

    if((taskId = taskSpawn ("tUsbKlsi", 
			    5, 
			    0, 
			    20000, 
			    (FUNCPTR) klsiClientThread, 
			    0, 0, 0, 0, 0, 0, 0, 0, 0, 0 )) ==ERROR)
	printf(" TaskSpawn Error...!\n");

    if (usbKlsiDynamicAttachRegister (usbKlsiAttachCallback, NULL) != OK)
	{
	printf ("usbKlsiDynamicAttachRegister() returned ERROR\n");
	}

    }
