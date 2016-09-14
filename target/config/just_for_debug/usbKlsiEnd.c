/* usbKlsiEnd.c - USB Ethernet driver for the KLSI USB-Ethernet adapter */

/* Copyright 2000-2001 Wind river systems, Inc */

/*
Modification history
--------------------
01g,15oct01,wef  fix SPR 70953 - fixes man page generation and 70716 - fixes
                 coding convention and some warnings
01f,08aug01,dat  Removing warnings
01e,01aug31,wef  fixed man page generation comments
01d,03may01,wef  moved ATTACH_CALLBACK typedef to this file from .h 
01c,30apr01,wef  changed USB_DEV to USB_KLSI_DEV
01b,11aug00,bri  Dynamic memory allocation for input and output buffers 
		 and removal of wrappers for klsiEndStart and klsiEndStop.
01a,03may00,bri  Created
*/

/*
DESCRIPTION
 
This module is the END (Enhanced Network Driver) driver for USB-ethernet 
adapters built around the Kawasaki-LSI KL5KUSB101 chip for the VxWorks 
operating system. This device falls under subclass Ethernet Networking 
Control Model (Clause 3.8.2) of USB Class Definition for Communication 
Devices (Ver1.1). 

This driver is designed to be moderately generic for all KLSI devices. 
To achieve this, the driver load routine requires an input string consisting 
of some target-specific values. These are described below.

As this adapter deviates from the standard specification in some respects, 
it calls for addition of certain extensions which are required to fulfill 
the requirements of the hot-plugging USB environment. 


EXTERNAL INTERFACE

There are only two external interfaces usbKlsiEndInit() and klsiEndLoad() 
in this driver.  usbKlsiEndInit() is called before the muxLoad() calls 
sysEndLoad(), to register with USBD. sysEndLoad() function calls klsiEndLoad().  
klsiEndLoad() function expects a <initString> parameter as input which 
describes some target specific parameters.  
This parameter is passed in a colon-delimited string of the format:

<initString>
"unit : vendorId : productId : noOfInBfrs : noOfIrps"

The klsiEndLoad() function uses strtok() to parse the string. 

TARGET-SPECIFIC PARAMETERS
\is
 \i <unit>
A convenient holdover from the former model. This parameter is used only 
in the string as name of the driver
 \i <vendorId>
This is a vendorId for the target device supplied by the manufacturer.
 \i <productId>
This is a productId for the target device supplied by the manufacturer.  
 \i <noOfInBfrs>
Tells the driver no of input buffers to be allocated for usage.
 \i <noOfIrps>
Tells the driver no of output IRPs to be allocated for usage.
\ie
 
DEVICE FUNCTIONALITY
 
The KLSI USB to ethernet adapter chip contains an USB serial interface,
ethernet MAC and embedded microcontroller (called the QT Engine).
The chip must have firmware loaded into it before it can operate.
The KLSI Chip supports 4 End Points. The first is the default end point, 
which is of control type. The Second and the Third are BULK IN and BULK OUT 
end points respectively for transferring the data into the Host and from the 
Host. The Fourth End Point is an Interrupt end point that is currently not used.

This device supports one configuration, which contains One Interface. This 
interface contains the 3 end points i.e. the Bulk IN/OUT and interrupt
end points. Issuing a SET_CONFIGURATION command will cause the MAC to be 
reset.
 
Apart from the traditional commands, the device supports as many as 12 
vendor specific commands. These commands are described in the device manual. 
This device even allows the user to change the contents of the EEPROM. 

Packets are passed between the chip and host via bulk transfers.
There is an interrupt endpoint mentioned in the software spec, however
it is currently unused. This device is 10Mbps half-duplex only, hence
there is no media selection logic. The MAC supports a 128 entry multicast 
filter, though the exact size of the filter can depend on the firmware. 
Curiously, while the software specification describes various ethernet 
statistics counters, this adapter and firmware combination claims not 
to support any statistics counters at all.

The device supports the following (vendor specific )commands :
\is 
\i USB_REQ_KLSI_ETHDESC_GET Retrieves the Ethernet functional descriptor from 
the device.
\i USB_REQ_KLSI_SET_MCAST_FILTER Sets the ethernet device multicast filters as 
specified in the sequential list if 48 bit addresses ethernet multicast
addresses.
\i USB_REQ_KLSI_SET_PACKET_FILTER This Sets the Ethernet packet filter settings.
\i USB_REQ_KLSI_GET_STATS Retrieves the device statistics of the feature
requested.
\i USB_REQ_KLSI_GET_AUX_INPUTS Reads four auxiliary input pins from the 
USB-Ethernet controller chip.
\i USB_REQ_KLSI_SET_AUX_OUTPUTS Sets four auxiliary input pins from the 
USB-Ethernet controller chip.
\i USB_REQ_KLSI_SET_TEMP_MAC Obtains the MAC address currently used by the 
ethernet adapter.
\i USB_REQ_KLSI_GET_TEMP_MAC Sets the MAC address to be used by the ethernet 
adapter.
\i USB_REQ_KLSI_SET_URB_SIZE Sets the USB Request Block size to be used by the 
ethernet adapter.
\i USB_REQ_KLSI_SET_SOFS_TO_WAIT Sets the no. of Start of Frames to wait while 
filling a URB before sending a ZLP.
\i USB_REQ_KLSI_SET_EVEN_PACKETS Specific to Win95. Not Applicable.
\i USB_REQ_KLSI_SCAN Request to modify the I2C EEPROM on the device at a 
specified address.
\ie




DRIVER FUNCTIONALITY
 
The function usbKlsiEndInit() is called at the time of usb system 
initialization. It registers as a client with the USBD. This function also
registers for the dynamic attachment and removal of the USB devices.
Ideally the registering should be done for a specific Class ID and a Subclass
ID. Since the device doesn't support these parameters in the Device descriptor, 
ALL kinds of devices are registered for. A list of the ethernet devices on USB 
is maintained in a linked list "klsiDevList". This list is created and 
maintained using the linked list library provided as a part of the USBD. 
API calls are provided to find if the device exists in the list, by taking 
either the device "node ID" or the vendorID and productID as the parameters. 
klsiAttachCallback(), which is the Callback function registered for the 
dynamic attachment/removal, will be called if any device is found on the 
USB or removed from the USB. This function checks if this is duplicate 
information, by checking if this device already exists in the List. If not, 
the device descriptor is parsed to obtain the Vendor ID and Product ID. If the
Vendor ID and Product ID match with KLSI IDs, then the device is added to the 
list of ethernet devices found on the USB.

klsiDevInit() does most of the device structure initialization. This routine 
checks if the device corresponding to the VendorID and ProductID match to any 
of the devices in the "List". If matched, a pointer structure on the list will 
be assigned to one of the device structure parameters. Next, InPut and OutPut 
end point details are found by parsing through the configuration descriptor and 
interface descriptor. Once these end point descriptors are found, respective 
input and output Pipes are created and assigned to the corresponding structure. 
At this moment device is triggered to reset.

This driver is a Polled mode driver. It keeps listening on the input pipe by 
calling "klsiListenToInput" all the time, from the first time it is called 
by klsiEndStart(). Pre allocated buffer IRP is submitted. Unless the IRP is 
cancelled (by klsiEndStop()), it will be submitted again and again.
If cancelled, it will again start listening only if klsiEndStart() is called.
If there is data (IRP successful), then it will be passed on to upper layer by
calling klsiEndRecv().

Rest of the functionality of the driver is straight forward and most of
the places is achieved by sending a vendor specific command from the list
described above, to the device.

INCLUDE FILES:
end.h endLib.h lstLib.h etherMultiLib.h usbPlatform.h usb.h usbListLib.h 
usbdLib.h usbLib.h usbKlsiEnd.h

SEE ALSO:
muxLib, endLib,  usbLib, usbdLib, ossLib 

.I "Writing and Enhanced Network Driver" and
.I "USB Developer's Kit User's Guide"

*/

/* includes */

#include "vxWorks.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "cacheLib.h"
#include "intLib.h"
#include "end.h"		/* Common END structures. */
#include "endLib.h"
#include "lstLib.h"		/* Needed to maintain protocol list */
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
#include "usb/ossLib.h" 	/* operations system services */
#include "usb/usb.h"		/* general USB definitions */
#include "usb/usbListLib.h"	/* linked list functions */
#include "usb/usbdLib.h"	/* USBD interface */
#include "usb/usbLib.h" 	/* USB utility functions */

#include "drv/usb/usbKlsiEnd.h"

/* defines */

/* for debugging */

#define KLSI_DBG

#ifdef	KLSI_DBG

#define KLSI_DBG_OFF		0x0000
#define KLSI_DBG_RX		0x0001
#define	KLSI_DBG_TX		0x0002
#define KLSI_DBG_MCAST		0x0004
#define	KLSI_DBG_ATTACH		0x0008
#define	KLSI_DBG_INIT		0x0010
#define	KLSI_DBG_START		0x0020
#define	KLSI_DBG_STOP		0x0040
#define	KLSI_DBG_RESET		0x0080
#define	KLSI_DBG_MAC		0x0100
#define	KLSI_DBG_POLL_RX	0x0200
#define	KLSI_DBG_POLL_TX	0x0400
#define	KLSI_DBG_LOAD		0x0800
#define	KLSI_DBG_IOCTL		0x1000
#define KLSI_DBG_DNLD		0x2000

int	klsiDebug = (0x0000);	

#define KLSI_LOG(FLG, X0, X1, X2, X3, X4, X5, X6)    \
	if (klsiDebug & FLG)                         \
            logMsg(X0, X1, X2, X3, X4, X5, X6);

#define KLSI_PRINT(FLG,X)                            \
	if (klsiDebug & FLG) printf X;

#else /*KLSI_DBG*/

#define KLSI_LOG(FLG, X0, X1, X2, X3, X4, X5, X6)

#define KLSI_PRINT(DBG_SW,X)

#endif /*KLSI_DBG*/


#if (CPU == PPC604)
 #undef CACHE_PIPE_FLUSH()
 #define CACHE_PIPE_FLUSH() vxEieio()
#endif

#define KLSI_FIRMWARE_BUF	4096

#define KLSI_CLIENT_NAME	"usb"

#define KLSI_BUFSIZ  		(ETHERMTU + ENET_HDR_REAL_SIZ + 6) 

#define EH_SIZE			(14)

#define END_SPEED_10M		10000000	/* 10Mbs */

#define KLSI_SPEED   		END_SPEED_10M

#define KLSI_NAME		"usb"

#define KLSI_NAME_LEN		sizeof(KLSI_NAME)+1


/* A shortcut for getting the hardware address from the MIB II stuff. */

#define END_HADDR(pEnd)	\
		((pEnd)->mib2Tbl.ifPhysAddress.phyAddress)

#define END_HADDR_LEN(pEnd) \
		((pEnd)->mib2Tbl.ifPhysAddress.addrLength)


#define KLSI_MIN_FBUF	(1514)	/* min first buffer size */


/* typedefs */

/*
 * This will only work if there is only a single unit, for multiple
 * unit device drivers these should be integrated into the KLSI_DEVICE
 * structure.
 */

M_CL_CONFIG klsiMclBlkConfig = 	/* network mbuf configuration table */
    {
    /* 
    no. mBlks		no. clBlks	memArea		memSize
    -----------		----------	-------		-------
    */
    0, 			0, 		NULL, 		0
    };

CL_DESC klsiClDescTbl [] = 	/* network cluster pool configuration table */
    {
    /* 
    clusterSize			num	memArea		memSize
    -----------			----	-------		-------
    */
    {ETHERMTU + EH_SIZE + 2,	0,	NULL,		0}
    }; 

int klsiClDescTblNumEnt = (NELEMENTS(klsiClDescTbl));



typedef struct attach_request
    {
    LINK reqLink;                       /* linked list of requests */
    USB_KLSI_ATTACH_CALLBACK callback;  /* client callback routine */
    pVOID callbackArg;                  /* client callback argument*/
    } ATTACH_REQUEST, *pATTACH_REQUEST;


/* globals */


USBD_CLIENT_HANDLE klsiHandle; 		/* our USBD client handle */

/* Locals */

/* Firmware Download details */

/*
 * NOTE: B6/C3 is data header signature   
 *       0xAA/0xBB is data length = total 
 *       bytes - 7, 0xCC is type, 0xDD is 
 *       interrupt to use.                
 */

/*
 *     klsiNewCode
 */
