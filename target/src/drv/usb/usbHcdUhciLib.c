/* usbHcdUhciLib.c - Defines entry point for UHCI HCD */

/* Copyright 2000-2001 Wind River Systems, Inc. */

/*
Modification history
--------------------
01m,22oct01,wef  bug in fncIrpSubmit() - was incorrectly flushing buffer list
                 instead of the actual list of buffers
01l,11oct01,wef  fix SPR's 70492, 69922 and 70716
01o,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce
01n,23jul01,wef  Fixed SPR #68202 and SPR #68209
01m,05jan01,wef  Fixed alignment problem w/ code.  General clean up
01l,12apr00,wef  fixed a comment that was messing up the man page generation 
01k,20mar00,rcb  Flush all cache buffers during assignTds() to avoid 
		 cache-line boundary problems with some CPU architectures
		 (e.g., MIPS).
01j,26jan00,rcb  Change references to "bytesPerFrame" to "bandwidth" in
		 pipe creation logic.
		 Modify isoch. TD creation to calculate each TD length in 
		 order to maintain average bandwidth.
01i,29nov99,rcb  Remove obsolete function HCD_FNC_BUS_RESET.
		 Correct code in rootFeature() to set port suspend status to
		 FALSE when issuing resume to a port.
01h,23nov99,rcb  Replace bandwidth alloc/release functions with pipe
		 create/destroy functions...generalizes approach for use
		 with OHCI HCD.
01g,11nov99,rcb  In destroyHost() cancel outstanding IRPs before disabling
		 host controller...prevents deadlock in hcSync() called from
		 cancelIrp().
01f,26sep99,rcb  Add code to synchronize with host controller after list
		 changes.  Add use of VOLATILE for QH/TD structures.
		 Add code to modify driver behavior based on specific
		 vendor/device combinations.
01e,24sep99,rcb  Remove extraneous end-of-loop test in unscheduleBulkIrp().
01d,21sep99,rcb  Change memory allocation strategy to use cacheDmaMalloc()
		.enables code to use CACHE_DMA_xxxx functions instead of
		 CACHE_USER_xxxx functions, yielding improved performance.
01c,07sep99,rcb  Add support for management callbacks and for set-bus-state.
01b,02sep99,rcb  Fix bug in delinking last bulk QH from work list.
01a,09jun99,rcb  First.
*/

/*
DESCRIPTION

This is the HCD (host controller driver) for UHCI.  This file implements
low-level functions required by the client (typically the USBD) to talk to
the underlying USB host controller hardware.  

The <param> to the HRB_ATTACH request should be a pointer to a 
PCI_CFG_HEADER which contains the PCI configuration header for the
universal host controller to be managed.  Each invocation of the HRB_ATTACH
function will return an HCD_CLIENT_HANDLE for which a single host controller
will be exposed.

NOTE: This HCD implementation assumes that the caller has already initialized
the osServices and handleFuncs libraries by calling ossInitialize() and
usbHandleInitialize(), respectively.  The USBD implementation guarantees that
these libraries have been initialized prior to invoking the HCD.

Regarding IRP callbacks...

There are two callback function pointers in each IRP, <usbdCallback> and
<userCallback>.  By convention, if a non-NULL <usbdCallback> is supplied,
then the HCD invokes only the <usbdCallback> upon IRP completion - and it is
the USBD's responsibility to invoke the <userCallback>.  If no <usbdCallback>
is provided, then the HCD invokes the <userCallback> directly.	Typically, 
all IRPs are delivered to the HCD through the USBD and a non-NULL <usbdCallback>
will in fact be provided.

Regarding UHCI frame lists...

We use the UHCI frame list in a way anticipated by, but not directly described
by the UHCI specfication.  As anticipated, we create an array of 1024 frame
list entries.  We also create an array of 1024 "interrupt anchor" QHs, one
"control anchor" QH and one "bulk anchor" QH.  Each frame list entry is 
initialized to point to a corresponding interrupt anchor.  Each interrupt anchor
points to the single control anchor, and the control anchor initially points to
the bulk anchor.  

When one or more interrupt transfers are pending, QHs for these transfers will
be inserted after the interrupt anchor QHs corresponding to the frames dictated 
by the interrupt scheduling interval.  The last pending interrupt QH in each
list points to the common control anchor QH.  While clients can request any
interrupt service interval they like, the algorithms here always choose an
interval which is the largest power of 2 less than or equal to the client's
desired interval.  For example, if a client requests an interval of 20msec, the
HCD will select a real interval of 16msec.  In each frame work list, the least
frequently scheduled QHs appear ahead of more frequently scheduled QHs.  Since
only a single QH is actually created for each interrupt transfer, the individual
frame lists actually "merge" at each interrupt QH.  Now, using the preceding
example of 16msec, suppose that there is a second interrupt QH with an interval
of 8 msec.  In half of the frame lists for which the 8msec interval transfer is
scheduled, the "interrupt anchor" QH will point to it directly.  In the other
half, the 16msec interval transfer will point to it.  

When control transfer QHs are scheduled, they are always placed in the list 
following the "control anchor" QH.  Similarly, bulk transfer QHs are always
placed after the "bulk anchor" QH.  When low speed control transfers are
pending, they are always inserted after the "control anchor" QH before any
high-speed control transfers.

The anchors themselves never describe work.  Instead, they are just
placeholders in the work lists to facilitate clean QH and TD list updates.
For example, when queuing a new control or bulk transfer, it is only necessary
to modify the work list after the single "control" or "bulk" anchor.  Similiarly,
when queuing interrupt transfers, it is only necessary to modify the work
list after the QH anchor in which the interrupt is to be scheduled.  Finally,
isochronous transfers can be added cleanly at the beginning of each frame's
work list; and the last isoch transfer TD in each frame always points to the
QH anchor corresponding to that frame.

In effect, this scheme decouples isoch, interrupt, control, and bulk transfers
in the TD/QH work lists.

Regarding bus time calculations...

The host controller driver is responsible for ensuring that certain kinds of
scheduled transfers never exceed the time available in a USB frame.  The HCD 
and the USBD work cooperatively to ensure this.  For its part, the USBD never 
allows isochronous and interrupt transfers to be scheduled which would exceed 
90% of the bus bandwidth.  However, the USBD will freely allow control and bulk 
pipes to be created, as these types of transfers take whatever bus time is left 
over after isochronous and interrupt transfers have been schedule - and the
HCD gives priority to control transfers.

The HCD keeps a running total of the worst case amount of bus time alloted to 
active isochronous and interrupt transfers.  As for control and bulk transfers,
the UHC theoretically allows us to schedule as many of them as we desire, and it
keeps track of how much time remains in each frame, executing only as many of
these transfers as will fit.  However, the UHC requires that the driver schedule
only as many low speed control transfers (as opposed to full speed control
transfers) as can actually fit within the frame.  Therefore, after taking into
account the time already allotted to isochronous and interrupt transfers, the
HCD only schedules as many low speed control transfers as can fit within the
current frame - and full speed control and bulk transfers follow. 
*/


/* includes */

#include "usb/usbPlatform.h"

#include "string.h"

#include "memLib.h"		/* memory sub-allocation functions */
#include "cacheLib.h"		/* cache functions */
#include "semLib.h"		/* semaphore functions */


#include "usb/ossLib.h"
#include "usb/usbHandleLib.h"
#include "usb/pciConstants.h"
#include "usb/usbPciLib.h"

#include "usb/usbLib.h"
#include "drv/usb/usbHcd.h"
#include "drv/usb/usbUhci.h"
#include "drv/usb/usbHcdUhciLib.h"


/* defines */

#define PENDING 	1

#define HCD_HOST_SIG	    ((UINT32) 0x00cd0000)
#define HCD_PIPE_SIG	    ((UINT32) 0x00cd0001)


#define MAX_INT_DEPTH	    8	/* max depth of pending interrupts */
#define INT_TIMEOUT	5000	/* wait 5 seconds for int routine to exit */
#define BIT_TIMEOUT	1000	/* max time to wait for a bit change */

#define UHC_ROOT_SRVC_INTERVAL	256 /* milliseconds */


/* UHC_HOST_DELAY and UHC_HUB_LS_SETUP are host-controller specific.
 * The following values are estimates for the UHCI controller.
 */

#define UHC_HOST_DELAY	    ((UINT32) 500L) /* 500 ns, est. */
#define UHC_HUB_LS_SETUP    ((UINT32) 500L) /* 500 ns, est. */


/*
 * MEMORY
 * 
 * To improve performance, a single block of (probably) "non-cached"
 * memory is allocated.  Then, all UHCI control structures are sub-allocated
 * from this block as needed.  The vxWorks CACHE_DMA_FLUSH/INVALIDATE macros
 * are used to ensure that this memory is flushed/invalidated at the correct
 * times (assuming that cacheable-memory *might* be allocated).
 */

#define DMA_MEMORY_SIZE 	0x10000 /* 64k */

#define DMA_MALLOC(bytes, alignment)	\
    memPartAlignedAlloc (pHost->memPartId, bytes, alignment)

#define DMA_FREE(pBfr)		memPartFree (pHost->memPartId, (char *) pBfr)

#define DMA_FLUSH(pBfr, bytes)	    CACHE_DMA_FLUSH (pBfr, bytes)
#define DMA_INVALIDATE(pBfr, bytes) CACHE_DMA_INVALIDATE (pBfr, bytes)

#define USER_FLUSH(pBfr, bytes)     CACHE_USER_FLUSH (pBfr, bytes)
#define USER_INVALIDATE(pBfr,bytes) CACHE_USER_INVALIDATE (pBfr, bytes)


/*
 * PLAN_AHEAD_TIME
 *
 * There is always going to be some latency betwen the time the UHC finishes
 * a portion of the frame list and before the HCD updates the frame list with
 * additional work.  It is not acceptable for certain transfers, like isoch.
 * transfers, to be interrupted by this latency.  Therefore, the UHCI HCD is
 * designed to plan ahead by the PLAN_AHEAD_TIME.  It is assumed that the
 * max interrupt latency will be *one half* of this time, and the HCD will
 * schedule itself accordingly.
 */

#define PLAN_AHEAD_TIME     125     /* plan UHCI TD list n msec in advance */
#define ISOCH_INT_INTERVAL  16	    /* generate isoch int every n msec */

#define MAX_IRP_TDS	32	/* overriding maximum number of TDs */
		    /* allowed for a single non-isoch IRP */

#define UHC_END_OF_LIST     TO_LITTLEL (UHCI_LINK_TERMINATE)


/* UHC I/O access macros.
 *
 * NOTE: These macros assume that the calling function defines pHost.
 */


#define UHC_BYTE_IN(p)	    uhciByteRead (pHost->ioBase + (p))
#define UHC_WORD_IN(p)	    uhciWordRead (pHost->ioBase + (p))
#define UHC_DWORD_IN(p)     USB_PCI_DWORD_IN (pHost->ioBase + (p))

#define UHC_BYTE_OUT(p,b)   USB_PCI_BYTE_OUT (pHost->ioBase + (p), (b))
#define UHC_WORD_OUT(p,w)   USB_PCI_WORD_OUT (pHost->ioBase + (p), (w))
#define UHC_DWORD_OUT(p,d)  USB_PCI_DWORD_OUT (pHost->ioBase + (p), (d))

#define UHC_SET_BITS(p,bits)	UHC_WORD_OUT (p, UHC_WORD_IN (p) | (bits))
#define UHC_CLR_BITS(p,bits)	UHC_WORD_OUT (p, UHC_WORD_IN (p) & ~(bits))


/* UHC run state flags to setUhcRunState() function */

#define UHC_RUN     1
#define UHC_STOP    0


/*
 * The HCD adds UHC_FRAME_SKIP to the current frame counter to determine
 * the first available frame for scheduling. This introduces a latency at
 * the beginning of each IRP, but also helps to ensure that the UHC won't
 * run ahead of the HCD while the HCD is scheduling a transaction.
 */

#define UHC_FRAME_SKIP		2


/* macro to produce frame list index from frame number */

#define FINDEX(f)	((f) & (UHCI_FRAME_LIST_ENTRIES - 1))


/* vendor/model identifiers */

#define PCI_VID_INTEL	    0x8086
#define PCI_VID_VIATECH     0x1106

#define PCI_DID_INTEL_PIIX3 0x7020
#define PCI_DID_INTEL_PIIX4 0x7112
#define PCI_DID_VIATECH_82C586	0x3038	    /* same for 83C572 */


/* UHC capabilities (vendor/model specific */

#define UHC_ATTR_BW_RECLAMATION 0x0001
#define UHC_ATTR_HC_SYNCH   0x0002


/* UHC_ATTR_DEFAULT is the combination of UHC attributes used for unrecognized
 * UHCI implementations.  It is generally the most conservative.
 */

#define UHC_ATTR_DEFAULT    UHC_ATTR_HC_SYNCH


/* Macros to interpret UHC capabilities. */

#define ENABLE_BANDWIDTH_RECLAMATION(pHost) \
    (((pHost->uhcAttributes & UHC_ATTR_BW_RECLAMATION) != 0) ? TRUE : FALSE)

#define ENABLE_HC_SYNCH(pHost)	\
    (((pHost->uhcAttributes & UHC_ATTR_HC_SYNCH) != 0) ? TRUE : FALSE)

#define HC_SYNCH(pHost) { if (ENABLE_HC_SYNCH (pHost)) hcSynch (pHost); }


/* defines for emulated USB descriptors */

#define USB_RELEASE	0x0110	/* USB level supported by this code */

#define UHC_MAX_PACKET_SIZE	8
    
#define UHC_CONFIG_VALUE	1

#define UHC_STATUS_ENDPOINT_ADRS    (1 | USB_ENDPOINT_IN)


/* interrupt bits */

#define UHC_INT_ENABLE_MASK (UHCI_INTR_SHORT | UHCI_INTR_COMPLETE | \
			     UHCI_INTR_RESUME | UHCI_INTR_TIME_CRC)

#define UHC_INT_PENDING_MASK	(UHCI_STS_PROCERR | UHCI_STS_HOSTERR | \
				 UHCI_STS_RESUME | UHCI_STS_USBERR | \
				 UHCI_STS_USBINT)


/* string identifiers */

#define UNICODE_ENGLISH     0x409

#define UHC_STR_MFG	1
#define UHC_STR_MFG_VAL     "Wind River Systems"

#define UHC_STR_PROD	    2
#define UHC_STR_PROD_VAL    "UHCI Root Hub"


/* PCI pointer macros */

#define TO_PCIPTR(p)	    TO_LITTLEL (USB_MEM_TO_PCI (p))
#define QH_TO_PCIPTR(p)     (TO_PCIPTR (p) | TO_LITTLEL (UHCI_LINK_QH))

#define FROM_PCIPTR(d)	    USB_PCI_TO_MEM (d)

#define QH_FROM_PCIPTR(d)   \
    ((pQH_WRAPPER) (FROM_PCIPTR (FROM_LITTLEL (d) & UHCI_LINK_PTR_MASK)))

#define TD_FROM_PCIPTR(d)   \
    ((pTD_WRAPPER) (FROM_PCIPTR (FROM_LITTLEL (d) & UHCI_LINK_PTR_MASK)))


/* typedefs */

/* UHC_TABLE
 *
 * Differences have been observed among UHCI implementations (including bugs
 * in some of those implementations) which favor a table-driven approach to
 * enabling certain features/operational modes.
 */

typedef struct uhc_table
    {
    UINT16 pciVendorId;     /* PCI vendor ID */
    UINT16 pciDeviceId;     /* vendor-assigned device ID */
    UINT16 uhcAttributes;   /* attributes */
    } UHC_TABLE, *pUHC_TABLE;


/*
 * TD_WRAPPER
 *
 * UHCI defines TD as 32 bytes long, with the last 16 bytes being reserved
 * for software use.  Each UHCI_TD must be aligned to a 16 byte boundary.
 */

typedef union td_wrapper
    {
    VOLATILE UHCI_TD td;	/* standard UHCI TD, 16 bytes */
    struct
	{
        VOLATILE UINT32 reserved [4]; /* 4 DWORDs used by TD */
        struct irp_workspace *pWork;/* pointer to IRP workspace */
        union td_wrapper *pNext;	/* next TD_WRAPPER used by the IRP */
        UINT32 nanoseconds;     /* calculated time for this TD to execute */
        UINT32 frameNo;	    /* used only for isoch. TDs */
	} sw;
    } TD_WRAPPER, *pTD_WRAPPER;

#define TD_WRAPPER_LEN	    32
#define TD_WRAPPER_ACTLEN   sizeof (TD_WRAPPER)


