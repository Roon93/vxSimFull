/* usbHcdOhciLib.c - Host Controller Driver (HCD) for OHCI */

/* Copyright 2000-2002 Wind River Systems, Inc. */

/*
Modification history
--------------------
01p,22may02,wef  Fix SPR 73864 - USB crashes when cable is disconnected while 
		 iscoh transfer is in progress - valid only on PPC, MIPS and 
		 STRONGARM, not X86.
010,29apr02,wef  Fix SPR's 71274, 71273 and clean up fncAttach().
01p,18mar02,h_k  made buffer swap's in several places - spr # 73896 and 73920.
01o,05feb01,wef  explicitly power to ports on initial HC attachment.
01k,13dec01,wef  merge from veloce view
01j,24jul01,wef  Fixed SPR #68617
01i,23jul01,wef  Fixed SPR #68202 and SPR #68209
01h,01jan01,wef  Fixed alignment problems w/ code, general clean up
01g,12apr00,wef  Fixed uninitialized variable warning: controlin assignTds(),
		 fixed a comment that was messing up the man page generation
01f,27mar00,rcb  Fix bug in scheduling interrupt EDs which allowed an ED
		 to be circularly-linked.
01e,20mar00,rcb  Flush all cache buffers during assignTds() to avoid 
		 cache-line boundary problems with some CPU architectures
		 (e.g., MIPS).
01d,17mar00,rcb  Add code to update tdHead in ED structure when removing
		 TDs from queue...corrects bug which allowed OHCI ctlr
		 to try to process an aborted TD.
01c,10mar00,rcb  Fix bug in pipe destroy logic which attempted to free
		 non-existent EDs for root hub.
		 Add "volatile" declaration in HC_DWORD_IN/OUT().
01b,26jan00,rcb  Change references to "bytesPerFrame" to "bandwidth" in
		 pipe creation logic.
		 Modify isoch. timing to maintain rate as specified by
		 pipe bandwidth parameter.
		 Fix big vs. little-endian bug in unscheduleControlIrp()
		 and unscheduleBulkIrp().
01a,05oct99,rcb  First.
*/

/*
DESCRIPTION

This is the HCD (host controller driver) for OHCI.  This file implements
low-level functions required by the client (typically the USBD) to talk to
the underlying USB host controller hardware.  

The <param> to the HRB_ATTACH request should be a pointer to a 
PCI_CFG_HEADER which contains the PCI configuration header for the
OpenHCI host controller to be managed.	Each invocation of the HRB_ATTACH
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

Regarding OHCI frame lists...

We use the OHCI frame list largely as anticipated in the OpenHCI specification.
Each of the 32 interrupt list heads points either to an interrupt ED (endpoint
descriptor) or to an isochronous ED "anchor" (at which point all 32 lists 
converge).  When one or more interrupt pipes are created, EDs for these pipes 
will be inserted in the interrupt list one or more times, corresponding to the
desired interrupt polling interval.  The last interrupt ED in each list points 
to the common isochronous anchor ED.  While clients can request any interrupt 
service interval they like, the algorithms here always choose an
interval which is the largest power of 2 less than or equal to the client's
desired interval.  For example, if a client requests an interval of 20msec, the
HCD will select a real interval of 16msec.  In each frame work list, the least
frequently scheduled EDs appear ahead of more frequently scheduled EDs.  Since
only a single ED is actually created for each interrupt transfer, the individual
frame lists actually "merge" at each interrupt ED.   

The OpenHCI host controller maintains separate lists for control and bulk
transfers.  OpenHCI allows for a control/bulk service ratio, so the control list
can be serviced n times for each servicing of the bulk list.  In the present
implementation, we use a service ratio of 3:1.

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
the HC theoretically allows us to schedule as many of them as we desire, and it
keeps track of how much time remains in each frame, executing only as many of
these transfers as will fit.  However, the HC requires that the driver schedule
only as many low speed control transfers (as opposed to full speed control
transfers) as can actually fit within the frame.  Therefore, after taking into
account the time already allotted to isochronous and interrupt transfers, the
HCD only schedules as many low speed control transfers as can fit within the
current frame - and full speed control and bulk transfers follow. 
*/


/* includes */

#include "usb/usbPlatform.h"

#include "string.h"

#include "memLib.h"	    /* memory sub-allocation functions */
#include "cacheLib.h"		/* cache functions */
#include "semLib.h"		/* cache functions */


#include "usb/ossLib.h"
#include "usb/usbHandleLib.h"
#include "usb/pciConstants.h"
#include "usb/usbPciLib.h"

#include "usb/usbLib.h"
#include "drv/usb/usbHcd.h"
#include "drv/usb/usbOhci.h"
#include "drv/usb/usbHcdOhciLib.h"


/* defines */

#define PENDING 	1

#define HCD_HOST_SIG	    ((UINT32) 0x00cd0080)
#define HCD_PIPE_SIG	    ((UINT32) 0x00cd0081)


#define MAX_INT_DEPTH	    8	    /* max depth of pending interrupts */
#define INT_TIMEOUT	5000	/* wait 5 seconds for int routine to exit */
#define BIT_TIMEOUT	1000	/* max time to wait for a bit change */

#define HC_TIMEOUT_SRVC_INTERVAL    1000    /* milliseconds */


/* HC_HOST_DELAY and HC_HUB_LS_SETUP are host-controller specific.
 * The following values are estimates for the OHCI controller.
 */

#define HC_HOST_DELAY	((UINT32) 500L)     /* 500 ns, est. */
#define HC_HUB_LS_SETUP ((UINT32) 500L)     /* 500 ns, est. */


/* OHCI constants unique to this driver implementation. */

#define REQUIRED_OHCI_LEVEL 0x10    /* OHCI rev 1.0 or higher */

#define MAX_FRAME_OVERHEAD  210 /* cf. OHCI spec. sec 5.4 */

#define MAX_ROOT_PORTS	    8	/* max root hub ports we support */
#define OHCI_HUB_CONTR_CURRENT	0   /* root hub controller current */
#define OHCI_HUB_INTERVAL   255 /* root hub polling interval */

#define TD_COUNT_GEN	    2	/* count of TDs for generic pipe */
#define TD_COUNT_ISO	    16	/* count of TDs for isoch pipe */

#ifndef USB_BUFFER_SWAP
#define USB_BUFFER_SWAP(pBuf,len)   /* default: don't swap buffers */
#endif  /* USB_BUFFER_SWAP */

#ifndef SYS_OHCI_RESET
#define SYS_OHCI_RESET()	/* default: don't need reset */
#endif  /* SYS_OHCI_RESET */


/*
 * MEMORY
 * 
 * To improve performance, a single block of (probably) "non-cached"
 * memory is allocated.  Then, all OHCI control structures are sub-allocated
 * from this block as needed.  The vxWorks CACHE_DMA_FLUSH/INVALIDATE macros
 * are used to ensure that this memory is flushed/invalidated at the correct
 * times (assuming that cacheable-memory *might* be allocated).
 */

#define DMA_MEMORY_SIZE 	0x10000 /* 64k */

/*  Ensure that alignment is actually a multiple of the cache line size */

#ifdef _CACHE_ALIGN_SIZE
#define DMA_MALLOC(bytes, alignment)    \
    memPartAlignedAlloc (pHost->memPartId, \
			 bytes, \
			 max(alignment, _CACHE_ALIGN_SIZE))
#else
#define DMA_MALLOC(bytes, alignment)    \
    memPartAlignedAlloc (pHost->memPartId, bytes, alignment)
#endif

#define DMA_FREE(pBfr)		memPartFree (pHost->memPartId, (char *) pBfr)

#define DMA_FLUSH(pBfr, bytes)	    CACHE_DMA_FLUSH (pBfr, bytes)
#define DMA_INVALIDATE(pBfr, bytes) CACHE_DMA_INVALIDATE (pBfr, bytes)

#define USER_FLUSH(pBfr, bytes)     CACHE_USER_FLUSH (pBfr, bytes)
#define USER_INVALIDATE(pBfr,bytes) CACHE_USER_INVALIDATE (pBfr, bytes)


/* HC I/O access macros.
 *
 * NOTE: These macros assume that the calling function defines pHost.
 * 
 * NOTE: These macros also require that the register offset, p, be evenly
 * divisible by 4, e.g., UINT32 access only.
 */


#define HC_DWORD_IN(p)     ohciLongRead(pHost, p)
#define HC_DWORD_OUT(p,d)   *((volatile UINT32 *) (pHost->memBase \
					 + (p) / 4))  = TO_LITTLEL (d)

#define HC_SET_BITS(p,bits) HC_DWORD_OUT (p, HC_DWORD_IN (p) | (bits))
#define HC_CLR_BITS(p,bits) HC_DWORD_OUT (p, HC_DWORD_IN (p) & ~(bits))


/* FINDEX() creates a valid OHCI frame number from the argument. */

#define FINDEX(f)   ((f) & (OHCI_FRAME_WINDOW - 1))


/*
 * The HCD adds HC_FRAME_SKIP to the current frame counter to determine
 * the first available frame for scheduling. This introduces a latency at
 * the beginning of each IRP, but also helps to ensure that the HC won't
 * run ahead of the HCD while the HCD is scheduling a transaction.
 */

#define HC_FRAME_SKIP	    2


/* defaults for IRPs which don't specify corresponding values */

#define DEFAULT_PACKET_SIZE 8	/* max packet size */
#define DEFAULT_INTERVAL    32	/* interrupt pipe srvc interval */


/* defines for emulated USB descriptors */

#define USB_RELEASE	0x0110	/* USB level supported by this code */

#define HC_MAX_PACKET_SIZE  8
    
#define HC_CONFIG_VALUE     1

#define HC_STATUS_ENDPOINT_ADRS (1 | USB_ENDPOINT_IN)


/* string identifiers */

#define UNICODE_ENGLISH     0x409

#define HC_STR_MFG	1
#define HC_STR_MFG_VAL	    "Wind River Systems"

#define HC_STR_PROD	2
#define HC_STR_PROD_VAL     "OHCI Root Hub"


/* PCI pointer macros */

#define TO_PCIPTR(p)	    (((p) == NULL) ? 0 : USB_MEM_TO_PCI (p))
#define FROM_PCIPTR(d)	    (((d) == 0) ? 0 : USB_PCI_TO_MEM (d))

#define ED_FROM_PCIPTR(d)   ((pED_WRAPPER) (FROM_PCIPTR (FROM_LITTLEL (d))))
#define TD_FROM_PCIPTR(d)   ((pTD_WRAPPER) (FROM_PCIPTR (FROM_LITTLEL (d))))


/* interrupt definitions */

#define INT_ENABLE_MASK \
    (OHCI_INT_WDH | OHCI_INT_RD | OHCI_INT_UE | OHCI_INT_RHSC | OHCI_INT_MIE)


/* typedefs */

/*
 * ED_WRAPPER
 *
 * ED_WRAPPER combines the OHCI_ED with software-specific data.
 */

typedef union ed_wrapper
    {
    VOLATILE OHCI_ED ed;	/* OHCI ED, 16 bytes */
    struct
	{
	UINT8 resvd [sizeof (OHCI_ED)];	/* space used by OHCI ED */
	struct hcd_pipe *pPipe;		/* pointer to owning pipe */
	} sw;
    } ED_WRAPPER, *pED_WRAPPER;


/*
 * TD_WRAPPER
 *
 * OHCI defines two TD formats: "general" for ctl/bulk/int and "isochronous".
 * The TD_WRAPPER combines these structures with software-specific data.
 */

typedef union td_wrapper
    {
    VOLATILE OHCI_TD_GEN tdg;		/* ctl/bulk/int OHCI TD, 16 bytes */
    VOLATILE OHCI_TD_ISO tdi;		/* isochronous OHCI TD, 32 bytes */
    struct
	{
	UINT8 resvd [sizeof (OHCI_TD_ISO)]; /* space used by OHCI TD */
	struct irp_workspace *pWork;	/* pointer to IRP workspace */
	UINT32 nanoseconds; 		/* exec time for this TD */
	UINT32 curBfrPtr;		/* original cbp for this TD */
	union td_wrapper *doneLink;     /* used when handling done queue */
	UINT32 inUse;			/* non-zero if TD in use */
	UINT32 pad [3];			/* pad to an even 64 bytes */
	} sw;
    } TD_WRAPPER, *pTD_WRAPPER;