static UINT8 klsiNewCode[] = 
{
    0xB6, 0xC3, 0xAA, 0xBB, 0xCC, 0xDD,
    0x9f, 0xcf, 0xbc, 0x08, 0xe7, 0x57, 0x00, 0x00,
    0x9a, 0x08, 0x97, 0xc1, 0xe7, 0x67, 0xff, 0x1f,
    0x28, 0xc0, 0xe7, 0x87, 0x00, 0x04, 0x24, 0xc0,
    0xe7, 0x67, 0xff, 0xf9, 0x22, 0xc0, 0x97, 0xcf,
    0xe7, 0x09, 0xa2, 0xc0, 0x94, 0x08, 0xd7, 0x09,
    0x00, 0xc0, 0xe7, 0x59, 0xba, 0x08, 0x94, 0x08,
    0x03, 0xc1, 0xe7, 0x67, 0xff, 0xf7, 0x24, 0xc0,
    0xe7, 0x05, 0x00, 0xc0, 0xa7, 0xcf, 0x92, 0x08,
    0xe7, 0x57, 0x00, 0x00, 0x8e, 0x08, 0xa7, 0xa1,
    0x8e, 0x08, 0x97, 0xcf, 0xe7, 0x57, 0x00, 0x00,
    0xf2, 0x09, 0x0a, 0xc0, 0xe7, 0x57, 0x00, 0x00,
    0xa4, 0xc0, 0xa7, 0xc0, 0x56, 0x08, 0x9f, 0xaf,
    0x70, 0x09, 0xe7, 0x07, 0x00, 0x00, 0xf2, 0x09,
    0xe7, 0x57, 0xff, 0xff, 0x90, 0x08, 0x9f, 0xa0,
    0x40, 0x00, 0xe7, 0x59, 0x90, 0x08, 0x94, 0x08,
    0x9f, 0xa0, 0x40, 0x00, 0xc8, 0x09, 0xa2, 0x08,
    0x08, 0x62, 0x9f, 0xa1, 0x14, 0x0a, 0xe7, 0x57,
    0x00, 0x00, 0x52, 0x08, 0xa7, 0xc0, 0x56, 0x08,
    0x9f, 0xaf, 0x04, 0x00, 0xe7, 0x57, 0x00, 0x00,
    0x8e, 0x08, 0xa7, 0xc1, 0x56, 0x08, 0xc0, 0x09,
    0xa8, 0x08, 0x00, 0x60, 0x05, 0xc4, 0xc0, 0x59,
    0x94, 0x08, 0x02, 0xc0, 0x9f, 0xaf, 0xee, 0x00,
    0xe7, 0x59, 0xae, 0x08, 0x94, 0x08, 0x02, 0xc1,
    0x9f, 0xaf, 0xf6, 0x00, 0x9f, 0xaf, 0x9e, 0x03,
    0xef, 0x57, 0x00, 0x00, 0xf0, 0x09, 0x9f, 0xa1,
    0xde, 0x01, 0xe7, 0x57, 0x00, 0x00, 0x78, 0x08,
    0x9f, 0xa0, 0xe4, 0x03, 0x9f, 0xaf, 0x2c, 0x04,
    0xa7, 0xcf, 0x56, 0x08, 0x48, 0x02, 0xe7, 0x09,
    0x94, 0x08, 0xa8, 0x08, 0xc8, 0x37, 0x04, 0x00,
    0x9f, 0xaf, 0x68, 0x04, 0x97, 0xcf, 0xe7, 0x57,
    0x00, 0x00, 0xa6, 0x08, 0x97, 0xc0, 0xd7, 0x09,
    0x00, 0xc0, 0xc1, 0xdf, 0xc8, 0x09, 0x9c, 0x08,
    0x08, 0x62, 0x1d, 0xc0, 0x27, 0x04, 0x9c, 0x08,
    0x10, 0x94, 0xf0, 0x07, 0xee, 0x09, 0x02, 0x00,
    0xc1, 0x07, 0x01, 0x00, 0x70, 0x00, 0x04, 0x00,
    0xf0, 0x07, 0x44, 0x01, 0x06, 0x00, 0x50, 0xaf,
    0xe7, 0x09, 0x94, 0x08, 0xae, 0x08, 0xe7, 0x17,
    0x14, 0x00, 0xae, 0x08, 0xe7, 0x67, 0xff, 0x07,
    0xae, 0x08, 0xe7, 0x07, 0xff, 0xff, 0xa8, 0x08,
    0xe7, 0x07, 0x00, 0x00, 0xa6, 0x08, 0xe7, 0x05,
    0x00, 0xc0, 0x97, 0xcf, 0xd7, 0x09, 0x00, 0xc0,
    0xc1, 0xdf, 0x48, 0x02, 0xd0, 0x09, 0x9c, 0x08,
    0x27, 0x02, 0x9c, 0x08, 0xe7, 0x09, 0x20, 0xc0,
    0xee, 0x09, 0xe7, 0xd0, 0xee, 0x09, 0xe7, 0x05,
    0x00, 0xc0, 0x97, 0xcf, 0x48, 0x02, 0xc8, 0x37,
    0x04, 0x00, 0x00, 0x0c, 0x0c, 0x00, 0x00, 0x60,
    0x21, 0xc0, 0xc0, 0x37, 0x3e, 0x00, 0x23, 0xc9,
    0xc0, 0x57, 0xb4, 0x05, 0x1b, 0xc8, 0xc0, 0x17,
    0x3f, 0x00, 0xc0, 0x67, 0xc0, 0xff, 0x30, 0x00,
    0x08, 0x00, 0xf0, 0x07, 0x00, 0x00, 0x04, 0x00,
    0x00, 0x02, 0xc0, 0x17, 0x4c, 0x00, 0x30, 0x00,
    0x06, 0x00, 0xf0, 0x07, 0xbe, 0x01, 0x0a, 0x00,
    0x48, 0x02, 0xc1, 0x07, 0x02, 0x00, 0xd7, 0x09,
    0x00, 0xc0, 0xc1, 0xdf, 0x51, 0xaf, 0xe7, 0x05,
    0x00, 0xc0, 0x97, 0xcf, 0x9f, 0xaf, 0x68, 0x04,
    0x9f, 0xaf, 0xe4, 0x03, 0x97, 0xcf, 0x9f, 0xaf,
    0xe4, 0x03, 0xc9, 0x37, 0x04, 0x00, 0xc1, 0xdf,
    0xc8, 0x09, 0x70, 0x08, 0x50, 0x02, 0x67, 0x02,
    0x70, 0x08, 0xd1, 0x07, 0x00, 0x00, 0xc0, 0xdf,
    0x9f, 0xaf, 0xde, 0x01, 0x97, 0xcf, 0xe7, 0x57,
    0x00, 0x00, 0xaa, 0x08, 0x97, 0xc1, 0xe7, 0x57,
    0x01, 0x00, 0x7a, 0x08, 0x97, 0xc0, 0xc8, 0x09,
    0x6e, 0x08, 0x08, 0x62, 0x97, 0xc0, 0x00, 0x02,
    0xc0, 0x17, 0x0e, 0x00, 0x27, 0x00, 0x34, 0x01,
    0x27, 0x0c, 0x0c, 0x00, 0x36, 0x01, 0xef, 0x57,
    0x00, 0x00, 0xf0, 0x09, 0x9f, 0xc0, 0xbe, 0x02,
    0xe7, 0x57, 0x00, 0x00, 0xb0, 0x08, 0x97, 0xc1,
    0xe7, 0x07, 0x09, 0x00, 0x12, 0xc0, 0xe7, 0x77,
    0x00, 0x08, 0x20, 0xc0, 0x9f, 0xc1, 0xb6, 0x02,
    0xe7, 0x57, 0x09, 0x00, 0x12, 0xc0, 0x77, 0xc9,
    0xd7, 0x09, 0x00, 0xc0, 0xc1, 0xdf, 0xe7, 0x77,
    0x00, 0x08, 0x20, 0xc0, 0x2f, 0xc1, 0xe7, 0x07,
    0x00, 0x00, 0x42, 0xc0, 0xe7, 0x07, 0x05, 0x00,
    0x90, 0xc0, 0xc8, 0x07, 0x0a, 0x00, 0xe7, 0x77,
    0x04, 0x00, 0x20, 0xc0, 0x09, 0xc1, 0x08, 0xda,
    0x7a, 0xc1, 0xe7, 0x07, 0x00, 0x01, 0x42, 0xc0,
    0xe7, 0x07, 0x04, 0x00, 0x90, 0xc0, 0x1a, 0xcf,
    0xe7, 0x07, 0x01, 0x00, 0x7a, 0x08, 0x00, 0xd8,
    0x27, 0x50, 0x34, 0x01, 0x17, 0xc1, 0xe7, 0x77,
    0x02, 0x00, 0x20, 0xc0, 0x79, 0xc1, 0x27, 0x50,
    0x34, 0x01, 0x10, 0xc1, 0xe7, 0x77, 0x02, 0x00,
    0x20, 0xc0, 0x79, 0xc0, 0x9f, 0xaf, 0xd8, 0x02,
    0xe7, 0x05, 0x00, 0xc0, 0x00, 0x60, 0x9f, 0xc0,
    0xde, 0x01, 0x97, 0xcf, 0xe7, 0x07, 0x01, 0x00,
    0xb8, 0x08, 0x06, 0xcf, 0xe7, 0x07, 0x30, 0x0e,
    0x02, 0x00, 0xe7, 0x07, 0x50, 0xc3, 0x12, 0xc0,
    0xe7, 0x05, 0x00, 0xc0, 0x97, 0xcf, 0xe7, 0x07,
    0x01, 0x00, 0xb8, 0x08, 0x97, 0xcf, 0xe7, 0x07,
    0x50, 0xc3, 0x12, 0xc0, 0xe7, 0x07, 0x30, 0x0e,
    0x02, 0x00, 0xe7, 0x07, 0x01, 0x00, 0x7a, 0x08,
    0xe7, 0x07, 0x05, 0x00, 0x90, 0xc0, 0x97, 0xcf,
    0xe7, 0x07, 0x00, 0x01, 0x42, 0xc0, 0xe7, 0x07,
    0x04, 0x00, 0x90, 0xc0, 0xe7, 0x07, 0x00, 0x00,
    0x7a, 0x08, 0xe7, 0x57, 0x0f, 0x00, 0xb2, 0x08,
    0x13, 0xc1, 0x9f, 0xaf, 0x2e, 0x08, 0xca, 0x09,
    0xac, 0x08, 0xf2, 0x17, 0x01, 0x00, 0x5c, 0x00,
    0xf2, 0x27, 0x00, 0x00, 0x5e, 0x00, 0xe7, 0x07,
    0x00, 0x00, 0xb2, 0x08, 0xe7, 0x07, 0x01, 0x00,
    0xb4, 0x08, 0xc0, 0x07, 0xff, 0xff, 0x97, 0xcf,
    0x9f, 0xaf, 0x4c, 0x03, 0xc0, 0x69, 0xb4, 0x08,
    0x57, 0x00, 0x9f, 0xde, 0x33, 0x00, 0xc1, 0x05,
    0x27, 0xd8, 0xb2, 0x08, 0x27, 0xd2, 0xb4, 0x08,
    0xe7, 0x87, 0x01, 0x00, 0xb4, 0x08, 0xe7, 0x67,
    0xff, 0x03, 0xb4, 0x08, 0x00, 0x60, 0x97, 0xc0,
    0xe7, 0x07, 0x01, 0x00, 0xb0, 0x08, 0x27, 0x00,
    0x12, 0xc0, 0x97, 0xcf, 0xc0, 0x09, 0xb6, 0x08,
    0x00, 0xd2, 0x02, 0xc3, 0xc0, 0x97, 0x05, 0x80,
    0x27, 0x00, 0xb6, 0x08, 0xc0, 0x99, 0x82, 0x08,
    0xc0, 0x99, 0xa2, 0xc0, 0x97, 0xcf, 0xe7, 0x07,
    0x00, 0x00, 0xb0, 0x08, 0xc0, 0xdf, 0x97, 0xcf,
    0xc8, 0x09, 0x72, 0x08, 0x08, 0x62, 0x02, 0xc0,
    0x10, 0x64, 0x07, 0xc1, 0xe7, 0x07, 0x00, 0x00,
    0x64, 0x08, 0xe7, 0x07, 0xc8, 0x05, 0x24, 0x00,
    0x97, 0xcf, 0x27, 0x04, 0x72, 0x08, 0xc8, 0x17,
    0x0e, 0x00, 0x27, 0x02, 0x64, 0x08, 0xe7, 0x07,
    0xd6, 0x05, 0x24, 0x00, 0x97, 0xcf, 0xd7, 0x09,
    0x00, 0xc0, 0xc1, 0xdf, 0xe7, 0x57, 0x00, 0x00,
    0x62, 0x08, 0x13, 0xc1, 0x9f, 0xaf, 0x70, 0x03,
    0xe7, 0x57, 0x00, 0x00, 0x64, 0x08, 0x13, 0xc0,
    0xe7, 0x09, 0x64, 0x08, 0x30, 0x01, 0xe7, 0x07,
    0xf2, 0x05, 0x32, 0x01, 0xe7, 0x07, 0x10, 0x00,
    0x96, 0xc0, 0xe7, 0x09, 0x64, 0x08, 0x62, 0x08,
    0x04, 0xcf, 0xe7, 0x57, 0x00, 0x00, 0x64, 0x08,
    0x02, 0xc1, 0x9f, 0xaf, 0x70, 0x03, 0xe7, 0x05,
    0x00, 0xc0, 0x97, 0xcf, 0xd7, 0x09, 0x00, 0xc0,
    0xc1, 0xdf, 0xc8, 0x09, 0x72, 0x08, 0x27, 0x02,
    0x78, 0x08, 0x08, 0x62, 0x03, 0xc1, 0xe7, 0x05,
    0x00, 0xc0, 0x97, 0xcf, 0x27, 0x04, 0x72, 0x08,
    0xe7, 0x05, 0x00, 0xc0, 0xf0, 0x07, 0x40, 0x00,
    0x08, 0x00, 0xf0, 0x07, 0x00, 0x00, 0x04, 0x00,
    0x00, 0x02, 0xc0, 0x17, 0x0c, 0x00, 0x30, 0x00,
    0x06, 0x00, 0xf0, 0x07, 0x64, 0x01, 0x0a, 0x00,
    0xc8, 0x17, 0x04, 0x00, 0xc1, 0x07, 0x02, 0x00,
    0x51, 0xaf, 0x97, 0xcf, 0xe7, 0x57, 0x00, 0x00,
    0x6a, 0x08, 0x97, 0xc0, 0xc1, 0xdf, 0xc8, 0x09,
    0x6a, 0x08, 0x27, 0x04, 0x6a, 0x08, 0x27, 0x52,
    0x6c, 0x08, 0x03, 0xc1, 0xe7, 0x07, 0x6a, 0x08,
    0x6c, 0x08, 0xc0, 0xdf, 0x17, 0x02, 0xc8, 0x17,
    0x0e, 0x00, 0x9f, 0xaf, 0x16, 0x05, 0xc8, 0x05,
    0x00, 0x60, 0x03, 0xc0, 0x9f, 0xaf, 0x80, 0x04,
    0x97, 0xcf, 0x9f, 0xaf, 0x68, 0x04, 0x97, 0xcf,
    0xd7, 0x09, 0x00, 0xc0, 0xc1, 0xdf, 0x08, 0x62,
    0x1c, 0xc0, 0xd0, 0x09, 0x72, 0x08, 0x27, 0x02,
    0x72, 0x08, 0xe7, 0x05, 0x00, 0xc0, 0x97, 0xcf,
    0x97, 0x02, 0xca, 0x09, 0xac, 0x08, 0xf2, 0x17,
    0x01, 0x00, 0x04, 0x00, 0xf2, 0x27, 0x00, 0x00,
    0x06, 0x00, 0xca, 0x17, 0x2c, 0x00, 0xf8, 0x77,
    0x01, 0x00, 0x0e, 0x00, 0x06, 0xc0, 0xca, 0xd9,
    0xf8, 0x57, 0xff, 0x00, 0x0e, 0x00, 0x01, 0xc1,
    0xca, 0xd9, 0x22, 0x1c, 0x0c, 0x00, 0xe2, 0x27,
    0x00, 0x00, 0xe2, 0x17, 0x01, 0x00, 0xe2, 0x27,
    0x00, 0x00, 0xca, 0x05, 0x00, 0x0c, 0x0c, 0x00,
    0xc0, 0x17, 0x41, 0x00, 0xc0, 0x67, 0xc0, 0xff,
    0x30, 0x00, 0x08, 0x00, 0x00, 0x02, 0xc0, 0x17,
    0x0c, 0x00, 0x30, 0x00, 0x06, 0x00, 0xf0, 0x07,
    0xdc, 0x00, 0x0a, 0x00, 0xf0, 0x07, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x0c, 0x08, 0x00, 0x40, 0xd1,
    0x01, 0x00, 0xc0, 0x19, 0xa6, 0x08, 0xc0, 0x59,
    0x98, 0x08, 0x04, 0xc9, 0x49, 0xaf, 0x9f, 0xaf,
    0xee, 0x00, 0x4a, 0xaf, 0x67, 0x10, 0xa6, 0x08,
    0xc8, 0x17, 0x04, 0x00, 0xc1, 0x07, 0x01, 0x00,
    0xd7, 0x09, 0x00, 0xc0, 0xc1, 0xdf, 0x50, 0xaf,
    0xe7, 0x05, 0x00, 0xc0, 0x97, 0xcf, 0xc0, 0x07,
    0x01, 0x00, 0xc1, 0x09, 0x7c, 0x08, 0xc1, 0x77,
    0x01, 0x00, 0x97, 0xc1, 0xd8, 0x77, 0x01, 0x00,
    0x12, 0xc0, 0xc9, 0x07, 0x4c, 0x08, 0x9f, 0xaf,
    0x64, 0x05, 0x04, 0xc1, 0xc1, 0x77, 0x08, 0x00,
    0x13, 0xc0, 0x97, 0xcf, 0xc1, 0x77, 0x02, 0x00,
    0x97, 0xc1, 0xc1, 0x77, 0x10, 0x00, 0x0c, 0xc0,
    0x9f, 0xaf, 0x86, 0x05, 0x97, 0xcf, 0xc1, 0x77,
    0x04, 0x00, 0x06, 0xc0, 0xc9, 0x07, 0x7e, 0x08,
    0x9f, 0xaf, 0x64, 0x05, 0x97, 0xc0, 0x00, 0xcf,
    0x00, 0x90, 0x97, 0xcf, 0x50, 0x54, 0x97, 0xc1,
    0x70, 0x5c, 0x02, 0x00, 0x02, 0x00, 0x97, 0xc1,
    0x70, 0x5c, 0x04, 0x00, 0x04, 0x00, 0x97, 0xcf,
    0xc0, 0x00, 0x60, 0x00, 0x30, 0x00, 0x18, 0x00,
    0x0c, 0x00, 0x06, 0x00, 0x00, 0x00, 0xcb, 0x09,
    0x88, 0x08, 0xcc, 0x09, 0x8a, 0x08, 0x0b, 0x53,
    0x11, 0xc0, 0xc9, 0x02, 0xca, 0x07, 0x78, 0x05,
    0x9f, 0xaf, 0x64, 0x05, 0x97, 0xc0, 0x0a, 0xc8,
    0x82, 0x08, 0x0a, 0xcf, 0x82, 0x08, 0x9f, 0xaf,
    0x64, 0x05, 0x97, 0xc0, 0x05, 0xc2, 0x89, 0x30,
    0x82, 0x60, 0x78, 0xc1, 0x00, 0x90, 0x97, 0xcf,
    0x89, 0x10, 0x09, 0x53, 0x79, 0xc2, 0x89, 0x30,
    0x82, 0x08, 0x7a, 0xcf, 0xc0, 0xdf, 0x97, 0xcf,
    0xe7, 0x09, 0x96, 0xc0, 0x66, 0x08, 0xe7, 0x09,
    0x98, 0xc0, 0x68, 0x08, 0x0f, 0xcf, 0xe7, 0x09,
    0x96, 0xc0, 0x66, 0x08, 0xe7, 0x09, 0x98, 0xc0,
    0x68, 0x08, 0xe7, 0x09, 0x64, 0x08, 0x30, 0x01,
    0xe7, 0x07, 0xf2, 0x05, 0x32, 0x01, 0xe7, 0x07,
    0x10, 0x00, 0x96, 0xc0, 0xd7, 0x09, 0x00, 0xc0,
    0x17, 0x02, 0xc8, 0x09, 0x62, 0x08, 0xc8, 0x37,
    0x0e, 0x00, 0xe7, 0x57, 0x04, 0x00, 0x68, 0x08,
    0x3d, 0xc0, 0xe7, 0x87, 0x00, 0x08, 0x24, 0xc0,
    0xe7, 0x09, 0x94, 0x08, 0xba, 0x08, 0xe7, 0x17,
    0x64, 0x00, 0xba, 0x08, 0xe7, 0x67, 0xff, 0x07,
    0xba, 0x08, 0xe7, 0x77, 0x2a, 0x00, 0x66, 0x08,
    0x30, 0xc0, 0x97, 0x02, 0xca, 0x09, 0xac, 0x08,
    0xe7, 0x77, 0x20, 0x00, 0x66, 0x08, 0x0e, 0xc0,
    0xf2, 0x17, 0x01, 0x00, 0x10, 0x00, 0xf2, 0x27,
    0x00, 0x00, 0x12, 0x00, 0xe7, 0x77, 0x0a, 0x00,
    0x66, 0x08, 0xca, 0x05, 0x1e, 0xc0, 0x97, 0x02,
    0xca, 0x09, 0xac, 0x08, 0xf2, 0x17, 0x01, 0x00,
    0x0c, 0x00, 0xf2, 0x27, 0x00, 0x00, 0x0e, 0x00,
    0xe7, 0x77, 0x02, 0x00, 0x66, 0x08, 0x07, 0xc0,
    0xf2, 0x17, 0x01, 0x00, 0x44, 0x00, 0xf2, 0x27,
    0x00, 0x00, 0x46, 0x00, 0x06, 0xcf, 0xf2, 0x17,
    0x01, 0x00, 0x60, 0x00, 0xf2, 0x27, 0x00, 0x00,
    0x62, 0x00, 0xca, 0x05, 0x9f, 0xaf, 0x68, 0x04,
    0x0f, 0xcf, 0x57, 0x02, 0x09, 0x02, 0xf1, 0x09,
    0x68, 0x08, 0x0c, 0x00, 0xf1, 0xda, 0x0c, 0x00,
    0xc8, 0x09, 0x6c, 0x08, 0x50, 0x02, 0x67, 0x02,
    0x6c, 0x08, 0xd1, 0x07, 0x00, 0x00, 0xc9, 0x05,
    0xe7, 0x09, 0x64, 0x08, 0x62, 0x08, 0xe7, 0x57,
    0x00, 0x00, 0x62, 0x08, 0x02, 0xc0, 0x9f, 0xaf,
    0x70, 0x03, 0xc8, 0x05, 0xe7, 0x05, 0x00, 0xc0,
    0xc0, 0xdf, 0x97, 0xcf, 0xd7, 0x09, 0x00, 0xc0,
    0x17, 0x00, 0x17, 0x02, 0x97, 0x02, 0xc0, 0x09,
    0x92, 0xc0, 0xe7, 0x87, 0x00, 0x08, 0x24, 0xc0,
    0xe7, 0x09, 0x94, 0x08, 0xba, 0x08, 0xe7, 0x17,
    0x64, 0x00, 0xba, 0x08, 0xe7, 0x67, 0xff, 0x07,
    0xba, 0x08, 0xe7, 0x07, 0x04, 0x00, 0x90, 0xc0,
    0xca, 0x09, 0xac, 0x08, 0xe7, 0x07, 0x00, 0x00,
    0x7a, 0x08, 0xe7, 0x07, 0x66, 0x03, 0x02, 0x00,
    0xc0, 0x77, 0x02, 0x00, 0x10, 0xc0, 0xef, 0x57,
    0x00, 0x00, 0xf0, 0x09, 0x04, 0xc0, 0x9f, 0xaf,
    0xd8, 0x02, 0x9f, 0xcf, 0x12, 0x08, 0xf2, 0x17,
    0x01, 0x00, 0x50, 0x00, 0xf2, 0x27, 0x00, 0x00,
    0x52, 0x00, 0x9f, 0xcf, 0x12, 0x08, 0xef, 0x57,
    0x00, 0x00, 0xf0, 0x09, 0x08, 0xc0, 0xe7, 0x57,
    0x00, 0x00, 0xb8, 0x08, 0xe7, 0x07, 0x00, 0x00,
    0xb8, 0x08, 0x0a, 0xc0, 0x03, 0xcf, 0xc0, 0x77,
    0x10, 0x00, 0x06, 0xc0, 0xf2, 0x17, 0x01, 0x00,
    0x58, 0x00, 0xf2, 0x27, 0x00, 0x00, 0x5a, 0x00,
    0xc0, 0x77, 0x80, 0x00, 0x06, 0xc0, 0xf2, 0x17,
    0x01, 0x00, 0x70, 0x00, 0xf2, 0x27, 0x00, 0x00,
    0x72, 0x00, 0xc0, 0x77, 0x08, 0x00, 0x1d, 0xc1,
    0xf2, 0x17, 0x01, 0x00, 0x08, 0x00, 0xf2, 0x27,
    0x00, 0x00, 0x0a, 0x00, 0xc0, 0x77, 0x00, 0x02,
    0x06, 0xc0, 0xf2, 0x17, 0x01, 0x00, 0x64, 0x00,
    0xf2, 0x27, 0x00, 0x00, 0x66, 0x00, 0xc0, 0x77,
    0x40, 0x00, 0x06, 0xc0, 0xf2, 0x17, 0x01, 0x00,
    0x5c, 0x00, 0xf2, 0x27, 0x00, 0x00, 0x5e, 0x00,
    0xc0, 0x77, 0x01, 0x00, 0x01, 0xc0, 0x37, 0xcf,
    0x36, 0xcf, 0xf2, 0x17, 0x01, 0x00, 0x00, 0x00,
    0xf2, 0x27, 0x00, 0x00, 0x02, 0x00, 0xef, 0x57,
    0x00, 0x00, 0xf0, 0x09, 0x18, 0xc0, 0xe7, 0x57,
    0x01, 0x00, 0xb2, 0x08, 0x0e, 0xc2, 0x07, 0xc8,
    0xf2, 0x17, 0x01, 0x00, 0x50, 0x00, 0xf2, 0x27,
    0x00, 0x00, 0x52, 0x00, 0x06, 0xcf, 0xf2, 0x17,
    0x01, 0x00, 0x54, 0x00, 0xf2, 0x27, 0x00, 0x00,
    0x56, 0x00, 0xe7, 0x07, 0x00, 0x00, 0xb2, 0x08,
    0xe7, 0x07, 0x01, 0x00, 0xb4, 0x08, 0xc8, 0x09,
    0x34, 0x01, 0xca, 0x17, 0x14, 0x00, 0xd8, 0x77,
    0x01, 0x00, 0x05, 0xc0, 0xca, 0xd9, 0xd8, 0x57,
    0xff, 0x00, 0x01, 0xc0, 0xca, 0xd9, 0xe2, 0x19,
    0x94, 0xc0, 0xe2, 0x27, 0x00, 0x00, 0xe2, 0x17,
    0x01, 0x00, 0xe2, 0x27, 0x00, 0x00, 0x9f, 0xaf,
    0x2e, 0x08, 0x9f, 0xaf, 0xde, 0x01, 0xe7, 0x57,
    0x00, 0x00, 0xaa, 0x08, 0x9f, 0xa1, 0xf0, 0x0b,
    0xca, 0x05, 0xc8, 0x05, 0xc0, 0x05, 0xe7, 0x05,
    0x00, 0xc0, 0xc0, 0xdf, 0x97, 0xcf, 0xc8, 0x09,
    0x6e, 0x08, 0x08, 0x62, 0x97, 0xc0, 0x27, 0x04,
    0x6e, 0x08, 0x27, 0x52, 0x70, 0x08, 0x03, 0xc1,
    0xe7, 0x07, 0x6e, 0x08, 0x70, 0x08, 0x9f, 0xaf,
    0x68, 0x04, 0x97, 0xcf, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x33, 0xcc,
    0x00, 0x00, 0x00, 0x00, 0xe7, 0x57, 0x00, 0x80,
    0xb2, 0x00, 0x06, 0xc2, 0xe7, 0x07, 0x52, 0x0e,
    0x12, 0x00, 0xe7, 0x07, 0x98, 0x0e, 0xb2, 0x00,
    0xe7, 0x07, 0xa4, 0x09, 0xf2, 0x02, 0xc8, 0x09,
    0xb4, 0x00, 0xf8, 0x07, 0x02, 0x00, 0x0d, 0x00,
    0xd7, 0x09, 0x0e, 0xc0, 0xe7, 0x07, 0x00, 0x00,
    0x0e, 0xc0, 0xc8, 0x09, 0xdc, 0x00, 0xf0, 0x07,
    0xff, 0xff, 0x09, 0x00, 0xf0, 0x07, 0xfb, 0x13,
    0x0b, 0x00, 0xe7, 0x09, 0xc0, 0x00, 0x58, 0x08,
    0xe7, 0x09, 0xbe, 0x00, 0x54, 0x08, 0xe7, 0x09,
    0x10, 0x00, 0x92, 0x08, 0xc8, 0x07, 0xb4, 0x09,
    0x9f, 0xaf, 0x8c, 0x09, 0x9f, 0xaf, 0xe2, 0x0b,
    0xc0, 0x07, 0x80, 0x01, 0x44, 0xaf, 0x27, 0x00,
    0x88, 0x08, 0x27, 0x00, 0x8a, 0x08, 0x27, 0x00,
    0x8c, 0x08, 0xc0, 0x07, 0x74, 0x00, 0x44, 0xaf,
    0x27, 0x00, 0xac, 0x08, 0x08, 0x00, 0x00, 0x90,
    0xc1, 0x07, 0x1d, 0x00, 0x20, 0x00, 0x20, 0x00,
    0x01, 0xda, 0x7c, 0xc1, 0x9f, 0xaf, 0x8a, 0x0b,
    0xc0, 0x07, 0x4c, 0x00, 0x48, 0xaf, 0x27, 0x00,
    0x56, 0x08, 0x9f, 0xaf, 0x72, 0x0c, 0xe7, 0x07,
    0x00, 0x80, 0x96, 0x08, 0xef, 0x57, 0x00, 0x00,
    0xf0, 0x09, 0x03, 0xc0, 0xe7, 0x07, 0x01, 0x00,
    0x1c, 0xc0, 0xe7, 0x05, 0x0e, 0xc0, 0x97, 0xcf,
    0x49, 0xaf, 0xe7, 0x87, 0x43, 0x00, 0x0e, 0xc0,
    0xe7, 0x07, 0xff, 0xff, 0x94, 0x08, 0x9f, 0xaf,
    0x8a, 0x0c, 0xc0, 0x07, 0x01, 0x00, 0x60, 0xaf,
    0x4a, 0xaf, 0x97, 0xcf, 0x00, 0x08, 0x09, 0x08,
    0x11, 0x08, 0x00, 0xda, 0x7c, 0xc1, 0x97, 0xcf,
    0x67, 0x04, 0xcc, 0x02, 0xc0, 0xdf, 0x51, 0x94,
    0xb1, 0xaf, 0x06, 0x00, 0xc1, 0xdf, 0xc9, 0x09,
    0xcc, 0x02, 0x49, 0x62, 0x75, 0xc1, 0xc0, 0xdf,
    0xa7, 0xcf, 0xd6, 0x02, 0x0e, 0x00, 0x24, 0x00,
    0xd6, 0x05, 0x22, 0x00, 0xc4, 0x06, 0xd0, 0x00,
    0xf0, 0x0b, 0xaa, 0x00, 0x0e, 0x0a, 0xbe, 0x00,
    0x2c, 0x0c, 0x10, 0x00, 0x20, 0x00, 0x04, 0x00,
    0xc4, 0x05, 0x02, 0x00, 0x66, 0x03, 0x06, 0x00,
    0x00, 0x00, 0x24, 0xc0, 0x04, 0x04, 0x28, 0xc0,
    0xfe, 0xfb, 0x1e, 0xc0, 0x00, 0x04, 0x22, 0xc0,
    0xff, 0xf0, 0xc0, 0x00, 0x60, 0x0b, 0x00, 0x00,
    0x00, 0x00, 0xff, 0xff, 0x34, 0x0a, 0x3e, 0x0a,
    0x9e, 0x0a, 0xa8, 0x0a, 0xce, 0x0a, 0xd2, 0x0a,
    0xd6, 0x0a, 0x00, 0x0b, 0x10, 0x0b, 0x1e, 0x0b,
    0x20, 0x0b, 0x28, 0x0b, 0x28, 0x0b, 0x27, 0x02,
    0xa2, 0x08, 0x97, 0xcf, 0xe7, 0x07, 0x00, 0x00,
    0xa2, 0x08, 0x0a, 0x0e, 0x01, 0x00, 0xca, 0x57,
    0x0e, 0x00, 0x9f, 0xc3, 0x2a, 0x0b, 0xca, 0x37,
    0x00, 0x00, 0x9f, 0xc2, 0x2a, 0x0b, 0x0a, 0xd2,
    0xb2, 0xcf, 0xf4, 0x09, 0xc8, 0x09, 0xde, 0x00,
    0x07, 0x06, 0x9f, 0xcf, 0x3c, 0x0b, 0xf0, 0x57,
    0x80, 0x01, 0x06, 0x00, 0x9f, 0xc8, 0x2a, 0x0b,
    0x27, 0x0c, 0x02, 0x00, 0x86, 0x08, 0xc0, 0x09,
    0x88, 0x08, 0x27, 0x00, 0x8a, 0x08, 0xe7, 0x07,
    0x00, 0x00, 0x84, 0x08, 0x27, 0x00, 0x5c, 0x08,
    0x00, 0x1c, 0x06, 0x00, 0x27, 0x00, 0x8c, 0x08,
    0x41, 0x90, 0x67, 0x50, 0x86, 0x08, 0x0d, 0xc0,
    0x67, 0x00, 0x5a, 0x08, 0x27, 0x0c, 0x06, 0x00,
    0x5e, 0x08, 0xe7, 0x07, 0x8a, 0x0a, 0x60, 0x08,
    0xc8, 0x07, 0x5a, 0x08, 0x41, 0x90, 0x51, 0xaf,
    0x97, 0xcf, 0x9f, 0xaf, 0xac, 0x0e, 0xe7, 0x09,
    0x8c, 0x08, 0x8a, 0x08, 0xe7, 0x09, 0x86, 0x08,
    0x84, 0x08, 0x59, 0xaf, 0x97, 0xcf, 0x27, 0x0c,
    0x02, 0x00, 0x7c, 0x08, 0x59, 0xaf, 0x97, 0xcf,
    0x09, 0x0c, 0x02, 0x00, 0x09, 0xda, 0x49, 0xd2,
    0xc9, 0x19, 0xac, 0x08, 0xc8, 0x07, 0x5a, 0x08,
    0xe0, 0x07, 0x00, 0x00, 0x60, 0x02, 0xe0, 0x07,
    0x04, 0x00, 0xd0, 0x07, 0x9a, 0x0a, 0x48, 0xdb,
    0x41, 0x90, 0x50, 0xaf, 0x97, 0xcf, 0x59, 0xaf,
    0x97, 0xcf, 0x59, 0xaf, 0x97, 0xcf, 0xf0, 0x57,
    0x06, 0x00, 0x06, 0x00, 0x26, 0xc1, 0xe7, 0x07,
    0x7e, 0x08, 0x5c, 0x08, 0x41, 0x90, 0x67, 0x00,
    0x5a, 0x08, 0x27, 0x0c, 0x06, 0x00, 0x5e, 0x08,
    0xe7, 0x07, 0x5c, 0x0b, 0x60, 0x08, 0xc8, 0x07,
    0x5a, 0x08, 0x41, 0x90, 0x51, 0xaf, 0x97, 0xcf,
    0x07, 0x0c, 0x06, 0x00, 0xc7, 0x57, 0x06, 0x00,
    0x10, 0xc1, 0xc8, 0x07, 0x7e, 0x08, 0x16, 0xcf,
    0x00, 0x0c, 0x02, 0x00, 0x00, 0xda, 0x40, 0xd1,
    0x27, 0x00, 0x98, 0x08, 0x1f, 0xcf, 0x1e, 0xcf,
    0x27, 0x0c, 0x02, 0x00, 0xa4, 0x08, 0x1a, 0xcf,
    0x00, 0xcf, 0x27, 0x02, 0x20, 0x01, 0xe7, 0x07,
    0x08, 0x00, 0x22, 0x01, 0xe7, 0x07, 0x13, 0x00,
    0xb0, 0xc0, 0x97, 0xcf, 0x41, 0x90, 0x67, 0x00,
    0x5a, 0x08, 0xe7, 0x01, 0x5e, 0x08, 0x27, 0x02,
    0x5c, 0x08, 0xe7, 0x07, 0x5c, 0x0b, 0x60, 0x08,
    0xc8, 0x07, 0x5a, 0x08, 0xc1, 0x07, 0x00, 0x80,
    0x50, 0xaf, 0x97, 0xcf, 0x59, 0xaf, 0x97, 0xcf,
    0x00, 0x60, 0x05, 0xc0, 0xe7, 0x07, 0x00, 0x00,
    0x9a, 0x08, 0xa7, 0xcf, 0x58, 0x08, 0x9f, 0xaf,
    0xe2, 0x0b, 0xe7, 0x07, 0x01, 0x00, 0x9a, 0x08,
    0x49, 0xaf, 0xd7, 0x09, 0x00, 0xc0, 0x07, 0xaf,
    0xe7, 0x05, 0x00, 0xc0, 0x4a, 0xaf, 0xa7, 0xcf,
    0x58, 0x08, 0xc0, 0x07, 0x40, 0x00, 0x44, 0xaf,
    0x27, 0x00, 0xa0, 0x08, 0x08, 0x00, 0xc0, 0x07,
    0x20, 0x00, 0x20, 0x94, 0x00, 0xda, 0x7d, 0xc1,
    0xc0, 0x07, 0xfe, 0x7f, 0x44, 0xaf, 0x40, 0x00,
    0x41, 0x90, 0xc0, 0x37, 0x08, 0x00, 0xdf, 0xde,
    0x50, 0x06, 0xc0, 0x57, 0x10, 0x00, 0x02, 0xc2,
    0xc0, 0x07, 0x10, 0x00, 0x27, 0x00, 0x76, 0x08,
    0x41, 0x90, 0x9f, 0xde, 0x40, 0x06, 0x44, 0xaf,
    0x27, 0x00, 0x74, 0x08, 0xc0, 0x09, 0x76, 0x08,
    0x41, 0x90, 0x00, 0xd2, 0x00, 0xd8, 0x9f, 0xde,
    0x08, 0x00, 0x44, 0xaf, 0x27, 0x00, 0x9e, 0x08,
    0x97, 0xcf, 0xe7, 0x87, 0x00, 0x84, 0x28, 0xc0,
    0xe7, 0x67, 0xff, 0xf3, 0x24, 0xc0, 0x97, 0xcf,
    0xe7, 0x87, 0x01, 0x00, 0xaa, 0x08, 0xe7, 0x57,
    0x00, 0x00, 0x7a, 0x08, 0x97, 0xc1, 0x9f, 0xaf,
    0xe2, 0x0b, 0xe7, 0x87, 0x00, 0x06, 0x22, 0xc0,
    0xe7, 0x07, 0x00, 0x00, 0x90, 0xc0, 0xe7, 0x67,
    0xfe, 0xff, 0x3e, 0xc0, 0xe7, 0x07, 0x2e, 0x00,
    0x0a, 0xc0, 0xe7, 0x87, 0x01, 0x00, 0x3e, 0xc0,
    0xe7, 0x07, 0xff, 0xff, 0x94, 0x08, 0x9f, 0xaf,
    0xf0, 0x0c, 0x97, 0xcf, 0x17, 0x00, 0xa7, 0xaf,
    0x54, 0x08, 0xc0, 0x05, 0x27, 0x00, 0x52, 0x08,
    0xe7, 0x87, 0x01, 0x00, 0xaa, 0x08, 0x9f, 0xaf,
    0xe2, 0x0b, 0xe7, 0x07, 0x0c, 0x00, 0x40, 0xc0,
    0x9f, 0xaf, 0xf0, 0x0c, 0xe7, 0x07, 0x00, 0x00,
    0x78, 0x08, 0x00, 0x90, 0xe7, 0x09, 0x88, 0x08,
    0x8a, 0x08, 0x27, 0x00, 0x84, 0x08, 0x27, 0x00,
    0x7c, 0x08, 0x9f, 0xaf, 0x8a, 0x0c, 0xe7, 0x07,
    0x00, 0x00, 0xb2, 0x02, 0xe7, 0x07, 0x00, 0x00,
    0xb4, 0x02, 0xc0, 0x07, 0x06, 0x00, 0xc8, 0x09,
    0xde, 0x00, 0xc8, 0x17, 0x03, 0x00, 0xc9, 0x07,
    0x7e, 0x08, 0x29, 0x0a, 0x00, 0xda, 0x7d, 0xc1,
    0x97, 0xcf, 0xd7, 0x09, 0x00, 0xc0, 0xc1, 0xdf,
    0x00, 0x90, 0x27, 0x00, 0x6a, 0x08, 0xe7, 0x07,
    0x6a, 0x08, 0x6c, 0x08, 0x27, 0x00, 0x6e, 0x08,
    0xe7, 0x07, 0x6e, 0x08, 0x70, 0x08, 0x27, 0x00,
    0x78, 0x08, 0x27, 0x00, 0x62, 0x08, 0x27, 0x00,
    0x64, 0x08, 0xc8, 0x09, 0x74, 0x08, 0xc1, 0x09,
    0x76, 0x08, 0xc9, 0x07, 0x72, 0x08, 0x11, 0x02,
    0x09, 0x02, 0xc8, 0x17, 0x40, 0x06, 0x01, 0xda,
    0x7a, 0xc1, 0x51, 0x94, 0xc8, 0x09, 0x9e, 0x08,
    0xc9, 0x07, 0x9c, 0x08, 0xc1, 0x09, 0x76, 0x08,
    0x01, 0xd2, 0x01, 0xd8, 0x11, 0x02, 0x09, 0x02,
    0xc8, 0x17, 0x08, 0x00, 0x01, 0xda, 0x7a, 0xc1,
    0x51, 0x94, 0xe7, 0x05, 0x00, 0xc0, 0x97, 0xcf,
    0xe7, 0x57, 0x00, 0x00, 0x52, 0x08, 0x97, 0xc0,
    0x9f, 0xaf, 0x04, 0x00, 0xe7, 0x09, 0x94, 0x08,
    0x90, 0x08, 0xe7, 0x57, 0xff, 0xff, 0x90, 0x08,
    0x04, 0xc1, 0xe7, 0x07, 0xf0, 0x0c, 0x8e, 0x08,
    0x97, 0xcf, 0xe7, 0x17, 0x32, 0x00, 0x90, 0x08,
    0xe7, 0x67, 0xff, 0x07, 0x90, 0x08, 0xe7, 0x07,
    0x26, 0x0d, 0x8e, 0x08, 0x97, 0xcf, 0xd7, 0x09,
    0x00, 0xc0, 0xc1, 0xdf, 0xe7, 0x57, 0x00, 0x00,
    0x96, 0x08, 0x23, 0xc0, 0xe7, 0x07, 0x00, 0x80,
    0x80, 0xc0, 0xe7, 0x07, 0x04, 0x00, 0x90, 0xc0,
    0xe7, 0x07, 0x00, 0x00, 0x80, 0xc0, 0xe7, 0x07,
    0x00, 0x80, 0x80, 0xc0, 0xc0, 0x07, 0x00, 0x00,
    0xc0, 0x07, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00,
    0xe7, 0x07, 0x00, 0x00, 0x80, 0xc0, 0xe7, 0x07,
    0x00, 0x80, 0x80, 0xc0, 0xe7, 0x07, 0x00, 0x80,
    0x40, 0xc0, 0xc0, 0x07, 0x00, 0x00, 0xe7, 0x07,
    0x00, 0x00, 0x40, 0xc0, 0xe7, 0x07, 0x00, 0x00,
    0x80, 0xc0, 0xef, 0x57, 0x00, 0x00, 0xf1, 0x09,
    0x9f, 0xa0, 0xc0, 0x0d, 0xe7, 0x07, 0x04, 0x00,
    0x90, 0xc0, 0xe7, 0x07, 0x00, 0x02, 0x40, 0xc0,
    0xe7, 0x07, 0x0c, 0x02, 0x40, 0xc0, 0xe7, 0x07,
    0x00, 0x00, 0x96, 0x08, 0xe7, 0x07, 0x00, 0x00,
    0x8e, 0x08, 0xe7, 0x07, 0x00, 0x00, 0xaa, 0x08,
    0xd7, 0x09, 0x00, 0xc0, 0xc1, 0xdf, 0x9f, 0xaf,
    0x9e, 0x03, 0xe7, 0x05, 0x00, 0xc0, 0x9f, 0xaf,
    0xde, 0x01, 0xe7, 0x05, 0x00, 0xc0, 0x97, 0xcf,
    0x9f, 0xaf, 0xde, 0x0d, 0xef, 0x77, 0x00, 0x00,
    0xf1, 0x09, 0x97, 0xc1, 0x9f, 0xaf, 0xde, 0x0d,
    0xef, 0x77, 0x00, 0x00, 0xf1, 0x09, 0x97, 0xc1,
    0xef, 0x07, 0x01, 0x00, 0xf1, 0x09, 0xe7, 0x87,
    0x00, 0x08, 0x1e, 0xc0, 0xe7, 0x87, 0x00, 0x08,
    0x22, 0xc0, 0xe7, 0x67, 0xff, 0xf7, 0x22, 0xc0,
    0xe7, 0x77, 0x00, 0x08, 0x20, 0xc0, 0x11, 0xc0,
    0xe7, 0x67, 0xff, 0xf7, 0x1e, 0xc0, 0xe7, 0x87,
    0x00, 0x08, 0x22, 0xc0, 0xe7, 0x67, 0xff, 0xf7,
    0x22, 0xc0, 0xe7, 0x77, 0x00, 0x08, 0x20, 0xc0,
    0x04, 0xc1, 0xe7, 0x87, 0x00, 0x08, 0x22, 0xc0,
    0x97, 0xcf, 0xe7, 0x07, 0x01, 0x01, 0xf0, 0x09,
    0xef, 0x57, 0x18, 0x00, 0xfe, 0xff, 0x97, 0xc2,
    0xef, 0x07, 0x00, 0x00, 0xf0, 0x09, 0x97, 0xcf,
    0xd7, 0x09, 0x00, 0xc0, 0x17, 0x00, 0x17, 0x02,
    0x97, 0x02, 0xe7, 0x57, 0x00, 0x00, 0x7a, 0x08,
    0x06, 0xc0, 0xc0, 0x09, 0x92, 0xc0, 0xc0, 0x77,
    0x09, 0x02, 0x9f, 0xc1, 0xea, 0x06, 0x9f, 0xcf,
    0x20, 0x08, 0xd7, 0x09, 0x0e, 0xc0, 0xe7, 0x07,
    0x00, 0x00, 0x0e, 0xc0, 0x9f, 0xaf, 0x66, 0x0e,
    0xe7, 0x05, 0x0e, 0xc0, 0x97, 0xcf, 0xd7, 0x09,
    0x00, 0xc0, 0x17, 0x02, 0xc8, 0x09, 0xb0, 0xc0,
    0xe7, 0x67, 0xfe, 0x7f, 0xb0, 0xc0, 0xc8, 0x77,
    0x00, 0x20, 0x9f, 0xc1, 0x64, 0xeb, 0xe7, 0x57,
    0x00, 0x00, 0xc8, 0x02, 0x9f, 0xc1, 0x80, 0xeb,
    0xc8, 0x99, 0xca, 0x02, 0xc8, 0x67, 0x04, 0x00,
    0x9f, 0xc1, 0x96, 0xeb, 0x9f, 0xcf, 0x4c, 0xeb,
    0xe7, 0x07, 0x00, 0x00, 0xa6, 0xc0, 0xe7, 0x09,
    0xb0, 0xc0, 0xc8, 0x02, 0xe7, 0x07, 0x03, 0x00,
    0xb0, 0xc0, 0x97, 0xcf, 0xc0, 0x09, 0x86, 0x08,
    0xc0, 0x37, 0x01, 0x00, 0x97, 0xc9, 0xc9, 0x09,
    0x88, 0x08, 0x02, 0x00, 0x41, 0x90, 0x48, 0x02,
    0xc9, 0x17, 0x06, 0x00, 0x9f, 0xaf, 0x64, 0x05,
    0x9f, 0xa2, 0xd6, 0x0e, 0x02, 0xda, 0x77, 0xc1,
    0x41, 0x60, 0x71, 0xc1, 0x97, 0xcf, 0x17, 0x02,
    0x57, 0x02, 0x43, 0x04, 0x21, 0x04, 0xe0, 0x00,
    0x43, 0x04, 0x21, 0x04, 0xe0, 0x00, 0x43, 0x04,
    0x21, 0x04, 0xe0, 0x00, 0xc1, 0x07, 0x01, 0x00,
    0xc9, 0x05, 0xc8, 0x05, 0x97, 0xcf,
    0,    0
};