/*
 * QH_WRAPPER
 *
 * UHCI defines QH as 8 bytes long.  This wrapper adds fields at the end
 * for software use, padding the QH_WRAPPER to 16 bytes in the process.  
 * Each UHCI_QH must be aligned to a 16 byte boundary.
 */

typedef union qh_wrapper
    {
    VOLATILE UHCI_QH qh;	/* standard UHCI QH, 8 bytes */
        struct
        {
        VOLATILE UINT32 reserved [2]; /* 2 DWORDs used by QH */
        UINT32 reserved1;	    /* 1 DWORD pad */
        struct irp_workspace *pWork;/* workspace which owns QH */
        } sw;
    } QH_WRAPPER, *pQH_WRAPPER;

#define QH_WRAPPER_LEN	    16
#define QH_WRAPPER_ACTLEN   sizeof (QH_WRAPPER)

#define INT_ANCHOR_LIST_SIZE	(QH_WRAPPER_ACTLEN * UHCI_FRAME_LIST_ENTRIES)


/*
 * HCD_PIPE
 *
 * HCD_PIPE maintains all information about an active pipe.
 */

typedef struct hcd_pipe
    {
    HCD_PIPE_HANDLE pipeHandle;     /* handle assigned to pipe */

    LINK link;		    /* linked list of pipes */

    UINT16 busAddress;		/* bus address of USB device */
    UINT16 endpoint;		/* device endpoint */
    UINT16 transferType;	/* transfer type */
    UINT16 direction;		/* transfer/pipe direction */
    UINT16 speed;	    /* transfer speed */
    UINT16 maxPacketSize;	/* packet size */
    UINT32 bandwidth;		/* bandwidth required by pipe */
    UINT16 interval;		/* service interval */

    UINT32 time;	    /* bandwidth (time) allocated to pipe */

    } HCD_PIPE, *pHCD_PIPE;


/*
 * IRP_WORKSPACE
 *
 * Associates QHs and TDs with the IRPs they are currently servicing.
 *
 * Note: The pTdInUse and pTdFree lists use the TD_WRAPPER.sw.pNext field to 
 * maintain lists of TDs.  When TDs are scheduled, their UHCI_TD.linkPtr
 * fields are also used to link the TDs in the correct order.  The reader
 * must be aware that the UHCI_TD.linkPtr fields are formatted for direct
 * access by the UHCI PCI controller, and may not be dereferenced directly
 * by software.  Instead, the TD_FROM_PCIPTR() macro may be used to 
 * dereference the UHCI_TD.linkPtr field if necessary.	There is no such
 * restriction for the TD_WRAPPER.sw.pNext field.
 *
 * Note also that the linkPtr field uses the UHC convention that the end 
 * of a TD list is marked by a pointer with only the UHC_LINK_TERMINATE bit 
 * set (unlike the more conventional use of NULL to mark end of list).
 */

typedef struct irp_workspace
    {
    pHCD_PIPE pPipe;		/* pointer to pipe for this IRP */
    pUSB_IRP pIrp;	    /* pointer to parent IRP */

    UINT16 qhCount;	    /* count of QHs allocated for IRP */
    pQH_WRAPPER pQh;		/* pointer to QH for this transfer */

    UINT16 tdCount;	    /* count of TDs allocated for IRP */
    pTD_WRAPPER pTdPool;	/* pointer to block of TDs allocated */
    pTD_WRAPPER pTdFree;	/* unused TDs */
    pTD_WRAPPER pTdInUse;	/* first TD in use */
    pTD_WRAPPER pLastTdInUse;	    /* pointer to last TD */

    UINT32 nanoseconds; 	/* bus time required for TDs in use */

    UINT16 bfrNo;	    /* highest IRP bfrList[] serviced */
    UINT32 bfrOffset;		/* offset into bfrList[].pBfr */
    BOOL zeroLenMapped; 	/* TRUE when zero len bfrList [] serviced */

    UINT16 interval;		/* how often interrupt xfr scheduled */

    UINT32 isochTdsCreated;	/* count of isoch TDs created, in total */
    UINT32 frameCount;		/* count of frames used for isoch pipe */
    UINT32 bytesSoFar;		/* bytes transferred so far for isoch pipe */

    UINT16 isochNext;		/* next isoch frame number to schedule */
    UINT16 isochCount;		/* count of isoch frames scheduled */
    pTD_WRAPPER pNextIsochTd;	    /* next isoch TD to schedule */

    BOOL irpRunning;		/* TRUE once IRP scheduled onto bus */
    UINT32 startTime;		/* time when IRP was scheduled onto bus */

    } IRP_WORKSPACE, *pIRP_WORKSPACE;


/*
 * HCD_HOST
 *
 * HCD_HOST maintains all information about a connection to a specific
 * universal host controller (UHC).
 */

typedef struct hcd_host
    {
    HCD_CLIENT_HANDLE handle;	/* handle associated with host */
    BOOL shutdown;	    	/* TRUE during shutdown */

    PCI_CFG_HEADER pciCfgHdr;	/* PCI config header for UHC */
    UINT16 uhcAttributes;	/* vendor/model specific attributes */
    UINT32 ioBase;	    	/* Base I/O address */

    USB_HCD_MNGMT_CALLBACK mngmtCallback; /* callback routine for mngmt evt */
    pVOID mngmtCallbackParam;	/* caller-defined parameter */

    MUTEX_HANDLE hostMutex;	/* guards host structure */

    THREAD_HANDLE intThread;	/* Thread used to handle interrupts */
    SEM_HANDLE intPending;	/* semaphore indicates int pending */
    BOOL intThreadExitRequest;	/* TRUE when intThread should terminate */
    SEM_HANDLE intThreadExit;	/* signalled when int thread exits */
    UINT32 intCount;		/* number of interrupts processed */
    BOOL intInstalled;		/* TRUE when h/w int handler installed */
    UINT16 intBits;	    	/* interrupt bits */

    UINT16 rootAddress; 	/* current address of root hub */
    UINT8 configValue;		/* current configuration value */
    BOOL powered [UHCI_PORTS];	/* TRUE if port is powered */
    BOOL cSuspend [UHCI_PORTS]; /* TRUE if suspend status change */
    BOOL cReset [UHCI_PORTS];	/* TRUE if reset status change */

    /* descriptor fields order-dependent */

    USB_DEVICE_DESCR devDescr;	    /* standard device descriptor */
    USB_CONFIG_DESCR configDescr;   /* standard config descriptor */
    USB_INTERFACE_DESCR ifDescr;    /* standard interface descriptor */
    USB_ENDPOINT_DESCR endpntDescr; /* standard endpoint descriptor */
    USB_HUB_DESCR hubDescr;	/* root hub descriptor */

    LIST_HEAD pipes;		/* active pipes */

    UINT16 rootIrpCount;	/* count of entries on pRootIrps */
    LIST_HEAD rootIrps; 	/* IRPs pending on root hub */

    LIST_HEAD busIrps;		/* IRPs pending on devices not */
			    	/* including root hub */

    char *dmaPool;	    	/* memory alloc'd by cacheDmaMalloc() */
    PART_ID memPartId;		/* memory partition ID */

    pUHCI_FRAME_LIST pFrameList;    /* UHC frame list */
    pQH_WRAPPER pIntAnchorQhs;	    /* UHC QH "anchor" list */
    pQH_WRAPPER pCtlAnchorQh;	    /* Anchor for ctl transfer QHs */
    pQH_WRAPPER pLastCtlQh;	/* pointer to last ctl QH in use */
    pQH_WRAPPER pBulkAnchorQh;	    /* Anchor for bulk transfer QHs */
    UINT32 pciPtrToBulkAnchorQh;    /* pre-calculated PCIPTR */
    pQH_WRAPPER pLastBulkQh;	    /* pointer to last bulk QH in use */

    UINT32 hostErrCount;	/* incremented if host ctlr reports err */

    UINT32 nanoseconds; 	/* current worst case of bus time */
		    		/* required for scheduled TDs */

    UINT16 sofInterval; 	/* current SOF interval */

    BOOL suspended;	     	/* TRUE when global suspend is TRUE */

    SEM_ID hcSyncSem;           /* syncronization semaphore */

    } HCD_HOST, *pHCD_HOST;


/* locals */

/* UHC vendor/device table.
 *
 * The following table defines the features/operating modes we use for
 * UHCI controllers with which we've tested.  
 *
 * Intel PIIX4: Hard system crashes have been observed when enabling
 * bandwidth reclamation on the PIIX4.	There are unconfirmed reports
 * that the PIIX3 can consume all available PCI bandwidth when all QHs
 * on the bandwidth reclamation loop are complete, thus hanging the
 * system.  We surmise that this may also be the cause of the problems
 * observed on the PIIX4.
 *
 * ViaTech: The ViaTech parts appear to work fine with bandwidth
 * reclamation enabled.
 */

#define UHC_ATTR_INTEL_PIIX (UHC_ATTR_HC_SYNCH)
#define UHC_ATTR_VIATECH    (UHC_ATTR_HC_SYNCH | UHC_ATTR_BW_RECLAMATION)

LOCAL UHC_TABLE uhcTable [] = {
    {PCI_VID_INTEL, PCI_DID_INTEL_PIIX3,    UHC_ATTR_INTEL_PIIX},
    {PCI_VID_INTEL, PCI_DID_INTEL_PIIX4,    UHC_ATTR_INTEL_PIIX},
    {PCI_VID_VIATECH,	PCI_DID_VIATECH_82C586, UHC_ATTR_VIATECH},
    {0, 	0,	    0}};


/* Language descriptor */

LOCAL USB_LANGUAGE_DESCR langDescr =
    {sizeof (USB_LANGUAGE_DESCR), USB_DESCR_STRING, 
    {TO_LITTLEW (UNICODE_ENGLISH)}};


/***************************************************************************
*
* uhciByteRead - Read a 8 bit value from the UHCI controller.
*
* RETURNS: A big-endian adjusted UINT8
*/
 
LOCAL UINT32 uhciByteRead
    (
    UINT32 offset
    )
    {
    CACHE_PIPE_FLUSH ();
 
    return USB_PCI_BYTE_IN (offset);
    }

/***************************************************************************
*
* uhciWordRead - Read a 16 bit value from the UHCI controller.
*
* RETURNS: A big-endian adjusted UINT16
*/
 
LOCAL UINT32 uhciWordRead
    (
    UINT32 offset
    )
    {
    CACHE_PIPE_FLUSH ();
 
    return USB_PCI_WORD_IN (offset);
    }

/***************************************************************************
*
* getUhcAttributes - get attributes based on type of UHCI controller
*
* Based on information in the PCI configuration header passed by the
* caller, searches for a matching entry in the uhcTable[].  If one is
* found, returns the corresponding attribute value.  If no entry is
* found, returns the default value.
*
* RETURNS: UINT16 vector containing UHC attributes
*/

LOCAL UINT16 getUhcAttributes
    (
    pPCI_CFG_HEADER pCfgHdr
    )

    {
    pUHC_TABLE pTable;

    for (pTable = &uhcTable [0]; pTable->pciVendorId != 0; pTable++)
	if (pTable->pciVendorId == pCfgHdr->vendorId &&
	    pTable->pciDeviceId == pCfgHdr->deviceId)
	    {
	    return pTable->uhcAttributes;
	    }

    return UHC_ATTR_DEFAULT;
    }


/***************************************************************************
*
* waitOnBits - wait for a word register bit to reach the desired state
*
* RETURNS: TRUE if successful, else FALSE if desired bit state not detected
*/

LOCAL BOOL waitOnBits
    (
    pHCD_HOST pHost,
    UINT16 p,
    UINT16 bitMask,
    UINT16 bitState
    )

    {
    UINT32 start = OSS_TIME ();
    BOOL desiredState;

    while (!(desiredState = ((UHC_WORD_IN (p) & bitMask) == bitState)) && 
	OSS_TIME () - start < BIT_TIMEOUT)
	;

    return desiredState;    
    }


/***************************************************************************
*
* getFrameNo - returns current hardware frame number
*
* RETURNS: value of current frame number
*/

LOCAL UINT16 getFrameNo
    (
    pHCD_HOST pHost
    )

    {
    return FINDEX (UHC_WORD_IN (UHCI_FRNUM));
    }


/***************************************************************************
*
* setIrpResult - sets result in IRP and executes IRP callback
*
* RETURNS: value from Irp result field
*/

LOCAL int setIrpResult
    (
    pUSB_IRP pIrp,
    int result
    )

    {
    if (result != PENDING)
	{
	pIrp->result = result;

	if (pIrp->usbdCallback != NULL)
	    (*pIrp->usbdCallback) (pIrp);

	else if (pIrp->userCallback != NULL)
	    (*pIrp->userCallback) (pIrp);
	}

    return result;
    }


/***************************************************************************
*
* planAheadTime - returns "plan-ahead time"
*
* The "plan ahead interval" is the maximum time that code should "plan
* ahead when setting up work for the UHC.  It is expected that interrupt
* latency will never exceed one half of this time.
*
* RETURNS: plan ahead time in msec
*/

LOCAL UINT16 planAheadTime
    (
    pHCD_HOST pHost
    )

    {
    return PLAN_AHEAD_TIME;
    }


/***************************************************************************
*
* calcIntInterval - calculates the scheduling interval for interrupt transfer
*
* RETURNS: Actual interval to be used for interrupt transfer
*/

LOCAL UINT16 calcIntInterval
    (
    UINT16 interval	/* 1 <= requested interval <= 255 */
    )

    {
    UINT16 i;

    /* Select an interval which is the largest power of two less than or
     * equal to the requested interval.
     */

    for (i = 2; i < 256; i <<= 1)
	{
	if (i > interval)
	    break;
	}

    return i >> 1;
    }


/***************************************************************************
*
* calcQhTd - calculates number of QHs and TDs to be allocated for IRP
*
* Calculates the number of TDs/QHs which need to be allocated for <pIrp>
* in order to complete the IRP or manage it through at least the "plan
* ahead interval."  Returns number of TDs in <pTdCount> and QHs in
* <pQhCount>.
*
* RETURNS: N/A
*/

LOCAL VOID calcQhTd
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe,
    pUSB_IRP pIrp,
    pUINT16 pQhCount,
    pUINT16 pTdCount
    )

    {
    UINT16 qhCount = 0;
    UINT16 tdCount = 0;
    UINT16 trialTdCount;
    UINT32 bytesPerFrame;


    /* The calculation of TDs/QHs varies based on the type of transfer */

    switch (pPipe->transferType)
    {
    case USB_XFRTYPE_ISOCH:

	/* IRPs for isochronous transfers have one or more bfrList[]
	 * entries which requires a variable number of TDs.  There
	 * will be a single TD per frame, and the number of TDs
	 * is bounded by the "plan ahead time".
	 *
	 * NOTE: for isoch transfers, maxPacketSize is the number
	 * of bytes to be transmitted each frame.
	 */

	qhCount = 0;

	bytesPerFrame = pPipe->bandwidth / 1000L;

	trialTdCount = (pIrp->transferLen + bytesPerFrame - 1) 
			/ bytesPerFrame;

	tdCount = min (trialTdCount, planAheadTime (pHost));
	break;

    case USB_XFRTYPE_INTERRUPT:

	/* IRPs for interrupt transfers have one or more bfrList[] 
	 * entries.  Each IRP requires a single QHs which may be linked
	 * into multiple frame lists (to allow it to be scheduled 
	 * periodically) plus a variable number of TDs.
	 */

	qhCount = 1;

	trialTdCount = (pIrp->transferLen + pPipe->maxPacketSize - 1)
			/ pPipe->maxPacketSize;

	tdCount = min (trialTdCount, MAX_IRP_TDS);
	break;

    case USB_XFRTYPE_CONTROL:

	/* IRPs for control transfers have two or more bfrList[]
	 * entries.  We need one QH for the transfer plus one TD
	 * for the Setup packet (first bfrList[] entry) plus
	 * a variable number of TDs for the additional bfr list entries
	 * plus a final TD for the status packet.  There is an overriding 
	 * limit on the number of TDs per control IRP set at MAX_IRP_TDS.
	 * 
	 * NOTE: For low-speed control transfers, we impose a limit of
	 * one oustanding TD at a time.
	 */

	qhCount = 1;


	if (pPipe->speed == USB_SPEED_LOW)
	    {
	    tdCount = 1;
	    }
	else
	    {	
	    tdCount = 2;	/* for Setup and status packets */

	    if (pIrp->bfrCount > 1)
		{
		trialTdCount = (pIrp->transferLen - pIrp->bfrList [0].bfrLen 
				+ pPipe->maxPacketSize - 1) 
				/ pPipe->maxPacketSize;

		tdCount += min (trialTdCount, MAX_IRP_TDS - tdCount);
		}
	    }

	break;

    case USB_XFRTYPE_BULK:

	/* IRPs for bulk transfers have one or more bfrList[] entries.	
	 * Each IRP requires a single QH plus a variable number of TDs.
	 */

	qhCount = 1;

	trialTdCount = (pIrp->transferLen + pPipe->maxPacketSize - 1)
			/ pPipe->maxPacketSize;

	tdCount = min (trialTdCount, MAX_IRP_TDS);
	break;
    }

    /* return results */

    if (pQhCount != NULL)
	   *pQhCount = qhCount;

    if (pTdCount != NULL)
	    *pTdCount = tdCount;
    }