#define TD_WRAPPER_LEN	    64			/* expected TD_WRAPPER length */
#define TD_WRAPPER_ACTLEN   sizeof (TD_WRAPPER)	/* actual */


/*
 * HCD_PIPE
 *
 * HCD_PIPE maintains all information about an active pipe.
 */

typedef struct hcd_pipe
    {
    HCD_PIPE_HANDLE pipeHandle;	/* handle assigned to pipe */

    LINK link;			/* linked list of pipes */

    UINT16 busAddress;		/* bus address of USB device */
    UINT16 endpoint;		/* device endpoint */
    UINT16 transferType;	/* transfer type */
    UINT16 direction;		/* transfer/pipe direction */
    UINT16 speed;		/* transfer speed */
    UINT16 maxPacketSize;	/* packet size */
    UINT32 bandwidth;		/* bandwidth required by pipe */
    UINT16 interval;		/* service interval */
    UINT16 actInterval; 	/* service interval we really use */

    UINT32 time;		/* bandwidth (time) allocated to pipe */

    pED_WRAPPER pEd;		/* ED allocated for this pipe */

    UINT16 tdCount;		/* number of TDs allocated for pipe */
    UINT16 freeTdCount; 	/* count of TDs currently available */
    UINT16 freeTdIndex; 	/* first available TD */
    pTD_WRAPPER pTds;		/* pointer to TD(s) */

    } HCD_PIPE, *pHCD_PIPE;


/*
 * IRP_WORKSPACE
 *
 * Associates EDs and TDs with the IRPs they are currently servicing.
 */

typedef struct irp_workspace
    {
    pHCD_PIPE pPipe;		/* pointer to pipe for this IRP */
    pUSB_IRP pIrp;		    /* pointer to parent IRP */

    UINT16 bfrNo;		/* highest IRP bfrList[] serviced */
    UINT32 bfrOffset;		/* offset into bfrList[].pBfr */
    BOOL zeroLenMapped; 	/* TRUE when zero len bfrList [] serviced */

    UINT32 isochTdsCreated;	/* count of isoch TDs created, in total */
    UINT32 frameCount;		/* count of frames used for isoch pipe */
    UINT32 bytesSoFar;		/* bytes transferred so far for isoch pipe */

    UINT16 isochNext;		/* next isoch frame number to schedule */

    BOOL irpRunning;		/* TRUE once IRP scheduled onto bus */
    UINT32 startTime;		/* time when IRP was scheduled onto bus */

    } IRP_WORKSPACE, *pIRP_WORKSPACE;


/*
 * HCD_HOST
 *
 * HCD_HOST maintains all information about a connection to a specific
 * host controller (HC).
 */

typedef struct hcd_host
    {
    HCD_CLIENT_HANDLE handle;	/* handle associated with host */
    BOOL shutdown;		/* TRUE during shutdown */

    PCI_CFG_HEADER pciCfgHdr;	/* PCI config header for HC */
    VOLATILE UINT32 * memBase;	    /* Base address */

    USB_HCD_MNGMT_CALLBACK mngmtCallback; /* callback routine for mngmt evt */
    pVOID mngmtCallbackParam;	/* caller-defined parameter */

    MUTEX_HANDLE hostMutex;	/* guards host structure */

    THREAD_HANDLE intThread;	/* Thread used to handle interrupts */
    SEM_HANDLE intPending;	/* semaphore indicates int pending */
    BOOL intThreadExitRequest;	/* TRUE when intThread should terminate */
    SEM_HANDLE intThreadExit;	/* signalled when int thread exits */
    UINT32 intCount;		/* number of interrupts processed */
    BOOL intInstalled;		/* TRUE when h/w int handler installed */

    UINT16 rootAddress; 	/* current address of root hub */
    UINT8 configValue;		/* current configuration value */

    UINT16 numPorts;		/* number of root ports */
    UINT16 pwrOn2PwrGood;	/* Power ON to power good time. */

    pUINT32 pRhPortChange;	/* port change status */

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

    char *dmaPool;		/* memory alloc'd by cacheDmaMalloc() */
    PART_ID memPartId;		/* memory partition ID */

    pOHCI_HCCA pHcca;		/* OHCI HCCA */

    pED_WRAPPER pIsochAnchorEd;	/* Anchor for isochronous transfers */

    UINT32 nanoseconds; 	/* current worst case of bus time */
				/* required for scheduled TDs */

    UINT16 sofInterval; 	/* current SOF interval */

    BOOL suspended;		/* TRUE when global suspend is TRUE */

    UINT32 errScheduleOverrun;	/* count of schedule overrun errors */
    UINT32 errUnrecoverable;	/* count of unrecoverabl HC errors */

    UINT32 pHcControlReg;	/* Control Register Copy */
    SEM_ID hcSyncSem;		/* syncronization semaphore */

    } HCD_HOST, *pHCD_HOST;


/* locals */

/* Language descriptor */

LOCAL USB_LANGUAGE_DESCR langDescr =
    {sizeof (USB_LANGUAGE_DESCR), USB_DESCR_STRING, 
    {TO_LITTLEW (UNICODE_ENGLISH)}};



/***************************************************************************
*
* ohciLongRead - Read a 32 bit value from the OHCI controller.
*
* RETURNS: A big-endian adjusted UINT32 
*/

LOCAL UINT32 ohciLongRead
    (
    pHCD_HOST pHost,
    UINT32 offset
    )
    {
    CACHE_PIPE_FLUSH ();

    return FROM_LITTLEL (*((volatile UINT32 *) (pHost->memBase + \
					(offset) / 4)));
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
    UINT32 p,
    UINT32 bitMask,
    UINT32 bitState
    )

    {
    UINT32 start = OSS_TIME ();
    BOOL desiredState;

    while (!(desiredState = ((HC_DWORD_IN (p) & bitMask) == bitState)) && \
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
    return OHCI_FMN_FN (HC_DWORD_IN (OHCI_HC_FM_NUMBER));
    }


/***************************************************************************
*
* setFrameInterval - sets OHCI frame interval 
*
* RETURNS: N/A
*/

LOCAL VOID setFrameInterval
    (
    pHCD_HOST pHost,
    UINT16 sofInterval
    )

    {
    UINT32 fmi;

    fmi = HC_DWORD_IN (OHCI_HC_FM_INTERVAL) & ~OHCI_FMI_FIT;
    fmi ^= OHCI_FMI_FIT;

    fmi |= OHCI_FMI_FI_FMT (sofInterval);
    fmi |= OHCI_FMI_FSMPS_FMT (((sofInterval - MAX_FRAME_OVERHEAD) * 6) / 7);

    HC_DWORD_OUT (OHCI_HC_FM_INTERVAL, fmi);

    pHost->sofInterval = sofInterval;
    }


/***************************************************************************
*
* hcSynch - give the host controller a chance to synchronize
*
* RETURNS: N/A
*/

LOCAL VOID hcSynch
    (
    pHCD_HOST pHost
    )

    {
    UINT16 currentFrameNo = getFrameNo (pHost);

    while (getFrameNo (pHost) == currentFrameNo);
	semTake (pHost->hcSyncSem, 1);

	semTake (pHost->hcSyncSem, 1);

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
* calcIntInterval - calculates the scheduling interval for interrupt transfer
*
* RETURNS: Actual interval to be used for interrupt transfer
*/

LOCAL UINT16 calcIntInterval
    (
    UINT16 interval /* 1 <= requested interval <= OHCI_INT_ED_COUNT */
    )

    {
    UINT16 i;

    /* Select an interval which is the largest power of two less than or
     * equal to the requested interval.
     */

    for (i = 2; i < OHCI_INT_ED_COUNT + 1; i <<= 1)
	{
	if (i > interval)
	    break;
	}

    return i >> 1;
    }


/***************************************************************************
*
* freeIrpWorkspace - releases IRP_WORKSPACE
*
* Releases the IRP_WORKSPACE associated with an IRP (if there is one).
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
	OSS_FREE (pWork);
	pIrp->hcdPtr = NULL;
	}
    }


/***************************************************************************
*
* allocIrpWorkspace - creates workspace for IRP
*
* Creates an IRP_WORKSPACE structure to manage the IRP data transfer.
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


    /* Allocate IRP_WORKSPACE */

    if ((pWork = OSS_CALLOC (sizeof (*pWork))) == NULL)
	return FALSE;

    pIrp->hcdPtr = pWork;
    pWork->pPipe = pPipe;
    pWork->pIrp = pIrp;

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
* unscheduleIsochPipe - removes an isoch pipe from the schedule
*
* RETURNS: N/A
*/

LOCAL VOID unscheduleIsochPipe
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe
    )

    {
    pUINT32 pPciPtrToEd;
    pED_WRAPPER pCurEd;


    /* Note: Instead of halting periodic transfers before delinking, we 
     * immediately delink them and afterward wait for the next frame to start...
     * This ensures that caller won't de-allocate the structures while the
     * HC may still be referencing them.
     */

    /* Walk the isoch. list until we find the ED to be removed. */

    pPciPtrToEd = (pUINT32) &pHost->pIsochAnchorEd->ed.nextEd;

    while ((pCurEd = ED_FROM_PCIPTR (*pPciPtrToEd)) != pPipe->pEd)
	{
	pPciPtrToEd = (pUINT32) &pCurEd->ed.nextEd;
	}

    *pPciPtrToEd = pPipe->pEd->ed.nextEd;
    DMA_FLUSH (pPciPtrToEd, sizeof (*pPciPtrToEd));


    /*  
     * shutOffListProcessing() halts isoch, interrupt and bulk transfers 
     * when a device is disconnected, we make sure they are re-enabled.
     */
 
    HC_SET_BITS(OHCI_HC_CONTROL,OHCI_CTL_PLE);
    HC_SET_BITS(OHCI_HC_CONTROL,OHCI_CTL_IE);
    HC_SET_BITS(OHCI_HC_CONTROL,OHCI_CTL_BLE);

    /* Wait for the HC to make sure the structure is really released. */

    hcSynch (pHost);
    }
    

/***************************************************************************
*
* scheduleIsochPipe - inserts isoch pipe into the schedule
*
* Schedules an isochronous transfer for the first time.
*
* RETURNS: N/A
*/

LOCAL VOID scheduleIsochPipe
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe
    )

    {
    pED_WRAPPER pLastEd;
    pED_WRAPPER pNextEd;


    /* Flush the ED out of the cache before giving it to the HC. */

    DMA_FLUSH (pPipe->pEd, sizeof (*pPipe->pEd));


    /* Append this ED to the end of the isoch. list. */

    pLastEd = pHost->pIsochAnchorEd;

    while ((pNextEd = ED_FROM_PCIPTR (pLastEd->ed.nextEd)) != NULL)
	pLastEd = pNextEd;


    pLastEd->ed.nextEd = TO_LITTLEL (TO_PCIPTR (pPipe->pEd));
    DMA_FLUSH (&pLastEd->ed.nextEd, sizeof (pLastEd->ed.nextEd));
    }


/***************************************************************************
*
* unscheduleInterruptPipe - removes interrupt pipe from the schedule
*
* RETURNS: N/A
*/

LOCAL VOID unscheduleInterruptPipe
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe
    )

    {
    pUINT32 pPciPtrToEd;
    pED_WRAPPER pCurEd;
    UINT16 i;


    /* Note: Instead of halting periodic transfers before delinking, we 
     * immediately delink them and afterward wait for the next frame to start...
     * This ensures that caller won't de-allocate the structures while the
     * HC may still be referencing them.
     */

    /* Remove the pipe from the interrupt lists. */

    for (i = 0; i < OHCI_INT_ED_COUNT; i += pPipe->actInterval)
	{
	/* Walk this list until we find the ED to be removed. */

	pPciPtrToEd = &pHost->pHcca->intEdTable [i];

	while ((pCurEd = ED_FROM_PCIPTR (*pPciPtrToEd)) != pHost->pIsochAnchorEd &&
		pCurEd->sw.pPipe->actInterval >= pPipe->actInterval &&
		pCurEd != pPipe->pEd)
	    {
	    pPciPtrToEd = (pUINT32) &pCurEd->ed.nextEd;
	    }

	if (pCurEd == pPipe->pEd)
	    {
	    *pPciPtrToEd = pPipe->pEd->ed.nextEd;
	    DMA_FLUSH (pPciPtrToEd, sizeof (*pPciPtrToEd));
	    }
	}

    /*  
     * shutOffListProcessing() halts isoch, interrupt and bulk transfers 
     * when a device is disconnected, we make sure they are re-enabled.
     */                 
 
    HC_SET_BITS(OHCI_HC_CONTROL,OHCI_CTL_PLE);
    HC_SET_BITS(OHCI_HC_CONTROL,OHCI_CTL_IE);
    HC_SET_BITS(OHCI_HC_CONTROL,OHCI_CTL_BLE);

    /* Wait for the HC to make sure the structure is really released. */

    hcSynch (pHost);
    }
    