/*
 *     klsiNewCodeFix
 */
static UINT8 klsiNewCodeFix[] = 
{
    0xB6, 0xC3, 0xAA, 0xBB, 0xCC, 0xDD,
    0x02, 0x00, 0x08, 0x00, 0x24, 0x00, 0x2e, 0x00,
    0x2c, 0x00, 0x3e, 0x00, 0x44, 0x00, 0x48, 0x00,
    0x50, 0x00, 0x5c, 0x00, 0x60, 0x00, 0x66, 0x00,
    0x6c, 0x00, 0x70, 0x00, 0x76, 0x00, 0x74, 0x00,
    0x7a, 0x00, 0x7e, 0x00, 0x84, 0x00, 0x8a, 0x00,
    0x8e, 0x00, 0x92, 0x00, 0x98, 0x00, 0x9c, 0x00,
    0xa0, 0x00, 0xa8, 0x00, 0xae, 0x00, 0xb4, 0x00,
    0xb2, 0x00, 0xba, 0x00, 0xbe, 0x00, 0xc4, 0x00,
    0xc8, 0x00, 0xce, 0x00, 0xd2, 0x00, 0xd6, 0x00,
    0xda, 0x00, 0xe2, 0x00, 0xe0, 0x00, 0xea, 0x00,
    0xf2, 0x00, 0xfe, 0x00, 0x06, 0x01, 0x0c, 0x01,
    0x1a, 0x01, 0x24, 0x01, 0x22, 0x01, 0x2a, 0x01,
    0x30, 0x01, 0x36, 0x01, 0x3c, 0x01, 0x4e, 0x01,
    0x52, 0x01, 0x58, 0x01, 0x5c, 0x01, 0x9c, 0x01,
    0xb6, 0x01, 0xba, 0x01, 0xc0, 0x01, 0xca, 0x01,
    0xd0, 0x01, 0xda, 0x01, 0xe2, 0x01, 0xea, 0x01,
    0xf0, 0x01, 0x0a, 0x02, 0x0e, 0x02, 0x14, 0x02,
    0x26, 0x02, 0x6c, 0x02, 0x8e, 0x02, 0x98, 0x02,
    0xa0, 0x02, 0xa6, 0x02, 0xba, 0x02, 0xc6, 0x02,
    0xce, 0x02, 0xe8, 0x02, 0xee, 0x02, 0xf4, 0x02,
    0xf8, 0x02, 0x0a, 0x03, 0x10, 0x03, 0x1a, 0x03,
    0x1e, 0x03, 0x2a, 0x03, 0x2e, 0x03, 0x34, 0x03,
    0x3a, 0x03, 0x44, 0x03, 0x4e, 0x03, 0x5a, 0x03,
    0x5e, 0x03, 0x6a, 0x03, 0x72, 0x03, 0x80, 0x03,
    0x84, 0x03, 0x8c, 0x03, 0x94, 0x03, 0x98, 0x03,
    0xa8, 0x03, 0xae, 0x03, 0xb4, 0x03, 0xba, 0x03,
    0xce, 0x03, 0xcc, 0x03, 0xd6, 0x03, 0xdc, 0x03,
    0xec, 0x03, 0xf0, 0x03, 0xfe, 0x03, 0x1c, 0x04,
    0x30, 0x04, 0x38, 0x04, 0x3c, 0x04, 0x40, 0x04,
    0x48, 0x04, 0x46, 0x04, 0x54, 0x04, 0x5e, 0x04,
    0x64, 0x04, 0x74, 0x04, 0x78, 0x04, 0x84, 0x04,
    0xd8, 0x04, 0xec, 0x04, 0xf0, 0x04, 0xf8, 0x04,
    0xfe, 0x04, 0x1c, 0x05, 0x2c, 0x05, 0x30, 0x05,
    0x4a, 0x05, 0x56, 0x05, 0x5a, 0x05, 0x88, 0x05,
    0x8c, 0x05, 0x96, 0x05, 0x9a, 0x05, 0xa8, 0x05,
    0xcc, 0x05, 0xd2, 0x05, 0xda, 0x05, 0xe0, 0x05,
    0xe4, 0x05, 0xfc, 0x05, 0x06, 0x06, 0x14, 0x06,
    0x12, 0x06, 0x1a, 0x06, 0x20, 0x06, 0x26, 0x06,
    0x2e, 0x06, 0x34, 0x06, 0x48, 0x06, 0x52, 0x06,
    0x64, 0x06, 0x86, 0x06, 0x90, 0x06, 0x9a, 0x06,
    0xa0, 0x06, 0xac, 0x06, 0xaa, 0x06, 0xb2, 0x06,
    0xb8, 0x06, 0xdc, 0x06, 0xda, 0x06, 0xe2, 0x06,
    0xe8, 0x06, 0xf2, 0x06, 0xf8, 0x06, 0xfc, 0x06,
    0x0a, 0x07, 0x10, 0x07, 0x14, 0x07, 0x24, 0x07,
    0x2a, 0x07, 0x32, 0x07, 0x38, 0x07, 0xb2, 0x07,
    0xba, 0x07, 0xde, 0x07, 0xe4, 0x07, 0x10, 0x08,
    0x14, 0x08, 0x1a, 0x08, 0x1e, 0x08, 0x30, 0x08,
    0x38, 0x08, 0x3c, 0x08, 0x44, 0x08, 0x42, 0x08,
    0x48, 0x08, 0xc6, 0x08, 0xcc, 0x08, 0xd2, 0x08,
    0xfe, 0x08, 0x04, 0x09, 0x0a, 0x09, 0x0e, 0x09,
    0x12, 0x09, 0x16, 0x09, 0x20, 0x09, 0x24, 0x09,
    0x28, 0x09, 0x32, 0x09, 0x46, 0x09, 0x4a, 0x09,
    0x50, 0x09, 0x54, 0x09, 0x5a, 0x09, 0x60, 0x09,
    0x7c, 0x09, 0x80, 0x09, 0xb8, 0x09, 0xbc, 0x09,
    0xc0, 0x09, 0xc4, 0x09, 0xc8, 0x09, 0xcc, 0x09,
    0xd0, 0x09, 0xd4, 0x09, 0xec, 0x09, 0xf4, 0x09,
    0xf6, 0x09, 0xf8, 0x09, 0xfa, 0x09, 0xfc, 0x09,
    0xfe, 0x09, 0x00, 0x0a, 0x02, 0x0a, 0x04, 0x0a,
    0x06, 0x0a, 0x08, 0x0a, 0x0a, 0x0a, 0x0c, 0x0a,
    0x10, 0x0a, 0x18, 0x0a, 0x24, 0x0a, 0x2c, 0x0a,
    0x32, 0x0a, 0x3c, 0x0a, 0x46, 0x0a, 0x4c, 0x0a,
    0x50, 0x0a, 0x54, 0x0a, 0x5a, 0x0a, 0x5e, 0x0a,
    0x66, 0x0a, 0x6c, 0x0a, 0x72, 0x0a, 0x78, 0x0a,
    0x7e, 0x0a, 0x7c, 0x0a, 0x82, 0x0a, 0x8c, 0x0a,
    0x92, 0x0a, 0x90, 0x0a, 0x98, 0x0a, 0x96, 0x0a,
    0xa2, 0x0a, 0xb2, 0x0a, 0xb6, 0x0a, 0xc4, 0x0a,
    0xe2, 0x0a, 0xe0, 0x0a, 0xe8, 0x0a, 0xee, 0x0a,
    0xf4, 0x0a, 0xf2, 0x0a, 0xf8, 0x0a, 0x0c, 0x0b,
    0x1a, 0x0b, 0x24, 0x0b, 0x40, 0x0b, 0x44, 0x0b,
    0x48, 0x0b, 0x4e, 0x0b, 0x4c, 0x0b, 0x52, 0x0b,
    0x68, 0x0b, 0x6c, 0x0b, 0x70, 0x0b, 0x76, 0x0b,
    0x88, 0x0b, 0x92, 0x0b, 0xbe, 0x0b, 0xca, 0x0b,
    0xce, 0x0b, 0xde, 0x0b, 0xf4, 0x0b, 0xfa, 0x0b,
    0x00, 0x0c, 0x24, 0x0c, 0x28, 0x0c, 0x30, 0x0c,
    0x36, 0x0c, 0x3c, 0x0c, 0x40, 0x0c, 0x4a, 0x0c,
    0x50, 0x0c, 0x58, 0x0c, 0x56, 0x0c, 0x5c, 0x0c,
    0x60, 0x0c, 0x64, 0x0c, 0x80, 0x0c, 0x94, 0x0c,
    0x9a, 0x0c, 0x98, 0x0c, 0x9e, 0x0c, 0xa4, 0x0c,
    0xa2, 0x0c, 0xa8, 0x0c, 0xac, 0x0c, 0xb0, 0x0c,
    0xb4, 0x0c, 0xb8, 0x0c, 0xbc, 0x0c, 0xce, 0x0c,
    0xd2, 0x0c, 0xd6, 0x0c, 0xf4, 0x0c, 0xfa, 0x0c,
    0x00, 0x0d, 0xfe, 0x0c, 0x06, 0x0d, 0x0e, 0x0d,
    0x0c, 0x0d, 0x16, 0x0d, 0x1c, 0x0d, 0x22, 0x0d,
    0x20, 0x0d, 0x30, 0x0d, 0x7e, 0x0d, 0x82, 0x0d,
    0x9a, 0x0d, 0xa0, 0x0d, 0xa6, 0x0d, 0xb0, 0x0d,
    0xb8, 0x0d, 0xc2, 0x0d, 0xc8, 0x0d, 0xce, 0x0d,
    0xd4, 0x0d, 0xdc, 0x0d, 0x1e, 0x0e, 0x2c, 0x0e,
    0x3e, 0x0e, 0x4c, 0x0e, 0x50, 0x0e, 0x5e, 0x0e,
    0xae, 0x0e, 0xb8, 0x0e, 0xc6, 0x0e, 0xca, 0x0e,
    0,    0
};