/***************************************************************************
*
* freeIrpWorkspace - releases IRP_WORKSPACE and related QHs and TDs
*
* Releases all QHs and TDs associated with an IRP_WORKSPACE and frees
* the IRP_WORKSPACE structure itself.
*
* RETURNS: N/A
*/

LOCAL VOID freeIrpWorkspace
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp
    )

    {
    pIRP_WORKSPACE pWork = (pIRP_WORKSPACE) pIrp->hcdPtr;

    if (pWork != NULL)
	{
	if (pWork->pQh != NULL)
	    DMA_FREE (pWork->pQh);

	if (pWork->pTdPool != NULL)
	    DMA_FREE (pWork->pTdPool);

	OSS_FREE (pWork);

	pIrp->hcdPtr = NULL;
	}
    }


/***************************************************************************
*
* allocIrpWorkspace - allocates QH and TD structures for IRP
*
* Calculates the required number of QH and TD structures for an IRP
* and allocates aligned memory.  Also creates an IRP_WORKSPACE structure
* to manage the QHs and TDs allocated for the IRP and links it into the
* IRP.
*
* RETURNS: TRUE if successful, else FALSE
*/

LOCAL BOOL allocIrpWorkspace
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe,
    pUSB_IRP pIrp
    )

    {
    pIRP_WORKSPACE pWork;
    UINT16 tdLen;
    UINT16 i;


    /* Allocate IRP_WORKSPACE */

    if ((pWork = OSS_CALLOC (sizeof (*pWork))) == NULL)
	return FALSE;

    pIrp->hcdPtr = pWork;
    pWork->pPipe = pPipe;
    pWork->pIrp = pIrp;


    /* Calculate number of QHs and TDs */

    calcQhTd (pHost, pPipe, pIrp, &pWork->qhCount, &pWork->tdCount);


    /* Allocate QHs and TDs */

    if (pWork->qhCount > 0)
	{
	if ((pWork->pQh = DMA_MALLOC (sizeof (QH_WRAPPER), 
	    UHCI_QH_ALIGNMENT)) == NULL)
	    {
	    freeIrpWorkspace (pHost, pIrp);
	    return FALSE;
	    }

	memset (pWork->pQh, 0, sizeof (QH_WRAPPER));
	pWork->pQh->sw.pWork = pWork;
	pWork->pQh->qh.tdLink = UHC_END_OF_LIST;
	}

    if (pWork->tdCount > 0)
	{
	tdLen = pWork->tdCount * sizeof (TD_WRAPPER);

	if ((pWork->pTdPool = DMA_MALLOC (tdLen, UHCI_TD_ALIGNMENT)) == NULL)
	    {
	    freeIrpWorkspace (pHost, pIrp);
	    return FALSE;
	    }

	memset (pWork->pTdPool, 0, tdLen);
	}


    /* Create a chain of TDs in the "free" list */

    pWork->pTdInUse = pWork->pTdFree = NULL;

    for (i = 0; i < pWork->tdCount; i++)
	{
	pWork->pTdPool [i].sw.pWork = pWork;

	pWork->pTdPool [i].sw.pNext = pWork->pTdFree;
	pWork->pTdFree = &pWork->pTdPool [i];
	}


    /* Initialize the pointer to the last TD in use */

    pWork->pLastTdInUse = NULL;


    return TRUE;
    }


/***************************************************************************
*
* isBandwidthTracked - determines if bandwidth is tracked for this transfer
*
* Since the USBD has already reserved adequate bandwidth for isochronous
* and interrupt transfers, we only track low speed control transfers.
*
* RETURNS: TRUE if bandwidth tracked, else FALSE.
*/

LOCAL BOOL isBandwidthTracked
    (
    pHCD_PIPE pPipe
    )

    {
    if (pPipe->transferType == USB_XFRTYPE_CONTROL && 
	pPipe->speed == USB_SPEED_LOW)
	return TRUE;

    return FALSE;
    }


/***************************************************************************
*
* dirFromPid - returns USB_DIR_xxxx based on USB_PID_xxxx
*
* RETURNS: USB_DIR_xxxx
*/

LOCAL UINT16 dirFromPid
    (
    UINT16 pid
    )

    {
    switch (pid)
    {
    case USB_PID_SETUP:
    case USB_PID_OUT:	
	return USB_DIR_OUT;

    case USB_PID_IN:	
	return USB_DIR_IN;

    default:	    
	return USB_DIR_IN;
    }
    }


/***************************************************************************
*
* assignTds - assign TDs for each IRP bfrList[] entry
*
* Assigns and initializes TDs to map the next bfrList[] entry in the IRP.  
* Stops when all all bfrList[] entries have been mapped, when all TDs for 
* the IRP have been exhausted, or when buffer bandwidth calculations indicate 
* we shouldn't map any more TDs at this time.  
*
* This function also updates the pHost->nanoseconds field with the bandwidth
* required by this transfer if appropriate.
*
* This mapping has the effect of moving TDs from the free list to the "in
* use" list in the IRP_WORKSPACE.  TDs in the "in use" list are always
* linked in the order in which they should be executed.
*
* NOTE: We choose to map only one bfrList[] entry at a time to simplify the
* the handling of input underrun.  When an underrun occurs, the 
* rescheduleTds() function releases all TDs scheduled up through that point.
* We then schedule TDs for the following bfrList[] entries if any.
*
* RETURNS: N/A
*/

LOCAL VOID assignTds
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp,
    pIRP_WORKSPACE pWork
    )

    {
    pHCD_PIPE pPipe = pWork->pPipe;
    pTD_WRAPPER pTd;
    pUSB_BFR_LIST pBfrList;
    UINT32 bytesThroughFrameCount;
    UINT16 maxLen;
    UINT32 nanoseconds;
    UINT32 nanosecondsAtStart;
    UINT32 ctlSts;
    UINT32 token;


    /* Record the bus time already used by this transfer. */

    nanosecondsAtStart = pWork->nanoseconds;


    /* Assign TDs to map the current bfrList[] entry.  Stop when the buffer is 
     * fully mapped, when we run out of TDs, or when bus bandwidth calculations 
     * indicate we should not allocate more TDs at this time.
     */

    pBfrList = &pIrp->bfrList [pWork->bfrNo];

    while ((pWork->bfrOffset < pBfrList->bfrLen ||
	   (pBfrList->bfrLen == 0 && !pWork->zeroLenMapped)) && 
	   pWork->pTdFree != NULL)
	{

	/* Calculate the length of this TD. */

	if (pPipe->transferType == USB_XFRTYPE_ISOCH)
	    {
	    pWork->frameCount++;

	    bytesThroughFrameCount = pWork->frameCount 
				     * pPipe->bandwidth 
				     / 1000L;

	    maxLen = min (bytesThroughFrameCount - pWork->bytesSoFar,
	    pBfrList->bfrLen - pWork->bfrOffset);

	    maxLen = min (maxLen, pPipe->maxPacketSize);

	    if (pIrp->dataBlockSize != 0 && maxLen > pIrp->dataBlockSize)
		    maxLen = (maxLen / pIrp->dataBlockSize) 
			     * pIrp->dataBlockSize;

	    pWork->bytesSoFar += maxLen;
	    }
	else
	    {
	    /* Transfer length calculation for non-isoch pipes. */

	    maxLen = min (pBfrList->bfrLen - pWork->bfrOffset, 
			  pPipe->maxPacketSize);
	    }


	/* Determine if there are any bandwidth limitations which would prevent 
	 * scheduling this TD at this time.
	 *
	 * NOTE: Since the USBD has already checked bandwidth availability for
	 * isochronous and interrupt transfers, we need only check bandwidth
	 * availability for low speed control transfers.
	 */

	if (isBandwidthTracked (pPipe))
	    {
	    nanoseconds = usbTransferTime (pPipe->transferType, 
	    dirFromPid (pBfrList->pid), pPipe->speed, maxLen, 
	    UHC_HOST_DELAY, UHC_HUB_LS_SETUP);

	    if (pHost->nanoseconds + nanoseconds > USB_LIMIT_ALL)
		{
		/* There isn't enough bandwidth at this time.  Stop 
		 * scheduling for this transfer.
		 */
		
		break;
		}
	    }
	else
	    {
	    nanoseconds = 0;
	    }


	/* If this is the first time we've mapped a part of this bfrList[] entry,
	 * then flush the cache.  Note that we do this for input buffers as well
	 * since we don't know where the user's buffer is allocated and some CPU
	 * cache architectures may get confused near cache-line boundaries.
	 */

	if (pWork->bfrOffset == 0 && pBfrList->bfrLen > 0)
	    {
	    USER_FLUSH (pBfrList->pBfr, pBfrList->bfrLen);
	    }


	/* Unlink the TD from the free list. */

	pTd = pWork->pTdFree;
	pWork->pTdFree = pTd->sw.pNext;


	/* Initialize TD buffer pointer. */

	pTd->td.bfrPtr = TO_PCIPTR (&pBfrList->pBfr [pWork->bfrOffset]);
	pWork->bfrOffset += maxLen;

	if (maxLen == 0)
	    pWork->zeroLenMapped = TRUE;


	/* Initialize TD control/status word */ 

	ctlSts = UHCI_TDCS_SHORT | UHCI_TDCS_ERRCTR_3ERR;
	ctlSts |= (pPipe->speed == USB_SPEED_LOW) ? UHCI_TDCS_LOWSPEED : 0;
	ctlSts |= (pPipe->transferType == USB_XFRTYPE_ISOCH) ? 
		   UHCI_TDCS_ISOCH : 0;
	ctlSts |= UHCI_TDCS_STS_ACTIVE;


	/* enable IOC (interrupt on complete) if this is the last TD.  If
	 * this is an isochronous transfer, also enable IOC on a regular
	 * interval defined by ISOCH_INT_INTERVAL. 
	 */

	if (pWork->bfrOffset == pBfrList->bfrLen || pWork->pTdFree == NULL)
	    ctlSts |= UHCI_TDCS_COMPLETE;

	if (pPipe->transferType == USB_XFRTYPE_ISOCH)
	    {
	    pWork->isochTdsCreated++;

	    if (pWork->isochTdsCreated % ISOCH_INT_INTERVAL == 0)
		    ctlSts |= UHCI_TDCS_COMPLETE;
	    }

	pTd->td.ctlSts = TO_LITTLEL (ctlSts);


	/* Initialize TD token word */

	token = UHCI_TDTOK_MAXLEN_FMT (maxLen);

	if (pPipe->transferType != USB_XFRTYPE_ISOCH)
	    {
	    /* Normally, the data toggle begins with DATA0 and alternates
	     * between DATA0 and DATA1.  However, the last transfer for
	     * a control transfer - the status packet - is always a DATA1. 
	     */

	    if (pPipe->transferType == USB_XFRTYPE_CONTROL &&
		pWork->bfrNo == pIrp->bfrCount - 1)
		{
		token |= UHCI_TDTOK_DATA_TOGGLE;
		}
	    else
		{
		if (pIrp->dataToggle == USB_DATA0)
		    {
		    pIrp->dataToggle = USB_DATA1;
		    }
		else 
		    {
		    token |= UHCI_TDTOK_DATA_TOGGLE;
		    pIrp->dataToggle = USB_DATA0;
		    }
		}
	    }
	    
	token |= UHCI_TDTOK_ENDPT_FMT (pPipe->endpoint);
	token |= UHCI_TDTOK_DEVADRS_FMT (pPipe->busAddress);
	token |= UHCI_TDTOK_PID_FMT (pBfrList->pid);

	pTd->td.token = TO_LITTLEL (token);


	/* Store the time required to execute this TD */

	pTd->sw.nanoseconds = nanoseconds;
	pWork->nanoseconds += nanoseconds;


	/* put the TD at the end of the "in use" list.
	 *
	 * NOTE: If this QH is already active, then as soon as we
	 * set the linkPtr of the last TD in use we must expect
	 * that the UHC may begin executing this TD (does not apply
	 * to isochronous TDs for which there is no QH).
	 */

	pTd->td.linkPtr = UHC_END_OF_LIST;
	pTd->sw.pNext = NULL;

	if (pWork->pTdInUse == NULL)
	    {
	    pWork->pTdInUse = pTd;
	    }

	if (pPipe->transferType == USB_XFRTYPE_ISOCH &&
	    pWork->pNextIsochTd == NULL)
	    {
	    pWork->pNextIsochTd = pTd;
	    }

	DMA_FLUSH (&pTd->td, sizeof (pTd->td));

	if (pWork->pLastTdInUse != NULL)
	    {
	    pWork->pLastTdInUse->sw.pNext = pTd;

	    /* For non-isochronous transfers, append this TD to the list
	     * of TDs assigned to the IRP.	(Isochronous TDs are inserted
	     * into the schedule by calling scheduleIsochFrames().
	     */

	    if (pPipe->transferType != USB_XFRTYPE_ISOCH)
		{
		/* NOTE: Setting the "Vf" bit improves performance by 
		 * allowing the host controller to exchange data 
		 * continuously with the device until the device NAKs.
		 */

		pWork->pLastTdInUse->td.linkPtr = TO_PCIPTR (pTd) 
						  | TO_LITTLEL (UHCI_LINK_VF);

		DMA_FLUSH (&pWork->pLastTdInUse->td.linkPtr, 
			   sizeof (pWork->pLastTdInUse->td.linkPtr));
		}
	    }

	pWork->pLastTdInUse = pTd;
	}


    /* If there is a QH, we should initialize it to point to the beginning of 
     * the TD list.
     */

    if (pWork->pQh != NULL && pWork->pQh->qh.tdLink == UHC_END_OF_LIST)
	{
	/* Initialize QH "vertical" pointer */

	pWork->pQh->qh.tdLink = TO_PCIPTR (pWork->pTdInUse);
	DMA_FLUSH (&pWork->pQh->qh.tdLink, sizeof (pWork->pQh->qh.tdLink));
	}

    /* Update the bandwidth-in-use for this controller.  
     *
     * NOTE: The pWork->nanoseconds field is 0 for all transfer types
     * except low speed control transfers, so the following calculation
     * often has no effect.
     */

    pHost->nanoseconds += pWork->nanoseconds - nanosecondsAtStart;
    }


/***************************************************************************
*
* unlinkIsochTd - unlink an isochronous TD
*
* Searches the indicated <frame> for a TD which corresponds to the
* IRP_WORKSPACE <pWork> (and hence which correspond to a given TD).
* If found, the TD is unlinked.
*
* RETURNS: N/A
*/

LOCAL VOID unlinkIsochTd
    (
    pHCD_HOST pHost,
    pIRP_WORKSPACE pWork,
    UINT32 frame
    )

    {
    pUINT32 pPciPtrToTd;
    pTD_WRAPPER pTd;


    /* Search frame for a TD belonging to this IRP and remove it from the 
     * work list.  We've exhausted the list of isochronous TDs when we 
     * reach the "interrupt anchor" QH.
     */

    pPciPtrToTd = &pHost->pFrameList->linkPtr [frame];
    pTd = TD_FROM_PCIPTR (*pPciPtrToTd);

    while (pTd != (pTD_WRAPPER) &pHost->pIntAnchorQhs [frame])
	{
	if (pTd->sw.pWork == pWork)
	    {
	    /* We found a TD for this IRP.	Unlink it. */

	    *pPciPtrToTd = pTd->td.linkPtr;
	    DMA_FLUSH (pPciPtrToTd, sizeof (*pPciPtrToTd));

	    --pWork->isochCount;

	    break;
	    }

	pPciPtrToTd = (pUINT32) &pTd->td.linkPtr;
	pTd = TD_FROM_PCIPTR (pTd->td.linkPtr);
	}
    }