/***************************************************************************
*
* scheduleInterruptPipe - inserts interrupt pipe into the schedule
*
* Schedules an interrupt transfer repeatedly in the frame list as
* indicated by the service interval.
*
* RETURNS: N/A
*/

LOCAL VOID scheduleInterruptPipe
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe
    )

    {
    pUINT32 pPciPtrToEd;
    pED_WRAPPER pCurEd;
    UINT16 i;


    /* Schedule the pipe onto the interrupt lists. */

    for (i = 0; i < OHCI_INT_ED_COUNT; i += pPipe->actInterval)
	{
	/* Walk this list until we hit the isoch. anchor or until we
	 * find an pipe with a smaller interval (higher frequency).
	 */

	pPciPtrToEd = &pHost->pHcca->intEdTable [i];

	while ((pCurEd = ED_FROM_PCIPTR (*pPciPtrToEd)) != pHost->pIsochAnchorEd &&
		pCurEd->sw.pPipe->actInterval >= pPipe->actInterval &&
		pCurEd != pPipe->pEd)
	    {
	    pPciPtrToEd = (pUINT32) &pCurEd->ed.nextEd;
	    }

	if (pCurEd != pPipe->pEd)
	    {
	    if (i == 0)
		{
		pPipe->pEd->ed.nextEd = *pPciPtrToEd;
		DMA_FLUSH (pPipe->pEd, sizeof (*pPipe->pEd));
		}

	    *pPciPtrToEd = TO_LITTLEL (TO_PCIPTR (pPipe->pEd));
	    DMA_FLUSH (pPciPtrToEd, sizeof (*pPciPtrToEd));
	    }
	}
    }


/***************************************************************************
*
* unscheduleControlPipe - removes control pipe from the schedule
*
* RETURNS: N/A
*/

LOCAL VOID unscheduleControlPipe
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe
    )

    {
    pED_WRAPPER pCurEd;
    pED_WRAPPER pNextEd;


    /* We need to halt processing of control transfers before de-linking. */

    HC_CLR_BITS (OHCI_HC_CONTROL, OHCI_CTL_CLE);
    hcSynch (pHost);


    /* Find the pipe's ED and de-link it. */

    if ((pCurEd = FROM_PCIPTR (HC_DWORD_IN (OHCI_HC_CONTROL_HEAD_ED)))
			      == pPipe->pEd)
	{
	/* ED is the head of the list */

	HC_DWORD_OUT (OHCI_HC_CONTROL_HEAD_ED, 
		      FROM_LITTLEL (pPipe->pEd->ed.nextEd));
	}
    else
	{
	/* Walk the list looking for the ED.
	 *
	 * NOTE: We know the ED must be on the list, so there's no need
	 * to check for the end of list as we proceed. 
	 */

	while ((pNextEd = ED_FROM_PCIPTR (pCurEd->ed.nextEd)) != pPipe->pEd)
					  pCurEd = pNextEd;

	pCurEd->ed.nextEd = pPipe->pEd->ed.nextEd;
	}

    /*  
     * shutOffListProcessing() halts isoch, interrupt and bulk transfers 
     * when a device is disconnected, we make sure they are re-enabled.
     */                 
 
    HC_SET_BITS(OHCI_HC_CONTROL,OHCI_CTL_PLE);
    HC_SET_BITS(OHCI_HC_CONTROL,OHCI_CTL_IE);
    HC_SET_BITS(OHCI_HC_CONTROL,OHCI_CTL_BLE);

    /* Re-enable the control list. */

    HC_SET_BITS (OHCI_HC_CONTROL, OHCI_CTL_CLE);

    }
    

/***************************************************************************
*
* scheduleControlPipe - inserts control pipe into the schedule
*
* Inserts the control transfer into the portion of the frame list 
* appropriate for control transfers.
*
* RETURNS: N/A
*/

LOCAL VOID scheduleControlPipe
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe
    )

    {
    pED_WRAPPER pLastEd = FROM_PCIPTR (HC_DWORD_IN (OHCI_HC_CONTROL_HEAD_ED));
    pED_WRAPPER pNextEd;


    /* Flush the ED out of the cache before giving it to the HC. */

    DMA_FLUSH (pPipe->pEd, sizeof (*pPipe->pEd));


    if (pLastEd == NULL)
	{
	/* This will be the first ED on the control list. */

	HC_DWORD_OUT (OHCI_HC_CONTROL_HEAD_ED, TO_PCIPTR (pPipe->pEd));
	}
    else
	{
	/* Append this ED to the end of the control list. */

	while ((pNextEd = ED_FROM_PCIPTR (pLastEd->ed.nextEd)) != NULL)
	    pLastEd = pNextEd;

	pLastEd->ed.nextEd = TO_LITTLEL (TO_PCIPTR (pPipe->pEd));
	DMA_FLUSH (&pLastEd->ed.nextEd, sizeof (pLastEd->ed.nextEd));
	}
    }


/***************************************************************************
*
* unscheduleBulkPipe - removes bulk transfer from the schedule
*
* RETURNS: N/A
*/

LOCAL VOID unscheduleBulkPipe
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe
    )

    {
    pED_WRAPPER pCurEd;
    pED_WRAPPER pNextEd;


    /* We need to halt processing of bulk transfers before de-linking. */

    HC_CLR_BITS (OHCI_HC_CONTROL, OHCI_CTL_BLE);
    hcSynch (pHost);


    /* Find the pipe's ED and de-link it. */

    if ((pCurEd = FROM_PCIPTR (HC_DWORD_IN (OHCI_HC_BULK_HEAD_ED)))
				== pPipe->pEd)
	{
	/* ED is the head of the list. */

	HC_DWORD_OUT (OHCI_HC_BULK_HEAD_ED, 
		      FROM_LITTLEL (pPipe->pEd->ed.nextEd));
	}
    else
	{
	/* Walk the list looking for the ED.
	 *
	 * NOTE: We know the ED must be on the list, so there's no need
	 * to check for the end of list as we proceed. 
	 */

	while ((pNextEd = ED_FROM_PCIPTR (pCurEd->ed.nextEd)) != pPipe->pEd)
					  pCurEd = pNextEd;

		pCurEd->ed.nextEd = pPipe->pEd->ed.nextEd;
	}

    /*  
     * shutOffListProcessing() halts isoch, interrupt and bulk transfers 
     * when a device is disconnected, we make sure they are re-enabled.
     */                 
 
    HC_SET_BITS(OHCI_HC_CONTROL,OHCI_CTL_PLE);
    HC_SET_BITS(OHCI_HC_CONTROL,OHCI_CTL_IE);
    HC_SET_BITS(OHCI_HC_CONTROL,OHCI_CTL_BLE);

    }
    

/***************************************************************************
*
* scheduleBulkPipe - inserts bulk pipe into the schedule
*
* Inserts the bulk transfer into the portion of the frame list appropriate 
* for bulk transfers.
*
* RETURNS: N/A
*/

LOCAL VOID scheduleBulkPipe
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe
    )

    {
    pED_WRAPPER pLastEd = FROM_PCIPTR (HC_DWORD_IN (OHCI_HC_BULK_HEAD_ED));
    pED_WRAPPER pNextEd;


    /* Flush the ED out of the cache before giving it to the HC. */

    DMA_FLUSH (pPipe->pEd, sizeof (*pPipe->pEd));


    if (pLastEd == NULL)
	{
	/* This will be the first ED on the bulk list. */

	HC_DWORD_OUT (OHCI_HC_BULK_HEAD_ED, TO_PCIPTR (pPipe->pEd));
	}
    else
	{
	/* Append this ED to the end of the bulk list. */

	while ((pNextEd = ED_FROM_PCIPTR (pLastEd->ed.nextEd)) != NULL) 
	    pLastEd = pNextEd;

	pLastEd->ed.nextEd = TO_LITTLEL (TO_PCIPTR (pPipe->pEd));
	DMA_FLUSH (&pLastEd->ed.nextEd, sizeof (pLastEd->ed.nextEd));
	}
    }


/***************************************************************************
*
* unschedulePipe - Removes pipe from schedule
*
* RETURNS: N/A
*/

LOCAL VOID unschedulePipe
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe
    )

    {
    /* un-Scheduling proceeds differently for each transaction type. */

    switch (pPipe->transferType)
	{
	case USB_XFRTYPE_ISOCH:
	    unscheduleIsochPipe (pHost, pPipe);
	    break;

	case USB_XFRTYPE_INTERRUPT: 
	    unscheduleInterruptPipe (pHost, pPipe);
	    break;

	case USB_XFRTYPE_CONTROL:
	    unscheduleControlPipe (pHost, pPipe);
	    break;

	case USB_XFRTYPE_BULK:
	    unscheduleBulkPipe (pHost, pPipe);
	    break;
	}
    }


/***************************************************************************
*
* schedulePipe - Adds a pipe to the schedule
*
* RETURNS: N/A
*/