const int 		lenKlsiNewCode = sizeof (klsiNewCode);
const int 		lenKlsiNewCodeFix = sizeof (klsiNewCodeFix);

LOCAL UINT16 		initCount = 0;	/* Count of init nesting */

LOCAL MUTEX_HANDLE 	klsiMutex;	/* to protect internal structs */
LOCAL LIST_HEAD		klsiDevList;	/* linked list of Device Structs */
LOCAL LIST_HEAD    	reqList;        /* Attach callback request list */		



LOCAL MUTEX_HANDLE 	klsiTxMutex;	/* to protect internal structs */
LOCAL MUTEX_HANDLE 	klsiRxMutex;	/* to protect internal structs */

/* forward declarartions */

/* END Specific Externally imported interfaces. */

IMPORT	int 	endMultiLstCnt 	(END_OBJ* pEnd);

/* Externally visible interfaces. */

END_OBJ * 	KlsiEndLoad 	(char * initString);
STATUS 		usbKlsiEndInit 	(void);

/*  LOCAL functions */

LOCAL STATUS	klsiEndStart	(KLSI_DEVICE * pDrvCtrl);
LOCAL STATUS	klsiEndStop	(KLSI_DEVICE * pDrvCtrl);
LOCAL int	klsiEndIoctl   	(KLSI_DEVICE * pDrvCtrl, 
				 int cmd, caddr_t data);
LOCAL STATUS	klsiEndUnload	(KLSI_DEVICE * pDrvCtrl);
LOCAL STATUS	klsiEndSend	(KLSI_DEVICE * pDrvCtrl, M_BLK_ID pBuf);
			  
LOCAL STATUS	klsiEndMCastAdd (KLSI_DEVICE * pDrvCtrl, char * pAddress);
LOCAL STATUS	klsiEndMCastDel (KLSI_DEVICE * pDrvCtrl, char * pAddress);
LOCAL STATUS	klsiEndMCastGet (KLSI_DEVICE * pDrvCtrl, 
				 MULTI_TABLE * pTable);
LOCAL STATUS	klsiEndPollSend	(KLSI_DEVICE * pDrvCtrl, M_BLK_ID pBuf);
LOCAL STATUS	klsiEndPollRcv 	(KLSI_DEVICE * pDrvCtrl, M_BLK_ID pBuf);
LOCAL STATUS	klsiEndParse	(KLSI_DEVICE * pDrvCtrl,	
    				 char * initString,		
				UINT16 * pVendorId,
    				UINT16 * pProductId
				);
LOCAL STATUS	klsiEndMemInit	(KLSI_DEVICE * pDrvCtrl);
LOCAL STATUS	klsiEndConfig	(KLSI_DEVICE * pDrvCtrl);
LOCAL STATUS 	klsiDevInit 	(KLSI_DEVICE* pDevCtrl, 
			  	 UINT16 vendorId, 
			  	 UINT16 productId
				);

LOCAL void 	klsiTxCallback	(pVOID p);
LOCAL void 	klsiRxCallback	(pVOID p);
LOCAL STATUS 	klsiShutdown 	(int errCode);
LOCAL STATUS 	klsiReset 	(KLSI_DEVICE* pDevCtrl);
LOCAL STATUS 	klsiInit	(KLSI_DEVICE * pDevCtrl);
LOCAL STATUS 	klsiSend	(KLSI_DEVICE* pDevCtrl,UINT8* pBfr,	
				 UINT32 size);
LOCAL STATUS 	klsiListenForInput   	(KLSI_DEVICE * pDevCtrl);
LOCAL USB_KLSI_DEV * klsiFindDevice		(USBD_NODE_ID nodeId);
LOCAL USB_KLSI_DEV * klsiEndFindDevice 	(UINT16 vendorId, UINT16 productId);
LOCAL void 	klsiDestroyDevice 	(KLSI_DEVICE * pDevCtrl);
LOCAL STATUS 	klsiMCastFilterSet 	(KLSI_DEVICE* pDevCtrl,
   			                 UINT8* pAddress,		
			                 UINT32 noOfFilters);
LOCAL STATUS 	klsiPacketFilterSet 	(KLSI_DEVICE* pDevCtrl,
				         UINT8 bitmap);
LOCAL STATUS 	klsiMacAddressSet	(KLSI_DEVICE* pDevCtrl,
    			                 UINT8 * pAddress);

LOCAL void 	klsiAttachCallback  	(USBD_NODE_ID nodeId, 
   				         UINT16 attachAction, 
    				         UINT16 configuration,
    				         UINT16 interface,
    				         UINT16 deviceClass, 
    				         UINT16 deviceSubClass, 
    				         UINT16 deviceProtocol);
LOCAL STATUS 	klsiEndRecv (KLSI_DEVICE *pDrvCtrl, UINT8* pData, UINT32 len);


LOCAL VOID 	notifyAttach 		(USBD_NODE_ID nodeId,
    					 UINT16 attachCode);


/*
 * Our driver function table.  This is static across all driver instances.
 */

LOCAL NET_FUNCS klsiEndFuncTable =
    {
    (FUNCPTR) klsiEndStart,	/* Function to start the device. */
    (FUNCPTR) klsiEndStop,	/* Function to stop the device. */
    (FUNCPTR) klsiEndUnload,	/* Unloading function for the driver. */
    (FUNCPTR) klsiEndIoctl,	/* Ioctl function for the driver. */
    (FUNCPTR) klsiEndSend,	/* Send function for the driver. */
    (FUNCPTR) klsiEndMCastAdd,	/* Multicast add function for the */
				/* driver. */
    (FUNCPTR) klsiEndMCastDel,	/* Multicast delete function for */
				/* the driver. */
    (FUNCPTR) klsiEndMCastGet,	/* Multicast retrieve function for */
				/* the driver. */
    (FUNCPTR) klsiEndPollSend,	/* Polling send function */
    (FUNCPTR) klsiEndPollRcv,	/* Polling receive function */
    endEtherAddressForm,	/* put address info into a NET_BUFFER */
    endEtherPacketDataGet,	/* get pointer to data in NET_BUFFER */
    endEtherPacketAddrGet  	/* Get packet addresses. */
    };


/***************************************************************************
*
* klsiAttachCallback - Gets called for attachment/detachment of devices
*
* The USBD will invoke this callback when a USB ethernet device is 
* attached to or removed from the system.  
* <nodeId> is the USBD_NODE_ID of the node being attached or removed.	
* <attachAction> is USBD_DYNA_ATTACH or USBD_DYNA_REMOVE.
* Communication device functionality resides at the interface level, with
* the exception of being the definition of the Communication device class
* code.so <configuration> and <interface> will indicate the configuration
* / interface that reports itself as a network device.
* <deviceClass> and <deviceSubClass> will match the class/subclass for 
* which registration is done.  
* <deviceProtocol> doesn't have any meaning for the ethernet devices.
* This field is ignored.
*
* NOTE: The USBD will invoke this function once for each configuration/
* interface, which reports itself as a network device.
* So, it is possible that a single device insertion/removal may trigger 
* multiple callbacks. All callbacks are ignored except the first  
* for a given device.
*
* RETURNS: N/A
*/