/***************************************************************************
*
* unscheduleIsochTransfer - removes isochronous transfer from work list
*
* RETURNS: N/A
*/

LOCAL VOID unscheduleIsochTransfer
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp,
    pIRP_WORKSPACE pWork
    )

    {
    UINT16 frame;


    /* Remove all pending isoch TDs for this IRP from the work list */

    for (frame = 0; 
         frame < UHCI_FRAME_LIST_ENTRIES && pWork->isochCount > 0; 
	 frame++)
	{
	unlinkIsochTd (pHost, pWork, frame);
	}
    }
    

/***************************************************************************
*
* scheduleIsochFrames - insert isoch frames into work list
*
* Schedules isochronous transfers into frames.
*
* RETURNS: N/A
*/

LOCAL VOID scheduleIsochFrames
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp,
    pIRP_WORKSPACE pWork
    )

    {
    pTD_WRAPPER pTd;
    UINT16 frame;

    /* Schedule all initialized TDs onto the work list */

    while ((pTd = pWork->pNextIsochTd) != NULL && 
	    pWork->isochCount < UHCI_FRAME_LIST_ENTRIES)
	{
	/* Link the TD into the work list. */

	pTd->sw.frameNo = frame = pWork->isochNext;

	pTd->td.linkPtr = pHost->pFrameList->linkPtr [frame];
	DMA_FLUSH (&pTd->td.linkPtr, sizeof (pTd->td.linkPtr));

	pHost->pFrameList->linkPtr [pWork->isochNext] = TO_PCIPTR (pTd);
	DMA_FLUSH (&pHost->pFrameList->linkPtr [frame], 
		   sizeof (pHost->pFrameList->linkPtr [frame]));


	/* Advance to the next isoch TD for this IRP. */

	pWork->pNextIsochTd = pTd->sw.pNext;

	if (++pWork->isochNext == UHCI_FRAME_LIST_ENTRIES)
	    pWork->isochNext = 0;

	pWork->isochCount++;
	}
    }


/***************************************************************************
*
* scheduleIsochTransfer - inserts isoch IRP in frame list
*
* Schedules an isochronous transfer for the first time.
*
* RETURNS: N/A
*/

LOCAL VOID scheduleIsochTransfer
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp,
    pIRP_WORKSPACE pWork
    )

    {
    /* Isochronous transfers are scheduled immediately if the
     * USB_IRP.flag USB_FLAG_ISO_ASAP is set else beginning in the
     * frame specified by USB_IRP.startFrame.  The number of frames
     * to be scheduled is equal to the number of TDs in the 
     * IRP_WORKSPACE.
     */

    if ((pIrp->flags & USB_FLAG_ISO_ASAP) != 0)
	    pWork->isochNext = FINDEX (getFrameNo (pHost) + UHC_FRAME_SKIP);
    else
	    pWork->isochNext = FINDEX (pIrp->startFrame);

    pWork->isochCount = 0;

    scheduleIsochFrames (pHost, pIrp, pWork);
    }


/***************************************************************************
*
* unscheduleInterruptTransfer - removes interrupt transfer from work list
*
* RETURNS: N/A
*/

LOCAL VOID unscheduleInterruptTransfer
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp,
    pIRP_WORKSPACE pWork
    )

    {
    pQH_WRAPPER pCurrentQh;
    pQH_WRAPPER pNextQh;
    UINT16 i;

    /* We need to search the list of interrupt QHs to find any references
     * to the indicated QH and purge each one.
     */

    for (i = 0; i < UHCI_FRAME_LIST_ENTRIES; i += pWork->interval)
	{
	pCurrentQh = &pHost->pIntAnchorQhs [i];

	while ((pNextQh = QH_FROM_PCIPTR (pCurrentQh->qh.qhLink))
		!= pHost->pCtlAnchorQh &&
		pNextQh->sw.pWork->interval >= pWork->interval &&
		pNextQh != pWork->pQh)
	    {
	    pCurrentQh = pNextQh;
	    }

	if (pNextQh == pWork->pQh)
	    {
	    /* We found a reference to this QH. Modify the QH list to
	     * skip over it.  NOTE: pNextQh == pWork->pQh. */

	    pCurrentQh->qh.qhLink = pNextQh->qh.qhLink;
	    DMA_FLUSH (&pCurrentQh->qh.qhLink, sizeof (pCurrentQh->qh.qhLink));
	    }
	}
    }
    

/***************************************************************************
*
* scheduleInterruptTransfer - inserts interrupt IRP in frame list
*
* Schedules an interrupt transfer repeatedly in the frame list as
* indicated by the service interval.
*
* RETURNS: N/A
*/

LOCAL VOID scheduleInterruptTransfer
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp,
    pIRP_WORKSPACE pWork
    )

    {
    pQH_WRAPPER pExistingQh;
    pQH_WRAPPER pNextQh;
    UINT16 i;


    /* Calculate the service interval we'll actually use. */

    pWork->interval = calcIntInterval (pWork->pPipe->interval);


    /* Schedule interrupt transfer periodically throughout frame list */

    for (i = 0; i < UHCI_FRAME_LIST_ENTRIES; i += pWork->interval)
	{
	/* Find the appropriate point in the list of interrupt QHs for this
	 * frame into which to schedule this QH.  This QH needs to be
	 * scheduled ahead of all interrupt QHs which are serviced more
	 * frequently (ie., least frequently serviced QHs come first).
	 */

	pExistingQh = &pHost->pIntAnchorQhs [i];

	while ((pNextQh = QH_FROM_PCIPTR (pExistingQh->qh.qhLink)) 
		!= pHost->pCtlAnchorQh &&
		pNextQh->sw.pWork->interval >= pWork->interval &&
		pNextQh != pWork->pQh)
	    {
	    pExistingQh = pNextQh;
	    }

	if (pNextQh != pWork->pQh)
	    {
	    if (i == 0)
		{
		pWork->pQh->qh.qhLink = pExistingQh->qh.qhLink;
		DMA_FLUSH (&pWork->pQh->qh, sizeof (pWork->pQh->qh));
		}

	    pExistingQh->qh.qhLink = QH_TO_PCIPTR (pWork->pQh);
	    DMA_FLUSH (&pExistingQh->qh.qhLink, sizeof (pExistingQh->qh.qhLink));
	    }
	}
    }


/***************************************************************************
*
* unscheduleControlTransfer - removes control transfer from work list
*
* RETURNS: N/A
*/

LOCAL VOID unscheduleControlTransfer
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp,
    pIRP_WORKSPACE pWork
    )

    {
    pQH_WRAPPER pCurrentQh;
    pQH_WRAPPER pNextQh;

    /* Find the control transfer in the work list and unlink it. */

    pCurrentQh = pHost->pCtlAnchorQh;

    while ((pNextQh = QH_FROM_PCIPTR (pCurrentQh->qh.qhLink)) != pWork->pQh)
	{
	pCurrentQh = pNextQh;
	}

    if (pNextQh == pWork->pQh)
	{
	/* Bipass pWork->pQh. */

	pCurrentQh->qh.qhLink = pWork->pQh->qh.qhLink;
	DMA_FLUSH (&pCurrentQh->qh.qhLink, sizeof (pCurrentQh->qh.qhLink));

	if (pHost->pLastCtlQh == pWork->pQh)
	    pHost->pLastCtlQh = pCurrentQh;
	}
    }
    

/***************************************************************************
*
* scheduleControlTransfer - inserts control IRP in frame list
*
* Inserts the control transfer into the portion of the frame list 
* appropriate for control transfers.
*
* RETURNS: N/A
*/

LOCAL VOID scheduleControlTransfer
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp,
    pIRP_WORKSPACE pWork
    )

    {
    pQH_WRAPPER pExistingQh;
    pQH_WRAPPER pNextQh;


    /* Low speed control transfers must always be scheduled before full
     * speed control transfers in order to ensure that they execute
     * completely within a specified frame.
     */

    if (pWork->pPipe->speed == USB_SPEED_LOW)
	{
	/* Insert this low speed transfer after currently scheduled low
	 * speed transfers and ahead of any high speed transfers.
	 */

	pExistingQh = pHost->pCtlAnchorQh;

	while ((pNextQh = QH_FROM_PCIPTR (pExistingQh->qh.qhLink)) 
		!= pHost->pBulkAnchorQh && 
		pNextQh->sw.pWork->pPipe->speed == USB_SPEED_LOW)
	    {
	    pExistingQh = pNextQh;
	    }
	}
    else
	{
	/* This is a full speed transfer.  Add the QH to the end of the list 
	 * of control QHs. 
	 */

	pExistingQh = pHost->pLastCtlQh;
	}


    /* Set this QH's forward link to pExistingQh's forward link */

    if ((pWork->pQh->qh.qhLink = pExistingQh->qh.qhLink) == 
	pHost->pciPtrToBulkAnchorQh)
	{
	pHost->pLastCtlQh = pWork->pQh;
	}

    DMA_FLUSH (&pWork->pQh->qh, sizeof (pWork->pQh->qh));

    /* Set the forward link of the previous QH to this QH. */

    pExistingQh->qh.qhLink = QH_TO_PCIPTR (pWork->pQh);
    DMA_FLUSH (&pExistingQh->qh.qhLink, sizeof (pExistingQh->qh.qhLink));
    }


/***************************************************************************
*
* unscheduleBulkTransfer - removes bulk transfer from work list
*
* RETURNS: N/A
*/

LOCAL VOID unscheduleBulkTransfer
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp,
    pIRP_WORKSPACE pWork
    )

    {
    pQH_WRAPPER pCurrentQh;
    pQH_WRAPPER pNextQh;

    /* Find the bulk transfer in the work list and unlink it. */

    pCurrentQh = pHost->pBulkAnchorQh;

    while ((pNextQh = QH_FROM_PCIPTR (pCurrentQh->qh.qhLink)) != pWork->pQh)
	{
	pCurrentQh = pNextQh;
	}

    if (pNextQh == pWork->pQh)
	{
	/* Bipass pWork->pQh. */

	if (ENABLE_BANDWIDTH_RECLAMATION (pHost))
	    {
	    if (pCurrentQh == pHost->pBulkAnchorQh &&
		pNextQh->qh.qhLink == pHost->pciPtrToBulkAnchorQh)
		{
		pCurrentQh->qh.qhLink = UHC_END_OF_LIST;
		}
	    else
		{
		pCurrentQh->qh.qhLink = pNextQh->qh.qhLink;
		}
	    }
	else
	    {
	    pCurrentQh->qh.qhLink = pNextQh->qh.qhLink;
	    }

	DMA_FLUSH (&pCurrentQh->qh.qhLink, sizeof (pCurrentQh->qh.qhLink));

	if (pHost->pLastBulkQh == pNextQh)
	    pHost->pLastBulkQh = pCurrentQh;
	}
    }
    

/***************************************************************************
*
* scheduleBulkTransfer - inserts bulk IRP in frame list
*
* Inserts the bulk transfer into the portion of the frame list appropriate 
* for bulk transfers.
*
* RETURNS: N/A
*/

LOCAL VOID scheduleBulkTransfer
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp,
    pIRP_WORKSPACE pWork
    )

    {
    /* Set the forward link of this QH to the forward link of the
     * QH which is currently last on the bulk list.
     */

    if (ENABLE_BANDWIDTH_RECLAMATION (pHost))
	{
	if (pHost->pLastBulkQh->qh.qhLink == UHC_END_OF_LIST)
	    pWork->pQh->qh.qhLink = pHost->pciPtrToBulkAnchorQh;
	else
	    pWork->pQh->qh.qhLink = pHost->pLastBulkQh->qh.qhLink;
	}
    else
	{
	pWork->pQh->qh.qhLink = pHost->pLastBulkQh->qh.qhLink;
	}

    DMA_FLUSH (&pWork->pQh->qh, sizeof (pWork->pQh->qh));


    /* Set the forward link of the previous QH to this QH. */

    pHost->pLastBulkQh->qh.qhLink = QH_TO_PCIPTR (pWork->pQh);
    DMA_FLUSH (&pHost->pLastBulkQh->qh.qhLink, 
		sizeof (pHost->pLastBulkQh->qh.qhLink));

    pHost->pLastBulkQh = pWork->pQh;
    }


/***************************************************************************
*
* unscheduleIrp - Removes QH/TDs for IRP from work UHC work lists
*
* RETURNS: N/A
*/

LOCAL VOID unscheduleIrp
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp,
    pIRP_WORKSPACE pWork
    )

    {
    /* un-Scheduling proceeds differently for each transaction type. */

    switch (pWork->pPipe->transferType)
	{
	case USB_XFRTYPE_ISOCH:
	    unscheduleIsochTransfer (pHost, pIrp, pWork);
	    break;

	case USB_XFRTYPE_INTERRUPT: 
	    unscheduleInterruptTransfer (pHost, pIrp, pWork);
	    break;

	case USB_XFRTYPE_CONTROL:
	    unscheduleControlTransfer (pHost, pIrp, pWork);
	    break;

	case USB_XFRTYPE_BULK:
	    unscheduleBulkTransfer (pHost, pIrp, pWork);
	    break;
	}
    }


/***************************************************************************
*
* scheduleIrp - try to schedule an IRP in the frame list
*
* RETURNS: N/A
*/

LOCAL VOID scheduleIrp
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp
    )

    {
    pIRP_WORKSPACE pWork = (pIRP_WORKSPACE) pIrp->hcdPtr;
    pHCD_PIPE pPipe = pWork->pPipe;


    /* prepare TDs */

    assignTds (pHost, pIrp, pWork);


    /* Mark the time the IRP was first scheduled. */

    if (!pWork->irpRunning)
	{
	pWork->irpRunning = TRUE;
	pWork->startTime = OSS_TIME ();
	}


    /* Scheduling proceeds differently for each transaction type. */

    switch (pPipe->transferType)
	{
	case USB_XFRTYPE_ISOCH:
	    scheduleIsochTransfer (pHost, pIrp, pWork);
	    break;

	case USB_XFRTYPE_INTERRUPT: 
	    scheduleInterruptTransfer (pHost, pIrp, pWork);
	    break;

	case USB_XFRTYPE_CONTROL:
	    scheduleControlTransfer (pHost, pIrp, pWork);
	    break;

	case USB_XFRTYPE_BULK:
	    scheduleBulkTransfer (pHost, pIrp, pWork);
	    break;
	}
    }


/***************************************************************************
*
* rescheduleTds - Examine the given IRP and see if it needs servicing
*
* <pIrp> is an IRP which may/may not have been assigned QHs and TDs and
* currently be schedule for execution.	Examine the <pIrp> and its
* associated USB_WORKSPACE in <pWork> (which might be NULL).  If no work
* has been scheduled or if no TDs have been completed by the UHC, then
* we do nothing.  If there are TDs which have been completed, then we
* release them to the "free" pool.  If work remains to be done, then we
* reschedule the TDs.  If no work remains to be done or if there is an
* error, then we report that the IRP is complete.
*
* RETURNS: OK = IRP completed ok
*      PENDING = IRP still being executed
*      S_usbHcdLib_xxx = IRP failed with error
*/