LOCAL VOID schedulePipe
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe
    )

    {
    /* Scheduling proceeds differently for each transaction type. */

    switch (pPipe->transferType)
	{
	case USB_XFRTYPE_ISOCH:
	    scheduleIsochPipe (pHost, pPipe);
	    break;

	case USB_XFRTYPE_INTERRUPT: 
	    scheduleInterruptPipe (pHost, pPipe);
	    break;

	case USB_XFRTYPE_CONTROL:
	    scheduleControlPipe (pHost, pPipe);
	    break;

	case USB_XFRTYPE_BULK:
	    scheduleBulkPipe (pHost, pPipe);
	    break;
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
* NOTE: We choose to map only one bfrList[] entry at a time to simplify the
* the handling of input underrun.  When an underrun occurs, the 
* releaseTd() function releases all TDs scheduled up through that point.
* We then schedule TDs for the following bfrList[] entries if any.
*
* RETURNS: N/A
*/

LOCAL VOID assignTds
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe,
    pUSB_IRP pIrp,
    pIRP_WORKSPACE pWork
    )

    {
    pUSB_BFR_LIST pBfrList;
    pTD_WRAPPER pTd;
    UINT32 bytesThroughFrameCount;
    UINT16 bytesThisFrame;
    UINT16 isochLen;
    UINT32 isochOffset;
    UINT16 maxLen;
    UINT32 nanoseconds;
    UINT32 control = 0;
    UINT32 curBfrPtr;
    UINT32 bfrEnd;
    UINT16 frameCount;
    UINT16 psw;
    UINT32 pciPtrToNextFree;
    UINT16 i;


    /* Assign TDs to map the current bfrList[] entry.  Stop when the buffer is 
     * fully mapped, when we run out of TDs, or when bus bandwidth calculations 
     * indicate we should not allocate more TDs at this time.
     */

    if (pWork->bfrNo == pIrp->bfrCount)
	return;

    pBfrList = &pIrp->bfrList [pWork->bfrNo];

    while ((pWork->bfrOffset < pBfrList->bfrLen ||
	    (pBfrList->bfrLen == 0 && 
	    !pWork->zeroLenMapped)) && 
	    pPipe->freeTdCount > 0)
    	{

        /* Calculate the length of this TD and determine if there are any
         * bandwidth limitations which would prevent scheduling it at this time.
         *
         * NOTE: Since the USBD has already checked bandwidth availability for
         * isochronous and interrupt transfers, we need only check bandwidth
         * availability for low speed control transfers.
         */

        if (isBandwidthTracked (pPipe))
	    {
    	    nanoseconds = usbTransferTime (pPipe->transferType, 
					   dirFromPid (pBfrList->pid), 
					   pPipe->speed, 
				   min (pBfrList->bfrLen - pWork->bfrOffset, 
 					pPipe->maxPacketSize), 
					   HC_HOST_DELAY, 
					   HC_HUB_LS_SETUP);

  	    if (pHost->nanoseconds + nanoseconds > USB_LIMIT_ALL)
		{
	    	/* There isn't enough bandwidth at this time.  Stop scheduling 
	     	 * for this transfer.
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
	    /* 
	     * By default USB_BUFFER_SWAP is a no-op, but it can be
	     * overridden in BSP stubs as necessary.
	     */

	    USB_BUFFER_SWAP ((pVOID *) pBfrList->pBfr, pBfrList->bfrLen);
	    USER_FLUSH (pBfrList->pBfr, pBfrList->bfrLen);
	    }


        /* Grab the first free TD for our use...Then advance the "free TD 
         * index" so that it points to the next free TD.  (Since freeTdCount
         * is initialized to be one less than tdCount, we are guaranteed that
         * there are always at least two free TDs when we reach this point in
         * the code.
         */

        pTd = &pPipe->pTds [pPipe->freeTdIndex];
        pTd->sw.inUse = 1;
        pPipe->freeTdCount--;

        do
    	    {
	    if (++pPipe->freeTdIndex == pPipe->tdCount)
		pPipe->freeTdIndex = 0;
	    }
        while (pPipe->pTds [pPipe->freeTdIndex].sw.inUse != 0);


        /* OHCI TD's can span up to two, 4k pages.	Based on this limit,
         * calculate the maximum amount of data we'll try to transfer with
         * this TD.
         */

        curBfrPtr = TO_PCIPTR ((pVOID) &pBfrList->pBfr [pWork->bfrOffset]);

        maxLen = OHCI_PAGE_SIZE - (curBfrPtr & ~OHCI_PAGE_MASK) + \
					 OHCI_PAGE_SIZE;

        maxLen = min (maxLen, pBfrList->bfrLen - pWork->bfrOffset);


        /* Initialize TD. 
         *
         * NOTE: By convention, TD is always cleared to 0's at initialization
         * or after being discarded by the previous transfer.
         */

        switch (pBfrList->pid)
    	    {
    	    case USB_PID_SETUP: 
		control = OHCI_TGCTL_PID_SETUP; 
		break;

	    case USB_PID_OUT:   
		control = OHCI_TGCTL_PID_OUT;   
		break;

	    case USB_PID_IN:    
		control = OHCI_TGCTL_PID_IN;    
		break;
	    }

        if (pPipe->transferType == USB_XFRTYPE_ISOCH)
	    {
	    /* Isochronous TD */

	    /* Map up to a complete TD (eight frames). */

	    frameCount = 0;
	    isochOffset = 0;

	    for (i = 0; i < OHCI_ISO_PSW_CNT && maxLen > 0; i++)
		{
		/* Calculate the packet length for the next frame. */

		bytesThroughFrameCount = (pWork->frameCount + 1) * 
					  pPipe->bandwidth / 1000L;

		bytesThisFrame = bytesThroughFrameCount - pWork->bytesSoFar;

		/* NOTE: If maxLen is less than the desired number of bytes
		 * for this frame *AND* we are not yet at the end of this
		 * BFR_LIST entry, then don't schedule any more frames for
		 * this TD...For this situation to arise we must be near a
		 * page boundary crossing...The next TD will begin one page
		 * later and will allow us to schedule the full frame as
		 * desired.
		 */

		if (maxLen < bytesThisFrame &&
		    pWork->bfrOffset + maxLen != pBfrList->bfrLen)
		    break;

		isochLen = min (bytesThisFrame, maxLen);

		isochLen = min (isochLen, pPipe->maxPacketSize);

		if (pIrp->dataBlockSize != 0 && isochLen > pIrp->dataBlockSize)
		    {
		    isochLen = (isochLen / pIrp->dataBlockSize) * 
		    pIrp->dataBlockSize;
		    }

		pWork->bytesSoFar += isochLen;

		maxLen -= isochLen;

		pWork->frameCount++;
		frameCount++;


		/* Initialize the TD PSW for this frame. */

		psw = OHCI_TIPSW_CC_FMT (OHCI_CC_NOT_ACCESSED);
		psw |= (curBfrPtr + isochOffset) & OHCI_ISO_OFFSET_MASK;
		    
		if ((curBfrPtr & OHCI_PAGE_MASK) != 
		    ((curBfrPtr + isochOffset) & OHCI_PAGE_MASK))
		    {
		    psw |= OHCI_ISO_OFFSET_BE;
		    }

		pTd->tdi.psw [i] = TO_LITTLEW (psw);

		isochOffset += isochLen;
		}

	    /*
	     * Packet status words need to be swapped before sending on
	     * some particular controllers.
	     * By default USB_BUFFER_SWAP is a no-op, but it can be
	     * overridden in BSP stubs as necessary.
	     */

	    USB_BUFFER_SWAP ((pVOID *) &pTd->tdi.psw [0],
			     frameCount * sizeof (UINT16));

	    bfrEnd = curBfrPtr + isochOffset - 1;

	    pWork->bfrOffset += isochOffset;


	    /* Initialize remaining fields in isochronous TD. */

	    control |= OHCI_TICTL_FC_FMT (frameCount);
	    control |= OHCI_TICTL_SF_FMT (pWork->isochNext);
	    pWork->isochNext = FINDEX (pWork->isochNext + frameCount);

	    pTd->tdi.control = TO_LITTLEL (control);
	    pTd->tdi.bp0 = TO_LITTLEL (curBfrPtr & OHCI_PAGE_MASK);
	    pTd->tdi.be = TO_LITTLEL (bfrEnd);
	    }
	else
	    {
	    /* General TD */

	    control |= OHCI_TGCTL_BFR_RND | OHCI_TGCTL_TOGGLE_USETD;

	    /* If this transfer does not complete the BFR_LIST entry, then
	     * truncate it to an even number of frames. 
	     */

	    if (maxLen > pPipe->maxPacketSize &&
		pWork->bfrOffset + maxLen != pBfrList->bfrLen)
		{
		maxLen = (maxLen / pPipe->maxPacketSize) * pPipe->maxPacketSize;
		}

	    if (maxLen == 0)
		{
		pWork->zeroLenMapped = TRUE;
		curBfrPtr = bfrEnd = 0;
		}
	    else
		{
		bfrEnd = curBfrPtr + maxLen - 1;
		}

	    pWork->bfrOffset += maxLen;


	    /* The last BFR_LIST entry for a control transfer (the status
	     * packet) is alwasy DATA1. 
	     */

	    if (pPipe->transferType == USB_XFRTYPE_CONTROL &&
		pWork->bfrNo == pIrp->bfrCount - 1)
		{
		control |= OHCI_TGCTL_TOGGLE_DATA1;
		}
	    else
		{
		control |= (pIrp->dataToggle == USB_DATA0) ? 
						OHCI_TGCTL_TOGGLE_DATA0 :
						OHCI_TGCTL_TOGGLE_DATA1;
		}

	    pTd->tdg.control = TO_LITTLEL (control);
	    pTd->tdg.cbp = TO_LITTLEL (curBfrPtr);
	    pTd->tdg.be = TO_LITTLEL (bfrEnd);
	    }

	pTd->sw.curBfrPtr = curBfrPtr;


	/* Link the TD back to the workspace for the IRP. */

	pTd->sw.pWork = pWork;


	/* Store the time required to execute this TD */

	pHost->nanoseconds += nanoseconds;
	pTd->sw.nanoseconds = nanoseconds;


	/* Append TD to the end of the ED's TD queue. 
	 *
	 * NOTE: pTd->tdg.nextTd == pTd->tdi.nextTd. */

	pciPtrToNextFree = TO_LITTLEL (TO_PCIPTR (&pPipe->pTds [pPipe->freeTdIndex]));

	pTd->tdg.nextTd = pciPtrToNextFree;

	DMA_FLUSH (pTd, sizeof (OHCI_TD_ISO)+sizeof(OHCI_TD_GEN));

	pPipe->pEd->ed.tdTail = pciPtrToNextFree;
	DMA_FLUSH (&pPipe->pEd->ed.tdTail, sizeof (pPipe->pEd->ed.tdTail));


	/* If necessary, notify the HC that new a new transfer is ready. */

	switch (pPipe->transferType)
	    {
	    case USB_XFRTYPE_CONTROL:

		HC_SET_BITS (OHCI_HC_COMMAND_STATUS, OHCI_CS_CLF);
		break;

	    case USB_XFRTYPE_BULK:

		HC_SET_BITS (OHCI_HC_COMMAND_STATUS, OHCI_CS_BLF);
		break;
	    }
	}
    }


/***************************************************************************
*
* scheduleOtherIrps - schedules other IRPs if TDs are available on a pipe
*
* RETURNS: N/A
*/

LOCAL VOID scheduleOtherIrps
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe
    )

    {
    pUSB_IRP pIrp;
    pUSB_IRP pNextIrp;
    pIRP_WORKSPACE pWork;

    pIrp = usbListFirst (&pHost->busIrps);

    while (pIrp != NULL && pPipe->freeTdCount > 0)
	{
	pNextIrp = usbListNext (&pIrp->hcdLink);

	pWork = (pIRP_WORKSPACE) pIrp->hcdPtr;

	if (pWork->pPipe == pPipe)
	    assignTds (pHost, pPipe, pIrp, pWork);

	pIrp = pNextIrp;
	}
    }


/***************************************************************************
*
* releaseTd - Update an IRP based on a completed IRP.
*
* Sets <result> field in <pIrp> to OK or S_usbHcdLib_xxx if IRP completed.
*
* NOTE: Automatically reschedules TDs.
*
* RETURNS: N/A
*/

LOCAL VOID releaseTd
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe,
    pUSB_IRP pIrp,
    pIRP_WORKSPACE pWork,
    pTD_WRAPPER pTd
    )

    {
    pUSB_BFR_LIST pBfrList;
    UINT32 control;
    UINT32 curBfrPtr;
    UINT32 bfrEnd;
    UINT32 tdHead;
    UINT16 frameCount;
    UINT16 psw;
    UINT16 actLen;
    BOOL underrun = FALSE;
    UINT16 i;
    

    /*
     * In some case, transmitted data is still used at either drivers or
     * applications and need to be re-swapped if they are swapped before
     * sending.
     */

    if (pIrp->bfrList[pWork->bfrNo].pid == USB_PID_SETUP ||
	pIrp->bfrList[pWork->bfrNo].pid == USB_PID_OUT)
	{
	if (pTd->sw.curBfrPtr != 0)
	    {
	    if (FROM_LITTLEL (pTd->tdg.cbp) == 0)
		{
		/*
		 * By default USB_BUFFER_SWAP is a no-op, but it can be
		 * overridden in BSP stubs as necessary.
		 */

		USB_BUFFER_SWAP ((pVOID *) pTd->sw.curBfrPtr,
			FROM_LITTLEL (pTd->tdg.be) - pTd->sw.curBfrPtr + 1);
		}
	    else
		{
		/*
		 * By default USB_BUFFER_SWAP is a no-op, but it can be
		 * overridden in BSP stubs as necessary.
		 */

		USB_BUFFER_SWAP ((pVOID *) pTd->sw.curBfrPtr,
			FROM_LITTLEL (pTd->tdg.cbp) - pTd->sw.curBfrPtr + 1);
		}
	    }
	}

    /* Release the bandwidth this TD was consuming. */

    pHost->nanoseconds -= pTd->sw.nanoseconds;


    /* Examine the TD. */

    control = FROM_LITTLEL (pTd->tdg.control);

    switch (OHCI_TGCTL_CC (control))
	{
	case OHCI_CC_NO_ERROR:	
		break;
	case OHCI_CC_CRC:
	case OHCI_CC_NO_RESPONSE:	
		pIrp->result = S_usbHcdLib_CRC_TIMEOUT; 
		break;
	case OHCI_CC_BITSTUFF:	
		pIrp->result = S_usbHcdLib_BITSTUFF_FAULT;  
		break;
	case OHCI_CC_STALL:     
		pIrp->result = S_usbHcdLib_STALLED;     
		break;
	case OHCI_CC_DATA_TOGGLE:	
		pIrp->result = S_usbHcdLib_DATA_TOGGLE_FAULT; 
		break;
	case OHCI_CC_PID_CHECK:
	case OHCI_CC_UNEXPECTED_PID: 
		pIrp->result = S_usbHcdLib_PID_FAULT;	
		break;
	case OHCI_CC_DATA_OVERRUN:
	case OHCI_CC_DATA_UNDERRUN:
	case OHCI_CC_BFR_OVERRUN:
	case OHCI_CC_BFR_UNDERRUN:	
		pIrp->result = S_usbHcdLib_DATA_BFR_FAULT;  
		break;
	default:		
		pIrp->result = S_usbHcdLib_GENERAL_FAULT;   
		break;
	}


    /* If there was an error, then the HC will have halted the ED.  Un-halt
     * it so future transfers can take place.
     */

    tdHead = FROM_LITTLEL (pPipe->pEd->ed.tdHead);

    if (pIrp->result != PENDING)
	{
	pPipe->pEd->ed.tdHead = TO_LITTLEL (tdHead & ~OHCI_PTR_HALTED);
	DMA_FLUSH (&pPipe->pEd->ed.tdHead, sizeof (pPipe->pEd->ed.tdHead));
	}


    /* If ok so far, calculate the actual amount of data transferred. */

    if (pIrp->result == PENDING)
	{
	pBfrList = &pIrp->bfrList [pWork->bfrNo];

	/* Note: In following assignment: pTd->tdg.be == pTd->tdi.be */

	bfrEnd = FROM_LITTLEL (pTd->tdg.be);

	if (pPipe->transferType == USB_XFRTYPE_ISOCH)
	    {
	    /* For an isoch transfer make sure each frame was executed. */

	    frameCount = OHCI_TICTL_FC (control);

	    /*
	     * Re-swap packet status words before reading if they are swapped
	     * at transmit.
	     * By default USB_BUFFER_SWAP is a no-op, but it can be
	     * overridden in BSP stubs as necessary.
	     */

	    USB_BUFFER_SWAP ((pVOID *) &pTd->tdi.psw [0],
			     frameCount * sizeof (UINT16));

	    for (actLen = i = 0; i < frameCount; i++)
		{
		psw = FROM_LITTLEW (pTd->tdi.psw [i]);

		if (OHCI_TIPSW_CC (psw) != OHCI_CC_NO_ERROR)
		    {
		    pIrp->result = S_usbHcdLib_ISOCH_FAULT;
		    break;
		    }
		}

	    actLen = bfrEnd - pTd->sw.curBfrPtr + 1;
	    }
	else
	    {
	    /* For a general transfer, calculate the actual data transferred */

	    curBfrPtr = FROM_LITTLEL (pTd->tdg.cbp);

	    if (pTd->sw.curBfrPtr == 0)
		{
		/* we requested a 0-length transfer */

		actLen = 0;
		}
	    else
		{
		/* we requested a transfer of non-zero length */

		if (curBfrPtr == 0)
		    {
		    /* transfer was successful...full length requested. */

		    actLen = bfrEnd - pTd->sw.curBfrPtr + 1;
		    }
		else
		    {
		    /* short packet was transferred.  curBfrPtr addresses
		     * the byte following the last byte transferred.
		     */

		    actLen = curBfrPtr - pTd->sw.curBfrPtr;
		    }
		}

	    /* Update the IRP's data toggle. */

	    pIrp->dataToggle = ((tdHead & OHCI_PTR_TGL_CARRY) == 0) ? 
	    USB_DATA0 : USB_DATA1;
	    } 

	
	/* Update the BFR_LIST entry and check for underrun. */

	pBfrList->actLen += actLen;

	if (pTd->sw.curBfrPtr != 0 &&
	    actLen < bfrEnd - pTd->sw.curBfrPtr + 1)
	    {
	    underrun = TRUE;

	    if ((pIrp->flags & USB_FLAG_SHORT_FAIL) != 0)
		{
		pIrp->result = S_usbHcdLib_SHORT_PACKET;
		}
	    }

	if (pBfrList->actLen == pBfrList->bfrLen || underrun)
	    {
	    pWork->bfrNo++;
	    pWork->bfrOffset = 0;
	    pWork->zeroLenMapped = FALSE;
	    }
	}


    /* Indicate that the TD is no longer in use. */

    memset (pTd, 0, sizeof (*pTd));
    pPipe->freeTdCount++;
    

    /* Check if the IRP is complete. */

    if (pIrp->result == PENDING)
	{
	if (pWork->bfrNo == pIrp->bfrCount)
	    {
	    /* Yes, all buffers for the IRP have been processed. */

	    pIrp->result = OK;
	    }
	else
	    {
	    /* No, schedule more work for the IRP. */

	    assignTds (pHost, pPipe, pIrp, pWork);
	    }
	}


    /* If this is an isochronous pipe and if we've fully mapped the current
     * IRP, then check if there are other IRPs which need to be scheduled.
     */

    if (pPipe->transferType == USB_XFRTYPE_ISOCH &&
			       ((pWork->bfrNo + 1 == pIrp->bfrCount && 
			       pWork->bfrOffset == 
			       pIrp->bfrList [pWork->bfrNo].bfrLen) ||
			       pWork->bfrNo == pIrp->bfrCount))
	{
	scheduleOtherIrps (pHost, pPipe);
	}


    /* If the IRP is complete, invalidate the cache for any input buffers. */

    if (pIrp->result != PENDING)
	{
	for (i = 0; i < pIrp->bfrCount; i++)
	    {
	    pBfrList = &pIrp->bfrList [i];
	
	    if (pBfrList->pid == USB_PID_IN && pBfrList->actLen > 0)
		{
		USER_INVALIDATE (pBfrList->pBfr, pBfrList->actLen);

		/* 
	 	 * By default USB_BUFFER_SWAP is a no-op, but can be
		 * overridden in BSP stubs as necessary.
	 	 */

		USB_BUFFER_SWAP ((pVOID *) pBfrList->pBfr, pBfrList->actLen);
		}
	    }
	}
    }