LOCAL void  klsiAttachCallback
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
     
    pUsbListDev pNewDev;

    UINT8 bfr[30];
    UINT16 actLen;

    UINT16 vendorId;
    UINT16 productId;

    int noOfSupportedDevices = (sizeof (klsiAdapterList) /
				 (2 * sizeof (UINT16)));

    int index = 0;


    OSS_MUTEX_TAKE (klsiMutex, OSS_BLOCK);


    switch (attachAction)
	{
	case USBD_DYNA_ATTACH :
	
	    /* a new device is found */

	    KLSI_LOG (KLSI_DBG_ATTACH, 
                      "New Device found : %0x Class %0x Subclass %0x Protocol "
                      "%0x Configuration %0x Interface  %0x nodeId \n", 
	 	      deviceClass, deviceSubClass, deviceProtocol, 
		      configuration, interface, (UINT)nodeId);

	    /* First Ensure that this device is not already on the list */
	
			 
	    if (klsiFindDevice (nodeId) != NULL)
		{
		KLSI_LOG (KLSI_DBG_ATTACH, "Device already exists. \n",
		          0, 0, 0, 0, 0, 0);
	        break;
		}

	    /* Now, Ensure that it is a KLSI device */
	
            if (usbdDescriptorGet (klsiHandle, 
				   nodeId, 
				   USB_RT_STANDARD | USB_RT_DEVICE, 
				   USB_DESCR_DEVICE, 
				   0, 
				   0, 
				   30, 
				   bfr, 
				   &actLen) 
				!= OK)
	        {
		break;
            	}
#if 0
	    for(index=0;index<actLen;index++)
		printf(" 0x%x",bfr[index]);
	    printf("\n");
#endif

            vendorId = (*(bfr+9) << 8)&0xff00;		
            vendorId |= *(bfr+8);		
            productId = (*(bfr+11) << 8)&0xff00;		
            productId |= *(bfr+10);		


            for (index = 0; index < noOfSupportedDevices; index++)
		if (vendorId == klsiAdapterList[index][0])
		    if (productId == klsiAdapterList[index][1])
			break;
	  
	    if (index == noOfSupportedDevices )
		{

		/* device not supported */

		KLSI_LOG (KLSI_DBG_ATTACH, 
                          " Unsupported device found vId %0x; pId %0x! \n", 
			  vendorId, productId, 0, 0, 0, 0);
		break;
		}
		
  	    KLSI_LOG (KLSI_DBG_ATTACH, 
                      " Found a KLSI Adapter!  %0x  %0x\n", 
                      vendorId, productId, 0, 0, 0, 0);
	
	    /* 
	     * Now create a strcture for the newly found device and add 
	     * it to the linked list 
	     */

            /* Try to allocate space for a new device struct */

	    if ((pNewDev = OSS_CALLOC (sizeof (*pNewDev))) == NULL)
 	    	{
		break;	    	
		}

	    /* Fill in the device structure */
	
	    pNewDev->nodeId = nodeId;
	    pNewDev->configuration = configuration;
	    pNewDev->interface = interface;
	    pNewDev->vendorId = vendorId;
	    pNewDev->productId = productId;

	    /* Add this device to the linked list */
	
	    usbListLink (&klsiDevList, pNewDev, &pNewDev->devLink, 
			 LINK_TAIL);

	 /* Notify registered callers that a klsi device has been added */

	    notifyAttach (pNewDev->nodeId, USB_KLSI_ATTACH); 	/*##*/

	    break;

	case USBD_DYNA_REMOVE:

	    KLSI_LOG (KLSI_DBG_ATTACH, "Device Removed  %x \n", (int) nodeId,
		    0, 0, 0, 0, 0);
	
	  /* First Ensure that this device is on the list */
	
	    if ((pNewDev = klsiFindDevice (nodeId)) == NULL)
	        break;

	     /* Check the connected flag  */

            if (pNewDev->connected == FALSE)
                break;    
	     	
	     pNewDev->connected = FALSE;

	    /* Notify registered callers that the klsi device has been
	     * removed 
	     *
	     * NOTE: We temporarily increment the device's lock count
	     * to prevent usbKlsiDevUnlock() from destroying the
	     * structure while we're still using it.
	     */

            pNewDev->lockCount++; 

            notifyAttach (pNewDev->nodeId, USB_KLSI_REMOVE); 

            pNewDev->lockCount--; 

	    if (pNewDev->lockCount == 0)
		klsiDestroyDevice((KLSI_DEVICE *)pNewDev->pDevStructure);	

	    break;

	}

    OSS_MUTEX_RELEASE (klsiMutex);

    }
		
/***************************************************************************
*
* usbKlsiEndInit - Initializes the klsi Library
*
* Initizes the klsi Library. The Library maintains an initialization
* count so that the calls to this function can be nested.
*
* This function initializes the system resources required for the library
* initializes the linked list for the ethernet devices found.
* This function registers the library as a client for the USBD calls and 
* registers for dynamic attachment notification of USB communication device
* class and Ethernet sub class of devices.
*
* This function is to be called after the USBD initialization and before 
* the endStart gets called. Otherwise the Library can't perform.
*
* RETURNS : OK or ERROR
*
* ERRNO :
*
* S_klsiLib_OUT_OF_RESOURCES
* S_klsiLib_USBD_FAULT
*/

STATUS usbKlsiEndInit (void)
    {

 
    /* see if already initialized. if not, initialize the library */

    initCount++;

    if(initCount != 1)	/* already registered */
	return OK;

    /* 
     * Initialize USBD
     * The assumption is made that the USB host stack has already been
     * initialized elsewhere.  
     */
    /* usbdInitialize(); */

    memset (&klsiDevList, 0, sizeof (klsiDevList));
    
    klsiMutex = NULL;
    klsiTxMutex = NULL;
    klsiRxMutex = NULL;

    klsiHandle = NULL;

    /* create the mutex */

    if (OSS_MUTEX_CREATE (&klsiMutex) != OK)
	return klsiShutdown (S_usbKlsiLib_OUT_OF_RESOURCES);
    
    if (OSS_MUTEX_CREATE (&klsiTxMutex) != OK)
	return klsiShutdown (S_usbKlsiLib_OUT_OF_RESOURCES);
    
    if (OSS_MUTEX_CREATE (&klsiRxMutex) != OK)
	return klsiShutdown (S_usbKlsiLib_OUT_OF_RESOURCES);

    /* 
     * Register the Library as a Client and register for 
     * dynamic attachment callback.
     */

    /* 
     if((usbdClientRegister (KLSI_CLIENT_NAME, &klsiHandle) != OK) ||
	usbdDynamicAttachRegister (klsiHandle, 
				   USB_CLASS_COMMDEVICE, 
				   USB_SUBCLASS_ENET, 
				   USBD_NOTIFY_ALL, 
				   klsiAttachCallback) 
				!= OK))
	{
    	return klsiShutdown (S_klsiLib_USBD_FAULT);
	}
    */
    /*
     * The above registration doesn't work for KLSI chip based adapters.
     * This is because the chip doesn't show up as a communications class
     * device. It shows up as  a proprietary device. Decifering that a 
     * given device is a KLSI device is done from its product ID and
     * vendor ID. This is done in the dynamic attachment call back function.
     */

     if((usbdClientRegister (KLSI_CLIENT_NAME, &klsiHandle) != OK) ||
	(usbdDynamicAttachRegister (klsiHandle, 
				    USBD_NOTIFY_ALL, 
				    USBD_NOTIFY_ALL, 
				    USBD_NOTIFY_ALL, 
				    (USBD_ATTACH_CALLBACK) klsiAttachCallback)
				 != OK))
	{
	logMsg(" Registration Failed..\n", 0, 0, 0, 0, 0, 0);
    	return klsiShutdown (S_usbKlsiLib_USBD_FAULT);
	}
    
    return OK;
    }

/***************************************************************************
*
* findEndpoint - Searches for a BULK endpoint of the indicated direction.
*
* RETURNS: pointer to matching endpoint descriptor or NULL if not found
* 
*/