LOCAL int rescheduleTds
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp,
    pIRP_WORKSPACE pWork
    )

    {
    pTD_WRAPPER pTd;
    pUSB_BFR_LIST pBfrList;
    UINT32 nanosecondsAtStart;
    UINT32 ctlSts;
    UINT32 token;
    UINT16 actLen;
    BOOL underrun = FALSE;
    int s = PENDING;
    UINT16 i;
    

    /* If there is no workspace, this IRP has not been scheduled yet. */

    if (pWork == NULL)
	return PENDING;


    /* Record the bus time already used by this transfer. */

    nanosecondsAtStart = pWork->nanoseconds;


    /* Examine the "in use" TDs.  If any are complete, return them to the
     * free pool for this IRP.	If any completed with errors, indicate that
     * the IRP has failed.
     *
     * NOTE: Complete TDs will always be in front.  If we encouter a TD
     * which is not complete, then we need not check any further TDs in the
     * list.  If we detect an underrun on an IN transfer, then we continue
     * to process the TD list anyway, moving TDs from the in-use list to the
     * free list.
     */

    while (s == PENDING && pWork->pTdInUse != NULL)
	{
	/* Examine the TDs status */

	pTd = pWork->pTdInUse;
	DMA_INVALIDATE (pTd, sizeof (*pTd));

	ctlSts = FROM_LITTLEL (pTd->td.ctlSts);

	if ((ctlSts & UHCI_TDCS_STS_ACTIVE) != 0 && !underrun)
	    {
	    /* TD is still active.	Stop checking this list. */

	    break;
	    }
	else
	    {
	    /* Release the bandwidth this TD was consuming. */

	    pWork->nanoseconds -= pTd->sw.nanoseconds;

	    if (!underrun)
		{

		/* This TD is no longer active.  Did it 
		 * complete successfully? 
		 */

		if ((ctlSts & UHCI_TDCS_STS_STALLED) != 0)
		    s = S_usbHcdLib_STALLED;
		else if ((ctlSts & UHCI_TDCS_STS_DBUFERR) != 0)
		    s = S_usbHcdLib_DATA_BFR_FAULT;
		else if ((ctlSts & UHCI_TDCS_STS_BABBLE) != 0)
		    s = S_usbHcdLib_BABBLE;
		else if ((ctlSts & UHCI_TDCS_STS_TIME_CRC) != 0)
		    s = S_usbHcdLib_CRC_TIMEOUT;
		else if ((ctlSts & UHCI_TDCS_STS_BITSTUFF) != 0)
		    s = S_usbHcdLib_BITSTUFF_FAULT;
		}

	    if (!underrun && s == PENDING)
		{
		/* There doesn't appear to be an error.  Update the actlen
		 * field in the IRP.
		 */

		pBfrList = &pIrp->bfrList [pWork->bfrNo];

		/* If this in an IN and the actLen was shorter than the 
		 * expected size, then the transfer is complete.  If the 
		 * IRP flag USB_FLAG_SHORT_FAIL is set, then fail the 
		 * transfer.
		 */

		actLen = UHCI_TDCS_ACTLEN (ctlSts);
		pBfrList->actLen += actLen;

		token = FROM_LITTLEL (pTd->td.token);

		if (UHCI_TDTOK_PID (token) == USB_PID_IN && 
		    actLen < UHCI_TDTOK_MAXLEN (token))
		    underrun = TRUE;

		if (pBfrList->actLen == pBfrList->bfrLen || underrun)
		    {
		    pWork->bfrNo++;
		    pWork->bfrOffset = 0;
		    pWork->zeroLenMapped = FALSE;
		    }

		if (underrun && (pIrp->flags & USB_FLAG_SHORT_FAIL) != 0)
		    s = S_usbHcdLib_SHORT_PACKET;


		if (underrun && pWork->pQh != NULL)
		    {
		    /* When an underrun is detected, there is still a QH 
		     * pointing to the TD which caused the underrun.  In 
		     * this case, we need to get the TD out of the work 
		     * list before we can re-use it.
		     */

		    pWork->pQh->qh.tdLink = UHC_END_OF_LIST;
		    DMA_FLUSH (&pWork->pQh->qh.tdLink, 
		    sizeof (pWork->pQh->qh.tdLink));
		    }
		}


	    /* If this is an isochronous transfer, delink it from the
	     * UHC work list.
	     */

	    if (pWork->pPipe->transferType == USB_XFRTYPE_ISOCH)
		unlinkIsochTd (pHost, pTd->sw.pWork, pTd->sw.frameNo);

	    /* Delink this TD from the in-use pool.  If this happens to
	     * be the last TD in use, then update pLastTdInUse.
	     */

	    if ((pWork->pTdInUse = pTd->sw.pNext) == NULL)
		pWork->pLastTdInUse = NULL;

	    /* Link TD to free pool. */

	    pTd->sw.pNext = pWork->pTdFree;
	    pWork->pTdFree = pTd;
	    }
	}


    /* Update the bus tally of bandwidth in use. */

    if (s == PENDING)
	{
	/* Transfer is ok.	Release bandwidth from TDs we processed */

	pHost->nanoseconds -= nanosecondsAtStart - pWork->nanoseconds;
	}
    else
	{
	/* Transfer has failed.  Release all associated bandwidth. */

	pHost->nanoseconds -= nanosecondsAtStart;
	}


    /* If the IRP has not failed, and if there is work remaining to be done,
     * and if there are entries on the free TD pool, then prepare additional
     * TDs.  If the IRP has not failed and there is no work remaining to be
     * done then mark the IRP as successful.
     */

    if (s == PENDING)
	{
	if (pWork->bfrNo == pIrp->bfrCount)
	    {
	    s = OK;
	    }
	else 
	    {
	    /* For interrupt, control, and bulk transfers the QH is already 
	     * in the UHC work list.  So, re-assigning TDs will continue the 
	     * transfer.  However, for isochronous transfers, fresh TDs need
	     * to be injected into the work list.
	     */

	    assignTds (pHost, pIrp, pWork);

	    if (pWork->pPipe->transferType == USB_XFRTYPE_ISOCH)
		scheduleIsochFrames (pHost, pIrp, pWork);
	    }
	}


    /* Check if IRP has finished */

    if (s != PENDING)
	{
	/* The IRP has finished, invalidate the cache for any input buffers */

	for (i = 0; i < pIrp->bfrCount; i++)
	    {
	    pBfrList = &pIrp->bfrList [i];
	
	    if (pBfrList->pid == USB_PID_IN && pBfrList->actLen > 0)
		USER_INVALIDATE (pBfrList->pBfr, pBfrList->actLen);
	    }
	}

    return s;
    }


/***************************************************************************
*
* hcSynch - give the host controller a chance to synchronize
*
* It appears that certain UHCI host controllers (e.g., Via Tech)
* pre-fetch QHs.  This creates a problem for us when we want to
* unlink the QH, as the host controller still has a record of it.
* To make sure the host controller doesn't declare a consistency
* error when it examines the QH/TDs associated with the QH, we
* wait until the current frame ends before freeing the QH/TD
* memory.  On the next frame, the host controller will re-fetch
* all QHs, and it won't see the ones we've removed.
*
* RETURNS: N/A
*/

LOCAL VOID hcSynch
    (
    pHCD_HOST pHost
    )

    {
    UINT16 currentFrameNo = getFrameNo (pHost);

    while (getFrameNo (pHost) == currentFrameNo)
	semTake(pHost->hcSyncSem, 1);
    }


/***************************************************************************
*
* cancelIrp - cancel's an outstanding IRP
*
* If the IRP's result code is not PENDING, then we cannot cancel
* the IRP as it has either already completed or it is not yet enqueued.
*
* RETURNS: OK if IRP canceled
*      S_usbHcdLib_CANNOT_CANCEL if IRP already completed
*/

LOCAL int cancelIrp
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp,
    int result		    /* error to assign to IRP */
    )

    {
    pIRP_WORKSPACE pWork;
    int s = OK;

    if (pIrp->result != PENDING)
	s = S_usbHcdLib_CANNOT_CANCEL;
    else
	{
	/* The IRP is pending.  Unlink it. */

	usbListUnlink (&pIrp->hcdLink);

	/* If this IRP is a "bus" IRP - as opposed to a root IRP - then
	 * remove it from the UHC's work list.
	 */

	if ((pWork = (pIRP_WORKSPACE) pIrp->hcdPtr) != NULL &&
	    pWork->pPipe->busAddress != pHost->rootAddress)
	    {
	    /* Remove QHs/TDs from the work list and release workspace. */

	    unscheduleIrp (pHost, pIrp, pWork);
	    HC_SYNCH (pHost);
	    freeIrpWorkspace (pHost, pIrp);
	    }

	setIrpResult (pIrp, result);    
	}

    return s;
    }


/***************************************************************************
*
* processUhcInterrupt - process a hardware interrupt from the UHC
*
* Determine the cause of a hardware interrupt and service it.  If the
* interrupt results in the completion of one or more IRPs, handle the
* completion processing for those IRPs.
*
* RETURNS: N/A
*/

LOCAL VOID processUhcInterrupt
    (
    pHCD_HOST pHost
    )

    {
    pUSB_IRP pIrp;
    pUSB_IRP pNextIrp;
    pIRP_WORKSPACE pWork;
    LIST_HEAD completeIrps = {0};
    int irpsCompleted = 0;
    int result;


    /* evaluate the interrupt condition */

    if ((pHost->intBits & (UHCI_STS_PROCERR | UHCI_STS_HOSTERR)) != 0)
	{
	/* No response, just clear error condition */

	pHost->intBits &= ~(UHCI_STS_PROCERR | UHCI_STS_HOSTERR);

	pHost->hostErrCount++;
	}

    if ((pHost->intBits & UHCI_STS_RESUME) != 0)
	{
	/* Remote device is driving resume signalling.  If the HCD client
	 * installed a management callback, invoke it.
	 */

	pHost->intBits &= ~UHCI_STS_RESUME;

	if (pHost->mngmtCallback != NULL)
	    (*pHost->mngmtCallback) (pHost->mngmtCallbackParam, pHost->handle,
	    0 /* bus number */, HCD_MNGMT_RESUME);
	}

    if ((pHost->intBits & (UHCI_STS_USBERR | UHCI_STS_USBINT)) != 0)
	{
	/* Completion of one or more USB transactions detected. */

	pHost->intBits &= ~(UHCI_STS_USBERR | UHCI_STS_USBINT);

	/* Search for one or more IRPs which have been completed
	 * (or partially completed).
	 */

	pIrp = usbListFirst (&pHost->busIrps);

	while (pIrp != NULL)
	    {
	    pNextIrp = usbListNext (&pIrp->hcdLink);
	    
	    /* As we walk the list of IRPs, try to reschedule QHs and TDs
	     * which are complete.
	     */

	    pWork = (pIRP_WORKSPACE) pIrp->hcdPtr;

	    if ((result = rescheduleTds (pHost, pIrp, pWork)) != PENDING)
		{
		/* The indicated IRP is complete. */

		irpsCompleted++;

		/* De-link the QH for this transfer.
		 *
		 * NOTE: We defer freeing the workspace associated with each
		 * IRP until we're ready to do the callbacks.  This gives us
		 * a chance to synchronize with the host controller, below.
		 */

		unscheduleIrp (pHost, pIrp, pWork);

		/* Unlink the IRP from the list of active IRPs */

		usbListUnlink (&pIrp->hcdLink);

		pIrp->result = result;
		usbListLink (&completeIrps, pIrp, &pIrp->hcdLink, LINK_TAIL);
		}

	    pIrp = pNextIrp;
	    }


	/* Invoke IRP callbacks for all completed IRPs. */

	if (irpsCompleted > 0)
	    HC_SYNCH (pHost);

	while ((pIrp = usbListFirst (&completeIrps)) != NULL)
	    {
	    usbListUnlink (&pIrp->hcdLink);
	    freeIrpWorkspace (pHost, pIrp);
	    setIrpResult (pIrp, pIrp->result);
	    }
	}
    }


/***************************************************************************
*
* checkIrpTimeout - Checks for IRPs which may have timed-out
*
* RETURNS: N/A
*/

LOCAL VOID checkIrpTimeout
    (
    pHCD_HOST pHost	/* host to check */
    )

    {
    pUSB_IRP pIrp;
    pUSB_IRP pNextIrp;
    pIRP_WORKSPACE pWork;
    UINT32 now;


    /* Search for one or more IRPs which have been completed
     * (or partially completed).
     */

    now = OSS_TIME ();

    pIrp = usbListFirst (&pHost->busIrps);

    while (pIrp != NULL)
	{
	pNextIrp = usbListNext (&pIrp->hcdLink);
	    
	pWork = (pIRP_WORKSPACE) pIrp->hcdPtr;

	if (pIrp->timeout != USB_TIMEOUT_NONE && pWork->irpRunning &&
	    now - pWork->startTime > pIrp->timeout)
	    {
	    /* This IRP has exceeded its run time. */

	    cancelIrp (pHost, pIrp, S_usbHcdLib_TIMEOUT);
	    }

	pIrp = pNextIrp;
	}
    }


/***************************************************************************
*
* isHubStatus - checks status on root hub
*
* RETURNS: hub and port status bitmap as defined by USB
*/

LOCAL UINT8 isHubStatus
    (
    pHCD_HOST pHost
    )

    {
    UINT8 hubStatus = 0;
    UINT16 port;
    UINT16 portReg;
    UINT16 portStatus;

    for (port = 0; port < UHCI_PORTS; port++)
	{

	portReg = UHCI_PORTSC1 + port * 2;
	portStatus = UHC_WORD_IN (portReg);

	if ((portStatus & UHCI_PORT_CNCTCHG) != 0 ||
	    (portStatus & UHCI_PORT_ENBCHG) != 0 ||
	    pHost->cSuspend [port] || 
	    pHost->cReset [port])
	    {
	    hubStatus |= USB_HUB_ENDPOINT_STS_PORT0 << port;
	    }
	}

    return hubStatus;
    }


/***************************************************************************
*
* intThread - Thread invoked when hardware interrupts are detected
*
* By convention, the <param> to this thread is a pointer to an HCD_HOST.
* This thread waits on the intPending semaphore in the HCD_HOST which is
* signalled by the actual interrupt handler.  This thread will continue
* to process interrupts until intThreadExitRequest is set to TRUE in the
* HCD_HOST structure.
*
* This thread wakes up every UHC_ROOT_SRVC_INTERVAL milliseconds and
* looks for IRPs on the root IRP list.	If any are present, this thread
* will check the root hub change status.  If there is a pending change,
* the IRP will be completed and its usbdCallback routine invoked.  The
* thread also checks for IRPs which have timed-out during this interval.
*
* RETURNS: N/A
*/

LOCAL VOID intThread
    (
    pVOID param
    )

    {
    pHCD_HOST pHost = (pHCD_HOST) param;
    UINT32 lastTime;
    UINT32 now;
    UINT32 interval;
    UINT8 hubStatus;
    UINT16 irpCount;
    pUSB_IRP pIrp;

    lastTime = OSS_TIME ();

    do
	{
	/* Wait for an interrupt to be signalled. */

	now = OSS_TIME ();
	interval = UHC_ROOT_SRVC_INTERVAL - 
		   min (now - lastTime, UHC_ROOT_SRVC_INTERVAL);

	if (OSS_SEM_TAKE (pHost->intPending, interval) == OK)
	    {
	    /* semaphore was signalled, int pending */

	    if (!pHost->intThreadExitRequest)
		{
		OSS_MUTEX_TAKE (pHost->hostMutex, OSS_BLOCK);
		processUhcInterrupt (pHost);
		OSS_MUTEX_RELEASE (pHost->hostMutex);
		}
	    }

	if ((now = OSS_TIME ()) - lastTime >= UHC_ROOT_SRVC_INTERVAL)
	    {
	    /* time to srvc root */

	    lastTime = now;

	    /* Take the hostMutex before continuing. */

	    OSS_MUTEX_TAKE (pHost->hostMutex, OSS_BLOCK);


	    /* Service any IRPs which may be addressed to the root hub */

	    /* If there is a pending status, then notify all IRPs
	     * currently pending on the root status endpoint.
	     *
	     * NOTE: The IRP callbacks may re-queue new requests.
	     * Those will be added to the end of the list and will
	     * not be removed in the following loop.
	     */

	    if ((hubStatus = isHubStatus (pHost)) != 0)
		{
		for (irpCount = pHost->rootIrpCount; irpCount > 0; 
		    irpCount--)
		    {

		    pIrp = usbListFirst (&pHost->rootIrps);

		    --pHost->rootIrpCount;
		    usbListUnlink (&pIrp->hcdLink);

		    *(pIrp->bfrList [0].pBfr) = hubStatus;
		    pIrp->bfrList [0].actLen = 1;

		    setIrpResult (pIrp, OK);
		    }
		}


	    /* Look for IRPs which may have timed-out. */

	    checkIrpTimeout (pHost);


	    /* Release the hostMutex */

	    OSS_MUTEX_RELEASE (pHost->hostMutex);
	    }
	}
    while (!pHost->intThreadExitRequest);


    /* Signal that we've acknowledged the exit request */

    OSS_SEM_GIVE (pHost->intThreadExit);
    }


/***************************************************************************
*
* intHandler - hardware interrupt handler
*
* This is the actual routine which receives hardware interrupts from the
* UHC.	This routine immediately reflects the interrupt to the intThread.
* interrupt handlers have execution restrictions which are not imposed
* on normal threads...So, this scheme gives the intThread complete
* flexibility to call other services and libraries while processing the
* interrupt condition.
*
* RETURNS: N/A
*/