/***************************************************************************
*
* unscheduleIrp - unschedules IRP
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
    pHCD_PIPE pPipe = pWork->pPipe;
    UINT16 i;
    UINT16 tdIndex;
    BOOL tdDeleted;


    /* If the IRP has no workspace, then it cannot be using TDs, so return 
     * immediately.
     */

    if (pWork == NULL)
	return;


    /* Halt processing on this ED. */

    pPipe->pEd->ed.control |= TO_LITTLEL (OHCI_EDCTL_SKIP);
    DMA_FLUSH (&pPipe->pEd->ed.control, sizeof (pPipe->pEd->ed.control));
    hcSynch (pHost);

    /* Search the pipe which owns this IRP to see if any of its TDs are
     * in use by this IRP.  If so, release them and allow another IRP (if
     * one exists) to use them.
     */

    tdIndex = pPipe->freeTdIndex;

    for (i = 0; i < pPipe->tdCount; i++)
	{
	tdDeleted = FALSE;

	if (pPipe->pTds [tdIndex].sw.pWork == pWork)
	    {
	    pPipe->freeTdCount++;
	    memset (&pPipe->pTds [tdIndex], 0, sizeof (pPipe->pTds [tdIndex]));
	    tdDeleted = TRUE;
	    }

	if (++tdIndex == pPipe->tdCount)
	    tdIndex = 0;

	/* If we deleted this TD, then advance head of TD list. */

	if (tdDeleted)
	    {
	    pPipe->pEd->ed.tdHead = 
	    TO_LITTLEL (TO_PCIPTR (&pPipe->pTds [tdIndex]));
	    DMA_FLUSH (&pPipe->pEd->ed.tdHead, sizeof (pPipe->pEd->ed.tdHead));
	    }
	}

    
    /* Allow the ED to resume. */

    pPipe->pEd->ed.control &= ~(TO_LITTLEL (OHCI_EDCTL_SKIP));
    DMA_FLUSH (&pPipe->pEd->ed.control, sizeof (pPipe->pEd->ed.control));
    }


/***************************************************************************
*
* serviceBusIrps - Services completed USB transactions
*
* RETURNS: N/A
*/

LOCAL VOID serviceBusIrps
    (
    pHCD_HOST pHost
    )

    {
    UINT32 pciPtrToTd;
    pTD_WRAPPER pFirstTd;
    pTD_WRAPPER pTd;
    pUSB_IRP pIrp;
    pIRP_WORKSPACE pWork;
    LIST_HEAD completeIrps = {0};


    /* Walk the list of complete TDs and update the status of their
     * respective IRPs.
     *
     * NOTE: TDs are in reverse order on the list...That is, the TDs
     * which executed first are behind TDs which executed later.  We
     * need to process them in the order in which they executed.
     */

    DMA_INVALIDATE (&pHost->pHcca->doneHead, sizeof (pHost->pHcca->doneHead));

    pFirstTd = NULL;

    pciPtrToTd = pHost->pHcca->doneHead & TO_LITTLEL (OHCI_PTR_MEM_MASK);

    while ((pTd = TD_FROM_PCIPTR (pciPtrToTd)) != NULL)
	{
	DMA_INVALIDATE (pTd, sizeof (*pTd));

	pTd->sw.doneLink = pFirstTd;
	pFirstTd = pTd;

	pciPtrToTd = pTd->tdg.nextTd & TO_LITTLEL (OHCI_PTR_MEM_MASK);
	}


    /* pFirstTd now points to a list of TDs in the order in which they
     * were completed.
     */

    while (pFirstTd != NULL)
	{
	pTd = pFirstTd;
	pFirstTd = pTd->sw.doneLink;

	pIrp = pTd->sw.pWork->pIrp;

	releaseTd (pHost, pTd->sw.pWork->pPipe, pIrp, pTd->sw.pWork, pTd);

	if (pIrp->result != PENDING)
	    {
	    /* IRP is finished...move it to the list of completed IRPs. */

	    usbListUnlink (&pIrp->hcdLink);
	    usbListLink (&completeIrps, pIrp, &pIrp->hcdLink, LINK_TAIL);
	    }
	}


    /* Invoke IRP callbacks for all completed IRPs. */

    while ((pIrp = usbListFirst (&completeIrps)) != NULL)
	{
	usbListUnlink (&pIrp->hcdLink);


	/* See if other IRPs can take advantage of freed TDs. */

	pWork = (pIRP_WORKSPACE) pIrp->hcdPtr;

	if (pIrp->result != OK)
	    unscheduleIrp (pHost, pIrp, pWork);

	scheduleOtherIrps (pHost, pWork->pPipe);


	/* Release IRP workspace and invoke callback. */

	freeIrpWorkspace (pHost, pIrp);
	setIrpResult (pIrp, pIrp->result);
	}
    }


/***************************************************************************
*
* scheduleIrp - schedules IRP for execution
*
* RETURNS: N/A
*/

LOCAL VOID scheduleIrp
    (
    pHCD_HOST pHost,
    pHCD_PIPE pPipe,
    pUSB_IRP pIrp
    )

    {
    pIRP_WORKSPACE pWork = (pIRP_WORKSPACE) pIrp->hcdPtr;


    /* Mark the time the IRP started */

    pWork->irpRunning = TRUE;	    /* running... */
    pWork->startTime = OSS_TIME (); /* and its start time */


    /* Assign starting frame number for isoch transfers */

    if (pPipe->transferType == USB_XFRTYPE_ISOCH)
	{
	if ((pIrp->flags & USB_FLAG_ISO_ASAP) != 0)
	    pWork->isochNext = FINDEX (getFrameNo (pHost) + HC_FRAME_SKIP);
	else
	    pWork->isochNext = FINDEX (pIrp->startFrame);
	}


    /* Assign TDs to direct data transfer. */

    assignTds (pHost, pPipe, pIrp, pWork);
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
    pHCD_PIPE pPipe;
    int s = OK;

    if (pIrp->result != PENDING)
	s = S_usbHcdLib_CANNOT_CANCEL;
    else
	{
	/* The IRP is pending.  Unlink it. */

	usbListUnlink (&pIrp->hcdLink);

	/* If this IRP is a "bus" IRP - as opposed to a root IRP - then
	 * remove it from the HC's work list.
	 */

	if ((pWork = (pIRP_WORKSPACE) pIrp->hcdPtr) != NULL &&
	    pWork->pPipe->busAddress != pHost->rootAddress)
	    {
	    /* Remove QHs/TDs from the work list and release workspace. */

	    pPipe = pWork->pPipe;

	    unscheduleIrp (pHost, pIrp, pWork);
	    freeIrpWorkspace (pHost, pIrp);


	    /* This may create an opportunity to schedule other IRPs. */

	    scheduleOtherIrps (pHost, pPipe);
	    }

	setIrpResult (pIrp, result);    
	}

    return s;
    }