LOCAL pUSB_ENDPOINT_DESCR findEndpoint
    (
    pUINT8 pBfr,		/* buffer to search for */
    UINT16 bfrLen,		/* buffer length */
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

/***************************************************************************
*
* klsiDevInit - Initializes the klsi Device structure.
*
* This function initializes the USB ethernet device. It is called by 
* klsiEndLoad() as part of the end driver initialization. klsiEndLoad()
* expects this routine to perform all the device and USB specific 
* initialization and fill in the corresponding member fields in the 
* KLSI_DEVICE structure.
*
* This function first checks to see if the device corresponding to
* <vendorId> and <productId> exits in the linkedlist klsiDevList.
* It allocates memory for the input and output buffers. The device 
* descriptors are retrieved andused to findout the IN and OUT
* bulk end points. Once the end points are found, the corresponding
* pipes are constructed. The Pipe handles are stored in the device 
* structure <pDevCtrl>. The device's Ethernet Address (MAC address) 
* is retrieved and the corresponding field in the device structure
* is updated. This is followed by setting up of the parameters 
* like Multicast address filter list, Packet Filter bitmap etc.
*
* RETURNS : OK or ERROR
*
*/

STATUS klsiDevInit
    (
    KLSI_DEVICE* pDevCtrl,	/* the device structure to be updated */
    UINT16 vendorId,		/* manufacturer id of the device */
    UINT16 productId    	/* product id of the device */
    )
    {

    USB_KLSI_DEV* pNewDev;

    pUSB_CONFIG_DESCR pCfgDescr;	

    pUSB_INTERFACE_DESCR pIfDescr;

    pUSB_ENDPOINT_DESCR pOutEp;
    pUSB_ENDPOINT_DESCR pInEp;
    
    KLSI_ENET_IRP * pIrpBfrs;
	
    UINT8 bfr[30];   

    pUINT8 pBfr;

    UINT8** pInBfr;
    UINT16 actLen;
    
    int index = 0;

    if(pDevCtrl == NULL)
	{
	KLSI_LOG (KLSI_DBG_INIT,"Null Device \n", 0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    /* Find if the device is in the found devices list */
     
    if ((pNewDev = klsiEndFindDevice (vendorId,productId)) == NULL)
	{
	printf("Could not find KLSI device.\n");
	return ERROR;
	}

    /* Link the End Structure and the device that is found */

    pDevCtrl->pDev = pNewDev;

    /* Allocate memory for the input and output buffers..*/

    if ((pIrpBfrs = (KLSI_ENET_IRP *) memalign (sizeof(ULONG), \
						pDevCtrl->noOfIrps * \
						sizeof (KLSI_ENET_IRP))) \
					 == NULL)
	{
	KLSI_LOG (KLSI_DBG_INIT,"Could not allocate memory for IRPs.\n",
		     0, 0, 0, 0, 0, 0);		
	return ERROR;
	}

    if ((pInBfr = (pUINT8*) memalign (sizeof(ULONG),
			   	     pDevCtrl->noOfInBfrs * sizeof (char *))) \
			    	==NULL)
	{
	KLSI_LOG (KLSI_DBG_INIT,"Could Not align Memory for pInBfrs ...\n",
	         0, 0, 0, 0, 0, 0);			
        return ERROR;
        }

    for (index=0;index<pDevCtrl->noOfInBfrs;index++)
	{
	if ((pInBfr[index] = (pUINT8)memalign(sizeof(ULONG),
	   KLSI_IN_BFR_SIZE+8)) == NULL)
	   {
	   KLSI_LOG (KLSI_DBG_INIT,"Could Not align Memory for InBfrs  %d...\n",
		    index, 0, 0, 0, 0, 0);
	   return ERROR;
	   }
	}

    pDevCtrl->pEnetIrp = pIrpBfrs;	
    pDevCtrl->pInBfrArray = pInBfr;

    for (index = 0; index < pDevCtrl->noOfIrps; index++)
        {
        pIrpBfrs->outIrpInUse = FALSE;
        pIrpBfrs ++;
        }

    pDevCtrl->rxIndex = 0;    
    pDevCtrl->txIrpIndex = 0;

    pDevCtrl->outBfrLen = KLSI_OUT_BFR_SIZE;
    pDevCtrl->inBfrLen = KLSI_IN_BFR_SIZE;
   
    /* Do the device specific initialization */
    
    klsiInit (pDevCtrl);
	
    /* 
     * Decifer the descriptors provided by the device 
     * and try to find out which end point is what.
     * Here it is assumed that there aren't any alternate 
     * settings for interfaces.
     */

    /* To start with, get the configuration descriptor */

    if (usbdDescriptorGet (klsiHandle, 
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
	KLSI_LOG (KLSI_DBG_INIT, "Could not GET Descriptor.\n",
		     0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    if ((pCfgDescr = (pUSB_CONFIG_DESCR) 
	usbDescrParse (bfr, actLen, USB_DESCR_CONFIGURATION)) == NULL)
	{
	KLSI_LOG (KLSI_DBG_INIT, "Could not find Config. Descriptor.\n",
		     0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    pBfr = bfr;

    /*
     * Since we registered for NOTIFY_ALL for attachment of devices,
     * the configuration no. and interface number as reported by the 
     * call back function doesn't have any meaning. The KLSI document 
     * says that it has only one interface with number 0. So the first 
     * (and only) interface found is the interface desited. 
     *  
     * If there are more interfaces and one of them meet our requirement
     * (as reported by callback function), then we need to parse
     * until the desired interface is found..
     */

    /* 
     * pBfr and bfr are needed in case there are multiple interfaces,
     * to allow multiple parses.
     */
        
    if ((pIfDescr = (pUSB_INTERFACE_DESCR) 
	usbDescrParseSkip (&pBfr, &actLen, USB_DESCR_INTERFACE)) == NULL)
	{
	KLSI_LOG (KLSI_DBG_INIT, "Could not find Interface Descriptor.\n",
		     0, 0, 0, 0, 0, 0);
    	return ERROR;
	}
    
     /* Find out the output and input end points ... */

    if ((pOutEp = findEndpoint (pBfr, actLen, USB_ENDPOINT_OUT)) == NULL)
	{
	KLSI_LOG (KLSI_DBG_INIT, "No Output End Point. \n",
		     0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    if ((pInEp = findEndpoint (pBfr, actLen, USB_ENDPOINT_IN)) == NULL)
	{
	KLSI_LOG (KLSI_DBG_INIT, "No Input End Point. \n",
		     0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    pDevCtrl->maxPower = pCfgDescr->maxPower; 

    /*
     * Now, set the configuration.
     * Note that any ConfigurationSet request will reset the device.
     * Thus, an explicit device reset is not required 
     */

    if (usbdConfigurationSet (klsiHandle, 
			      pNewDev->nodeId, 
			      pCfgDescr->configurationValue, 
			      pCfgDescr->maxPower * USB_POWER_MA_PER_UNIT) 
			   != OK)
	return ERROR;
 
    /* Now, Create the Pipes... */

    if (usbdPipeCreate (klsiHandle, 
			pNewDev->nodeId, 
			pOutEp->endpointAddress, 
			pCfgDescr->configurationValue, 
			pNewDev->interface, 
			USB_XFRTYPE_BULK, 
			USB_DIR_OUT, 
			FROM_LITTLEW (pOutEp->maxPacketSize), 
			0, 
			0, 
			&pDevCtrl->outPipeHandle) 
		!= OK)	
	{
	KLSI_LOG (KLSI_DBG_INIT, "Output Pipe could not be created. \n",
		     0, 0, 0, 0, 0, 0);
	return ERROR;
	}
    
    if (usbdPipeCreate (klsiHandle, 
			pNewDev->nodeId, 
			pInEp->endpointAddress, 
			pCfgDescr->configurationValue, 
			pNewDev->interface, 
			USB_XFRTYPE_BULK, 
			USB_DIR_IN, 
			FROM_LITTLEW (pInEp->maxPacketSize), 
			0, 
			0, 
			&pDevCtrl->inPipeHandle) 
		    != OK)       
	{
	KLSI_LOG (KLSI_DBG_INIT, "Input Pipe could not be created \n",
	     	     0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    pDevCtrl->inIrpInUse = FALSE;

    /* 
     * Read the (Ethernet) Function Descriptor and get the details.
     * Though this is described as a "descriptor", it is a vendor specific 
     * command for KLSI and is a control Pipe 0 transaction.
     */
   
    if (usbdDescriptorGet (klsiHandle, 
			   pNewDev->nodeId, 
			   USB_RT_STANDARD | USB_RT_DEVICE, 
			   USB_DESCR_STRING, 
			   1, 
			   0x409, 
			   sizeof (bfr), 
			   bfr, 
			   &actLen) 
			!= OK)
	{
	KLSI_LOG (KLSI_DBG_INIT, "Could not GET String Descriptor.\n",
		     0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    /* UNICODE to ASCII conversion and then finally to MAC Address */

    if (usbdVendorSpecific (klsiHandle, 
 			    pNewDev->nodeId, 
 			    USB_RT_VENDOR |USB_RT_DEV_TO_HOST, 
 			    USB_REQ_KLSI_ETHDESC_GET, 
 			    0, 
 			    0, 
 			    sizeof(bfr), 
 			    bfr, 
 			    &actLen) 
 			  != OK)
	{
	KLSI_LOG (KLSI_DBG_INIT, "Error retrieving Ethernet descriptor. \n",
                  0, 0, 0, 0, 0, 0);
	return ERROR;
	}

#if 0
    printf("Eth Desc = "); 
    for(index=0;index<actLen;index++)
	printf("%x ",bfr[index]);
    printf("\n");
#endif

    for (index = 0; index< 6; index++)
	{
	pDevCtrl->macAdrs[index] = bfr[3+index];
	}
#if 0    
    printf("MAC Addrs = "); 
    for(index=0;index<6;index++)
	printf("%x ",pDevCtrl->macAdrs[index]);
    printf("\n");
#endif

 
    /* Set URB Size */

    if (usbdVendorSpecific (klsiHandle, 
			    pNewDev->nodeId, 
			    USB_RT_VENDOR |USB_RT_HOST_TO_DEV, 
			    USB_REQ_KLSI_SET_URB_SIZE, 
			    64, 
			    0, 
			    0, 
			    NULL, 
			    NULL) 
			!= OK)
	{
	KLSI_LOG (KLSI_DBG_INIT, "Error Setting URB Size. \n",
		  0, 0, 0, 0, 0, 0);
	return ERROR;
	}

     
    if (usbdVendorSpecific (klsiHandle, 
			    pNewDev->nodeId, 
			    USB_RT_VENDOR |USB_RT_HOST_TO_DEV, 
			    USB_REQ_KLSI_SET_SOFS_TO_WAIT, 
			    0x1, 
			    0, 
			    0, 
			    NULL, 
			    NULL) 
			  != OK)
	{
	KLSI_LOG (KLSI_DBG_INIT, "Error setting SOFs \n",
		  0, 0, 0, 0, 0, 0);
	return ERROR;
	}
    
    /* Set No Multicasting */

    if (usbdVendorSpecific (klsiHandle, 
			    pNewDev->nodeId, 
			    USB_RT_VENDOR |USB_RT_HOST_TO_DEV, 
			    USB_REQ_KLSI_SET_MCAST_FILTER, 
			    0, 
			    0, 
			    0, 
			    NULL, 
			    NULL) 
			  != OK)
	{
	KLSI_LOG (KLSI_DBG_INIT, "Error setting Multicast Addresses \n",
		  0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    /* Set Only directed Packets to be received */
    
    if (usbdVendorSpecific (klsiHandle, 
			    pNewDev->nodeId, 
			    USB_RT_VENDOR |USB_RT_HOST_TO_DEV, 
			    USB_REQ_KLSI_SET_PACKET_FILTER, 
			    0x0004, 
			    0, 
			    0, 
			    NULL, 
			    NULL) 
			 != OK)
	{
	KLSI_LOG (KLSI_DBG_INIT, "Error setting packet bitmap. \n",
		     0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    return OK;

    }

/***************************************************************************
*
* klsiEndStart - Starts communications over Ethernet via USB (device)
*
* Since there is no interrupt available, the device is to be prevented from 
* communicating in some other way. Here, since the reception is based 
* on polling and the klsiListenForInput() is called for such polling,
* the listening can be delayed to any packet coming in  by calling
* klsiListenForInput(). This way, the traditional "interrupt enabling"   
* is mimicked. This is for packet reception.
*
* For packet transmission, communicateOk flag is used. Data is transmitted 
* only if this flag is true. This flag will be set to TRUE and will be
* reset to FALSE in klsiEndStop().
*
* RETURNS : OK or ERROR
*/

STATUS klsiEndStart
    (
    KLSI_DEVICE* pDevCtrl	        /* Device to be started */
    )
    {

    KLSI_LOG (KLSI_DBG_START, "Entered klsiEndStart.\n",
		 0, 0, 0, 0, 0, 0);
    
    /*
     * start listening on the BULK input pipe for any  
     *  ethernet packet coming in. This is how   
     * "connecting the interrupt" of the END Model is simulated.
     */

    pDevCtrl->communicateOk = TRUE;

    if (klsiListenForInput (pDevCtrl) != OK)
	{
        pDevCtrl->communicateOk = FALSE;
        KLSI_LOG (KLSI_DBG_START, 
                  "klsiEndStart:..Unable to listen for input...\n",
                  0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    /* 
     * The above will effectively preempt any possibility of packet reception.
     * For preempting such possibility for transmission, the communicateOk 
     * flag is used. Ideally a semaphore should be used here, but
     * the use of a flag will suffice.
     */

    return OK;

    }

/***************************************************************************
*
* klsiEndStop - Disables communication over Ethernet via USB (device)
* 
* The pipes will be aborted. If there are any pending transfers on these
* Pipes, USBD will see to it that they are also cancelled. The IRPs for
* these transfers will return a S_usbHcdLib_IRP_CANCELED error code.
* It waits for the IRPs to be informed of their Cancelled Status and then
* the function returns.
* 
* RETURNS: OK or ERROR
*/

STATUS klsiEndStop
    (
    KLSI_DEVICE* pDevCtrl	    /* Device to be Stopped */
    )
    {

    KLSI_LOG (KLSI_DBG_STOP, "klsiEndStop:..entered.\n",
	      0, 0, 0, 0, 0, 0);

    /* 
     * Abort the transfers the input and output Pipes.
     * once such requests are issued, usbd will take care of aborting the 
     * outstanding transfers, if any, associated with the Pipes.
     */

    pDevCtrl->communicateOk = FALSE;
    klsiDestroyDevice(pDevCtrl);
    taskDelay(sysClkRateGet()*5);
    return OK;

    }

/***************************************************************************
* klsiListenForInput - Listens for data on the ethernet (Bulk In Pipe) 
*
* Input IRP will be initialized to listen on the BULK input pipe and will
* be submitted to the usbd.
*
* RETURNS : OK or ERROR
*
* 
*/
     
LOCAL STATUS klsiListenForInput
    (
    KLSI_DEVICE* pDevCtrl		/* device to receive from */
    )
    {

    pUSB_IRP pIrp = &pDevCtrl->inIrp;		


    if (pDevCtrl == NULL)
	return ERROR;

    /* Initialize IRP */

    memset (pIrp, 0, sizeof (*pIrp));		

    pIrp->userPtr = pDevCtrl;
    pIrp->irpLen = sizeof (*pIrp);
    pIrp->userCallback = klsiRxCallback;
    pIrp->timeout = USB_TIMEOUT_DEFAULT;
    pIrp->transferLen = pDevCtrl->inBfrLen; 
    pIrp->flags = USB_FLAG_SHORT_OK;

    pIrp->bfrCount = 1;

    pIrp->bfrList[0].pid = USB_PID_IN;
    pIrp->bfrList[0].bfrLen = pDevCtrl->inBfrLen; 
    pIrp->bfrList[0].pBfr = (pUINT8)pDevCtrl->pInBfrArray[pDevCtrl->rxIndex] - 2;  

    /* Submit IRP */

    if (usbdTransfer (klsiHandle, pDevCtrl->inPipeHandle, (pUSB_IRP)pIrp) != OK)
	return ERROR;

    pDevCtrl->inIrpInUse = TRUE;

    return OK;
    }

/***************************************************************************
*
* klsiSend - Initiates data transmission to the device
*
* This function initiates transmission on the ethernet.
*
* RETURNS: OK or ERROR
*/

STATUS klsiSend
    (
    KLSI_DEVICE* pDevCtrl,		/* device to send to */
    UINT8* pBfr,			/* data to send */
    UINT32 size				/* data size */
    )
    {

    pUSB_IRP pIrp;       /* = &pDevCtrl->outIrp; */
    KLSI_ENET_IRP * pIrpBfr;	

    KLSI_LOG (KLSI_DBG_TX, "klsiSend:..entered. %d bytes\n", 
	      size, 0, 0, 0, 0, 0); 

    if ((pDevCtrl == NULL) || (pBfr == NULL))
	{
	return ERROR;
	}

    if (size == 0)
	return ERROR;
	

    pIrpBfr = (KLSI_ENET_IRP *)(pDevCtrl->pEnetIrp + pDevCtrl->txIrpIndex);
    pIrp =(pUSB_IRP) &pIrpBfr->outIrp;		
    pIrpBfr->outIrpInUse = TRUE;
    pDevCtrl->txIrpIndex++;
    pDevCtrl->txIrpIndex %= pDevCtrl->noOfIrps;

    /* Initialize IRP */

    memset (pIrp, 0, sizeof (*pIrp));

    pIrp->userPtr = pDevCtrl;
    pIrp->irpLen = sizeof (*pIrp);
    pIrp->userCallback = klsiTxCallback;
    pIrp->timeout = USB_TIMEOUT_NONE;
    pIrp->transferLen = size;

    pIrp->bfrCount = 1;
    pIrp->bfrList[0].pid = USB_PID_OUT;
    pIrp->bfrList[0].pBfr = pBfr;
    pIrp->bfrList[0].bfrLen = size;

    /* Submit IRP */

    KLSI_LOG (KLSI_DBG_TX, "klsiSend:..Sumitting IRP for Trans. %d bytes\n", 
		 size, 0, 0, 0, 0, 0); 


    if (usbdTransfer (klsiHandle, 
			pDevCtrl->outPipeHandle, 
			(pUSB_IRP)pIrp) 
			!= OK)
	return ERROR;				

    KLSI_LOG (KLSI_DBG_TX, "klsiSend:..done !!!!!!\n", 
		 0, 0, 0, 0, 0, 0); 

/*    OSS_MUTEX_RELEASE (klsiMutex); */
 

    return OK;
    }

/***************************************************************************
*
* klsiTxCallback - Invoked upon Transmit IRP completion/cancellation
*
* RETURNS : N/A
*
* 
*/


LOCAL void klsiTxCallback
    (
    pVOID p			/* completed IRP */
    )

    {
    pUSB_IRP pIrp = (pUSB_IRP) p;

    KLSI_ENET_IRP * pIrpBfr;
    int index = 0;

    KLSI_DEVICE* pDevCtrl = pIrp->userPtr;

    /* Output IRP completed */

    for (index = 0; index < pDevCtrl->noOfIrps; index++)
	{
	
	pIrpBfr = pDevCtrl->pEnetIrp + index;	
	if (pIrp ==(pUSB_IRP) &pIrpBfr->outIrp)		
	    {
	    break;
	    }
	}
   
    if (index == pDevCtrl->noOfIrps)
	{
	return;
	}
 
    KLSI_LOG (KLSI_DBG_TX, "Tx Callback for  %d IRP.\n",
	    index, 0, 0, 0, 0, 0);

    pIrpBfr = pDevCtrl->pEnetIrp + index;
    pIrpBfr->outIrpInUse = FALSE;


	     	
    free (pIrp->bfrList[0].pBfr);		

    if (pIrp->result != OK)
	{
        KLSI_LOG (KLSI_DBG_TX, "Tx error %x.\n",
	    pIrp->result, 0, 0, 0, 0, 0);

	if (pIrp->result == S_usbHcdLib_STALLED)
	    {
  	    if (usbdFeatureClear (klsiHandle, 
				  pDevCtrl->pDev->nodeId,
				  USB_RT_STANDARD | USB_RT_ENDPOINT, 
				  0, 
				  1) == ERROR)
		{
	        KLSI_LOG (KLSI_DBG_TX, "Could not clear STALL.\n",
	  	          pIrp->result, 0, 0, 0, 0, 0);
		}
	    }
  
        pDevCtrl->outErrors++;	/* Should also Update MIB */
	}
    else
	{
            KLSI_LOG (KLSI_DBG_TX, "Tx finished.\n",
		      0, 0, 0, 0, 0, 0);
	}
	
    }

/***************************************************************************
*
* klsiRxCallback - Invoked when a Packet is received.
*
*
* RETURNS : N/A
*
* 
*/

LOCAL void klsiRxCallback
    (
    pVOID p			/* completed IRP */
    )

    {
    pUSB_IRP pIrp = (pUSB_IRP) p;
    KLSI_DEVICE* pDevCtrl = pIrp->userPtr;
    BOOL	irpStalled = FALSE;	

    OSS_MUTEX_TAKE (klsiMutex, OSS_BLOCK);	

    /* Input IRP completed */

    pDevCtrl->inIrpInUse = FALSE;

    /*
     * If the IRP was successful then pass the data back to the client.
     * Note that the netJobAdd() is not necessary here as the function 
     * is not getting executed in isr context.
     * If irp STALL occurs and could not be cleared then we do not submit 
     * next Irp for listening input.		
     */
    if (pIrp->result != OK)
	{
        pDevCtrl->inErrors++;	/* Should also update MIB */
  
	if(pIrp->result == S_usbHcdLib_STALLED)
	    {
	    if(usbdFeatureClear (klsiHandle,
				 pDevCtrl->pDev->nodeId,
				 USB_RT_STANDARD | USB_RT_ENDPOINT, 
				 0, 
				 0) !=OK)
 		{		
	        KLSI_LOG (KLSI_DBG_TX, "Irp STALLED ..Could not clear\n",
	                  0, 0, 0, 0, 0, 0);
		irpStalled = TRUE;
		}
	    }
	}
    else
	{
	if( pIrp->bfrList[0].actLen >= 2)
	    {
  	    klsiEndRecv (pDevCtrl,(pUINT8)(pIrp->bfrList[0].pBfr+2), 
			pIrp->bfrList[0].actLen-2);			 		
	    pDevCtrl->rxIndex++;
	    pDevCtrl->rxIndex %= pDevCtrl->noOfInBfrs;    
	    }
	}       

    /*
     * Unless the IRP was cancelled,stalled - implying the channel is being
     * torn down, re-initiate the "in" IRP to listen for more data.
     */

    if ((pIrp->result != S_usbHcdLib_IRP_CANCELED) && (irpStalled !=TRUE))
	klsiListenForInput (pDevCtrl);
    else
        KLSI_LOG (KLSI_DBG_TX, "Rxcallback....pIrp->result %d\n",
	             pIrp->result, 0, 0, 0, 0, 0);

    OSS_MUTEX_RELEASE (klsiMutex);	 
	
    }


/***************************************************************************
*
* klsiMCastFilterSet - Sets a Multicast Address Filter for the device
*
* Even if the host wishes to change a single multicast filter in the device,
* it must reprogram the entire list of filters using this function. 
* <pAddress> shall contain a pointer to this list of multicast addresses
* and <noOfFilters> shall hold the number of multicast address filters
* being programmed to the device.
*
* RETURNS : OK or ERROR
*/

STATUS klsiMCastFilterSet
    (
    KLSI_DEVICE* pDevCtrl,	/* device to add the mcast filters */
    UINT8* pAddress,		/* Mcast address filters list */
    UINT32 noOfFilters		/* no. of filters to add */
    )
    {
    
    KLSI_LOG (KLSI_DBG_MCAST, "klsiMCasrFilterSet:..entered.\n",
		 0, 0, 0, 0, 0, 0);

    if (pDevCtrl == NULL)
	return ERROR;

    /* Check if  this many number of Filters are supported by the device */

    if ((noOfFilters < 0) || 
	(noOfFilters > (pDevCtrl->mCastFilters.noMCastFilters)))
	return ERROR;

    if (((noOfFilters == 0) && (pAddress != NULL)) ||
	((noOfFilters > 0) && (pAddress == NULL)))
	return ERROR;
    
    /* Set the Filters */

    if (usbdVendorSpecific (klsiHandle, 
			    pDevCtrl->pDev->nodeId,
			    USB_RT_VENDOR | USB_RT_HOST_TO_DEV, 
			    USB_REQ_KLSI_SET_MCAST_FILTER, 
			    noOfFilters, 
			    0, 
			    noOfFilters * 6, 
			    pAddress, 
			    NULL) != OK)
	{
	return ERROR;
	}

    return OK;

    }

/***************************************************************************
*
* klsiPacketFilterSet - Sets a Ethernet Packet Filter for the device
*
* This function configures the Ethernet device for reception of various 
* kinds of ethernet packets. <bitmap> is an inclusive OR of the BITs
* listed below. Additional Filtering, if needed must be performed in the 
* host software.
* 
* .IP "PACKET_TYPE_MULTICAST"
* This bit will enable all multicast packets enumerated in the
* devices multicast address list to be forwarded to the host.
* .IP "PACKET_TYPE_BROADCAST"
* This bit enables all broadcast packets to be forwarded to host.
* .IP "PACKET_TYPE_DIRECTED"	
* This bit enables all normal packets to be forwarded to host.
* .IP "PACKET_TYPE_ALL_MULTICAST"
* This bit enables all multi cast packets to be forwarded to host.
* .IP "PACKET_TYPE_PROMISCOUS"	
* This bit enables all packets to be forwarded to host.
* 
* The Rest of the bits in <bitmap> shall be reset to Zero.
* 
* RETURNS: OK or ERROR
*/

STATUS klsiPacketFilterSet
    (
    KLSI_DEVICE* pDevCtrl,		/* device to set he packet filter */
    UINT8 bitmap			/* packet filter bitmap */
    )
    {

    UINT16 pktFilterBitmap = 0x0000;    

    KLSI_LOG (KLSI_DBG_MCAST, "klsiPacketFilterSet:..entered.\n",
		 0, 0, 0, 0, 0, 0);

    if (pDevCtrl == NULL)
	return ERROR;

    bitmap |= PACKET_TYPE_DIRECTED;	/* this is always maintained  */

    pktFilterBitmap |= bitmap;

    /* Set the Filters */

    if (usbdVendorSpecific (klsiHandle, 
			    pDevCtrl->pDev->nodeId,
			    USB_RT_VENDOR | USB_RT_HOST_TO_DEV, 
			    USB_REQ_KLSI_SET_PACKET_FILTER, 
			    pktFilterBitmap, 
			    0, 
			    0, 
			    NULL, 
			    NULL) != OK)
	{
	return ERROR;
	}

    return OK;
       
    }   

/***************************************************************************
*
* klsiStatsGet - Gets the statistics from the device
* 
* RETURNS : OK or ERROR 
*
* 
*/

STATUS klsiStatsGet
    (
    KLSI_DEVICE* pDevCtrl,		/* device to retrieve the Statistic */
    UINT8 feature,			/* statistic to get */
    UINT8* pStat			/* place to store the stats */
    )
    {

    KLSI_LOG (KLSI_DBG_MCAST, "Entered klsiStatsGet.\n",
		 0, 0, 0, 0, 0, 0);

    if (pDevCtrl == NULL)
	return ERROR;

    /* see if this particular statistic is supported by the device */

    if ((pDevCtrl->stats.bitmap & (0x1 << feature)) == 0)
	return ERROR;


    if (usbdVendorSpecific (klsiHandle, 
			    pDevCtrl->pDev->nodeId,
	                    USB_RT_VENDOR | USB_RT_DEV_TO_HOST, 
	                    USB_REQ_KLSI_GET_STATS, 
			    feature, 
                            0, 
			    4, 
			    pStat, 
			    0) != OK)
	{
	return ERROR;
	}

    /* the driver structure is not updated now */

    return OK;
    
    }   


/***************************************************************************
*
* klsiReset - Resets the USB-ethernet adapter
*
* This function resets the Ethernet Adapter. 
*
* RETURNS : OK or ERROR
*/

STATUS klsiReset
    (
    KLSI_DEVICE* pDevCtrl	/* device to reset */
    )

    {

    KLSI_LOG (KLSI_DBG_RESET, "Entered klsiReset.\n",
		 0, 0, 0, 0, 0, 0);

    if (pDevCtrl == NULL)
	return ERROR;

    /* SET_CONFIGURATION command shall cause the device to reset */

    if (usbdConfigurationSet (klsiHandle, 
				pDevCtrl->pDev->nodeId,
				0, 
				pDevCtrl->maxPower * USB_POWER_MA_PER_UNIT) 
				!= OK)
	{
        KLSI_LOG (KLSI_DBG_RESET, " Could not reset the device. \n",
		  0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    return OK;
    
    }  
  

/***************************************************************************
*
* klsiFindDevice - Searches for a USB ethernet device for indicated <nodeId>
*
* RETURNS: pointer to matching device structure, or NULL if device not found
* 
*/

USB_KLSI_DEV* klsiFindDevice
    (
    USBD_NODE_ID nodeId		/* Node Id to find */
    )

    {

    USB_KLSI_DEV * pDev = usbListFirst (&klsiDevList);

    while (pDev != NULL)
	{
	if (pDev->nodeId == nodeId)
	    break;

	pDev = usbListNext (&pDev->devLink);
	}

    return pDev;
    }

/***************************************************************************
*
* klsiEndFindDevice - Searches USB ethernet device 
* 
*
* RETURNS: pointer to matching device structure, or NULL if not found
* 
*/

USB_KLSI_DEV* klsiEndFindDevice
    (
    UINT16 vendorId,		/* Vendor Id to search for */
    UINT16 productId		/* Product Id to search for */
    )

    {
    USB_KLSI_DEV * pDev = usbListFirst (&klsiDevList);

    while (pDev != NULL)
	{
	if ((pDev->vendorId == vendorId) && (pDev->productId == productId))
	    break;

	pDev = usbListNext (&pDev->devLink);
	}

    return pDev;
    }
  
     
/***************************************************************************
*
* klsiMacAddressSet - Sets a Ethernet Address for the device
*
* This function will do the address setting for the given device.
*
* RETURNS : OK or ERROR 
*/

STATUS klsiMacAddressSet
    (
    KLSI_DEVICE* pDevCtrl,  /* Device to set the address for */
    UINT8 * pAddress	    /* The MAC Address to be Set */
    )
   
    {

    KLSI_LOG (KLSI_DBG_MAC, "Entered klsiMacAddressSet...\n",
		 0, 0, 0, 0, 0, 0);

    if ((pDevCtrl == NULL) || (pAddress == NULL))
	return ERROR;

    /* Set the MAC Address */

    if (usbdVendorSpecific (klsiHandle, 
			    pDevCtrl->pDev->nodeId,
	                    USB_RT_VENDOR | USB_RT_HOST_TO_DEV, 
	                    USB_REQ_KLSI_SET_TEMP_MAC, 
			    0, 
			    0, 
			    6, 
			    pAddress,
	                    NULL) != OK)
	{
	return ERROR;
	}

    return OK;
    
    }   
    
/***************************************************************************
*
* klsiShutdown - Shuts down USB EnetLib
*
* <errCode> should be OK or S_klsiLib_xxxx.  This value will be
* passed to ossStatus() and the return value from ossStatus() is the
* return value of this function.
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS klsiShutdown
    (
    int errCode
    )

    {
   
    KLSI_DEVICE * pDev;

    /* Dispose of any open connections. */

    while ((pDev = usbListFirst (&klsiDevList)) != NULL)
	klsiDestroyDevice (pDev);
	
    /*	
     * Release our connection to the USBD.  The USBD automatically 
     * releases any outstanding dynamic attach requests when a client
     * unregisters.
     */

    if (klsiHandle != NULL)
	{
	usbdClientUnregister (klsiHandle);
	klsiHandle = NULL;
	}

    /* Release resources. */

    if (klsiMutex != NULL)
	{
	OSS_MUTEX_DESTROY (klsiMutex);
	klsiMutex = NULL;
	}
    
    if (klsiTxMutex != NULL)
	{
	OSS_MUTEX_DESTROY (klsiTxMutex);
	klsiTxMutex = NULL;
	}
    
    if (klsiRxMutex != NULL)
	{
	OSS_MUTEX_DESTROY (klsiRxMutex);
	klsiRxMutex = NULL;
	}

   
    usbdShutdown();

    return ossStatus (errCode);
    }

     
/***************************************************************************
*
* klsiDestroyDevice - disposes of a KLSI_DEVICE structure
*
* Unlinks the indicated KLSI_DEVICE structure and de-allocates
* resources associated with the channel.
*
* RETURNS: N/A
*/

void klsiDestroyDevice
    (
    KLSI_DEVICE* pDevCtrl
    )
    {
    USB_KLSI_DEV *pDev;

    if (pDevCtrl != NULL)
	{

	pDev = pDevCtrl->pDev;

	/* Unlink the structure. */

	usbListUnlink (&pDev->devLink);
  
	/* Release pipes and wait for IRPs to be cancelled if necessary. */

	if (pDevCtrl->outPipeHandle != NULL)
	    usbdPipeDestroy (klsiHandle, pDevCtrl->outPipeHandle);

	if (pDevCtrl->inPipeHandle != NULL)
	    usbdPipeDestroy (klsiHandle, pDevCtrl->inPipeHandle);

/*	taskDelay(sysClkRateGet()*2);*/


	while (pDevCtrl->pEnetIrp->outIrpInUse || pDevCtrl->inIrpInUse)
	   OSS_THREAD_SLEEP (1);

 	/*  Release Input buffers*/
	
	if ( pDevCtrl->pInBfrArray !=NULL)
	    {	
            OSS_FREE(pDevCtrl->pInBfrArray);
	    taskDelay(sysClkRateGet()*1);
	    }	
	/*  Release EnetIrp buffers*/
	if ( pDevCtrl->pEnetIrp != NULL)
	    {	
           OSS_FREE(pDevCtrl->pEnetIrp);
	   taskDelay(sysClkRateGet()*1);
            }

	/* Release structure. */

	if (pDev !=NULL)
	    OSS_FREE (pDev);

	}
    }


/**************************************************************************
*
* klsiEndLoad - Initialize the driver and device
*
* This routine initializes the driver and the device to the operational state.
* All of the device specific parameters are passed in the initString.
* 
* This function first extracts the vendorId and productId of the device 
* from the initialization string using the klsiEndParse() function. It then 
* passes these parameters and its control structure to the klsiDevInit()
* function. klsiDevInit() does most of the device specific initialization
* and brings the device to the operational state. Please refer to klsiLib.c
* for more details about usbenetDevInit(). This driver will be attached to MUX
* and then the memory initialization of the device is carried out using
* klsiEndMemInit(). 
*
* This function doesn't do any thing device specific. Instead, it delegates
* such initialization to klsiDevInit(). This routine handles the other part
* of the driver initialization as required by MUX.
*
* muxDevLoad() calls this function twice. First time this function is called, 
* initialization string will be NULL . We are required to fill in the device 
* name ("usb") in the string and return. When next time this function is called,
* the initialization string will be proper.
*
* <initString> will be of the format :
* "unit : vendorId : productId : noOfInBfrs : noOfIrps"
*
* PARAMETERS
*
* .IP <initString>
* The device initialization string.
*
* RETURNS: An END object pointer, or NULL on error.
*/

END_OBJ * klsiEndLoad
    (
    char * initString	                            /* initialization string */
    )
    {
    KLSI_DEVICE * pDrvCtrl;                      /* driver structure */

    UINT16 vendorId;                                /* vendor information */
    UINT16 productId;                               /* product information */

    KLSI_LOG (KLSI_DBG_LOAD, "Loading usb end...\n", 1, 2, 3, 4, 5, 6);

    if (initString == NULL)
	return (NULL);
    
    if (initString[0] == EOS)
	{

        /* Fill in the device name and return peacefully */

	bcopy ((char *)KLSI_NAME, (void *)initString, KLSI_NAME_LEN);
	return (0);
	}

    /* allocate the device structure */

    pDrvCtrl = (KLSI_DEVICE *)calloc (sizeof (KLSI_DEVICE), 1);


    if (pDrvCtrl == NULL)
	{
	KLSI_LOG (KLSI_DBG_LOAD, "No Memory!!...\n", 1, 2, 3, 4, 5, 6);
	goto errorExit;
	}

    /* parse the init string, filling in the device structure */

    if (klsiEndParse (pDrvCtrl, initString, &vendorId, &productId) == ERROR)
	{
	KLSI_LOG (KLSI_DBG_LOAD, "Parse Failed.\n", 1, 2, 3, 4, 5, 6);
	goto errorExit;
	}

    /* Ask the klsiLib to do the necessary initialization. */

    if (klsiDevInit(pDrvCtrl,vendorId,productId) == ERROR)
	{
	KLSI_LOG (KLSI_DBG_LOAD, "EnetDevInitFailed.\n", 
		    1, 2, 3, 4, 5, 6);
	goto errorExit;
	}

    /* initialize the END and MIB2 parts of the structure */

    if (END_OBJ_INIT (&pDrvCtrl->endObj, 
		      (DEV_OBJ *)pDrvCtrl, 
		      "usb",
                      pDrvCtrl->unit, 
		      &klsiEndFuncTable,
                      KLSI_NAME) == ERROR
     || END_MIB_INIT (&pDrvCtrl->endObj, 
		      M2_ifType_ethernet_csmacd,
                      &pDrvCtrl->macAdrs[0], 
		      6, 
		      KLSI_BUFSIZ,
                      KLSI_SPEED)
		    == ERROR)
	{
	KLSI_LOG (KLSI_DBG_LOAD, "END MACROS FAILED...\n", 
		1, 2, 3, 4, 5, 6);
	goto errorExit;
	}

    /* Perform memory allocation/distribution */

    if (klsiEndMemInit (pDrvCtrl) == ERROR)
	{
	KLSI_LOG (KLSI_DBG_LOAD, "endMemInit() Failed...\n", 
		    1, 2, 3, 4, 5, 6);	

	klsiDestroyDevice(pDrvCtrl);
	goto errorExit;
	}
 
    /* set the flags to indicate readiness */

    END_OBJ_READY (&pDrvCtrl->endObj,
		    IFF_UP | IFF_RUNNING | IFF_NOTRAILERS | IFF_BROADCAST
		    | IFF_MULTICAST);			

    KLSI_LOG (KLSI_DBG_LOAD, "Done loading usb end..\n", 
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


/***************************************************************************
*
* klsiEndParse - parse the init string
*
* Parse the input string.  Fill in values in the driver control structure.
*
* The muxLib.o module automatically prepends the unit number to the user's
* initialization string from the BSP (configNet.h).
* 
* This function parses the input string and fills in the places pointed
* to by <pVendorId> and <pProductId>. Unit Number, no of Irps and no of Input 
* buffers of the string will be stored in the device structure pointed to by 
* <pDrvCtrl>.
*
* .IP <pDrvCtrl>
* Pointer to the device structure.
* .IP <initString>
* Initialization string for the device. It will be of the following format :
* "unit:vendorId:productId:noOfInbfrs:noOfIrps"
* Device unit number, a small integer.
* .IP <pVendorId>
* Pointer to the place holder of the device vendor id.
* .IP <pProductId>
*  Pointer to the place holder of the device product id.
* .IP <noOfInbfrs>
* Holds the number of input buffers.
* .IP <noOfIrps>
*  Holds the number of output IRP's.
*
* RETURNS: OK or ERROR
* 
*/

STATUS klsiEndParse
    (
    KLSI_DEVICE * pDrvCtrl,	/* device pointer */
    char * initString,		/* information string */
    UINT16 * pVendorId,		
    UINT16 * pProductId
    )
    {

    char *	tok;
    char *	pHolder = NULL;
    
    /* Parse the initString */

    /* Unit number. (from muxLib.o) */

    tok = strtok_r (initString, ":", &pHolder);

    if (tok == NULL)
	return ERROR;

    pDrvCtrl->unit = atoi (tok);

    KLSI_LOG (KLSI_DBG_LOAD, "Parse: Unit : %d..\n", 
		pDrvCtrl->unit, 2, 3, 4, 5, 6); 

    /* Vendor Id. */

    tok = strtok_r (NULL, ":", &pHolder);

    if (tok == NULL)
	return ERROR;

    *pVendorId = atoi (tok);

    KLSI_LOG (KLSI_DBG_LOAD, "Parse: VendorId : 0x%x..\n", 
		*pVendorId, 2, 3, 4, 5, 6); 

    /* Product Id. */

    tok = strtok_r (NULL, ":", &pHolder);

    if (tok == NULL)
	return ERROR;

    *pProductId = atoi (tok);
  
    KLSI_LOG (KLSI_DBG_LOAD, "Parse: ProductId : 0x%x..\n", 
		*pProductId, 2, 3, 4, 5, 6); 
	

    /* no of in buffers */

    tok = strtok_r (NULL, ":", &pHolder);

    if (tok == NULL)
	return ERROR;


    pDrvCtrl->noOfInBfrs  = atoi (tok);
  
    KLSI_LOG (KLSI_DBG_LOAD, "Parse: NoInBfrs : %x..\n", 
		pDrvCtrl->noOfInBfrs, 2, 3, 4, 5, 6); 
	
    /* no of out IRPs */

    tok = strtok_r (NULL, ":", &pHolder);

    if (tok == NULL)
	return ERROR;

    pDrvCtrl->noOfIrps = atoi (tok);
  
    KLSI_LOG (KLSI_DBG_LOAD, "Parse: NoOutIrps : %x..\n", 
		 pDrvCtrl->noOfIrps, 2, 3, 4, 5, 6);

    KLSI_LOG (KLSI_DBG_LOAD, "Parse: Processed all arguments\n", 
		1, 2, 3, 4, 5, 6);

    return OK;
    }


/***************************************************************************
*
* klsiEndSend - the driver send routine
*
* This routine takes a M_BLK_ID sends off the data in the M_BLK_ID. The data 
* contained in the MBlks is copied to a character buffer and the buffer is 
* handed over  to klsiSend(). The buffer must already have the addressing 
* information properly installed in it.  This is done by a higher layer.   
* The device requires that the first two bytes of the data sent to it (for 
* transmission over ethernet) contain the length of the data. This is added
* here. 
*
* During the course of testing the driver, it is found that the device is 
* corrupting some bytes of the packet if exact length of the data as handed 
* over by MUX is sent. This is resulting in packet not being received
* by the addressed destination, packet checksum errors etc. The remedy is to
* pad up few bytes to the data packet. This solved the problem.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS klsiEndSend
    (
    KLSI_DEVICE * pDrvCtrl,	/* device ptr */
    M_BLK_ID     pMblk		/* data to send */
    )
    {

    UINT8 *      pBuf; 		/* buffer to hold the data */
    UINT32	noOfBytes;      /* noOfBytes to be transmitted */

#if 0
    int i=0;	
#endif

    KLSI_LOG (KLSI_DBG_TX, "klsiEndSend: Entered.\n", 0, 0, 0, 0, 0, 0);  

 
    if ((pDrvCtrl == NULL) || (pMblk == NULL))
	return ERROR;

/*   pBuf = (UINT8 *) malloc(KLSI_OUT_BFR_SIZE);*/
     pBuf = (UINT8 *) memalign(sizeof(ULONG),KLSI_OUT_BFR_SIZE);
  
    

    if (pBuf == NULL)
	{
	printf(" klsiEndSend : Could not allocate memory \n");
	return ERROR;
	}

    /* copy the MBlk chain to a buffer */

    noOfBytes = netMblkToBufCopy(pMblk,(char *)pBuf+2,NULL); 


    KLSI_LOG (KLSI_DBG_LOAD, "klsiEndSend: %d bytes to be sent.\n", 
		noOfBytes, 0, 0, 0, 0, 0);

    if (noOfBytes == 0) 
	return ERROR;

    /* 
     * Padding : how much to pad is decided by trial and error.
     * Note that there is no need to add any extra bytes in the buffer.
     * since these bytes are not used, they can be any junk 
     * which is already in the buffer.
     * We are just interested in the count.
     */
    
    if (noOfBytes < 60)
	noOfBytes = 60;

    /* (Required by the device) Fill in the Length in the first Two Bytes */

/*    *(UINT16 *)pBuf = noOfBytes; */

    pBuf[0] = (UINT8) (noOfBytes&0x00ff); 
    pBuf[1] = (UINT8) ((noOfBytes >> 8)&0x00ff);

#if 0
    for(i=0;i<noOfBytes+2;i++)
	printf("%x ",pBuf[i]);
    printf("\n"); 
#endif

    /* Transmit the data */

    if (klsiSend (pDrvCtrl, pBuf, noOfBytes+2) == ERROR)
	return ERROR;
 

    KLSI_LOG (KLSI_DBG_TX, "klsiEndSend: Pkt submitted for tx.\n", 
		0, 0, 0, 0, 0, 0);
 
    /* Bump the statistic counter. */

    END_ERR_ADD (&pDrvCtrl->endObj, MIB2_OUT_UCAST, +1);

    /*
     * Cleanup.  The driver frees the packet now.
     */

    netMblkClChainFree (pMblk);	

    return (OK);
    }


/***************************************************************************
*
* klsiEndRecv - process the next incoming packet
*
* klsiEndRecv is called by the klsiRxCallBack() upon successful execution
* of an input IRP. This means some proper data is received. This function  
* will be called with the pointer to be buffer and the length of data.
* What is done here is to construct an MBlk structure with the data received 
* and pass it onto the upper layer.
*
* RETURNS: N/A.
* 
*/

STATUS klsiEndRecv
    (
    KLSI_DEVICE *pDrvCtrl,	/* device structure */
    UINT8*  pData,              /* pointer to data buffer */
    UINT32  len                 /* length of data */
    )
    {
    
    UCHAR *     pNewCluster;    /* Clsuter to store the data */
    CL_BLK_ID	pClBlk;         /* Control block to "control" the cluster */
    M_BLK_ID 	pMblk;          /* and an MBlk to complete a MBlk contract */   
#if 0
    int i=0;
#endif

    /* Add one to our unicast data. */

    END_ERR_ADD (&pDrvCtrl->endObj, MIB2_IN_UCAST, +1);

    pNewCluster = (UCHAR *)pData; 		

#if 0
     for (i=0;i<len;i++)
	printf("%x ",pNewCluster[i]);
     printf("\n");
#endif

    /* Grab a cluster block to marry to the cluster received. */

    if ((pClBlk = netClBlkGet (pDrvCtrl->endObj.pNetPool, M_DONTWAIT)) == NULL)
        {
        netClFree (pDrvCtrl->endObj.pNetPool, pNewCluster);
	KLSI_LOG (KLSI_DBG_RX, "Out of Cluster Blocks!\n", 
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
        netClFree (pDrvCtrl->endObj.pNetPool, pNewCluster);
	KLSI_LOG (KLSI_DBG_RX, "Out of M Blocks!\n", 
		    1, 2, 3, 4, 5, 6);
	END_ERR_ADD (&pDrvCtrl->endObj, MIB2_IN_ERRS, +1);
	goto cleanRXD;
        }

    END_ERR_ADD (&pDrvCtrl->endObj, MIB2_IN_UCAST, +1);
    
    /* Join the cluster to the MBlock */

    netClBlkJoin (pClBlk, (char *)pNewCluster, len, NULL, 0, 0, 0);
    netMblkClJoin (pMblk, pClBlk);

    pMblk->mBlkHdr.mLen = len;
    pMblk->mBlkHdr.mFlags |= M_PKTHDR;
    pMblk->mBlkPktHdr.len = len;


    END_RCV_RTN_CALL(&pDrvCtrl->endObj, pMblk);    

cleanRXD:
				
    return OK;
    }

/***************************************************************************
*
* klsiEndMemInit - initialize memory for the device.
*
* The END's Network memory pool is setup. This code is highly generic and 
* very simple. The technique described in netBufLib is followed.
* It is not even needed to make the memory cache DMA coherent. This is because
* unlike other END drivers, which act right on the top of the hardware,  
* we are layers above the hardware. 
*
* RETURNS: OK or ERROR.
* 
*/

STATUS klsiEndMemInit
    (
    KLSI_DEVICE * pDrvCtrl	/* device to be initialized */
    )
    {
  
    /*
     * This is how END netPool is setup using netBufLib(1).
     * This code is very generic.
     */
    
    if ((pDrvCtrl->endObj.pNetPool = malloc (sizeof (NET_POOL))) == NULL)
        return (ERROR);

    klsiMclBlkConfig.mBlkNum = KLSI_M_BULK_NUM;
    klsiClDescTbl[0].clNum = KLSI_CL_NUMBER;
    klsiMclBlkConfig.clBlkNum = klsiClDescTbl[0].clNum;

    /* Calculate the total memory for all the M-Blks and CL-Blks. */

    klsiMclBlkConfig.memSize = (klsiMclBlkConfig.mBlkNum *
				    (MSIZE + sizeof (long))) +
			      (klsiMclBlkConfig.clBlkNum * 
				    (CL_BLK_SZ + sizeof(long)));

    if ((klsiMclBlkConfig.memArea = (char *) memalign (sizeof(long),
                         klsiMclBlkConfig.memSize)) == NULL)
        return (ERROR);
    
    /* Calculate the memory size of all the clusters. */

    klsiClDescTbl[0].memSize = (klsiClDescTbl[0].clNum *
				    (KLSI_BUFSIZ + 8)) + sizeof(int);

    /* Allocate the memory for the clusters from cache safe memory. */

    klsiClDescTbl[0].memArea =
        (char *) cacheDmaMalloc (klsiClDescTbl[0].memSize);

    if (klsiClDescTbl[0].memArea == NULL)
        {
        KLSI_LOG (KLSI_DBG_LOAD,"klsiEndMemInit:system memory "
		    "unavailable\n", 1, 2, 3, 4, 5, 6);
        return (ERROR);
        }
    
    /* Initialize the memory pool. */

    if (netPoolInit(pDrvCtrl->endObj.pNetPool, 
		    &klsiMclBlkConfig,
                    &klsiClDescTbl[0], 
		    klsiClDescTblNumEnt,
		    NULL) == ERROR)
        {
        KLSI_LOG (KLSI_DBG_LOAD, "Could not init buffering\n",
		1, 2, 3, 4, 5, 6);
        return (ERROR);
        }


    if ((pDrvCtrl->pClPoolId = netClPoolIdGet (pDrvCtrl->endObj.pNetPool,
					       ETHERMTU, 
					       FALSE)) 
					    == NULL)
	{
        KLSI_LOG (KLSI_DBG_LOAD, "netClPoolIdGet() not successful \n",
  		    1, 2, 3, 4, 5, 6);
	return (ERROR);
        }

    KLSI_LOG (KLSI_DBG_LOAD, "Memory setup complete\n", 
		1, 2, 3, 4, 5, 6);

    return OK;
    }


/***************************************************************************
*
* klsiEndConfig - reconfigure the interface under us.
*
* Reconfigure the interface setting promiscuous/ broadcast etc modes, and 
* changing the multicast interface list.
*
* RETURNS: N/A.
*/

LOCAL STATUS klsiEndConfig
    (
    KLSI_DEVICE *pDrvCtrl	/* device to be re-configured */
    )
    {

    UINT8 bitmap = 0x00;    
    MULTI_TABLE* pList = NULL;


    /* Set the modes asked for. */

    if (END_FLAGS_GET(&pDrvCtrl->endObj) & IFF_PROMISC)
	{
	KLSI_LOG (KLSI_DBG_IOCTL, "Setting Promiscuous mode on!\n",
		1, 2, 3, 4, 5, 6);
	bitmap |= PACKET_TYPE_PROMISCOUS;
	}

    if (END_FLAGS_GET(&pDrvCtrl->endObj) & IFF_MULTICAST)
	{
	KLSI_LOG (KLSI_DBG_IOCTL, "Setting Multicast mode On!\n",
		1, 2, 3, 4, 5, 6);
	bitmap |= PACKET_TYPE_MULTICAST;
	}

    if (END_FLAGS_GET(&pDrvCtrl->endObj) & IFF_ALLMULTI)
	{
	KLSI_LOG (KLSI_DBG_IOCTL, "Setting ALLMULTI mode On!\n",
		1, 2, 3, 4, 5, 6);
	bitmap |= PACKET_TYPE_ALL_MULTICAST;
	}

    if (END_FLAGS_GET(&pDrvCtrl->endObj) & IFF_BROADCAST)
	{
	KLSI_LOG (KLSI_DBG_IOCTL, "Setting Broadcast mode On!\n",
		1, 2, 3, 4, 5, 6);
	bitmap |= PACKET_TYPE_BROADCAST;
	}

    /* Setup the packet filter */

    if (klsiPacketFilterSet (pDrvCtrl, bitmap) == ERROR)
        return ERROR;
         
    
    /* Set up address filter for multicasting. */
	
    if (END_MULTI_LST_CNT(&pDrvCtrl->endObj) > 0)
	{

	/* get the list of address to send to the device */
 
 	if (etherMultiGet (&pDrvCtrl->endObj.multiList, pList)
			   == ERROR)
	    return ERROR;   

        /* Set the Filter!!! */

        if (klsiMCastFilterSet(pDrvCtrl,(pUINT8)pList->pTable,
	    (pList->len) / 6) == ERROR)		
	    return ERROR;
	}

    return OK;
    }

/***************************************************************************
*
* klsiEndMCastAdd - add a multicast address for the device
*
* This routine adds a multicast address to whatever the chip is already 
* listening for. The USB Ethernet device specifically requires that even 
* if a small modification (addition or removal) of the filter list is desired,
* the entire list has to be downloaded to the device. The generic etherMultiLib
* functions are used and then klsiMCastFilterSet() is called to achieve the
* functionality.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS klsiEndMCastAdd
    (
    KLSI_DEVICE *pDrvCtrl,		/* device pointer */
    char* pAddress			/* new address to add */
    )
    {
    
    int error;
    MULTI_TABLE* pList = NULL;

    if ((pDrvCtrl == NULL) || (pAddress == NULL))
	return ERROR;

    /* First, add this address to the local list */
    
    if ((error = etherMultiAdd (&pDrvCtrl->endObj.multiList,
				pAddress)) == ERROR)
	return ERROR;
     
    /* Then get the list of address to send to the device */
 
    if ((error = etherMultiGet (&pDrvCtrl->endObj.multiList, pList))
				== ERROR)
	return ERROR;   

    /* Set the Filter!!! */

    if ((error = klsiMCastFilterSet (pDrvCtrl, 
				     (pUINT8)pList->pTable, 
				     (pList->len) / 6))
				== ERROR)
	return ERROR;   

    return (OK);
    }

/***************************************************************************
*
* klsiEndMCastDel - delete a multicast address for the device
*
* This routine removes a multicast address from whatever the driver
* listening for. The USB Ethernet device specifically requires that even 
* if a small modification (addition or removal) of the filter list is desired,
* the entire list has to be downloaded to the device. The generic etherMultiLib
* functions are used and then klsiMCastFilterSet() is called to achieve the
* functionality.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS klsiEndMCastDel
    (
    KLSI_DEVICE *pDrvCtrl,		/* device pointer */
    char* pAddress			/* new address to add */
    )
    {
    int error;
    MULTI_TABLE* pList = NULL;

    if ((pDrvCtrl == NULL) || (pAddress == NULL))
	return ERROR;

    /* First, add this address to the local list */
    
    if ((error = etherMultiDel (&pDrvCtrl->endObj.multiList,
	pAddress)) == ERROR)
	return ERROR;
     
    /* Then get the list of address to send to the device */
 
    if ((error = etherMultiGet (&pDrvCtrl->endObj.multiList, pList))
	== ERROR)
	return ERROR;   

    if ((error = klsiMCastFilterSet (pDrvCtrl,
				     (pUINT8)pList->pTable, 
				     (pList->len) / 6))	
				== ERROR)
	return ERROR;

    return (OK);
    }


/***************************************************************************
*
* klsiEndMCastGet - get the multicast address list for the device
*
* This routine gets the multicast list of whatever the driver
* is already listening for.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS klsiEndMCastGet
    (
    KLSI_DEVICE *pDrvCtrl,		/* device pointer */
    MULTI_TABLE* pTable			/* address table to be filled in */
    )
    {
    return (etherMultiGet (&pDrvCtrl->endObj.multiList, pTable));
    }


/***************************************************************************
*
* klsiEndIoctl - the driver I/O control routine
*
* Process an ioctl request.
*
* RETURNS: A command specific response, usually OK or ERROR.
*/

int klsiEndIoctl
    (
    KLSI_DEVICE * pDrvCtrl,	/* device receiving command */
    int cmd,			/* ioctl command code */
    caddr_t data		/* command argument */
    )
    {

    int error = 0;
    long value;

    switch (cmd)
        {
        case EIOCSADDR	:			/* Set Device Address */

	    if (data == NULL)
		return (EINVAL);

            bcopy ((char *)data, 
		   (char *)END_HADDR (&pDrvCtrl->endObj),
		   END_HADDR_LEN (&pDrvCtrl->endObj));

	    if(klsiMacAddressSet (pDrvCtrl, (UINT8 *)data) == ERROR)
		return ERROR;

            break;

        case EIOCGADDR	:			/* Get Device Address */

	    if (data == NULL)
		return (EINVAL);

            bcopy ((char *)END_HADDR (&pDrvCtrl->endObj), 
				      (char *)data,
				      END_HADDR_LEN (&pDrvCtrl->endObj));

            break;


        case EIOCSFLAGS	:			/* Set Device Flags */
	  
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

	    klsiEndConfig (pDrvCtrl);
            break;


        case EIOCGFLAGS:			/* Get Device Flags */

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


        case EIOCGFBUF	:	/* return minimum First Buffer for chaining */

            if (data == NULL)
                return (EINVAL);

            * (int *)data = KLSI_MIN_FBUF;

            break;


	case EIOCMULTIADD : 	/* Add a Multicast Address */

	    if (data == NULL)
		return (EINVAL);

	    if (klsiEndMCastAdd (pDrvCtrl, (char *)data) == ERROR)
		return ERROR;

	    break;


	case EIOCMULTIDEL : 	/* Delete a Multicast Address */

	    if (data == NULL)
		return (EINVAL);

	    if (klsiEndMCastDel (pDrvCtrl, (char *)data) == ERROR)
		return ERROR;

	    break;


	case EIOCMULTIGET : 	/* Get the Multicast List */

	    if (data == NULL)
		return (EINVAL);

	    if (klsiEndMCastGet (pDrvCtrl, (MULTI_TABLE *)data) == ERROR)
		return ERROR;

	    break;

       default:
            error = EINVAL;
	
        }

    return (error);
    }


/***************************************************************************
*
* klsiEndUnload - unload a driver from the system
*
* This function first brings down the device, and then frees any
* stuff that was allocated by the driver in the load function.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS klsiEndUnload
    (
    KLSI_DEVICE* pDrvCtrl	/* device to be unloaded */
    )
    {
    END_OBJECT_UNLOAD (&pDrvCtrl->endObj);
   
    if ((pDrvCtrl->pDev->lockCount == 0) && (!pDrvCtrl->pDev->connected))
	{	
 	klsiDestroyDevice (pDrvCtrl);
    	taskDelay(sysClkRateGet() *1);	

    	netPoolDelete (pDrvCtrl->endObj.pNetPool);
    	taskDelay(sysClkRateGet() *1);	
    
    	if (cfree((char *)pDrvCtrl)!=OK)
	   {
	    printf(" Error in memory clearing of device Structure\n");
	    return (ERROR);
	    }	
	}

    return (OK);
    }


/***************************************************************************
*
* klsiEndPollRcv - routine to receive a packet in polled mode.
*
* This routine is NOT supported
*
* RETURNS: ERROR Always
*/

LOCAL STATUS klsiEndPollRcv
    (
    KLSI_DEVICE * pDrvCtrl,	/* device to be polled */
    M_BLK_ID      pMblk		/* ptr to buffer */
    )
    {
    
    KLSI_LOG (KLSI_DBG_POLL_RX, "Poll Recv: NOT SUPPORTED", 
	1, 2, 3, 4, 5, 6);

    return ERROR;

    }

/***************************************************************************
*
* klsiEndPollSend - routine to send a packet in polled mode.
*
* This routine is NOT SUPPORTED
*
* RETURNS: ERROR always
*/

LOCAL STATUS klsiEndPollSend
    (
    KLSI_DEVICE* 	pDrvCtrl,	/* device to be polled */
    M_BLK_ID    pMblk			/* packet to send */
    )
    {
    
    KLSI_LOG (KLSI_DBG_POLL_TX, "Poll Send : NOT SUPPORTED", 
	1, 2, 3, 4, 5, 6);

    return ERROR;
    }

/***************************************************************************
*
* klsiDownloadFirmware - Downloads firmware to the KLSI Chip
*
* This routine downloads firmaware to the KLSI chip.  It will not function
* until this occurs.
*
*
* RETURNS : OK or ERROR
*/

LOCAL STATUS klsiDownloadFirmware
    (
    KLSI_DEVICE * pDevCtrl,	/* device to download */
    UINT8 * pData,		/* Firmware */
    UINT16 len,			/* size of firmware */
    UINT8 interrupt,		/* Interrupt to use */
    UINT8 type
    )
    {

    UINT16 actLen = 0;

    if (len > KLSI_FIRMWARE_BUF)
	{
	KLSI_LOG (KLSI_DBG_DNLD," Firmware too BIG : %d bytes \n", 
	    len, 0, 0, 0, 0, 0);
	return ERROR;
	}    

    pData[2] = (len & 0xFF) - 7;
    pData[3] = len >> 8;
    pData[4] = type;
    pData[5] = interrupt;

    if (usbdVendorSpecific (klsiHandle, 
			    pDevCtrl->pDev->nodeId,
			    USB_RT_VENDOR | USB_RT_DEVICE | USB_RT_HOST_TO_DEV, 
			    USB_REQ_KLSI_SCAN, 
			    0, 
			    0, 
			    len, 
			    pData,
			    &actLen) != OK)	
	{
	return ERROR;
	}
    
    return OK;
    }        


/***************************************************************************
*
* klsiTriggerFirmware - Triggers firmware of KLSI Chip
*
* RETURNS : OK or ERROR
*/

LOCAL STATUS klsiTriggerFirmware
    (
    KLSI_DEVICE * pDevCtrl,		/* device to trigger */
    UINT8 interrupt			/* interrupt to use */
    )
    {

    UINT16 actLen = 0;

    UINT8 triggerBuf[8];

    triggerBuf[0] = 0xB6;
    triggerBuf[1] = 0xC3;
    triggerBuf[2] = 1;
    triggerBuf[3] = 0;
    triggerBuf[4] = 6;
    triggerBuf[5] = 100;
    triggerBuf[6] = 0;
    triggerBuf[7] = 0;
	
    if (usbdVendorSpecific (klsiHandle, 
			    pDevCtrl->pDev->nodeId,
			    USB_RT_VENDOR | USB_RT_DEVICE | USB_RT_HOST_TO_DEV, 
			    USB_REQ_KLSI_SCAN, 
			    0, 
			    0, 
			    8, 
			    triggerBuf,
			    &actLen) 
			!= OK)	
	{
	return ERROR;
	}
    

    return OK;
    }        


/***************************************************************************
*
* klsiInit - Initialization for the KLSI Chip
*
* This device requires that a piece of "Firmware" be downloaded to the Chip 
* before it starts functioning as an Ethernet adapter. This function does 
* the device specific initialization, it basically downloads the firmware 
* on to the chip. Also sets which Interrupt of the device is to be used and 
* then reset the device.
*
* RETURNS : OK or ERROR
*/

STATUS klsiInit
    (
    KLSI_DEVICE * pDevCtrl
    )
    {

    if (klsiDownloadFirmware (pDevCtrl, 
			      klsiNewCode, 
			      lenKlsiNewCode, 
			      KLSI_INTERRUPT_TO_USE, 
			      2) 
			    == ERROR)
	{
	KLSI_LOG (KLSI_DBG_DNLD, " Couldnot download : NewCode \n",
	    0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    if (klsiDownloadFirmware (pDevCtrl, 
			      klsiNewCodeFix, 
			      lenKlsiNewCodeFix, 
			      KLSI_INTERRUPT_TO_USE, 
			      3) 
			    == ERROR)
	{
	KLSI_LOG (KLSI_DBG_DNLD, " Couldnot download : NewCode Fix\n", 
	    0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    if (klsiTriggerFirmware (pDevCtrl, KLSI_INTERRUPT_TO_USE) == ERROR )
	{
	KLSI_LOG (KLSI_DBG_DNLD, " Couldnot Trigger \n", 
	    0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    if (klsiReset (pDevCtrl) == ERROR)
	{
	KLSI_LOG (KLSI_DBG_DNLD, " Couldnot Reset the device \n",
	    0, 0, 0, 0, 0, 0);
	return ERROR;
	}

    return OK;

    }    
     

/***************************************************************************
*
* notifyAttach - Notifies registered callers of attachment/removal
*
* RETURNS: N/A
*/

LOCAL VOID notifyAttach
    (
    USBD_NODE_ID nodeId,
    UINT16 attachCode
    )

    {
    pATTACH_REQUEST pRequest = usbListFirst (&reqList);
    
    while (pRequest != NULL)
    {

    (*pRequest->callback) (pRequest->callbackArg, 
	                   nodeId, 
			   attachCode);

    pRequest = usbListNext (&pRequest->reqLink);
    }
    }

/***************************************************************************
*
* usbKlsiDynamicAttachRegister - Register KLSI device attach callback.
*
* <callback> is a caller-supplied function of the form:
*
* .CS
* typedef (*USB_KLSI_ATTACH_CALLBACK) 
*     (
*     pVOID arg,
*     USBD_NODE_ID nodeId,
*     UINT16 attachCode
*     );
* .CE
*
* usbKlsiEnd will invoke <callback> each time a KLSI device
* is attached to or removed from the system.  <arg> is a caller-defined
* parameter which will be passed to the <callback> each time it is
* invoked.  The <callback> will also be passed the nodeID of the device 
* being created/destroyed and an attach code of USB_KLSI_ATTACH or 
* USB_KLSI_REMOVE.
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
*   S_usbKlsiLib_BAD_PARAM
*   S_usbKlsiLib_OUT_OF_MEMORY
*/

STATUS usbKlsiDynamicAttachRegister
    (
    USB_KLSI_ATTACH_CALLBACK callback,	/* new callback to be registered */
    pVOID arg                           /* user-defined arg to callback  */
    )

    {
    pATTACH_REQUEST   pRequest;
    USB_KLSI_DEV  *	      pKlsiDev;
    int status = OK;


    /* Validate parameters */

    if (callback == NULL)
        return (ossStatus (S_usbKlsiLib_BAD_PARAM));

    OSS_MUTEX_TAKE (klsiMutex, OSS_BLOCK);

    /* Create a new request structure to track this callback request. */

    if ((pRequest = OSS_CALLOC (sizeof (*pRequest))) == NULL)
        {
        status = ossStatus (S_usbKlsiLib_OUT_OF_MEMORY);
        }
    else
        {
        pRequest->callback    = callback;
        pRequest->callbackArg = arg;

        usbListLink (&reqList, pRequest, &pRequest->reqLink, LINK_TAIL) ;
    
       /* 
        * Perform an initial notification of all currrently attached
        * KLSI devices.
        */

        pKlsiDev = usbListFirst (&klsiDevList);

        while (pKlsiDev != NULL)
	    {
            if (pKlsiDev->connected)
                (*callback) (arg, pKlsiDev->nodeId, USB_KLSI_ATTACH);

	    pKlsiDev = usbListNext (&pKlsiDev->devLink);
	    }

        }

    OSS_MUTEX_RELEASE (klsiMutex);

    return (ossStatus (status));
    }


/***************************************************************************
*
* usbKlsiDynamicAttachUnregister - Unregisters KLSI attach callback.
*
* This function cancels a previous request to be dynamically notified for
* KLSI device attachment and removal.  The <callback> and <arg> paramters 
* must exactly match those passed in a previous call to 
* usbKlsiDynamicAttachRegister().
*
* RETURNS: OK, or ERROR if unable to unregister callback
*
* ERRNO:
*   S_usbKlsiLib_NOT_REGISTERED
*/

STATUS usbKlsiDynamicAttachUnregister
    (
    USB_KLSI_ATTACH_CALLBACK callback, /* callback to be unregistered  */
    pVOID arg                          /* user-defined arg to callback */
    )

    {
    pATTACH_REQUEST pRequest;
    int status = S_usbKlsiLib_NOT_REGISTERED;

    OSS_MUTEX_TAKE (klsiMutex, OSS_BLOCK);

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

    OSS_MUTEX_RELEASE (klsiMutex);

    return (ossStatus (status));
    }


/***************************************************************************
*
* usbKlsiDevLock - Marks USB_KLSI_DEV structure as in use.
*
* A caller uses usbKlsiDevLock() to notify usbKlsiEnd that
* it is using the indicated USB_KLSI_DEV structure.  usbKlsiEnd maintains
* a count of callers using a particular USB_KLSI_DEV structure so that it 
* knows when it is safe to dispose of a structure when the underlying
* USB_KLSI_DEV is removed from the system.  So long as the "lock count"
* is greater than zero, usbKlsiEnd will not dispose of an USB_KLSI_DEV
* structure.
*
* RETURNS: OK, or ERROR if unable to mark USB_KLSI_DEV structure in use.
*/

STATUS usbKlsiDevLock
    (
    USBD_NODE_ID nodeId   /* NodeId of the USB_KLSI_DEV to be marked as in use */
    )

    {
    USB_KLSI_DEV* pKlsiDev = klsiFindDevice (nodeId);

    if ( pKlsiDev == NULL)
        return (ERROR);

    pKlsiDev->lockCount++;

    return (OK);
    }


/***************************************************************************
*
* usbKlsiDevUnlock - Marks USB_KLSI_DEV structure as unused.
*
* This function releases a lock placed on an USB_KLSI_DEV structure.  When a
* caller no longer needs an USB_KLSI_DEV structure for which it has previously
* called usbKlsiDevLock(), then it should call this function to
* release the lock.
*
* RETURNS: OK, or ERROR if unable to mark USB_KLSI_DEV structure unused
*
* ERRNO:
*   S_usbKlsiLib_NOT_LOCKED
*/

STATUS usbKlsiDevUnlock
    (
    USBD_NODE_ID nodeId    /* NodeId of the BLK_DEV to be marked as unused */
    )

    {
    int status = OK;

    USB_KLSI_DEV *      pKlsiDev = klsiFindDevice (nodeId);
 
    if ( pKlsiDev == NULL)
        return (ERROR);

    OSS_MUTEX_TAKE (klsiMutex, OSS_BLOCK);

    if (pKlsiDev->lockCount == 0)
        {
        status = S_usbKlsiLib_NOT_LOCKED;
        }
    else
    	{
       /* 
	* If this is the last lock and the underlying KLSI device is
        * no longer connected, then dispose of the device.
        */
     if ((--pKlsiDev->lockCount == 0) && (!pKlsiDev->connected))
	klsiDestroyDevice ((KLSI_DEVICE *)pKlsiDev->pDevStructure);
    	}

    OSS_MUTEX_RELEASE (klsiMutex);

    return (ossStatus (status));
    }