LOCAL VOID intHandler
    (
    pVOID param
    )

    {
    pHCD_HOST pHost = (pHCD_HOST) param;
    UINT16 intBits;


    /* Is there an interrupt pending in the UHCI status reg? */

    if ((intBits = (UHC_WORD_IN (UHCI_USBSTS) & UHC_INT_PENDING_MASK)) != 0)
	{

	pHost->intCount++;

	/* A USB interrupt is pending. Record the interrupting condition */

	pHost->intBits |= intBits;

	/* Clear the bit(s). 
	 *
	 * NOTE: The UHC hardware generates "short packet detect"
	 * repeatedly until the QH causing the interrupt is removed from
	 * the work list.  These extra interrupts are benign and occur at
	 * at relatively low frequency.  Under normal operation, the
	 * interrupt thread will clean up the condition quickly, and the
	 * extra interrupts are tolerable.
	 */

	UHC_SET_BITS (UHCI_USBSTS, intBits);

	/* Signal the interrupt thread to process the interrupt. */

	OSS_SEM_GIVE (pHost->intPending);
	}

	if(pHost->hcSyncSem) 
	    semGive (pHost->hcSyncSem);

    }


/***************************************************************************
*
* validateHrb - validate an HRB
*
* Checks the HRB length set in the HRB_HEADER against the <expectedLen>
* passed by the caller. 
*
* RETURNS: S_usbHcdLib_xxxx
*/

LOCAL int validateHrb
    (
    pVOID pHrb,
    UINT16 expectedLen
    )

    {
    pHRB_HEADER pHeader = (pHRB_HEADER) pHrb;

    if (pHeader->hrbLength != expectedLen)
	return S_usbHcdLib_BAD_PARAM;

    return OK;
    }


/***************************************************************************
*
* destroyPipe - tears down an HCD_PIPE
*
* RETURNS: N/A
*/

LOCAL void destroyPipe
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe
    )

    {
    /* Release bandwidth associated with pipe. */

    pHost->nanoseconds -= pPipe->time;


    /* Release pipe */

    usbListUnlink (&pPipe->link);
    usbHandleDestroy (pPipe->pipeHandle);
    OSS_FREE (pPipe);
    }


/***************************************************************************
*
* destroyHost - tears down an HCD_HOST
*
* RETURNS: N/A
*/

LOCAL VOID destroyHost
    (
    pHCD_HOST pHost
    )
    
    {
    pUSB_IRP pIrp;
    pHCD_PIPE pPipe;


    /* Mark host as being shut down */

    pHost->shutdown = TRUE;


    /* release any pending IRPs */

    while ((pIrp = usbListFirst (&pHost->rootIrps)) != NULL)
	cancelIrp (pHost, pIrp, S_usbHcdLib_IRP_CANCELED);

    while ((pIrp = usbListFirst (&pHost->busIrps)) != NULL)
	cancelIrp (pHost, pIrp, S_usbHcdLib_IRP_CANCELED);


    /* release any pending pipes */

    while ((pPipe = usbListFirst (&pHost->pipes)) != NULL)
	destroyPipe (pHost, pPipe);


    /* Disable the UHC */

    UHC_CLR_BITS (UHCI_USBCMD, UHCI_CMD_RS);
    UHC_SET_BITS (UHCI_USBCMD, UHCI_CMD_HCRESET);


    /* restore original interrupt handler */

    if (pHost->intInstalled)
	usbPciIntRestore (intHandler, pHost, pHost->pciCfgHdr.intLine);


    /* terminate/destroy interrupt handler thread */

    if (pHost->intThread != NULL)
	{
	/* Terminate the interrupt service thread */

	pHost->intThreadExitRequest = TRUE;
	OSS_SEM_GIVE (pHost->intPending);
	OSS_SEM_TAKE (pHost->intThreadExit, INT_TIMEOUT);
	OSS_THREAD_DESTROY (pHost->intThread);
	}


    /* release other resources */

    if (pHost->hostMutex != NULL)
	OSS_MUTEX_DESTROY (pHost->hostMutex);

    if (pHost->intThreadExit != NULL)
	OSS_SEM_DESTROY (pHost->intThreadExit);

    if (pHost->intPending != NULL)
	OSS_SEM_DESTROY (pHost->intPending);

    if (pHost->handle != NULL)
	usbHandleDestroy (pHost->handle);


    /* eliminate QH anchors */

    if (pHost->pBulkAnchorQh != NULL)
	DMA_FREE (pHost->pBulkAnchorQh);

    if (pHost->pCtlAnchorQh != NULL)
	DMA_FREE (pHost->pCtlAnchorQh);

    if (pHost->pIntAnchorQhs != NULL)
	DMA_FREE (pHost->pIntAnchorQhs);


    /* eliminate frame list */

    if (pHost->pFrameList != NULL)
	DMA_FREE (pHost->pFrameList);


    /* eliminate DMA memory pool */

    if (pHost->dmaPool != NULL)
	cacheDmaFree (pHost->dmaPool);

    OSS_FREE (pHost);
    }


/***************************************************************************
*
* fncAttach - Initialize the HCD and attach to specified bus(es)
*
* The convention for the UHCI HCD is that the param passed in the HRB
* is a pointer to a PCI_CFG_HEADER structure for the UHC to be managed.
*
* RETURNS: S_usbHcdLib_xxxx
*/

LOCAL int fncAttach
    (
    pHRB_ATTACH pHrb
    )

    {
    pHCD_HOST pHost;
    pPCI_CFG_HEADER pCfgHdr;
    UINT32 ioBase;
    int i;

    int s;

    /* validate parameters */

    if ((s = validateHrb (pHrb, sizeof (*pHrb))) != OK)
	return s;


    /* Check to make sure structures compiled to correct size */

    if (UHCI_TD_LEN != UHCI_TD_ACTLEN || UHCI_QH_LEN != UHCI_QH_ACTLEN)
	return S_usbHcdLib_STRUCT_SIZE_FAULT;


    /* determine io base address */

    pCfgHdr = (pPCI_CFG_HEADER) pHrb->param;
    ioBase = 0;

    for (i = 0; i < PCI_CFG_NUM_BASE_REG; i++)
	if ((pCfgHdr->baseReg [i] & PCI_CFG_BASE_IO) != 0 &&
	    (ioBase = pCfgHdr->baseReg [i] & PCI_CFG_IOBASE_MASK) != 0)
	    break;

    if (ioBase == 0)
	return S_usbHcdLib_HW_NOT_READY;


    /* create/initialize an HCD_HOST structure to manage the UHC. */

    if ((pHost = OSS_CALLOC (sizeof (*pHost))) == NULL)
	return S_usbHcdLib_OUT_OF_MEMORY;

    memcpy (&pHost->pciCfgHdr, pCfgHdr, sizeof (*pCfgHdr));

    pHost->mngmtCallback = pHrb->mngmtCallback;
    pHost->mngmtCallbackParam = pHrb->mngmtCallbackParam;


    /* Determine characteristics of UHC */

    pHost->uhcAttributes = getUhcAttributes (pCfgHdr);


    /* initialize pHost->ioBase...cannot use UHC_SET_BITS, etc. until 
     * ioBase is initialized.
     */

    pHost->ioBase = ioBase;


    /* reset the UHC 
     *
     * NOTE: The HCRESET bit automatically clears itself after reset. 
     */

    UHC_SET_BITS (UHCI_USBCMD, UHCI_CMD_HCRESET);

    if (!waitOnBits (pHost, UHCI_USBCMD, UHCI_CMD_HCRESET, 0))
	{
	destroyHost (pHost);
	return S_usbHcdLib_HW_NOT_READY;
	}


    /* Allocate structures, resources, etc. */

    if (
	(pHost->dmaPool = cacheDmaMalloc (DMA_MEMORY_SIZE)) == NULL ||
	(pHost->memPartId = memPartCreate (pHost->dmaPool, DMA_MEMORY_SIZE)) == NULL 
	||
	(pHost->pFrameList = DMA_MALLOC (sizeof (UHCI_FRAME_LIST),
			     UHCI_FRAME_LIST_ALIGNMENT)) == NULL 
	||
	(pHost->pIntAnchorQhs = DMA_MALLOC (INT_ANCHOR_LIST_SIZE, 
					    UHCI_QH_ALIGNMENT)) == NULL 
	||
	(pHost->pCtlAnchorQh = DMA_MALLOC (sizeof (QH_WRAPPER),
					   UHCI_QH_ALIGNMENT)) == NULL 
	||
	(pHost->pBulkAnchorQh = DMA_MALLOC (sizeof (QH_WRAPPER),
					    UHCI_QH_ALIGNMENT)) == NULL 
	||
	usbHandleCreate (HCD_HOST_SIG, pHost, &pHost->handle) != OK 
	||
	OSS_SEM_CREATE (MAX_INT_DEPTH, 0, &pHost->intPending) != OK 
	||
	OSS_SEM_CREATE (1, 0, &pHost->intThreadExit) != OK 
	||
	OSS_MUTEX_CREATE (&pHost->hostMutex) != OK 
	||
	OSS_THREAD_CREATE (intThread, pHost, OSS_PRIORITY_INTERRUPT,
			   "tUhciInt", &pHost->intThread) != OK
	)
	{
	destroyHost (pHost);
	return S_usbHcdLib_OUT_OF_RESOURCES;
	}
    /*
     * Create a semaphore to syncronize the host controller.  This is used
     * to make the HC wait a frame for traffic to settle
     */
 
    pHost->hcSyncSem = semBCreate(SEM_Q_FIFO, SEM_EMPTY);

    if (pHost->hcSyncSem == NULL)
        return S_usbHcdLib_OUT_OF_RESOURCES;
	

    memset (pHost->pIntAnchorQhs, 0, INT_ANCHOR_LIST_SIZE);
    memset (pHost->pCtlAnchorQh, 0, sizeof (QH_WRAPPER));
    memset (pHost->pBulkAnchorQh, 0, sizeof (QH_WRAPPER));

    
    /* initialize the emulated root hub descriptor */

    pHost->hubDescr.length = USB_HUB_DESCR_LEN;
    pHost->hubDescr.descriptorType = USB_DESCR_HUB;
    pHost->hubDescr.nbrPorts = UHCI_PORTS;
    pHost->hubDescr.hubCharacteristics = TO_LITTLEW (USB_HUB_INDIVIDUAL_POWER |
						     USB_HUB_NOT_COMPOUND | 
						     USB_HUB_INDIVIDUAL_OVERCURRENT);
    pHost->hubDescr.pwrOn2PwrGood = UHCI_PWR_ON_2_PWR_GOOD;
    pHost->hubDescr.hubContrCurrent = UHCI_HUB_CONTR_CURRENT;
    pHost->hubDescr.deviceRemovable [0] = 0;
    pHost->hubDescr.portPwrCtrlMask [0] = 0xff; /* per hub spec. */


    /* initialize the emulated endpoint descriptor */

    pHost->endpntDescr.length = USB_ENDPOINT_DESCR_LEN;
    pHost->endpntDescr.descriptorType = USB_DESCR_ENDPOINT;
    pHost->endpntDescr.endpointAddress = UHC_STATUS_ENDPOINT_ADRS;
    pHost->endpntDescr.attributes = USB_ATTR_INTERRUPT;
    pHost->endpntDescr.maxPacketSize = TO_LITTLEW (UHC_MAX_PACKET_SIZE);
    pHost->endpntDescr.interval = UHCI_HUB_INTERVAL;


    /* initialize the emulated interface descriptor */

    pHost->ifDescr.length = USB_INTERFACE_DESCR_LEN;
    pHost->ifDescr.descriptorType = USB_DESCR_INTERFACE;
    pHost->ifDescr.numEndpoints = 1;
    pHost->ifDescr.interfaceClass = USB_CLASS_HUB;


    /* initialize the emulated configuration descriptor */

    pHost->configDescr.length = USB_CONFIG_DESCR_LEN;
    pHost->configDescr.descriptorType = USB_DESCR_CONFIGURATION;
    pHost->configDescr.totalLength = TO_LITTLEW (pHost->configDescr.length +
						 pHost->ifDescr.length + 
						 pHost->endpntDescr.length);
    pHost->configDescr.numInterfaces = 1;
    pHost->configDescr.configurationValue = UHC_CONFIG_VALUE;
    pHost->configDescr.attributes = USB_ATTR_SELF_POWERED;
    pHost->configDescr.maxPower = UHCI_HUB_CONTR_CURRENT;


    /* initialize the emulated device descriptor */

    pHost->devDescr.length = sizeof (USB_DEVICE_DESCR);
    pHost->devDescr.descriptorType = USB_DESCR_DEVICE;
    pHost->devDescr.bcdUsb = TO_LITTLEW (USB_RELEASE);
    pHost->devDescr.deviceClass = USB_CLASS_HUB;
    pHost->devDescr.maxPacketSize0 = UHC_MAX_PACKET_SIZE;
    pHost->devDescr.manufacturerIndex = UHC_STR_MFG;
    pHost->devDescr.productIndex = UHC_STR_PROD;
    pHost->devDescr.numConfigurations = 1;


    /* initialize other hub/port state information */

    pHost->rootAddress = 0;
    pHost->configValue = 0;
    

    /* NOTE: the initialization of powered[] will change if power-
     * management code is added.
     */

    for (i = 0; i < UHCI_PORTS; i++)
	pHost->powered [i] = TRUE;
    

    /* initialize the frame list and program the UHC frame list register */

    pHost->pciPtrToBulkAnchorQh = QH_TO_PCIPTR (pHost->pBulkAnchorQh);
    pHost->pLastBulkQh = pHost->pBulkAnchorQh;

    pHost->pBulkAnchorQh->qh.qhLink = UHC_END_OF_LIST;
    pHost->pBulkAnchorQh->qh.tdLink = UHC_END_OF_LIST;
    DMA_FLUSH (pHost->pBulkAnchorQh, sizeof (*pHost->pBulkAnchorQh));

    pHost->pLastCtlQh = pHost->pCtlAnchorQh;

    pHost->pCtlAnchorQh->qh.qhLink = pHost->pciPtrToBulkAnchorQh;
    pHost->pCtlAnchorQh->qh.tdLink = UHC_END_OF_LIST;
    DMA_FLUSH (pHost->pCtlAnchorQh, sizeof (*pHost->pCtlAnchorQh));

    for (i = 0; i < UHCI_FRAME_LIST_ENTRIES; i++)
	{
	pHost->pIntAnchorQhs [i].qh.qhLink = QH_TO_PCIPTR (pHost->pCtlAnchorQh);
	pHost->pIntAnchorQhs [i].qh.tdLink = UHC_END_OF_LIST;

	pHost->pFrameList->linkPtr [i] = QH_TO_PCIPTR (&pHost->pIntAnchorQhs [i]);
	}

    DMA_FLUSH (pHost->pIntAnchorQhs, INT_ANCHOR_LIST_SIZE);
    DMA_FLUSH (pHost->pFrameList, sizeof (*pHost->pFrameList));

    UHC_DWORD_OUT (UHCI_FRBASEADD, USB_MEM_TO_PCI (pHost->pFrameList));

    UHC_SET_BITS (UHCI_USBCMD, UHCI_CMD_MAXP);	/* allow 64-byte packets */
    UHC_SET_BITS (UHCI_USBCMD, UHCI_CMD_RS);


    /* hook the hardware interrupt for the UHC */

    if (usbPciIntConnect (intHandler, pHost, pHost->pciCfgHdr.intLine) != OK)
	{
	destroyHost (pHost);
	return S_usbHcdLib_INT_HOOK_FAILED;
	}

    pHost->intInstalled = TRUE;


    /* Enable interrupts */

    UHC_SET_BITS (UHCI_USBINTR, UHC_INT_ENABLE_MASK);


    /* return handle to caller */

    pHrb->header.handle = pHost->handle;
    pHrb->busCount = 1;

    return OK;
    }


/***************************************************************************
*
* fncDetach - Disconnect from specified bus(es) and shut down
*
* RETURNS: S_usbHcdLib_xxxx
*/

LOCAL int fncDetach
    (
    pHRB_DETACH pHrb,
    pHCD_HOST pHost
    )

    {
    int s;

    /* validate parameters */

    if ((s = validateHrb (pHrb, sizeof (*pHrb))) != OK)
	return s;


    /* Release resources associated with this HCD_HOST */

    destroyHost (pHost);

    return OK;
    }


/***************************************************************************
*
* fncSetBusState - Sets bus suspend/resume state
*
* RETURNS: S_usbHcdLib_xxxx
*/