/***************************************************************************
*
* shutOffListProcessing - shut of host controller interrupts for list proc.
*
* Permits the host controller from processing the periodic, interrupt and bulk
* TD lists.
*
* RETURNS: N/A.
*/

LOCAL void shutOffListProcessing
    (
    pHCD_HOST pHost
    )

    {
    UINT32 controlRegChange;

    hcSynch (pHost);

    /* 
     * Read the register and mask off the bits that denote list processing 
     * is enabled 
     */

    /*  Periodic List = Isoc transfers */

    controlRegChange = HC_DWORD_IN (OHCI_HC_CONTROL) & (OHCI_CTL_PLE);
    pHost->pHcControlReg = controlRegChange;
    HC_CLR_BITS (OHCI_HC_CONTROL, pHost->pHcControlReg);

    /*  Interrupt List = Interrupt transfers */

    controlRegChange = HC_DWORD_IN (OHCI_HC_CONTROL) &(OHCI_CTL_IE);
    pHost->pHcControlReg = controlRegChange;
    HC_CLR_BITS (OHCI_HC_CONTROL, pHost->pHcControlReg);

    /*  Bulk List = Bulk transfers */

    controlRegChange = HC_DWORD_IN (OHCI_HC_CONTROL) & OHCI_CTL_BLE;
    pHost->pHcControlReg = controlRegChange; 
    HC_CLR_BITS (OHCI_HC_CONTROL, pHost->pHcControlReg);

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
    UINT32 portRegOffset;
    UINT32 portChange;
    UINT32 hcRhPortStatusReg;

    for (port = 0; port < pHost->numPorts; port++)
	{
	portRegOffset = OHCI_HC_RH_PORT_STATUS + port * sizeof (UINT32);

	/* Read & clear pending change status */

	hcRhPortStatusReg = HC_DWORD_IN (portRegOffset);

	portChange = hcRhPortStatusReg & 
		     (OHCI_RHPS_CSC | 
		     OHCI_RHPS_PESC | 
		     OHCI_RHPS_PSSC | 
		     OHCI_RHPS_OCIC | 
		     OHCI_RHPS_PRSC);

	if (portChange != 0)
	    HC_DWORD_OUT (portRegOffset, portChange);

        hcSynch (pHost);

	/* Combine the change bits reported by the HC with those we
	 * already know about.  Then report if a change is pending.
	 */

	pHost->pRhPortChange [port] |= portChange;

	if ((pHost->pRhPortChange [port] & 
	    (OHCI_RHPS_CSC | 
	    OHCI_RHPS_PESC | 
	    OHCI_RHPS_PSSC | 
	    OHCI_RHPS_OCIC | 
	    OHCI_RHPS_PRSC)) != 0)
	    {
	    hubStatus |= USB_HUB_ENDPOINT_STS_PORT0 << port;
	    }

	/*  there was a change - a device has been plugged, or unplugged */

	if ( (hcRhPortStatusReg & OHCI_RHPS_CSC) !=0)

	    /*  A device has been unplugged */

	    if ( (hcRhPortStatusReg & OHCI_RHPS_CCS) == 0 )
		{
		/* Stop the host controller from processing TD's */

	        shutOffListProcessing (pHost);
		}

	}

    return hubStatus;
    }


/***************************************************************************
*
* serviceRootIrps - Services root hub transactions
*
* RETURNS: N/A
*/

LOCAL VOID serviceRootIrps
    (
    pHCD_HOST pHost
    )

    {
    UINT8 hubStatus;
    UINT16 irpCount;
    pUSB_IRP pIrp;


    if ((hubStatus = isHubStatus (pHost)) != 0)
	{
	for (irpCount = pHost->rootIrpCount; irpCount > 0; irpCount--)
	    {
	    pIrp = usbListFirst (&pHost->rootIrps);

	    --pHost->rootIrpCount;
	    usbListUnlink (&pIrp->hcdLink);

	    *(pIrp->bfrList [0].pBfr) = hubStatus;
	    pIrp->bfrList [0].actLen = 1;

	    setIrpResult (pIrp, OK);
	    }
	}
    }


/***************************************************************************
*
* processInterrupt - process a hardware interrupt from the HC
*
* Determine the cause of a hardware interrupt and service it.  If the
* interrupt results in the completion of one or more IRPs, handle the
* completion processing for those IRPs.
*
* RETURNS: N/A
*/