LOCAL int fncSetBusState
    (
    pHRB_SET_BUS_STATE pHrb,
    pHCD_HOST pHost
    )

    {
    if ((pHrb->busState & HCD_BUS_SUSPEND) != 0)
	{
	/* SUSPEND bus */

	if (!pHost->suspended)
	    {
	    pHost->suspended = TRUE;

	    /* Turn off the Run/stop bit...
	     *
	     * As a biproduct of this, all outstanding ERPs will
	     * eventually time-out.
	     */

	    UHC_CLR_BITS (UHCI_USBCMD, UHCI_CMD_RS);
	    waitOnBits (pHost, UHCI_USBCMD, UHCI_CMD_RS, 0);

	    /* Force global suspend */

	    UHC_SET_BITS (UHCI_USBCMD, UHCI_CMD_EGSM);
	    }
	}

    if ((pHrb->busState & HCD_BUS_RESUME) != 0)
	{
	/* RESUME bus */

	if (pHost->suspended)
	    {
	    pHost->suspended = FALSE;

	    /* stop forcing suspend */

	    UHC_CLR_BITS (UHCI_USBCMD, UHCI_CMD_EGSM);
	
	    /* Force global resume */

	    UHC_SET_BITS (UHCI_USBCMD, UHCI_CMD_FGR);
	    OSS_THREAD_SLEEP (USB_TIME_RESUME);
	    UHC_CLR_BITS (UHCI_USBCMD, UHCI_CMD_FGR);

	    /* Re-enable Run/stop */

	    UHC_SET_BITS (UHCI_USBCMD, UHCI_CMD_RS);
	    }
	}

    return OK;
    }


/***************************************************************************
*
* fncCurrentFrameGet - Return current bus frame number
*
* RETURNS: S_usbHcdLib_xxxx
*/

LOCAL int fncCurrentFrameGet
    (
    pHRB_CURRENT_FRAME_GET pHrb,
    pHCD_HOST pHost
    )

    {
    int s;

    /* validate parameters */

    if ((s = validateHrb (pHrb, sizeof (*pHrb))) != OK)
	return s;

    /* Return the current frame number and the size of the frame 
     * scheduling window
     */

    pHrb->frameNo = getFrameNo (pHost);

    /* NOTE: We don't expose the UHC's frame counter for the frame
     * scheduling window.  Instead, we expose the size of the frame
     * list.  Otherwise, we would need to keep track of each time the
     * frame counter rolls over.
     */

    pHrb->frameWindow = UHCI_FRAME_LIST_ENTRIES;

    return OK;
    }


/***************************************************************************
*
* rootFeature - sets/clears a root feature
*
* RETURNS: S_usbHcdLib_xxxx
*/

LOCAL int rootFeature
    (
    pHCD_HOST pHost,
    pUSB_SETUP pSetup,
    BOOL setFeature
    )

    {
    UINT16 port;
    UINT16 portReg;
    BOOL bitsOk;
    int s = OK;

    if ((pSetup->requestType & ~USB_RT_DEV_TO_HOST) == 
	(USB_RT_CLASS | USB_RT_DEVICE))
	{
	/* hub class features, directed to hub itself */

	switch (FROM_LITTLEW (pSetup->value))
	    {
	    case USB_HUB_FSEL_C_HUB_LOCAL_POWER:
	    case USB_HUB_FSEL_C_HUB_OVER_CURRENT:
	
		/* Not implemented for UHCI */
		break;

	    }
	}
    else if ((pSetup->requestType & ~USB_RT_DEV_TO_HOST) ==
	     (USB_RT_CLASS | USB_RT_OTHER))
	{
	/* port class features, directed to port */

	/* NOTE: port parameter is 1-based, not 0-based, in Setup packet. */

	if ((port = FROM_LITTLEW (pSetup->index) - 1) >= UHCI_PORTS)
	    return S_usbHcdLib_BAD_PARAM;

	portReg = UHCI_PORTSC1 + port * 2;

	switch (FROM_LITTLEW (pSetup->value))
	    {
	    case USB_HUB_FSEL_PORT_ENABLE:

		if (setFeature)
		    {
		    if (pHost->powered [port])
			{
			UHC_SET_BITS (portReg, UHCI_PORT_ENABLE);
			if (!waitOnBits (pHost, portReg, UHCI_PORT_ENABLE, 
			    UHCI_PORT_ENABLE))
			    s = S_usbHcdLib_HW_NOT_READY;	
			}
		    }
		else
		    {
		    UHC_CLR_BITS (portReg, UHCI_PORT_ENABLE);
		    if (!waitOnBits (pHost, portReg, UHCI_PORT_ENABLE, 0))
			s = S_usbHcdLib_HW_NOT_READY;
		    }
		break;

	    case USB_HUB_FSEL_PORT_SUSPEND:

		if (setFeature)
		    {
		    if ((UHC_WORD_IN (portReg) & UHCI_PORT_SUSPEND) == 0)
			{
			UHC_CLR_BITS (portReg, UHCI_PORT_RESUME);
			bitsOk = waitOnBits (pHost, portReg, 
					     UHCI_PORT_RESUME, 0);

			UHC_SET_BITS (portReg, UHCI_PORT_SUSPEND);
			bitsOk = bitsOk && waitOnBits (pHost, portReg, 
						       UHCI_PORT_SUSPEND, 
						       UHCI_PORT_SUSPEND);

			pHost->cSuspend [port] = TRUE;

			if (!bitsOk)
			    s = S_usbHcdLib_HW_NOT_READY;
			}
		    }
		else
		    {
		    if ((UHC_WORD_IN (portReg) & UHCI_PORT_SUSPEND) != 0)
			{
			UHC_CLR_BITS (portReg, UHCI_PORT_SUSPEND);
			bitsOk = waitOnBits (pHost, portReg, 
					     UHCI_PORT_SUSPEND, 0);

			UHC_SET_BITS (portReg, UHCI_PORT_RESUME);
			bitsOk = bitsOk && waitOnBits (pHost, portReg, 
				 UHCI_PORT_RESUME, UHCI_PORT_RESUME);

			pHost->cSuspend [port] = FALSE;

			if (!bitsOk)
			    s = S_usbHcdLib_HW_NOT_READY;
			}
		    }
		break;
	    
	    case USB_HUB_FSEL_PORT_RESET:

		if (setFeature && pHost->powered [port])
		    {
		    UHC_SET_BITS (portReg, UHCI_PORT_RESET);
		    bitsOk = waitOnBits (pHost, portReg, UHCI_PORT_RESET,
		    UHCI_PORT_RESET);

		    OSS_THREAD_SLEEP (USB_TIME_RESET);

		    UHC_CLR_BITS (portReg, UHCI_PORT_RESET);
		    bitsOk = bitsOk && waitOnBits (pHost, portReg, 
		    UHCI_PORT_RESET, 0);

		    /* Immediately after reset the UHC (VT82C586B) appears to
		     * miss the following write to enable the port.  So, we loop
		     * here until the enable bit is recognized.
		     */

		    do
			{
			UHC_SET_BITS (portReg, UHCI_PORT_ENABLE);
			}
		    while ((UHC_WORD_IN (portReg) & UHCI_PORT_ENABLE) == 0);

		    OSS_THREAD_SLEEP (USB_TIME_RESET_RECOVERY);

		    pHost->cReset [port] = TRUE;

		    if (!bitsOk)
			s = S_usbHcdLib_HW_NOT_READY;
		    }
		break;

	    case USB_HUB_FSEL_PORT_POWER:

		pHost->powered [port] = setFeature;
		break;

	    case USB_HUB_FSEL_C_PORT_CONNECTION:

		if (!setFeature)
		    UHC_SET_BITS (portReg, UHCI_PORT_CNCTCHG);
		break;

	    case USB_HUB_FSEL_C_PORT_ENABLE:

		if (!setFeature)
		    UHC_SET_BITS (portReg, UHCI_PORT_ENBCHG);
		break;
	
	    case USB_HUB_FSEL_C_PORT_SUSPEND:

		if (!setFeature)
		    pHost->cSuspend [port] = FALSE;
		break;

	    case USB_HUB_FSEL_C_PORT_OVER_CURRENT:

		break;

	    case USB_HUB_FSEL_C_PORT_RESET:

		if (!setFeature) 
		    pHost->cReset [port] = FALSE;
		break;
	    }
	}

    return s;
    }


/***************************************************************************
*
* rootGetDescriptor - process a get descriptor request
*
* RETURNS: S_usbHcdLib_xxxx
*/

LOCAL int rootGetDescriptor
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp,
    pUSB_SETUP pSetup
    )

    {
    UINT8 bfr [USB_CONFIG_DESCR_LEN + USB_INTERFACE_DESCR_LEN
    + USB_ENDPOINT_DESCR_LEN];

    int s = OK;


    if (pIrp->bfrCount < 2 || pIrp->bfrList [1].pBfr == NULL)
	return S_usbHcdLib_BAD_PARAM;

    if (pSetup->requestType == (USB_RT_DEV_TO_HOST | USB_RT_STANDARD | 
				USB_RT_DEVICE))
	{
	switch (MSB (FROM_LITTLEW (pSetup->value)))
	    {
	    case USB_DESCR_DEVICE:	
		usbDescrCopy32 (pIrp->bfrList [1].pBfr, 
				&pHost->devDescr, 
				pIrp->bfrList [1].bfrLen, 
				&pIrp->bfrList [1].actLen);
		break;

	    case USB_DESCR_CONFIGURATION:
		memcpy (bfr, 
			&pHost->configDescr, 
			USB_CONFIG_DESCR_LEN);
		memcpy (&bfr [USB_CONFIG_DESCR_LEN], 
			&pHost->ifDescr,
			USB_INTERFACE_DESCR_LEN);
		memcpy (&bfr [USB_CONFIG_DESCR_LEN + 
			USB_INTERFACE_DESCR_LEN],
			&pHost->endpntDescr, 
			USB_ENDPOINT_DESCR_LEN);

		pIrp->bfrList [1].actLen = min (pIrp->bfrList [1].bfrLen,
						USB_CONFIG_DESCR_LEN + 
						USB_INTERFACE_DESCR_LEN + 
						USB_ENDPOINT_DESCR_LEN);

		memcpy (pIrp->bfrList [1].pBfr, bfr, pIrp->bfrList [1].actLen);

		break;

	    case USB_DESCR_INTERFACE:
		usbDescrCopy32 (pIrp->bfrList [1].pBfr, 
				&pHost->ifDescr, 
				pIrp->bfrList [1].bfrLen, 
				&pIrp->bfrList [1].actLen);
		break;

	    case USB_DESCR_ENDPOINT:
		usbDescrCopy32 (pIrp->bfrList [1].pBfr, 
				&pHost->endpntDescr, 
				pIrp->bfrList [1].bfrLen, 
				&pIrp->bfrList [1].actLen);
		break;
		
	    case USB_DESCR_STRING:
		switch (LSB (FROM_LITTLEW (pSetup->value)))
		    {
		    case 0: /* language descriptor */
			usbDescrCopy32 (pIrp->bfrList [1].pBfr, 
					&langDescr, 
					pIrp->bfrList [1].bfrLen, 
					&pIrp->bfrList [1].actLen);
			break;

		    case UHC_STR_MFG:
			usbDescrStrCopy32 (pIrp->bfrList [1].pBfr, 
					   UHC_STR_MFG_VAL, 
					   pIrp->bfrList [1].bfrLen, 
					   &pIrp->bfrList [1].actLen);
			break;

		    case UHC_STR_PROD:
			usbDescrStrCopy32 (pIrp->bfrList [1].pBfr, 
					   UHC_STR_PROD_VAL,
					   pIrp->bfrList [1].bfrLen, 
					   &pIrp->bfrList [1].actLen);
			break;
		    }
		break;
	    }
	}
    else if (pSetup->requestType == (USB_RT_DEV_TO_HOST | 
				     USB_RT_CLASS | 
				     USB_RT_DEVICE))
	{
	if (MSB (FROM_LITTLEW (pSetup->value)) == USB_DESCR_HUB)
	    usbDescrCopy32 (pIrp->bfrList [1].pBfr, 
			    &pHost->hubDescr, 
			    pIrp->bfrList [1].bfrLen, 
			    &pIrp->bfrList [1].actLen);
	}

    return s;
    }


/***************************************************************************
*
* copyStatus - copies status
*
* RETURNS: N/A
*/

LOCAL VOID copyStatus
    (
    pUSB_BFR_LIST pBfrList,
    pVOID pStatus,
    UINT16 maxLen
    )

    {
    pBfrList->actLen = min (pBfrList->bfrLen, maxLen);
    memcpy (pBfrList->pBfr, pStatus, pBfrList->actLen);
    }


/***************************************************************************
*
* rootGetStatus - process a get status request
*
* RETURNS: S_usbHcdLib_xxxx
*/

LOCAL int rootGetStatus
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp,
    pUSB_SETUP pSetup
    )

    {
    union 
    {
    USB_STANDARD_STATUS std;
    USB_HUB_STATUS hub;
    } status;
    
    UINT16 port;
    UINT16 portReg;
    UINT16 portStatus;


    /* validate parameters */

    if (pIrp->bfrCount < 2 || pIrp->bfrList [1].pBfr == NULL)
	return S_usbHcdLib_BAD_PARAM;


    /* return status based on type of request received */

    memset (&status, 0, sizeof (status));

    if (pSetup->requestType == (USB_RT_DEV_TO_HOST | 
				USB_RT_STANDARD | 
				USB_RT_DEVICE))
	{
	/* retrieve standard device status */

	status.std.status = TO_LITTLEW (USB_DEV_STS_LOCAL_POWER);
	copyStatus (&pIrp->bfrList [1], &status.std, sizeof (status.std));
	}
    else if (pSetup->requestType == (USB_RT_DEV_TO_HOST | 
				     USB_RT_STANDARD | 
				     USB_RT_INTERFACE))
	{
	/* retrieve status of interface */

	status.std.status = TO_LITTLEW (0);
	copyStatus (&pIrp->bfrList [1], &status.std, sizeof (status.std));
	}
    else if (pSetup->requestType == (USB_RT_DEV_TO_HOST | 
				     USB_RT_STANDARD | 
				     USB_RT_ENDPOINT))
	{
	/* retrieve status of endpoint */

	status.std.status = TO_LITTLEW (0);
	copyStatus (&pIrp->bfrList [1], &status.std, sizeof (status.std));
	}
    else if (pSetup->requestType == (USB_RT_DEV_TO_HOST | 
				     USB_RT_CLASS | 
				     USB_RT_DEVICE))
	{
	/* retrieve hub status */

	status.hub.status = TO_LITTLEW (USB_HUB_STS_LOCAL_POWER);
	status.hub.change = TO_LITTLEW (0);
	copyStatus (&pIrp->bfrList [1], &status.hub, sizeof (status.hub));
	}
    else if (pSetup->requestType == (USB_RT_DEV_TO_HOST | 
				     USB_RT_CLASS | 
				     USB_RT_OTHER))
	{
	/* retrieve hub port status */

	/* NOTE: port index is 1-based, not 0-based, in Setup packet. */

	if ((port = FROM_LITTLEW (pSetup->index) - 1) >= UHCI_PORTS)
	    return S_usbHcdLib_BAD_PARAM;

	portReg = UHCI_PORTSC1 + port * 2;
	portStatus = UHC_WORD_IN (portReg);

	/* initialize port status word */

	if ((portStatus & UHCI_PORT_CNCTSTS) != 0)
	    status.hub.status |= TO_LITTLEW (USB_HUB_STS_PORT_CONNECTION);

	if ((portStatus & UHCI_PORT_ENABLE) != 0)
	    status.hub.status |= TO_LITTLEW (USB_HUB_STS_PORT_ENABLE);

	if ((portStatus & UHCI_PORT_SUSPEND) != 0)
	    status.hub.status |= TO_LITTLEW (USB_HUB_STS_PORT_SUSPEND);

	if (pHost->powered [port])
	    status.hub.status |= TO_LITTLEW (USB_HUB_STS_PORT_POWER);

	if ((portStatus & UHCI_PORT_LOWSPEED) != 0)
	    status.hub.status |= TO_LITTLEW (USB_HUB_STS_PORT_LOW_SPEED);

	/* initialize port change word */

	if ((portStatus & UHCI_PORT_CNCTCHG) != 0)
	    status.hub.change |= TO_LITTLEW (USB_HUB_C_PORT_CONNECTION);

	if ((portStatus & UHCI_PORT_ENBCHG) != 0)
	    status.hub.change |= TO_LITTLEW (USB_HUB_C_PORT_ENABLE);

	if (pHost->cSuspend [port])
	    status.hub.change |= TO_LITTLEW (USB_HUB_C_PORT_SUSPEND);

	if (pHost->cReset [port])
	    status.hub.change |= TO_LITTLEW (USB_HUB_C_PORT_RESET);

	copyStatus (&pIrp->bfrList [1], &status.hub, sizeof (status.hub));
	}

    return OK;
    }


/***************************************************************************
*
* rootControl - interprets a transfer to the root hub control pipe
*
* RETURNS: S_usbHcdLib_xxxx
*/

LOCAL int rootControl
    (
    pHCD_HOST pHost,
    pUSB_IRP pIrp
    )

    {
    pUSB_SETUP pSetup;
    int s = OK;


    /* Control transfers always place the Setup packet in the first
     * USB_BFR_LIST entry in the IRP and any optional data is placed
     * in the second.
     */

    if (pIrp->bfrCount < 1 
        || pIrp->bfrList [0].bfrLen < sizeof (USB_SETUP)
        || (pSetup = (pUSB_SETUP) pIrp->bfrList [0].pBfr) 
	== NULL)

	return S_usbHcdLib_BAD_PARAM;

    /* execute request */

    switch (pSetup->request)
	{
	case USB_REQ_CLEAR_FEATURE:

	    s = rootFeature (pHost, pSetup, FALSE);
	    break;

	case USB_REQ_SET_FEATURE:

	    s = rootFeature (pHost, pSetup, TRUE);
	    break;

	case USB_REQ_GET_CONFIGURATION:

	    if (pIrp->bfrCount < 2 || pIrp->bfrList [1].pBfr == NULL)
		{
		s = S_usbHcdLib_BAD_PARAM;
		break;
		}

	    *pIrp->bfrList [1].pBfr = pHost->configValue;
	    break;

	case USB_REQ_SET_CONFIGURATION:

	    pHost->configValue = LSB (FROM_LITTLEW (pSetup->value));
	    break;

	case USB_REQ_GET_DESCRIPTOR:

	    s = rootGetDescriptor (pHost, pIrp, pSetup);
	    break;

	case USB_REQ_GET_STATUS:

	    s = rootGetStatus (pHost, pIrp, pSetup);
	    break;

	case USB_REQ_SET_ADDRESS:

	    pHost->rootAddress = FROM_LITTLEW (pSetup->value);
	    break;

	default:
	    s = S_usbHcdLib_NOT_SUPPORTED;
	    break;
	}

    return s;
    }


/***************************************************************************
*
* rootInterrupt - interprets an interrupt endpoint request
*
* RETURNS: S_usbHcdLib_xxxx
*/

LOCAL int rootInterrupt
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe,
    pUSB_IRP pIrp
    )

    {
    int s = PENDING;

    /* make sure the request is to the correct endpoint, etc. */

    if (pPipe->endpoint != UHC_STATUS_ENDPOINT_ADRS ||
			   pIrp->bfrCount < 1 || 
			   pIrp->bfrList [0].pBfr 
			   == NULL)
	
	return S_usbHcdLib_BAD_PARAM;


    /* link this IRP to the list of root IRPs */

    usbListLink (&pHost->rootIrps, pIrp, &pIrp->hcdLink, LINK_TAIL);
    ++pHost->rootIrpCount;

    return s;
    }


/***************************************************************************
*
* rootIrpHandler - interprets and executes IRPs sent to the root hub
*
* NOTE: Unlike IRPs which are routed to the actual USB, IRPs which are
* diverted to the rootHandler() execute synchronously.	Therefore, we
* force a callback completion at the end of this function.
*
* RETURNS: S_usbHcdLib_xxxx
*/

LOCAL int rootIrpHandler
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe,
    pUSB_IRP pIrp
    )

    {
    int s;

    /* fan-out based on the type of transfer to the root */

    switch (pPipe->transferType)
	{
	case USB_XFRTYPE_CONTROL:	
		s = rootControl (pHost, pIrp);
		break;

	case USB_XFRTYPE_INTERRUPT: 
		s = rootInterrupt (pHost, pPipe, pIrp);
		break;

	case USB_XFRTYPE_ISOCH:
	case USB_XFRTYPE_BULK:
	default:
		s = S_usbHcdLib_BAD_PARAM;
		break;
	}

    return s;
    }


/***************************************************************************
*
* busIrpHandler - schedule an IRP for execution on the bus
*
* RETURNS: S_usbHcdLib_xxxx
*/

LOCAL int busIrpHandler
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe,
    pUSB_IRP pIrp
    )

    {
    /* Allocate QHs and TDs for this IRP */

    if (!allocIrpWorkspace (pHost, pPipe, pIrp))
    return S_usbHcdLib_OUT_OF_MEMORY;


    /* Add this IRP to the scheduling list and invoke the IRP scheduler. */

    usbListLink (&pHost->busIrps, pIrp, &pIrp->hcdLink, LINK_TAIL);
    scheduleIrp (pHost, pIrp);

    return PENDING;
    }


/***************************************************************************
*
* fncIrpSubmit - Queue an IRP for execution
*
* If this function returns a result other than PENDING, then
* the IRP callback routine will already have been invoked.  If the
* result is PENDING, then the IRP's callback routine will be
* called asynchronously to signal IRP completion.
*
* RETURNS: S_usbHcdLib_xxxx
*/

LOCAL int fncIrpSubmit
    (
    pHRB_IRP_SUBMIT pHrb,
    pHCD_HOST pHost
    )

    {
    pHCD_PIPE pPipe;
    pUSB_IRP pIrp;
    int s;
    int index;

    /* validate parameters */

    if ((s = validateHrb (pHrb, sizeof (*pHrb))) != OK)
	return s;

    if (usbHandleValidate (pHrb->pipeHandle, 
			   HCD_PIPE_SIG, 
			   (pVOID *) &pPipe) != OK)
	return S_usbHcdLib_BAD_HANDLE;

    if ((pIrp = pHrb->pIrp) == NULL)
	return S_usbHcdLib_BAD_PARAM;

    /*
     * Since buffers that are handed to us are not garaunteed to be
     * cache safe, we need to flush them to memory before passing
     * them off to the host controller
     */

    for (index = 0; index < pIrp->bfrCount; index++)
        USER_FLUSH (pIrp->bfrList[index].pBfr, pIrp->bfrList[index].bfrLen);

    
    /* If this IRP is intended for the root hub, then divert it to the
     * root hub handler.
     */

    pIrp->result = PENDING; /* Mark IRP as pending */

    if (pPipe->busAddress == pHost->rootAddress)
	s = rootIrpHandler (pHost, pPipe, pIrp);
    else
	s = busIrpHandler (pHost, pPipe, pIrp);

    return setIrpResult (pIrp, s);
    }


/***************************************************************************
*
* fncIrpCancel - Attempts to cancel a pending IRP
*
* Attempts to cancel the specified IRP.
*
* RETURNS: OK if IRP canceled.
*      S_usbHcdLib_CANNOT_CANCEL if IRP already completed.
*/

LOCAL int fncIrpCancel
    (
    pHRB_IRP_CANCEL pHrb,
    pHCD_HOST pHost
    )

    {
    pUSB_IRP pIrp;
    int s;

    /* validate parameters */

    if ((s = validateHrb (pHrb, sizeof (*pHrb))) != OK)
	return s;

    if ((pIrp = pHrb->pIrp) == NULL)
	return S_usbHcdLib_BAD_PARAM;

    
    /* cancel the IRP */

    return cancelIrp (pHost, pIrp, S_usbHcdLib_IRP_CANCELED);
    }


/***************************************************************************
*
* fncPipeCreate - create pipe & calculate / reserve bandwidth
*
* RETURNS: S_usbHcdLib_xxxx
*/

LOCAL int fncPipeCreate
    (
    pHRB_PIPE_CREATE pHrb,
    pHCD_HOST pHost
    )

    {
    pHCD_PIPE pPipe;
    int s;

    /* validate parameters */

    if ((s = validateHrb (pHrb, sizeof (*pHrb))) != OK)
	return s;

    
    /* Calculate the time to transfer a packet of the indicated size. */

    pHrb->time = usbRecurringTime (pHrb->transferType, 
				   pHrb->direction,
				   pHrb->speed, 
				   pHrb->maxPacketSize, 
				   pHrb->bandwidth,
				   UHC_HOST_DELAY, 
				   UHC_HUB_LS_SETUP);


    /* Is the indicated amount of bandwidth available? */

    if (pHost->nanoseconds + pHrb->time > USB_LIMIT_ISOCH_INT)
	return S_usbHcdLib_BANDWIDTH_FAULT;


    /* Create a pipe structure to describe this pipe */

    if ((pPipe = OSS_CALLOC (sizeof (*pPipe))) == NULL)
	return S_usbHcdLib_OUT_OF_MEMORY;

    if (usbHandleCreate (HCD_PIPE_SIG, pPipe, &pPipe->pipeHandle) != OK)
	{
	OSS_FREE (pPipe);
	return S_usbHcdLib_OUT_OF_RESOURCES;
	}

    pPipe->busAddress = pHrb->busAddress;
    pPipe->endpoint = pHrb->endpoint;
    pPipe->transferType = pHrb->transferType;
    pPipe->direction = pHrb->direction;
    pPipe->speed = pHrb->speed;
    pPipe->maxPacketSize = pHrb->maxPacketSize;
    pPipe->bandwidth = pHrb->bandwidth;
    pPipe->interval = pHrb->interval;

    pPipe->time = pHrb->time;

    usbListLink (&pHost->pipes, pPipe, &pPipe->link, LINK_TAIL);


    /* Record bandwidth in use. */

    pHost->nanoseconds += pHrb->time;


    /* Return pipe handle to caller */

    pHrb->pipeHandle = pPipe->pipeHandle;

    return OK;
    }


/***************************************************************************
*
* fncPipeDestroy - destroy pipe and release associated bandwidth
*
* RETURNS: S_usbdHcdLib_xxxx
*/

LOCAL int fncPipeDestroy
    (
    pHRB_PIPE_DESTROY pHrb,
    pHCD_HOST pHost
    )

    {
    pHCD_PIPE pPipe;
    int s;

    /* validate parameters */

    if ((s = validateHrb (pHrb, sizeof (*pHrb))) != OK)
	return s;

    if (usbHandleValidate (pHrb->pipeHandle, 
			   HCD_PIPE_SIG, 
			   (pVOID *) &pPipe) 
			   != OK)
	return S_usbHcdLib_BAD_HANDLE;


    /* destroy pipe */

    destroyPipe (pHost, pPipe);

    return OK;
    }


/***************************************************************************
*
* fncPipeModify - modify pipe characteristics
*
* RETURNS: S_usbdHcdLib_xxxx
*/

LOCAL int fncPipeModify
    (
    pHRB_PIPE_MODIFY pHrb,
    pHCD_HOST pHost
    )

    {
    pHCD_PIPE pPipe;
    int s;

    /* validate parameters */

    if ((s = validateHrb (pHrb, sizeof (*pHrb))) != OK)
	return s;

    if (usbHandleValidate (pHrb->pipeHandle, 
			   HCD_PIPE_SIG, 
			   (pVOID *) &pPipe) 
			   != OK)
	return S_usbHcdLib_BAD_HANDLE;


    /* update pipe charactistics */

    if (pHrb->busAddress != 0)
	pPipe->busAddress = pHrb->busAddress;

    if (pHrb->maxPacketSize != 0)
	pPipe->maxPacketSize = pHrb->maxPacketSize;

    return OK;
    }


/***************************************************************************
*
* fncSofIntervalGet - retrieves current SOF interval
*
* RETURNS: S_usbdHcdLib_xxxx
*/

LOCAL int fncSofIntervalGet
    (
    pHRB_SOF_INTERVAL_GET_SET pHrb,
    pHCD_HOST pHost
    )

    {
    int s;

    /* validate parameters */

    if ((s = validateHrb (pHrb, sizeof (*pHrb))) != OK)
	return s;

    if (pHrb->busNo != 0)
	return S_usbHcdLib_BAD_PARAM;


    /* Retrieve the current SOF interval. */

    pHrb->sofInterval = UHC_BYTE_IN (UHCI_SOFMOD) + UHCI_SOF_BASE;

    return OK;
    }


/***************************************************************************
*
* fncSofIntervalSet - set new SOF interval
*
* RETURNS: S_usbdHcdLib_xxxx
*/

LOCAL int fncSofIntervalSet
    (
    pHRB_SOF_INTERVAL_GET_SET pHrb,
    pHCD_HOST pHost
    )

    {
    int s;

    /* validate parameters */

    if ((s = validateHrb (pHrb, sizeof (*pHrb))) != OK)
	return s;

    if (pHrb->busNo != 0)
	return S_usbHcdLib_BAD_PARAM;

    if (pHrb->sofInterval < UHCI_SOF_BASE || pHrb->sofInterval > UHCI_SOF_MAX)
	return S_usbHcdLib_SOF_INTERVAL_FAULT;


    /* Set the new SOF interval. */

    UHC_BYTE_OUT (UHCI_SOFMOD, 
		 pHrb->sofInterval - UHCI_SOF_BASE);
    pHost->sofInterval = pHrb->sofInterval;

    return OK;
    }


/***************************************************************************
*
* usbHcdUhciExec - HCD_EXEC_FUNC entry point for UHCI HCD
*
* RETURNS: OK or ERROR
*
* ERRNO:
*  S_usbHcdLib_BAD_PARAM
*  S_usbHcdLib_BAD_HANDLE
*  S_usbHcdLib_SHUTDOWN
*/

STATUS usbHcdUhciExec
    (
    pVOID pHrb		    /* HRB to be executed */
    )

    {
    pHRB_HEADER pHeader = (pHRB_HEADER) pHrb;
    pHCD_HOST pHost;
    int s;


    /* Validate parameters */

    if (pHeader == NULL || pHeader->hrbLength < sizeof (HRB_HEADER))
	return ossStatus (S_usbHcdLib_BAD_PARAM);

    if (pHeader->function != HCD_FNC_ATTACH)
	{
	if (usbHandleValidate (pHeader->handle, HCD_HOST_SIG, (pVOID *) &pHost) 
			      != OK)
	    return ossStatus (S_usbHcdLib_BAD_HANDLE);

	if (pHost->shutdown)
	    return ossStatus (S_usbHcdLib_SHUTDOWN);
	}


    /* Guard against other tasks */

    if (pHeader->function != HCD_FNC_ATTACH && 
	pHeader->function != HCD_FNC_DETACH)
	{
	OSS_MUTEX_TAKE (pHost->hostMutex, OSS_BLOCK);
	}


    /* Fan-out to appropriate function processor */

    switch (pHeader->function)
	{
	case HCD_FNC_ATTACH:
	    s = fncAttach ((pHRB_ATTACH) pHeader);
	    break;

	case HCD_FNC_DETACH:
	    s = fncDetach ((pHRB_DETACH) pHeader, pHost);
	    break;

	case HCD_FNC_SET_BUS_STATE:
	    s = fncSetBusState ((pHRB_SET_BUS_STATE) pHeader, pHost);
	    break;

	case HCD_FNC_SOF_INTERVAL_GET:
	    s = fncSofIntervalGet ((pHRB_SOF_INTERVAL_GET_SET) pHeader, pHost);
	    break;

	case HCD_FNC_SOF_INTERVAL_SET:
	    s = fncSofIntervalSet ((pHRB_SOF_INTERVAL_GET_SET) pHeader, pHost);
	    break;

	case HCD_FNC_CURRENT_FRAME_GET:
	    s = fncCurrentFrameGet ((pHRB_CURRENT_FRAME_GET) pHeader, pHost);
	    break;

	case HCD_FNC_IRP_SUBMIT:
	    s = fncIrpSubmit ((pHRB_IRP_SUBMIT) pHeader, pHost);
	    break;

	case HCD_FNC_IRP_CANCEL:
	    s = fncIrpCancel ((pHRB_IRP_CANCEL) pHeader, pHost);
	    break;

	case HCD_FNC_PIPE_CREATE:
	    s = fncPipeCreate ((pHRB_PIPE_CREATE) pHeader, pHost);
	    break;

	case HCD_FNC_PIPE_DESTROY:
	    s = fncPipeDestroy ((pHRB_PIPE_DESTROY) pHeader, pHost);
	    break;

	case HCD_FNC_PIPE_MODIFY:
	    s = fncPipeModify ((pHRB_PIPE_MODIFY) pHeader, pHost);
	    break;

	default:
	    s = S_usbHcdLib_BAD_PARAM;
	    break;
	}

    /* Release guard mutex */

    if (pHeader->function != HCD_FNC_ATTACH && 
	pHeader->function != HCD_FNC_DETACH)
	{
	OSS_MUTEX_RELEASE (pHost->hostMutex);
	}


    /* Return status */

    if (s == OK || s == PENDING)
	return OK;
    else
	return ossStatus (s);
    }


/* End of file. */