LOCAL VOID processInterrupt
    (
    pHCD_HOST pHost
    )

    {
    UINT32 intBits = HC_DWORD_IN (OHCI_HC_INT_STATUS) & INT_ENABLE_MASK;


    /* evaluate the interrupt condition */

    if ((intBits & OHCI_INT_SO) != 0)
	{
	/* Scheduling overrun detected.  Record it. */

	pHost->errScheduleOverrun++;
	HC_DWORD_OUT (OHCI_HC_INT_STATUS, OHCI_INT_SO); /* acknowledge */
	}

    if ((intBits & OHCI_INT_UE) != 0)
	{
	/* Unrecoverable error detected.  Record it. */

	pHost->errUnrecoverable++;
	HC_DWORD_OUT (OHCI_HC_INT_STATUS, OHCI_INT_UE); /* acknowledge */
	}

    if ((intBits & OHCI_INT_RD) != 0)
	{
	/* Resume detected. */

	if (pHost->mngmtCallback != NULL)
	    (*pHost->mngmtCallback) (pHost->mngmtCallbackParam, 
				     pHost->handle,
				     0 /* bus number */, 
				     HCD_MNGMT_RESUME);

	HC_DWORD_OUT (OHCI_HC_INT_STATUS, OHCI_INT_RD); /* acknowledge */
	}

    if ((intBits & OHCI_INT_RHSC) != 0)
	{
	/* Root host change detected. */

	serviceRootIrps (pHost);
	HC_DWORD_OUT (OHCI_HC_INT_STATUS, OHCI_INT_RHSC); /* acknowledge */
	}

    if ((intBits & OHCI_INT_WDH) != 0)
	{
	/* The HC has updated the done queue.  This implies completion 
	 * of one or more USB transactions detected. 
	 */

	serviceBusIrps (pHost);
	pHost->pHcca->doneHead = 0;
	HC_DWORD_OUT (OHCI_HC_INT_STATUS, OHCI_INT_WDH); /* acknowledge */
	}


    /* Re-enable HC interrupts */

    HC_DWORD_OUT (OHCI_HC_INT_ENABLE, INT_ENABLE_MASK);
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
* intThread - Thread invoked when hardware interrupts are detected
*
* By convention, the <param> to this thread is a pointer to an HCD_HOST.
* This thread waits on the intPending semaphore in the HCD_HOST which is
* signalled by the actual interrupt handler.  This thread will continue
* to process interrupts until intThreadExitRequest is set to TRUE in the
* HCD_HOST structure.
*
* This thread wakes up every HC_TIMEOUT_SRVC_INTERVAL milliseconds and 
* checks for IRPs which may have timed-out.
*
* RETURNS: N/A
*/

LOCAL VOID intThread
    (
    pVOID param
    )

    {
    pHCD_HOST pHost = (pHCD_HOST) param;
    UINT32 interval;
    UINT32 now;
    UINT32 lastTime = OSS_TIME ();

    do
	{
	/* Wait for an interrupt to be signalled. */

	now = OSS_TIME ();
	interval = HC_TIMEOUT_SRVC_INTERVAL - 
		   min (now - lastTime, HC_TIMEOUT_SRVC_INTERVAL);

	if (OSS_SEM_TAKE (pHost->intPending, interval) == OK)
	    {
	    /* semaphore was signalled, int pending */

	    if (!pHost->intThreadExitRequest)
		{
		OSS_MUTEX_TAKE (pHost->hostMutex, OSS_BLOCK);
		processInterrupt (pHost);
		OSS_MUTEX_RELEASE (pHost->hostMutex);
		}
	    }

	if ((now = OSS_TIME ()) - lastTime >= HC_TIMEOUT_SRVC_INTERVAL)
	    {
	    /* check for timed-out IRPs */

	    lastTime = now;

	    /* Look for IRPs which may have timed-out. */

	    OSS_MUTEX_TAKE (pHost->hostMutex, OSS_BLOCK);
	    checkIrpTimeout (pHost);
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
* HC.  This routine immediately reflects the interrupt to the intThread.
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


    /* Is there an interrupt pending in the OHCI interrupt status reg? */

    if ((HC_DWORD_IN (OHCI_HC_INT_STATUS) & INT_ENABLE_MASK) != 0)
	{

	pHost->intCount++;

	/* Disable further interrupts until the intThread takes over */

	HC_DWORD_OUT (OHCI_HC_INT_DISABLE, OHCI_INT_MIE);

	/* Signal the interrupt thread to process the interrupt. */

	OSS_SEM_GIVE (pHost->intPending);
	}

    /* Prevent the hcSync routine from busy waiting. */

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
    if (pPipe->pEd != NULL) 
	{
	/* Release ED allocated for pipe. */
	unschedulePipe (pHost, pPipe);
	DMA_FREE (pPipe->pEd);
	}


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


    /* Disable the HC */

    HC_DWORD_OUT (OHCI_HC_CONTROL, 0);
    HC_DWORD_OUT (OHCI_HC_COMMAND_STATUS, OHCI_CS_HCR);

    /* 
     * By default SYS_OHCI_RESET is a no-op, but it can be overridden
     * in BSP stubs as necessary.
     */

    SYS_OHCI_RESET();

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


    /* release root hub per-port data */

    if (pHost->pRhPortChange != NULL)
	OSS_FREE (pHost->pRhPortChange);


    /* eliminate DMA memory pool.
     *
     * NOTE: This automatically deletes any objects allocated within the
     * DMA memory pool, e.g., the OHCI HCCA.
     */

    if (pHost->dmaPool != NULL)
	cacheDmaFree (pHost->dmaPool);

    OSS_FREE (pHost);
    }


/***************************************************************************
*
* fncAttach - Initialize the HCD and attach to specified bus(es)
*
* The convention for the OHCI HCD is that the param passed in the HRB
* is a pointer to a PCI_CFG_HEADER structure for the HC to be managed.
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
    UINT32 memBase;
    int i;

    int s;

    /* validate parameters */

    if ((s = validateHrb (pHrb, sizeof (*pHrb))) != OK)
	return s;


    /* Check to make sure structures compiled to correct size */

    if (OHCI_HCCA_ACTLEN != OHCI_HCCA_LEN || 
	OHCI_ED_ACTLEN != OHCI_ED_LEN ||
	OHCI_TD_GEN_ACTLEN != OHCI_TD_GEN_LEN ||
	OHCI_TD_ISO_ACTLEN != OHCI_TD_ISO_LEN ||
	TD_WRAPPER_ACTLEN != TD_WRAPPER_LEN)

	{
	return S_usbHcdLib_STRUCT_SIZE_FAULT;
	}


    /* determine io base address */

    pCfgHdr = (pPCI_CFG_HEADER) pHrb->param;
    memBase = 0;

    for (i = 0; i < PCI_CFG_NUM_BASE_REG; i++)
	if ((pCfgHdr->baseReg [i] & PCI_CFG_BASE_IO) == 0 &&
	    (memBase = pCfgHdr->baseReg [i] & PCI_CFG_MEMBASE_MASK) != 0)
	    break;

    if (memBase == 0)
	return S_usbHcdLib_HW_NOT_READY;


    /* create/initialize an HCD_HOST structure to manage the HC. */

    if ((pHost = OSS_CALLOC (sizeof (*pHost))) == NULL)
	return S_usbHcdLib_OUT_OF_MEMORY;

    memcpy (&pHost->pciCfgHdr, pCfgHdr, sizeof (*pCfgHdr));

    pHost->mngmtCallback = pHrb->mngmtCallback;
    pHost->mngmtCallbackParam = pHrb->mngmtCallbackParam;


    /* initialize pHost->memBase...cannot use HC_SET_BITS, etc. until 
     * ioBase is initialized.
     */

    pHost->memBase = (pUINT32) (memBase + USB_PCI_MEMIO_OFFSET());

    /* 
     * Create a semaphore to syncronize the host controller.  This is used
     * to make the HC wait a frame for traffic to settle 
     */

    pHost->hcSyncSem = semBCreate(SEM_Q_FIFO, SEM_EMPTY);

    if (pHost->hcSyncSem == NULL)
        return S_usbHcdLib_OUT_OF_RESOURCES;    

    /* Check the hardware revision level */

    if ((HC_DWORD_IN (OHCI_HC_REVISION) & OHCI_RREV_REV_MASK) 
	< REQUIRED_OHCI_LEVEL)
    
	return S_usbHcdLib_HW_NOT_READY;


    /* reset the host controller.
     *
     * NOTE: At the same time, we disable anything else which may be
     * enabled in the OHCI control register.
     *
     * NOTE: We allow the HC to revert to the nominal (default) frame
     * interval in response to the reset.
     */

    /* Initiate software reset */

    HC_SET_BITS (OHCI_HC_COMMAND_STATUS, OHCI_CS_HCR);

    /* Make ports power swiched */

    HC_CLR_BITS (OHCI_HC_RH_DESCR_A, OHCI_RHDA_NPS);
 
    /* All ports are powered at the same time */

    HC_CLR_BITS (OHCI_HC_RH_DESCR_A, OHCI_RHDA_PSM);

    HC_DWORD_OUT (OHCI_HC_CONTROL, 0);

    /*  Wait for propper reset signaling to occur. */

    OSS_THREAD_SLEEP (USB_TIME_RESET);

    /* 
     * By default SYS_OHCI_RESET is a no-op, but it can be overridden
     * in BSP stubs as necessary.
     */

    SYS_OHCI_RESET();

    if (!waitOnBits (pHost, OHCI_HC_COMMAND_STATUS, OHCI_CS_HCR, 0))
	{
	destroyHost (pHost);
	return S_usbHcdLib_HW_NOT_READY;
	}

    if ((pHost->dmaPool = cacheDmaMalloc (DMA_MEMORY_SIZE))  == NULL || 
	(pHost->memPartId = memPartCreate (pHost->dmaPool, 
					   DMA_MEMORY_SIZE)) == NULL || 
	(pHost->pRhPortChange = OSS_CALLOC (pHost->numPorts * sizeof (UINT32))) == NULL	|| 
	(pHost->pHcca = DMA_MALLOC (sizeof (*pHost->pHcca), 
				    OHCI_HCCA_ALIGNMENT)) == NULL || 
	(pHost->pIsochAnchorEd = DMA_MALLOC (sizeof (*pHost->pIsochAnchorEd), 
					     OHCI_TD_ISO_ALIGNMENT)) == NULL || 
	usbHandleCreate (HCD_HOST_SIG, 
			pHost, 
			&pHost->handle) != OK || 
	OSS_SEM_CREATE (MAX_INT_DEPTH, 
			0, 
			&pHost->intPending) != OK || 
	OSS_SEM_CREATE (1, 
			0, 
			&pHost->intThreadExit) != OK || 
	OSS_MUTEX_CREATE (&pHost->hostMutex) != OK || 
	OSS_THREAD_CREATE (intThread, 
			   pHost, 
			   OSS_PRIORITY_INTERRUPT,
			   "tOhciInt", 
			   &pHost->intThread) != OK)
	{
	destroyHost (pHost);
	return S_usbHcdLib_OUT_OF_RESOURCES;
	}


    /* 
     *  Provide an empty HCCA before the first reset of the part, so that
     *  the controller has a valid spot to write its frame count.
     */

    memset (pHost->pHcca, 0, sizeof(*pHost->pHcca));
    DMA_FLUSH (pHost->pHcca, sizeof (*pHost->pHcca));
    HC_DWORD_OUT (OHCI_HC_HCCA, USB_MEM_TO_PCI (pHost->pHcca));
    /* Determine the number of ports supported by this host controller. */

    pHost->numPorts = OHCI_RHDA_NDP (HC_DWORD_IN (OHCI_HC_RH_DESCR_A));
    pHost->numPorts = min (pHost->numPorts, MAX_ROOT_PORTS);


    /* Determine root hub power characteristics. */

    pHost->pwrOn2PwrGood = OHCI_RHDA_POTPGT (HC_DWORD_IN (OHCI_HC_RH_DESCR_A));

    /* Place the HC in the operational state so we can start talking
     * to it (e.g., cannot necessarily write to OHCI_HC_RH_STATUS unless
     * HC is in operational state).
     */

    HC_DWORD_OUT (OHCI_HC_CONTROL, OHCI_CTL_HCFS_OP);
    hcSynch (pHost);

    /* Explictly enable power to the USB ports. */

    HC_SET_BITS (OHCI_HC_RH_DESCR_A, OHCI_RHDA_NPS);

    HC_SET_BITS (OHCI_HC_RH_DESCR_A, OHCI_RHDA_PSM);

    /* initialize the emulated root hub descriptor */

    pHost->hubDescr.length = USB_HUB_DESCR_LEN;
    pHost->hubDescr.descriptorType = USB_DESCR_HUB;
    pHost->hubDescr.nbrPorts = pHost->numPorts;
    pHost->hubDescr.hubCharacteristics = 
    TO_LITTLEW (USB_HUB_INDIVIDUAL_POWER | 
		USB_HUB_NOT_COMPOUND | 
		USB_HUB_INDIVIDUAL_OVERCURRENT);
    pHost->hubDescr.pwrOn2PwrGood = pHost->pwrOn2PwrGood;
    pHost->hubDescr.hubContrCurrent = OHCI_HUB_CONTR_CURRENT;
    pHost->hubDescr.deviceRemovable [0] = 0;
    pHost->hubDescr.portPwrCtrlMask [0] = 0xff; /* per hub spec. */


    /* initialize the emulated endpoint descriptor */

    pHost->endpntDescr.length = USB_ENDPOINT_DESCR_LEN;
    pHost->endpntDescr.descriptorType = USB_DESCR_ENDPOINT;
    pHost->endpntDescr.endpointAddress = HC_STATUS_ENDPOINT_ADRS;
    pHost->endpntDescr.attributes = USB_ATTR_INTERRUPT;
    pHost->endpntDescr.maxPacketSize = TO_LITTLEW (HC_MAX_PACKET_SIZE);
    pHost->endpntDescr.interval = OHCI_HUB_INTERVAL;


    /* initialize the emulated interface descriptor */

    pHost->ifDescr.length = USB_INTERFACE_DESCR_LEN;
    pHost->ifDescr.descriptorType = USB_DESCR_INTERFACE;
    pHost->ifDescr.numEndpoints = 1;
    pHost->ifDescr.interfaceClass = USB_CLASS_HUB;


    /* initialize the emulated configuration descriptor */

    pHost->configDescr.length = USB_CONFIG_DESCR_LEN;
    pHost->configDescr.descriptorType = USB_DESCR_CONFIGURATION;
    pHost->configDescr.totalLength = 
    TO_LITTLEW (pHost->configDescr.length + 
		pHost->ifDescr.length + 
		pHost->endpntDescr.length);
    pHost->configDescr.numInterfaces = 1;
    pHost->configDescr.configurationValue = HC_CONFIG_VALUE;
    pHost->configDescr.attributes = USB_ATTR_SELF_POWERED;
    pHost->configDescr.maxPower = OHCI_HUB_CONTR_CURRENT;


    /* initialize the emulated device descriptor */

    pHost->devDescr.length = USB_DEVICE_DESCR_LEN;
    pHost->devDescr.descriptorType = USB_DESCR_DEVICE;
    pHost->devDescr.bcdUsb = TO_LITTLEW (USB_RELEASE);
    pHost->devDescr.deviceClass = USB_CLASS_HUB;
    pHost->devDescr.maxPacketSize0 = HC_MAX_PACKET_SIZE;
    pHost->devDescr.manufacturerIndex = HC_STR_MFG;
    pHost->devDescr.productIndex = HC_STR_PROD;
    pHost->devDescr.numConfigurations = 1;


    /* initialize other hub/port state information */

    pHost->rootAddress = 0;
    pHost->configValue = 0;
    

    /* initialize the interrupt ED list and program the HCCA register */

    memset (pHost->pHcca, 0, sizeof (*pHost->pHcca));
    memset (pHost->pIsochAnchorEd, 0, sizeof (*pHost->pIsochAnchorEd));

    pHost->pIsochAnchorEd->ed.control = 
    TO_LITTLEL (OHCI_EDCTL_FMT_ISO | OHCI_EDCTL_SKIP);

    for (i = 0; i < OHCI_INT_ED_COUNT; i++)
	pHost->pHcca->intEdTable [i] = TO_LITTLEL (TO_PCIPTR (pHost->pIsochAnchorEd));

    DMA_FLUSH (pHost->pIsochAnchorEd, sizeof (*pHost->pIsochAnchorEd));
    DMA_FLUSH (pHost->pHcca, sizeof (*pHost->pHcca));

    HC_DWORD_OUT (OHCI_HC_HCCA, USB_MEM_TO_PCI (pHost->pHcca));


    /* hook the hardware interrupt for the HC */

    if (usbPciIntConnect (intHandler, pHost, pHost->pciCfgHdr.intLine) != OK)
	{
	destroyHost (pHost);
	return S_usbHcdLib_INT_HOOK_FAILED;
	}

    pHost->intInstalled = TRUE;


    /* Set up the frame interval register. */

    setFrameInterval (pHost, OHCI_FMI_FI_DEFAULT);


    /* Set the periodic start equal to the frame interval.
     *
     * NOTE: The OHCI spec. suggests setting periodic start equal to 90% of
     * the frame interval.  However, in this implementation, we set periodic
     * start equal to 100% of frame interval.  This should force all periodic
     * transfers to start at the beginning of each frame.  Elsewhere, we
     * ensure that no more than 90% of the bus is allocated to periodic
     * (interrupt and isoch) transfers, so we achieve the same outcome.
     */

    HC_DWORD_OUT (OHCI_HC_PERIODIC_START, OHCI_PS_PS_FMT (OHCI_FMI_FI_DEFAULT));


    /* Enable root hub port pwr */

    HC_DWORD_OUT (OHCI_HC_RH_STATUS, OHCI_RHS_SET_GPWR);


    /* Enable the HC (operational state was set earlier) */

    HC_SET_BITS (OHCI_HC_CONTROL, 
		OHCI_CTL_CBSR_3TO1 |
		OHCI_CTL_PLE | 
		OHCI_CTL_IE | 
		OHCI_CTL_CLE | 
		OHCI_CTL_BLE |
		OHCI_CTL_RWC | 
		OHCI_CTL_RWE);


    /* Enable interrupts */

    HC_DWORD_OUT (OHCI_HC_INT_ENABLE, INT_ENABLE_MASK);


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

	    /* Set the operational state to suspend...
	     *
	     * As a biproduct of this, all outstanding ERPs will
	     * eventually time-out.
	     */

	    HC_DWORD_OUT (OHCI_HC_CONTROL, 
			  (HC_DWORD_IN (OHCI_HC_CONTROL) & 
			  ~OHCI_CTL_HCFS_MASK) | 
			  OHCI_CTL_HCFS_SUSPEND);
	    }
	}

    if ((pHrb->busState & HCD_BUS_RESUME) != 0)
	{
	/* RESUME bus */

	if (pHost->suspended)
	    {
	    pHost->suspended = FALSE;

	    /* Force global resume */

	    HC_DWORD_OUT (OHCI_HC_CONTROL, 
			  (HC_DWORD_IN (OHCI_HC_CONTROL) & 
			  ~OHCI_CTL_HCFS_MASK) | 
			  OHCI_CTL_HCFS_RESUME);

	    OSS_THREAD_SLEEP (USB_TIME_RESUME);

	    HC_DWORD_OUT (OHCI_HC_CONTROL, 
			  (HC_DWORD_IN (OHCI_HC_CONTROL) & 
			  ~OHCI_CTL_HCFS_MASK) | 
			  OHCI_CTL_HCFS_OP);
	    }
	}

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

    pHrb->sofInterval = OHCI_FMI_FI (HC_DWORD_IN (OHCI_HC_FM_INTERVAL));

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


    /* Set the new SOF interval. */

    setFrameInterval (pHost, pHrb->sofInterval);

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

    /* NOTE: We don't expose the HC's frame counter for the frame
     * scheduling window.  Instead, we expose the size of the frame
     * list.  Otherwise, we would need to keep track of each time the
     * frame counter rolls over.
     */

    pHrb->frameWindow = OHCI_FRAME_WINDOW;

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
    UINT32 portReg;
    int s = OK;

    if ((pSetup->requestType & ~USB_RT_DEV_TO_HOST) == 
	(USB_RT_CLASS | USB_RT_DEVICE))
	{
	/* hub class features, directed to hub itself */

	switch (FROM_LITTLEW (pSetup->value))
	    {
	    case USB_HUB_FSEL_C_HUB_LOCAL_POWER:
	    case USB_HUB_FSEL_C_HUB_OVER_CURRENT:
	
	    /* Not implemented */
	    break;
	    }
	}
    else if ((pSetup->requestType & ~USB_RT_DEV_TO_HOST) ==
	     (USB_RT_CLASS | USB_RT_OTHER))
	{
	/* port class features, directed to port */

	/* NOTE: port parameter is 1-based, not 0-based, in Setup packet. */

	if ((port = FROM_LITTLEW (pSetup->index) - 1) >= pHost->numPorts)
	    return S_usbHcdLib_BAD_PARAM;

	portReg = OHCI_HC_RH_PORT_STATUS + port * sizeof (UINT32);

	switch (FROM_LITTLEW (pSetup->value))
	    {
	    case USB_HUB_FSEL_PORT_ENABLE:

		if (setFeature)
		    HC_SET_BITS (portReg, OHCI_RHPS_SET_PE);
		else
		    HC_SET_BITS (portReg, OHCI_RHPS_CLR_PE);
		break;

	    case USB_HUB_FSEL_PORT_SUSPEND:

		if (setFeature)
		    HC_SET_BITS (portReg, OHCI_RHPS_SET_PS);
		else
		    HC_SET_BITS (portReg, OHCI_RHPS_CLR_PS);
		break;
	    
	    case USB_HUB_FSEL_PORT_RESET:

		if (setFeature)
		    {
		    /* Note: The port is supposed to be enabled upon 
		     * completion of reset.
		     */

		    HC_SET_BITS (portReg, OHCI_RHPS_PRS | OHCI_RHPS_PES);
		    OSS_THREAD_SLEEP (USB_TIME_RESET);

		    if (waitOnBits (pHost, 
				    portReg, 
				    OHCI_RHPS_PRSC, 
				    OHCI_RHPS_PRSC))
			{
			OSS_THREAD_SLEEP (USB_TIME_RESET_RECOVERY);
			}
		    else
			{
			s = S_usbHcdLib_HW_NOT_READY;
			}
		    }
		break;

	    case USB_HUB_FSEL_PORT_POWER:

		if (setFeature)
		    HC_SET_BITS (portReg, OHCI_RHPS_SET_PWR);
		else
		    HC_SET_BITS (portReg, OHCI_RHPS_CLR_PWR);
		break;

	    case USB_HUB_FSEL_C_PORT_CONNECTION:

		if (!setFeature)
		    pHost->pRhPortChange [port] &= ~OHCI_RHPS_CSC;
		break;

	    case USB_HUB_FSEL_C_PORT_ENABLE:

		if (!setFeature)
		    pHost->pRhPortChange [port] &= ~OHCI_RHPS_PESC;
		break;
	
	    case USB_HUB_FSEL_C_PORT_SUSPEND:

		if (!setFeature)
		    pHost->pRhPortChange [port] &= ~OHCI_RHPS_PSSC;
		break;

	    case USB_HUB_FSEL_C_PORT_OVER_CURRENT:

		if (!setFeature)
		    pHost->pRhPortChange [port] &= ~OHCI_RHPS_OCIC;
		break;

	    case USB_HUB_FSEL_C_PORT_RESET:

		if (!setFeature) 
		    pHost->pRhPortChange [port] &= ~OHCI_RHPS_PRSC;
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

    if (pSetup->requestType == (USB_RT_DEV_TO_HOST | 
				USB_RT_STANDARD | 
				USB_RT_DEVICE))
	{
	switch (MSB (FROM_LITTLEW (pSetup->value)))
	    {
	    case USB_DESCR_DEVICE:	
		usbDescrCopy32 (pIrp->bfrList [1].
				pBfr, &pHost->devDescr, 
				pIrp->bfrList [1].bfrLen, 
				&pIrp->bfrList [1].actLen);
		break;

	    case USB_DESCR_CONFIGURATION:
		memcpy (bfr, &pHost->configDescr, USB_CONFIG_DESCR_LEN);
		memcpy (&bfr [USB_CONFIG_DESCR_LEN], 
			&pHost->ifDescr,
			USB_INTERFACE_DESCR_LEN);
		memcpy (&bfr [USB_CONFIG_DESCR_LEN + USB_INTERFACE_DESCR_LEN],
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

		    case HC_STR_MFG:
			usbDescrStrCopy32 (pIrp->bfrList [1].pBfr, 
					   HC_STR_MFG_VAL, 
					   pIrp->bfrList [1].bfrLen, 
					   &pIrp->bfrList [1].actLen);
			break;

		    case HC_STR_PROD:
			usbDescrStrCopy32 (pIrp->bfrList [1].pBfr, 
					   HC_STR_PROD_VAL, 
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
    UINT32 portReg;
    UINT32 portStatus;


    /* validate parameters */

    if (pIrp->bfrCount < 2 || pIrp->bfrList [1].pBfr == NULL)
	return S_usbHcdLib_BAD_PARAM;


    /* return status based on type of request received */

    memset (&status, 0, sizeof (status));

    if (pSetup->requestType ==(USB_RT_DEV_TO_HOST | 
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

	if ((port = FROM_LITTLEW (pSetup->index) - 1) >= pHost->numPorts)
	    return S_usbHcdLib_BAD_PARAM;

	portReg = OHCI_HC_RH_PORT_STATUS + port * sizeof (UINT32);
	portStatus = HC_DWORD_IN (portReg) | pHost->pRhPortChange [port];

	status.hub.status = TO_LITTLEW (portStatus & 0xffff);
	status.hub.change = TO_LITTLEW (portStatus >> 16);

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

    if (pIrp->bfrCount < 1 || 
	pIrp->bfrList [0].bfrLen < sizeof (USB_SETUP) || 
	(pSetup = (pUSB_SETUP) pIrp->bfrList [0].pBfr) == NULL)
    
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
    /* make sure the request is to the correct endpoint, etc. */

    if (pPipe->endpoint != HC_STATUS_ENDPOINT_ADRS ||
	pIrp->bfrCount < 1 || pIrp->bfrList [0].pBfr == NULL)
    return S_usbHcdLib_BAD_PARAM;


    /* link this IRP to the list of root IRPs */

    usbListLink (&pHost->rootIrps, pIrp, &pIrp->hcdLink, LINK_TAIL);
    ++pHost->rootIrpCount;


    /* See if there is anything to report. */

    serviceRootIrps (pHost);

    return PENDING;
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
    /* Allocate workspace for this IRP */

    if (!allocIrpWorkspace (pHost, pPipe, pIrp))
	return S_usbHcdLib_OUT_OF_MEMORY;


    /* Add this IRP to the scheduling list and invoke the IRP scheduler. */

    usbListLink (&pHost->busIrps, pIrp, &pIrp->hcdLink, LINK_TAIL);
    scheduleIrp (pHost, pPipe, pIrp);

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
    UINT32 control;
    UINT16 tdLen;
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
				   HC_HOST_DELAY, 
				   HC_HUB_LS_SETUP);


    /* Is the indicated amount of bandwidth available? */

    if (pHost->nanoseconds + pHrb->time > USB_LIMIT_ISOCH_INT)
	return S_usbHcdLib_BANDWIDTH_FAULT;


    /* Create a pipe structure to describe this pipe */

    if ((pPipe = OSS_CALLOC (sizeof (*pPipe))) == NULL)
	return S_usbHcdLib_OUT_OF_MEMORY;

    if (usbHandleCreate (HCD_PIPE_SIG, pPipe, &pPipe->pipeHandle) != OK)
	{
	destroyPipe (pHost, pPipe);
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
    pPipe->actInterval = calcIntInterval (pPipe->interval);

    pPipe->time = pHrb->time;


    if (pPipe->busAddress != pHost->rootAddress)
	{
	/* Initialize TDs for pipe. */

	pPipe->tdCount = (pPipe->transferType == USB_XFRTYPE_ISOCH) ? 
			  TD_COUNT_ISO : 
			  TD_COUNT_GEN;

	pPipe->freeTdCount = pPipe->tdCount - 1;
					/* one always not used */
					/* cf OHCI sec. 5.2.8.2 */

	tdLen = pPipe->tdCount * sizeof (TD_WRAPPER);

	if ((pPipe->pTds = DMA_MALLOC (tdLen, OHCI_TD_ISO_ALIGNMENT)) == NULL)
	    {
	    destroyPipe (pHost, pPipe);
	    return S_usbHcdLib_OUT_OF_MEMORY;
	    }

	memset (pPipe->pTds, 0, tdLen);


	/* Initialize ED (endpoint descriptor) for pipe. */

	if ((pPipe->pEd = DMA_MALLOC (sizeof (*pPipe->pEd), 
				      OHCI_ED_ALIGNMENT)) == NULL)
	    {
	    destroyPipe (pHost, pPipe);
	    return S_usbHcdLib_OUT_OF_MEMORY;
	    }
	
	memset (pPipe->pEd, 0, sizeof (*pPipe->pEd));

	control = OHCI_EDCTL_FA_FMT (pPipe->busAddress) | 
				     OHCI_EDCTL_EN_FMT (pPipe->endpoint) | 
				     OHCI_EDCTL_DIR_TD | 
				     OHCI_EDCTL_MPS_FMT (pPipe->maxPacketSize);

	control |= (pPipe->speed == USB_SPEED_LOW) ? 
		   OHCI_EDCTL_SPD_LOW : 
		   OHCI_EDCTL_SPD_FULL;

	control |= (pPipe->transferType == USB_XFRTYPE_ISOCH) ? 
		   OHCI_EDCTL_FMT_ISO : 
		   OHCI_EDCTL_FMT_GEN;

	pPipe->pEd->ed.control = TO_LITTLEL (control);
	pPipe->pEd->ed.tdTail = pPipe->pEd->ed.tdHead =	
				TO_LITTLEL (TO_PCIPTR (pPipe->pTds));

	pPipe->pEd->sw.pPipe = pPipe;

	schedulePipe (pHost, pPipe);
	}


    /* Link pipe to host controller */

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
			   (pVOID *) &pPipe) != OK)
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
    UINT32 control;
    int s;

    /* validate parameters */

    if ((s = validateHrb (pHrb, sizeof (*pHrb))) != OK)
	return s;

    if (usbHandleValidate (pHrb->pipeHandle, 
			   HCD_PIPE_SIG, 
			   (pVOID *) &pPipe) != OK)

	return S_usbHcdLib_BAD_HANDLE;


    /* update pipe charactistics */

    if (pHrb->busAddress != 0)
	pPipe->busAddress = pHrb->busAddress;

    if (pHrb->maxPacketSize != 0)
	pPipe->maxPacketSize = pHrb->maxPacketSize;


    if (pPipe->pEd != NULL)
	{
	/* Modify OHCI_ED allocated for this pipe. */

	control = FROM_LITTLEL (pPipe->pEd->ed.control);

	control &= ~(OHCI_EDCTL_FA_MASK | OHCI_EDCTL_MPS_MASK);
	control |= OHCI_EDCTL_FA_FMT (pPipe->busAddress) | 
				      OHCI_EDCTL_MPS_FMT (pPipe->maxPacketSize);

	pPipe->pEd->ed.control = TO_LITTLEL (control);

	DMA_FLUSH (&pPipe->pEd->ed.control, sizeof (pPipe->pEd->ed.control));
	}


    return OK;
    }


/***************************************************************************
*
* usbHcdOhciExec - HCD_EXEC_FUNC entry point for OHCI HCD
*
* RETURNS: OK or ERROR
*
* ERRNO:
*  S_usbHcdLib_BAD_PARAM
*  S_usbHcdLib_BAD_HANDLE
*  S_usbHcdLib_SHUTDOWN
*/

STATUS usbHcdOhciExec
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
	if (usbHandleValidate (pHeader->handle, 
			       HCD_HOST_SIG, 
			       (pVOID *) &pHost) != OK)

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

