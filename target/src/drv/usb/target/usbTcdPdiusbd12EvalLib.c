/* usbTcdPdiusbd12EvalLib.c - USB target controller driver for Philips PDIUSBD12 */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01e,12apr00wef   Fixed uninitialized variable warning: pTarget in 
		 usbTcdPdiusbd12EvalExec() 
01d,17mar00,rcb  Add code to block re-entrant processing of the ERP
		 queue...formerly, a fast host could cause the interrupt
		 thread to decend endlessly down the stack if the target
		 application re-submits an ERP on the ERP completion
		 callback.
		 Change order in which endpoint interrupt causes are
		 evaluated to make sure a Setup transaction received just
		 after another transaction doesn't get processed out of
		 order.
		 Add code in destroyTarget() to cancel outstanding ERPs.
01c,23nov99,rcb  Change #include ../xxx references to lower case.
01b,12nov99,rcb  Short path names...affects "#include" directives.
01a,09aug99,rcb  First.
*/

/*
DESCRIPTION

This module implements the USB TCD (Target Controller Driver) for the Philips 
PDIUSBD12 used in conjuction with the PDIUSBD12 ISA evaluation kit from Philips.

This module exports a single entry point, usbTcdPdiusbd12EvalExec().  This is the
USB_TCD_EXEC_FUNC for this TCD.  The caller passes requests to the TCD by 
constructing TRBs, or Target Request Blocks, and passing them to this entry point.

The USB TCD works in conjuction with the usbTargLib module.  In order to simplify
the creation of a TCD for a given USB target controller, the usbTargLib handles
most parameter and logical validation of requests.  The TCD, therefore, is largely
free to execute requests without extensive validation, knowing that usbTargLib
will have validated the requests beforehand.

TCDs are initialized by invoking the TCD_FNC_ATTACH function.  In response to this
function, the TCD returns information about the target controller, including its
USB speed, the number of endpoints it supports, and information about each
endpoint.  

Endpoint information is maintained in an array of USB_TARG_ENDPOINT_INFO
structures.  These structures are allocated and owned by the TCD.  The usbTargLib
and the callers of the usbTargLib are allowed to examine this array, but are not
allowed to modify it.  All modification of the array is performed generally by
invoking the TCDs TCD_FNC_ENDPOINT_ASSIGN and TCD_FNC_ENDPOINT_RELEASE functions.
These functions are invoked upon the creation and deletion of USB pipes,
respectively.

By convention, each endpoint exposed in the array of USB_TARG_ENDPOINT_INFO 
structures is unidirectional, ie., either and OUT or IN endpoint.  The TCD is
required to present the endpoints in this way, even if the underlying hardware
presents the endpoints as bi-directional.  In this way, the calling application
can choose to assign endpoints to pipes as it sees fit.  Also, by convention, the
first two endpoints in the array MUST be the OUT and IN endpoints which are most
appropriate for use as the default control pipe (endpoint #0).

Once pipes have been created, the TCD is instructed to wait for data transfers
initiated by the host USB controller.  The TCD callers pass ERPs (Endpoint
Request Blocks) to the TCD to request and manage data transfers.  It is important
to remember that all USB data transfers are actually initiated by the host, so 
no data transfers will actually be performed by the TCD and target controller
hardware unless the USB host controller is polling the target.

It is also important to note a convention regarding the direction of data
transfers.  The transfer of data is always viewed from the host's perspective.
Therefore, an OUT transfer is a transfer from the host to the device (target) and
an IN transfer is a transfer from the device to the host.  Therefore,
USB_DIR_OUT, USB_PID_SETUP, and USB_PID_OUT refer to host->device transfers 
while USB_DIR_IN and USB_PID_IN refer to device->host transfers.

During TCD_FNC_ATTACH processing, the TCD creates a task to handle target
controller interrupts.	Interrupts are first received by a normal interrupt
handler, and this interrupt handler immediately reflects the interrupt to the
interrupt task.  All TCD callbacks to the usbTargLib are made either from the
caller's task or from the interrupt task.  No callbacks are made from the
interrupt context itself.

Conditional code:

The use of DMA (PC XT/AT style 8237 slave-mode DMA) is enabled by defining
the constant D12_USE_DMA.  Leaving this undefined causes the driver to run
exclusively in "programmed-I/O" mode.

In testing the Philips PDIUSBD12 has appeared to set the "data success" bit
incorrectly, causing software to declare a failure.  The use of this bit
can be enabled or disabled by definining the constant D12_CHECK_DATA_SUCCESS_BIT.
Leaving this constant undefined disables error checking.
*/

/* includes */

#include "usb/usbPlatform.h"		    /* platform definitions */

#include "string.h"


#include "usb/ossLib.h" 		    /* OS services */
#include "usb/usbListLib.h"		    /* linked list functions */
#include "usb/usb.h"			    /* USB definitions */
#include "usb/target/usbIsaLib.h"	    /* ISA bus functions */
#include "drv/usb/target/usbTcd.h"	    /* generic TCD interface */
#include "drv/usb/target/usbPdiusbd12.h"    /* Philips PDIUSBD12 defn */
#include "drv/usb/target/usbPdiusbd12Eval.h"	    /* eval board definitions */
#include "drv/usb/target/usbTcdPdiusbd12EvalLib.h"  /* our API */


/* defines */

/* BUILD OPTIONS */

#define D12_USE_DMA			/* Allow use of DMA */


#define INT_THREAD_NAME "tD12Int"	/* name of int handler thread */

#define INT_THREAD_EXIT_TIMEOUT 5000	/* 5 seconds */


/* macros used to read/write PDIUSBD12 registers */

#define OUT_D12_CMD(b)	\
    USB_ISA_BYTE_OUT (pTarget->ioBase + D12EVAL_D12REG + D12_CMD_REG, (b))

#define OUT_D12_DATA(b) \
    USB_ISA_BYTE_OUT (pTarget->ioBase + D12EVAL_D12REG + D12_DATA_REG, (b))

#define IN_D12_DATA()	\
    USB_ISA_BYTE_IN (pTarget->ioBase + D12EVAL_D12REG + D12_DATA_REG)


/* macros used to read/write PDIUSBD12 eval general IN/OUT registers */

#define OUT_EVAL_GOUT(b)    \
    USB_ISA_BYTE_OUT (pTarget->ioBase + D12EVAL_GOUT_REG, (b))

#define IN_EVAL_GIN()	\
    USB_ISA_BYTE_IN (pTarget->ioBase + D12EVAL_GIN_REG)


/* macro to invoke caller's management callback */

#define MNGMT_CALLBACK(pTarget, mngmtCode) \
    (*(pTarget)->mngmtCallback) ((pTarget)->mngmtCallbackParam, \
    (TCD_HANDLE) (pTarget), (mngmtCode))


/* Parameters to certain PDIUSBD12 functions */

#define ENABLE	TRUE
#define DISABLE FALSE


/* LEDs on PDIUSBD12 evaluation board used for debugging. */

#define ATTACH_LED	D12EVAL_GOUT_LED_D2
#define ENABLE_LED	D12EVAL_GOUT_LED_D3
#define INT_THREAD_LED	D12EVAL_GOUT_LED_D4


/* typedefs */

/* ERP_WORKSPACE
 *
 * An ERP_WORKSPACE structure is allocated for each ERP as it is enqueued.  The
 * workspace provides storage for TCD-specific data needed to manage the ERP.
 */

typedef struct erp_workspace
    {
    UINT16 curBfr;		    /* Current bfrList[] entry being processed */
    UINT16 curBfrOffset;	    /* Offset into current bfrList[] entry */

    BOOL inStatusPending;	    /* TRUE for ERP on "IN" pipe when waiting */
				    /* for packet status from hardware */

    BOOL dmaInUse;		    /* TRUE if DMA in use for ERP/endpoint */
    UINT16 dmaXfrLen;		    /* length of DMA transfer */

    } ERP_WORKSPACE, *pERP_WORKSPACE;


/* TARGET
 * 
 * TARGET is the primary data structure created to manage an individual target
 * controller.
 */

typedef struct target
    {
    MUTEX_HANDLE tcdMutex;	    /* guards TARGET data structure */
    BOOL shutdown;		    /* TRUE when target is being shut down */

    UINT32 ioBase;		    /* I/O base address */
    UINT16 irq; 		    /* IRQ channel */
    UINT16 dma; 		    /* DMA channel */

    pUSB_TARG_ENDPOINT_INFO pEndpoints; /* ptr to array of endpoints */

    USB_TCD_MNGMT_CALLBACK mngmtCallback; /* caller's management callback */
    pVOID mngmtCallbackParam;		  /* param to mngmt callback */

    THREAD_HANDLE intThread;	    /* Thread used to handle interrupts */
    SEM_HANDLE intPending;	    /* semaphore indicates int pending */
    BOOL intThreadExitRequest;	    /* TRUE when intThread should terminate */
    SEM_HANDLE intThreadExit;	    /* signalled when int thread exits */
    UINT32 intCount;		    /* number of interrupts processed */
    BOOL intInstalled;		    /* TRUE when h/w int handler installed */

    UINT16 deviceAddress;	    /* current address of target */
    BOOL enableFlag;		    /* TRUE if device enabled */

    LIST_HEAD erps [D12_NUM_ENDPOINTS]; 	 /* queued ERPs */
    ERP_WORKSPACE workspace [D12_NUM_ENDPOINTS]; /* workspace */

    UINT8 goutByte;		    /* bits set for eval GOUT reg */
    UINT8 configByte;		    /* bits set for config mode byte */
    UINT8 clkDivByte;		    /* bits set for clkdiv mode byte */
    UINT8 dmaByte;		    /* bits set for DMA config register */

    UINT16 epMainCount; 	    /* count of times endpt #2 enabled */
    UINT16 epOneAndTwoCount;	    /* count of times endpts #1 & #2 enabled */

    BOOL transPending [D12_NUM_ENDPOINTS]; /* TRUE if transStatus valid */
    UINT8 transStatus [D12_NUM_ENDPOINTS]; /* last trans status for endpoints */

    BOOL dmaInUse;		    /* TRUE if DMA in use */
    UINT16 dmaEndpointId;	    /* endpoint using DMA */
    pUSB_ERP pDmaErp;		    /* pointer to ERP currently using DMA */
    BOOL dmaEot;		    /* TRUE when EOT interrupt signalled */

    BOOL endpointNeedsService [D12_NUM_ENDPOINTS];

    } TARGET, *pTARGET;


/* forward declarations */

LOCAL VOID processErpQueue
    (
    pTARGET pTarget,
    UINT16 endpointId
    );


/* globals */

/* The following IMPORTed variables define the location and size of a 
 * low-memory buffer used by the floppy diskette driver.  This buffer
 * is guaranteed to be reachable by the i8237 DMAC (24-bit address
 * range).  In order to demonstrate the proper operation of the Philips
 * PDIUSBD12 with DMA, we use these buffers for DMA transfers.
 */

IMPORT UINT sysFdBuf;		    /* physical address of DMA bfr */
IMPORT UINT sysFdBufSize;	    /* size of DMA buffer */


/* functions */

/***************************************************************************
*
* d12SetAddress - Issues Set Address/Enable command to PDIUSBD12
*
* RETURNS: N/A
*/

LOCAL VOID d12SetAddress
    (
    pTARGET pTarget
    )

    {
    UINT8 byte;

    byte = pTarget->deviceAddress & D12_CMD_SA_ADRS_MASK;
    byte |= (pTarget->enableFlag) ? D12_CMD_SA_ENABLE : 0;

    OUT_D12_CMD (D12_CMD_SET_ADDRESS);
    OUT_D12_DATA (byte);
    }


/***************************************************************************
*
* d12SetEndpointEnable - Issues Set Endpoint Enable command to PDIUSBD12
*
* RETURNS: N/A
*/

LOCAL VOID d12SetEndpointEnable
    (
    pTARGET pTarget,
    BOOL enable
    )

    {
    OUT_D12_CMD (D12_CMD_SET_ENDPOINT_ENABLE);
    OUT_D12_DATA ((enable) ? D12_CMD_SEE_ENABLE : 0);
    }


/***************************************************************************
*
* d12SetMode - Issues Set Mode command to PDIUSBD12
*
* RETURNS: N/A
*/

LOCAL VOID d12SetMode
    (
    pTARGET pTarget
    )

    {
    OUT_D12_CMD (D12_CMD_SET_MODE);
    OUT_D12_DATA (pTarget->configByte);
    OUT_D12_DATA (pTarget->clkDivByte);
    }


/***************************************************************************
*
* d12SetDma - Issues Set DMA command to PDIUSBD12
*
* RETURNS: N/A
*/

LOCAL VOID d12SetDma
    (
    pTARGET pTarget
    )

    {
    OUT_D12_CMD (D12_CMD_SET_DMA);
    OUT_D12_DATA (pTarget->dmaByte);
    }


/***************************************************************************
*
* d12ReadIntReg - Issues Read Interrupt Register commadn to PDIUSBD12
*
* RETURNS: UINT16 value with first byte in LSB and second byte in MSB
*/

LOCAL UINT16 d12ReadIntReg
    (
    pTARGET pTarget
    )

    {
    UINT8 firstByte;

    OUT_D12_CMD (D12_CMD_READ_INTERRUPT_REG);

    firstByte = IN_D12_DATA ();
    return firstByte | (IN_D12_DATA () << 8);
    }


/***************************************************************************
*
* d12SelectEndpoint - Issues Select Endpoint command to PDIUSBD12
*
* <endpoint> parameter must be D12_ENDPOINT_xxxx.
*
* RETURNS: UINT8 value read after issuing select endpoint command
*/

LOCAL UINT8 d12SelectEndpoint
    (
    pTARGET pTarget,
    UINT16 endpoint
    )

    {
    OUT_D12_CMD (D12_CMD_SELECT_ENDPOINT | endpoint);
    return IN_D12_DATA ();
    }


/***************************************************************************
*
* d12ReadLastTransStatus - Issues Read Last Transaction Status command
*
* <endpoint> parameter must be D12_ENDPOINT_xxxx.
*
* RETURNS: UINT8 value read after issuing command
*/

LOCAL UINT8 d12ReadLastTransStatus
    (
    pTARGET pTarget,
    UINT16 endpoint
    )

    {
    OUT_D12_CMD (D12_CMD_READ_LAST_TRANS_STATUS | endpoint);
    return IN_D12_DATA ();
    }


/***************************************************************************
*
* d12ClearBfr - Issues Clear Buffer command to PDIUSBD12
*
* RETURNS: N/A
*/

LOCAL VOID d12ClearBfr
    (
    pTARGET pTarget
    )

    {
    OUT_D12_CMD (D12_CMD_CLEAR_BUFFER);
    }


/***************************************************************************
*
* d12ValidateBfr - Issues Validate Buffer command to PDIUSBD12
*
* RETURNS: N/A
*/

LOCAL VOID d12ValidateBfr
    (
    pTARGET pTarget
    )

    {
    OUT_D12_CMD (D12_CMD_VALIDATE_BUFFER);
    }


/***************************************************************************
*
* d12ReadBfr - Issues Read Buffer command and reads buffer data
*
* Reads data from the <endpoint> into the next bfrList[] entry/entries
* in the <pErp>. 
*
* NOTE: This function automatically invokes d12ClearBfr() after reading 
* buffer data.	In order to avoid losing bytes, the <pErp> passed by the
* caller should be able to accept all data in the buffer.  
*
* RETURNS: N/A
*/

LOCAL VOID d12ReadBfr
    (
    pTARGET pTarget,
    pUSB_TARG_ENDPOINT_INFO pEndpoint,
    pUSB_ERP pErp,
    pERP_WORKSPACE pWork
    )

    {
    pUSB_BFR_LIST pBfrList;
    UINT16 actLen;


    /* Select the endpoint */

    d12SelectEndpoint (pTarget, pEndpoint->endpointId);


    /* Read the number of bytes in the buffer. */

    OUT_D12_CMD (D12_CMD_READ_BUFFER);

    actLen = IN_D12_DATA ();	    /* throw away first byte */
    actLen = IN_D12_DATA ();


    /* Read buffer data */

    pBfrList = &pErp->bfrList [pWork->curBfr];

    while (actLen > 0 && pWork->curBfrOffset < pBfrList->bfrLen)
	{
	pBfrList->pBfr [pWork->curBfrOffset] = IN_D12_DATA ();
	pBfrList->actLen++;
	--actLen;

	if (++pWork->curBfrOffset == pBfrList->bfrLen)
	    {
	    pBfrList = &pErp->bfrList [++pWork->curBfr];
	    pWork->curBfrOffset = 0;
	    }

	if (pWork->curBfr == pErp->bfrCount)
	    break;
	}

    /* Issue command to flush (clear) buffer */

    d12ClearBfr (pTarget);
    }


/***************************************************************************
*
* d12WriteBfr - Issues Write Buffer command and writes buffer data
*
* <pEndpoint> must point to the USB_TARG_ENDPOINT_INFO structure for the
* endpoint. 
*
* Writes the maximum packet size number of bytes from the next ERP bfrList[]
* entry/entries to the indicated <endpoint>.
*
* NOTE: This function automatically invokes d12ValidateBfr() after
* writing data to the buffer.
*
* RETURNS: N/A
*/

LOCAL VOID d12WriteBfr
    (
    pTARGET pTarget,
    pUSB_TARG_ENDPOINT_INFO pEndpoint,
    pUSB_ERP pErp,
    pERP_WORKSPACE pWork
    )

    {
    pUSB_BFR_LIST pBfrList;
    UINT16 bfrAvail;
    UINT16 numWrite;
    UINT16 i;


    /* Select the endpoint */

    d12SelectEndpoint (pTarget, pEndpoint->endpointId);


    /* Sum the amount of data available in the bfrList[]. */

    bfrAvail = pErp->bfrList [pWork->curBfr].bfrLen - pWork->curBfrOffset;

    for (i = pWork->curBfr + 1; i < pErp->bfrCount; i++)
	bfrAvail += pErp->bfrList [i].bfrLen;

    numWrite = min (pEndpoint->maxPacketSize, bfrAvail);


    /* Issue write buffer command */

    OUT_D12_CMD (D12_CMD_WRITE_BUFFER);
    OUT_D12_DATA (0);
    OUT_D12_DATA (numWrite);


    /* Write buffer data */

    pBfrList = &pErp->bfrList [pWork->curBfr];

    for (i = 0; i < numWrite; i++)
	{
	OUT_D12_DATA (pBfrList->pBfr [pWork->curBfrOffset]);
	pBfrList->actLen++;

	if (++pWork->curBfrOffset == pBfrList->bfrLen)
	    {
	    pBfrList = &pErp->bfrList [++pWork->curBfr];
	    pWork->curBfrOffset = 0;
	    }

	if (pWork->curBfr == pErp->bfrCount)
	    break;
	}


    /* Mark buffer as filled (validate it) */

    d12ValidateBfr (pTarget);
    }


/***************************************************************************
*
* d12SetEndpointStatus - Issues Set Endpoint Status command to PDIUSBD12
*
* <endpoint> parameter must be D12_ENDPOINT_xxxx.
*
* RETURNS: N/A
*/

LOCAL VOID d12SetEndpointStatus
    (
    pTARGET pTarget,
    UINT16 endpoint,
    UINT8 status
    )

    {
    OUT_D12_CMD (D12_CMD_SET_ENDPOINT_STATUS | endpoint);
    OUT_D12_DATA (status);
    }


#if 0

/***************************************************************************
*
* d12ReadEndpointStatus - Issues Read Endpoint Status command to PDIUSBD12
*
* <endpoint> parameter must be D12_ENDPOINT_xxxx.
*
* RETURNS: UINT8 status value
*/

LOCAL UINT8 d12ReadEndpointStatus
    (
    pTARGET pTarget,
    UINT16 endpoint
    )

    {
    OUT_D12_CMD (D12_CMD_READ_ENDPOINT_STATUS | endpoint);
    return IN_D12_DATA ();
    }

#endif /* #if 0 */


/***************************************************************************
*
* d12AckSetup - Issues Acknowledge setup command to PDIUSBD12
*
* <endpoint> parameter must be D12_ENDPOINT_xxxx.
*
* RETURNS: N/A
*/

LOCAL VOID d12AckSetup
    (
    pTARGET pTarget,
    UINT16 endpoint
    )

    {
    d12SelectEndpoint (pTarget, endpoint);
    OUT_D12_CMD (D12_CMD_ACK_SETUP);
    }


/***************************************************************************
*
* d12SendResume - Issues Send Resume commadn to PDIUSBD12
*
* RETURNS: N/A
*/

LOCAL VOID d12SendResume
    (
    pTARGET pTarget
    )

    {
    OUT_D12_CMD (D12_CMD_SEND_RESUME);
    }


/***************************************************************************
*
* d12ReadCurrentFrameNumber - Issues Read Current Frame Number command
*
* RETURNS: Current frame number returned by PDIUSBD12
*/

LOCAL UINT16 d12ReadCurrentFrameNumber
    (
    pTARGET pTarget
    )

    {
    UINT8 firstByte;

    OUT_D12_CMD (D12_CMD_READ_CURRENT_FRAME_NO);

    firstByte = IN_D12_DATA ();
    return firstByte | (IN_D12_DATA () << 8);
    }


/***************************************************************************
*
* d12ReadChipId - Issues Read Chip Id command
*
* RETURNS: chip ID returned by PDIUSBD12
*/

LOCAL UINT16 d12ReadChipId
    (
    pTARGET pTarget
    )

    {
    UINT8 firstByte;

    OUT_D12_CMD (D12_CMD_READ_CHIP_ID);

    firstByte = IN_D12_DATA ();
    return firstByte | (IN_D12_DATA () << 8);
    }


/***************************************************************************
*
* finishErp - sets ERP result and invokes ERP callback
*
* NOTE: This function also releases any DMA resources used by the ERP.
*
* RETURNS: ERP result
*/

LOCAL int finishErp
    (
    pTARGET pTarget,
    pUSB_ERP pErp,
    int result
    )

    {
    /* unlink ERP */

    usbListUnlink (&pErp->tcdLink);


    /* store result */

    pErp->result = result;


    /* check/release DMA resources */

    if (pTarget->dmaInUse && pTarget->pDmaErp == pErp)
	{
	pTarget->dmaInUse = FALSE;

	/* Disable the D12's DMA and re-enable interrupts for the endpoint */

	pTarget->dmaByte &= ~D12_CMD_SD_DMA_ENABLE;

	if (pErp->endpointId == D12_ENDPOINT_2_OUT)
	    pTarget->dmaByte |= D12_CMD_SD_ENDPT_2_OUT_INTRPT;
	else
	    pTarget->dmaByte |= D12_CMD_SD_ENDPT_2_IN_INTRPT;

	d12SetDma (pTarget);
	}


    /* invoke callback */

    if (pErp->targCallback != NULL)
	(*pErp->targCallback) (pErp);

    else if (pErp->userCallback != NULL)
	(*pErp->userCallback) (pErp);

    return result;
    }


/***************************************************************************
*
* cancelErp - Attempts to cancel an ERP
*
* RETURNS: OK or S_usbTcdLib_xxxx if unable to cancel ERP
*/

LOCAL int cancelErp
    (
    pTARGET pTarget,
    pUSB_ERP pErp
    )

    {
    /* If the ERP has completed, then we can cancel it. */

    if (pErp->result != 0)
	return S_usbTcdLib_CANNOT_CANCEL;

    /* Store the ERP completion status. */

    finishErp (pTarget, pErp, S_usbTcdLib_ERP_CANCELED);

    return OK;
    }


/***************************************************************************
*
* stallEndpoint - mark an endpoint as stalled

* RETURNS: N/A
*/

LOCAL VOID stallEndpoint
    (
    pTARGET pTarget,
    UINT16 endpoint
    )

    {
    d12SetEndpointStatus (pTarget, endpoint, D12_CMD_SES_STALLED);
    }


/***************************************************************************
*
* checkDma - Check if an ERP/endpoint is serviced by DMA and update accordingly
*
* RETURNS: TRUE if ERP was handled, else FALSE if ERP requires non-DMA processing
*/

LOCAL BOOL checkDma
    (
    pTARGET pTarget,
    pUSB_TARG_ENDPOINT_INFO pEndpoint,
    pUSB_ERP pErp,
    pERP_WORKSPACE pWork
    )

    {
#ifndef D12_USE_DMA

    return FALSE;

#else /* #ifndef D12_USE_DMA */

    UINT16 direction;

    /* Determine transfer direction based on endpoint in use */

    direction = (pEndpoint->endpointId == D12_ENDPOINT_2_OUT) ?
	DMA_MEM_WRITE : DMA_MEM_READ;


    /* If this ERP/endpoint is using DMA, check for completion of the
     * ERP operation.
     */

    if (pWork->dmaInUse && pTarget->dmaEot)
	{
	/* If this is a write to memory, copy from the temporary buffer. */

	if (direction == DMA_MEM_WRITE)
	    {
	    USB_ISA_MEM_INVALIDATE ((char *) sysFdBuf, pWork->dmaXfrLen);

	    memcpy (&pErp->bfrList [pWork->curBfr].pBfr [pWork->curBfrOffset],
		(char *) sysFdBuf, pWork->dmaXfrLen);
	    }

	pWork->curBfrOffset += pWork->dmaXfrLen;

	/* Update the buffer counter. */

	pErp->bfrList [pWork->curBfr].actLen += pWork->dmaXfrLen;

	if (pWork->curBfrOffset == pErp->bfrList [pWork->curBfr].bfrLen)
	    {
	    pWork->curBfr++;
	    pWork->curBfrOffset = 0;
	    }

	if (pWork->curBfr == pErp->bfrCount)
	    {
	    /* Mark the ERP as complete. */

	    finishErp (pTarget, pErp, OK);

	    /* Automatically start the next ERP for this endpoint. */

	    processErpQueue (pTarget, pEndpoint->endpointId);

	    return TRUE;
	    }
	}


    /* If DMA has already been started for this ERP/endpoint, then continue 
     * only if we've detected a DMA end-of-transfer 
     */

    if (pWork->dmaInUse && !pTarget->dmaEot)
	return TRUE;


    /* If this ERP/endpoint should use DMA, then initialize the dma. */

    if (pTarget->dma != 0 &&
	(pEndpoint->endpointId == D12_ENDPOINT_2_OUT ||
	 pEndpoint->endpointId == D12_ENDPOINT_2_IN) &&
	(!pTarget->dmaInUse || 
	 pTarget->dmaEndpointId == pEndpoint->endpointId) &&
	(pTarget->pDmaErp == NULL || pTarget->pDmaErp == pErp))
	{
	/* Initialize DMA controller */

	pWork->dmaXfrLen = 
	    min (pErp->bfrList [pWork->curBfr].bfrLen - 
	    pErp->bfrList [pWork->curBfr].actLen, sysFdBufSize);

	if (direction == DMA_MEM_READ)
	    {
	    memcpy ((char *) sysFdBuf, 
		&pErp->bfrList [pWork->curBfr].pBfr [pWork->curBfrOffset],
		pWork->dmaXfrLen);

	    USB_ISA_MEM_FLUSH ((char *) sysFdBuf, pWork->dmaXfrLen);
	    }
		
	USB_ISA_DMA_SETUP (direction, sysFdBuf, pWork->dmaXfrLen, 
	    pTarget->dma);


	/* Initialize D12 for DMA operation */

	pTarget->dmaByte &= ~D12_CMD_SD_DMA_DIRECTION_MASK;

	if (pEndpoint->endpointId == D12_ENDPOINT_2_OUT)
	    {
	    pTarget->dmaByte &= ~D12_CMD_SD_ENDPT_2_OUT_INTRPT;
	    pTarget->dmaByte |= D12_CMD_SD_DMA_DIRECTION_READ;
	    }
	else
	    {
	    pTarget->dmaByte &= ~D12_CMD_SD_ENDPT_2_IN_INTRPT;
	    pTarget->dmaByte |= D12_CMD_SD_DMA_DIRECTION_WRITE;
	    }

	pTarget->dmaByte |= D12_CMD_SD_DMA_ENABLE;

	d12SetDma (pTarget);


	/* Update state variables */

	pWork->dmaInUse = TRUE;
	
	pTarget->dmaInUse = TRUE;
	pTarget->dmaEndpointId = pEndpoint->endpointId;
	pTarget->pDmaErp = pErp;
	pTarget->dmaEot = FALSE;


	/* throw away any pending transaction status for this endpoint. */

	pTarget->transPending [pEndpoint->endpointId] = FALSE;

	return TRUE;
	}

    return FALSE;

#endif /* ifndef D12_USE_DMA */
    }


/***************************************************************************
*
* processInEndpoint - update an ERP for an IN endpoint
*
* Tries to put data from an ERP to an endpoint.  If successful, and if the
* ERP is complete, then stores ERP status.
*
* RETURNS: N/A
*/

LOCAL VOID processInEndpoint
    (
    pTARGET pTarget,
    pUSB_TARG_ENDPOINT_INFO pEndpoint,
    pUSB_ERP pErp,
    pERP_WORKSPACE pWork
    )

    {
    UINT8 tStatus;


    /* Check if this ERP/endpoint is handled via DMA */

    if (checkDma (pTarget, pEndpoint, pErp, pWork))
	return;


    /* If this ERP is awaiting status, check the status.  If the ERP
     * is not awaiting status, then the status must be stale, so
     * discard it. 
     */

    if (pTarget->transPending [pEndpoint->endpointId])
	{
	/* "consume" the last status read for this endpoint */

	tStatus = pTarget->transStatus [pEndpoint->endpointId];
	pTarget->transPending [pEndpoint->endpointId] = FALSE;

	if (pWork->inStatusPending)
	    {
	    /* If the last transaction failed, fail the ERP. */

#ifdef D12_CHECK_DATA_SUCCESS_BIT

	    if ((tStatus & D12_CMD_RLTS_DATA_SUCCESS) == 0)
		{
		finishErp (pTarget, pErp, S_usbTcdLib_COMM_FAULT);
		return;
		}

#endif /* #ifdef D12_CHECK_DATA_SUCCESS_BIT */

	    pWork->inStatusPending = FALSE;

	    /* Is the ERP complete? */

	    if (pWork->curBfr == pErp->bfrCount ||
		(pErp->bfrList [0].pBfr == NULL && pErp->bfrList [0].bfrLen == 0))
		{
		finishErp (pTarget, pErp, OK);
		return;
		}
	    }
	}


    /* If the endpoint can accept data, put data into it now. */

    if ((d12SelectEndpoint (pTarget, pErp->endpointId) &
	D12_CMD_SE_FULL_EMPTY) != 0)
	return;

    d12WriteBfr (pTarget, pEndpoint, pErp, pWork);

    pWork->inStatusPending = TRUE;
    }


/***************************************************************************
*
* processOutEndpoint - update an ERP for an OUT endpoint
*
* Validates data received by the device on an OUT endpoint and puts the
* data into the indicated ERP.	If the ERP completes (successfully or with
* an error), this function sets the ERP status.
*
* RETURNS: N/A
*/

LOCAL VOID processOutEndpoint
    (
    pTARGET pTarget,
    pUSB_TARG_ENDPOINT_INFO pEndpoint,
    pUSB_ERP pErp,
    pERP_WORKSPACE pWork
    )

    {
    UINT8 tStatus;


    /* Check if this ERP/endpoint is handled via DMA */

    if (checkDma (pTarget, pEndpoint, pErp, pWork))
	return;


    /* Is there a pending transaction for this endpoint? */

    if (!pTarget->transPending [pEndpoint->endpointId])
	return;


    /* Pick up the last status read for this endpoint */

    tStatus = pTarget->transStatus [pEndpoint->endpointId];


    /* Check the incoming data against the expected PID. */

    if (pErp->transferType == USB_XFRTYPE_CONTROL)
	{
	if (((tStatus & D12_CMD_RLTS_SETUP_PACKET) != 0 &&
	    pErp->bfrList [0].pid != USB_PID_SETUP) ||

	    ((tStatus & D12_CMD_RLTS_SETUP_PACKET) == 0 && 
	    pErp->bfrList [0].pid != USB_PID_OUT))
	    {
	    /* PID mismatch */

	    stallEndpoint (pTarget, pEndpoint->endpointId);
	    finishErp (pTarget, pErp, S_usbTcdLib_PID_MISMATCH);
	    return;
	    }
	}
    

    /* Now that we're sure we're processing the expected kind of packet,
     * "consume" the endpoint status.
     */

    pTarget->transPending [pEndpoint->endpointId] = FALSE;


    /* If this was a setup packet, let the D12 know we acknowledge it. */

    if ((tStatus & D12_CMD_RLTS_SETUP_PACKET) != 0)
	{
	d12AckSetup (pTarget, D12_ENDPOINT_CONTROL_IN);
	d12AckSetup (pTarget, D12_ENDPOINT_CONTROL_OUT);
	}


    /* Was the last transaction successful?  If not, fail the ERP. */

#ifdef D12_CHECK_DATA_SUCCESS_BIT

    if ((tStatus & D12_CMD_RLTS_DATA_SUCCESS) == 0)
	{
	finishErp (pTarget, pErp, S_usbTcdLib_COMM_FAULT);
	return;
	}

#endif /* #ifdef D12_CHECK_DATA_SUCCESS_BIT */


    /* Check the DATA0/DATA1 flag against the expected flag */

    if (((tStatus & D12_CMD_RLTS_DATA1) == 0) != (pErp->dataToggle == USB_DATA0))
	{
	/* Data toggle mismatch */

	stallEndpoint (pTarget, pEndpoint->endpointId);
	finishErp (pTarget, pErp, S_usbTcdLib_DATA_TOGGLE_FAULT);
	return;
	}

    pErp->dataToggle = (pErp->dataToggle == USB_DATA0) ? USB_DATA1 : USB_DATA0;


    /* Read data into the ERP. */

    d12ReadBfr (pTarget, pEndpoint, pErp, pWork);


    /* Is the ERP complete? */

    if (pWork->curBfr == pErp->bfrCount ||
	(pErp->bfrList [0].pBfr == NULL && pErp->bfrList [0].bfrLen == 0))
	{
	finishErp (pTarget, pErp, OK);
	}
    }


/***************************************************************************
*
* processErpQueue - See if we can update the queue for an endpoint
*
* This function examines the ERP queue for the specified <endpointId>.
* If the queue can be updated - that is, if data can be transferred to/
* from the queue or an error condition can be logged - then we update
* the queue.
*
* RETURNS: N/A
*/

LOCAL VOID processErpQueue
    (
    pTARGET pTarget,
    UINT16 endpointId
    )

    {
    pUSB_TARG_ENDPOINT_INFO pEndpoint = &pTarget->pEndpoints [endpointId];
    pUSB_ERP pErp;
    pERP_WORKSPACE pWork;
    

    /* Process the next ERP */

    if ((pErp = usbListFirst (&pTarget->erps [endpointId])) != NULL)
	{
	/* Get ERP_WORKSPACE for this ERP/endpoint. */

	pWork = &pTarget->workspace [endpointId];
    
	if (!((BOOL) pErp->tcdPtr))
	    {
	    pErp->tcdPtr = (pVOID) TRUE;
	    memset (pWork, 0, sizeof (*pWork));
	    }

	/* Fan-out based on OUT or IN endpoint. */

	switch (pErp->endpointId)
	    {
	    case D12_ENDPOINT_CONTROL_OUT:
	    case D12_ENDPOINT_1_OUT:
	    case D12_ENDPOINT_2_OUT:

		processOutEndpoint (pTarget, pEndpoint, pErp, pWork);
		break;

	    case D12_ENDPOINT_CONTROL_IN:
	    case D12_ENDPOINT_1_IN:
	    case D12_ENDPOINT_2_IN:

		processInEndpoint (pTarget, pEndpoint, pErp, pWork);
		break;
	    }
	}
    else
	{
	/* There is an interrupt pending, but the ERP queue is empty.  If
	 * this is the default control OUT pipe, and if the newly received
	 * packet is a setup packet, then we need to abort any pending
	 * transfers on the default control IN pipe.
	 */

	if (endpointId == D12_ENDPOINT_CONTROL_OUT &&
	    pTarget->transPending [endpointId] &&
	    (pTarget->transStatus [endpointId] & D12_CMD_RLTS_SETUP_PACKET) != 0 &&
	    (pErp = usbListFirst (&pTarget->erps [D12_ENDPOINT_CONTROL_IN])) != NULL)
	    {
	    finishErp (pTarget, pErp, S_usbTcdLib_NEW_SETUP_PACKET);
	    }
	}
    }


/***************************************************************************
*
* processErpQueueInt - Reads interrupt status and invokes processErpQueue()
*
* RETURNS: N/A
*/

LOCAL VOID processErpQueueInt
    (
    pTARGET pTarget,
    UINT16 endpointId
    )

    {
    /* Select the endpoint and read the last transaction status.
     *
     * NOTE: Reading the transaction status clears the interrupt condition
     * associated with a pipe. 
     */

    pTarget->transStatus [endpointId] = 
	d12ReadLastTransStatus (pTarget, endpointId);

    pTarget->transPending [endpointId] = TRUE;

    processErpQueue (pTarget, endpointId);
    }


/***************************************************************************
*
* processBusReset - Processes a bus reset event
*
* RETURNS: N/A
*/

LOCAL VOID processBusReset
    (
    pTARGET pTarget
    )

    {
    UINT8 ginByte = IN_EVAL_GIN ();


    /* The D12 has reported a bus reset. */

    pTarget->deviceAddress = 0;     /* reverts to power-ON default */


    /* Report the bus reset to our caller. */

    MNGMT_CALLBACK (pTarget, TCD_MNGMT_BUS_RESET);


    /* Follow-up by determining if Vbus is present or not */

    if ((ginByte & D12EVAL_BUS_POWER) == 0)
	MNGMT_CALLBACK (pTarget, TCD_MNGMT_VBUS_LOST);
    else
	MNGMT_CALLBACK (pTarget, TCD_MNGMT_VBUS_DETECT);
    }


/***************************************************************************
*
* processSuspendChange - Interprets a change in the bus SUSPEND state
*
* RETURNS: N/A
*/

LOCAL VOID processSuspendChange
    (
    pTARGET pTarget
    )

    {
    UINT8 ginByte = IN_EVAL_GIN ();


    /* The D12 has reported a change in the suspend state.  Reflect the
     * current suspend state to the caller.
     */

    if ((ginByte & D12EVAL_SUSPEND) == 0)
	MNGMT_CALLBACK (pTarget, TCD_MNGMT_RESUME);
    else
	MNGMT_CALLBACK (pTarget, TCD_MNGMT_SUSPEND);
    }


/***************************************************************************
*
* processDmaEot - Handles a DMA end-of-transfer
*
* RETURNS: N/A
*/

LOCAL VOID processDmaEot
    (
    pTARGET pTarget
    )

    {
    /* The D12 reports that a DMA operation has completed. */

    if (pTarget->dmaInUse)
	{
	pTarget->dmaEot = TRUE;
	processErpQueue (pTarget, pTarget->dmaEndpointId);
	}
    }


/***************************************************************************
*
* processD12Int - evaluate and process an interrupt from the PDIUSBD12
*
* RETURNS: N/A
*/

LOCAL VOID processD12Int
    (
    pTARGET pTarget
    )

    {
    UINT16 intStatus;


    /* Read interrupt status.
     *
     * NOTE: Reading the interrupt status clears interrupt conditions
     * which are not associated with a specific pipe.
     */

    intStatus = d12ReadIntReg (pTarget);


    /* Examine interrupt conditions.
     *
     * NOTE: Examine control endpoint last...at the end of a transaction,
     * it is possible for a new setup packet to come in before we've
     * handled the interrupt associated with the just-ended transaction.
     */

    if ((intStatus & D12_CMD_RIR_ENDPOINT_2_IN) != 0)
	processErpQueueInt (pTarget, D12_ENDPOINT_2_IN);

    if ((intStatus & D12_CMD_RIR_ENDPOINT_2_OUT) != 0)
	processErpQueueInt (pTarget, D12_ENDPOINT_2_OUT);

    if ((intStatus & D12_CMD_RIR_ENDPOINT_1_IN) != 0)
	processErpQueueInt (pTarget, D12_ENDPOINT_1_IN);

    if ((intStatus & D12_CMD_RIR_ENDPOINT_1_OUT) != 0)
	processErpQueueInt (pTarget, D12_ENDPOINT_1_OUT);

    if ((intStatus & D12_CMD_RIR_CONTROL_IN) != 0)
	processErpQueueInt (pTarget, D12_ENDPOINT_CONTROL_IN);

    if ((intStatus & D12_CMD_RIR_CONTROL_OUT) != 0)
	processErpQueueInt (pTarget, D12_ENDPOINT_CONTROL_OUT);

    if ((intStatus & D12_CMD_RIR_BUS_RESET) != 0)
	processBusReset (pTarget);

    if ((intStatus & D12_CMD_RIR_SUSPEND) != 0)
	processSuspendChange (pTarget);

    if ((intStatus & D12_CMD_RIR_DMA_EOT) != 0)
	processDmaEot (pTarget);
    }


/***************************************************************************
*
* usbTcdPdiusbd12IntThread - Handles PDIUSBD12 interrupts
*
* This thread normally waits on the TARGET.intPending semaphore.  When
* the semaphore is signalled, this thread interrogates the target controller
* to determine the cause of the interrupt and serives it.
*
* By convention, the <param> to this thread is a pointer to the TARGET
* structure for this target controller.
*
* When this thread exits, it signals the TARGET.intThreadExit semaphore
* so the foreground thread can no that the thread has terminated successully.
*
* RETURNS: N/A
*/

LOCAL VOID usbTcdPdiusbd12IntThread
    (
    pVOID param
    )

    {
    pTARGET pTarget = (pTARGET) param;
    UINT16 i;

    do
	{
	/* Wait for an interrupt to be signalled. */

	if (OSS_SEM_TAKE (pTarget->intPending, OSS_BLOCK) == OK)
	    {
	    if (!pTarget->intThreadExitRequest)
		{
		OSS_MUTEX_TAKE (pTarget->tcdMutex, OSS_BLOCK);

		/* Light LED to indicate we're processing an interrupt */

		pTarget->goutByte |= INT_THREAD_LED;
		OUT_EVAL_GOUT (pTarget->goutByte);


		/* Process the interrupt */

		processD12Int (pTarget);


		/* See if any queued ERPs need processing. */

		for (i = 0; i < D12_NUM_ENDPOINTS; i++)
		    {
		    if (pTarget->endpointNeedsService [i])
			{
			pTarget->endpointNeedsService [i] = FALSE;
			processErpQueue (pTarget, i);
			}
		    }


		/* Re-enable interrupts. */

		pTarget->goutByte &= ~INT_THREAD_LED;
		pTarget->goutByte |= D12EVAL_GOUT_INTENB;
		OUT_EVAL_GOUT (pTarget->goutByte);

		OSS_MUTEX_RELEASE (pTarget->tcdMutex);
		}
	    }
	}
    while (!pTarget->intThreadExitRequest);

    OSS_SEM_GIVE (pTarget->intThreadExit);
    }


/***************************************************************************
*
* usbTcdPdiusbd12IntHandler - hardware interrupt handler
*
* This is the actual routine which receives hardware interrupts from the
* target controller.  This routine immediately reflects the interrupt to 
* the usbTcdPdiusbd12IntThread.  Interrupt handlers have execution 
* restrictions which are not imposed on normal threads...So, this scheme 
* gives the usbTcdPdiusbd12IntThread complete flexibility to call other 
* services and libraries while processing the interrupt condition.
*
* RETURNS: N/A
*/

LOCAL VOID usbTcdPdiusbd12IntHandler
    (
    pVOID param
    )

    {
    pTARGET pTarget = (pTARGET) param;


    /* Is there an interrupt pending in the UHCI status reg? 
     *
     * NOTE: INT_N is an active low signal.  The interrupt is asserted
     * when INT_N == 0.
     */

    if ((IN_EVAL_GIN () & D12EVAL_INTN) == 0)
	{
	pTarget->intCount++;

	/* A USB interrupt is pending. Disable interrupts until the
	 * interrupt thread can process it. */

	pTarget->goutByte &= ~D12EVAL_GOUT_INTENB;
	OUT_EVAL_GOUT (pTarget->goutByte);

	/* Signal the interrupt thread to process the interrupt. */

	OSS_SEM_GIVE (pTarget->intPending);
	}
    }


/***************************************************************************
*
* cancelAllErps - Cancels all outstanding ERPs
*
* RETURNS: N/A
*/

LOCAL VOID cancelAllErps
    (
    pTARGET pTarget
    )

    {
    UINT16 i;
    pUSB_ERP pErp;


    /* Cancel all pending ERPs */

    for (i = 0; i < D12_NUM_ENDPOINTS; i++)
	{
	while ((pErp = usbListFirst (&pTarget->erps [i])) != NULL)
	    {
	    finishErp (pTarget, pErp, S_usbTcdLib_ERP_CANCELED);
	    }
	}
    }


/***************************************************************************
*
* doDisable - Disable the target controller
*
* Disables the target controller so it won't appear to be connected to
* the USB.
*
* RETURNS: N/A
*/

LOCAL VOID doDisable
    (
    pTARGET pTarget
    )

    {
    /* Cancel all outstanding ERPs. */

    cancelAllErps (pTarget);

    /* Disable controller */

    pTarget->configByte &= ~D12_CMD_SM_CFG_SOFTCONNECT;
    d12SetMode (pTarget);

    pTarget->enableFlag = DISABLE;
    d12SetAddress (pTarget);

    pTarget->goutByte &= ~ENABLE_LED;
    OUT_EVAL_GOUT (pTarget->goutByte);
    }


/***************************************************************************
*
* destroyTarget - tears down a TARGET structure
*
* RETURNS: N/A
*/

LOCAL VOID destroyTarget
    (
    pTARGET pTarget
    )

    {
    if (pTarget)
	{
	/* Cancel all outstanding ERPs. */

	cancelAllErps (pTarget);


	/* Disable interrupts, turn off LEDs */

	if (pTarget->ioBase != 0)
	    OUT_EVAL_GOUT (0);
    
	if (pTarget->intInstalled)
	    USB_ISA_INT_RESTORE (usbTcdPdiusbd12IntHandler, pTarget, 
		pTarget->irq);

	/* Shut down interrupt handler thread */

	if (pTarget->intThread != NULL)
	    {
	    pTarget->intThreadExitRequest = TRUE;
	    OSS_SEM_GIVE (pTarget->intPending);
	    OSS_SEM_TAKE (pTarget->intThreadExit, INT_THREAD_EXIT_TIMEOUT);
	    OSS_THREAD_DESTROY (pTarget->intThread);
	    }

	if (pTarget->intPending != NULL)
	    OSS_SEM_DESTROY (pTarget->intPending);

	if (pTarget->intThreadExit != NULL)
	    OSS_SEM_DESTROY (pTarget->intThreadExit);

	/* Release target memory */

	if (pTarget->pEndpoints != NULL)
	    OSS_FREE (pTarget->pEndpoints);

	OSS_FREE (pTarget);
	}
    }


/***************************************************************************
*
* fncAttach - Executes TCD_FNC_ATTACH
*
* This function initializes the target controller for operation.
*
* RETURNS: OK or S_usbTcdLib_xxxx if an error is detected
*/

LOCAL int fncAttach
    (
    pTRB_ATTACH pTrb
    )

    {
    pUSB_TCD_PDIUSBD12_PARAMS pParams;
    pTARGET pTarget;
    pUSB_TARG_ENDPOINT_INFO pEndpoint;
    UINT16 i;


    /* Validate parameters */

    if ((pParams = (pUSB_TCD_PDIUSBD12_PARAMS) pTrb->tcdParam) == NULL ||
	pTrb->mngmtCallback == NULL)
	return S_usbTcdLib_BAD_PARAM;


    /* Create a TARGET structure to manage this target controller. */

    if ((pTarget = OSS_CALLOC (sizeof (*pTarget))) == NULL ||

	(pTarget->pEndpoints = OSS_CALLOC (sizeof (USB_TARG_ENDPOINT_INFO) *
	    D12_NUM_ENDPOINTS)) == NULL ||

	OSS_MUTEX_CREATE (&pTarget->tcdMutex) != OK ||

	OSS_SEM_CREATE (1, 0, &pTarget->intPending) != OK ||
	OSS_SEM_CREATE (1, 0, &pTarget->intThreadExit) != OK ||
	OSS_THREAD_CREATE (usbTcdPdiusbd12IntThread, pTarget, 
	    OSS_PRIORITY_INTERRUPT, INT_THREAD_NAME, &pTarget->intThread) != OK)
	{
	destroyTarget (pTarget);
	return S_usbTcdLib_OUT_OF_RESOURCES;
	}

    pTarget->ioBase = pParams->ioBase;
    pTarget->irq = pParams->irq;
    pTarget->dma = pParams->dma;

    pTarget->mngmtCallback = pTrb->mngmtCallback;
    pTarget->mngmtCallbackParam = pTrb->mngmtCallbackParam;


    /* Initialize endpoint information */

    /* default control OUT endpoint */

    pEndpoint = &pTarget->pEndpoints [D12_ENDPOINT_CONTROL_OUT];

    pEndpoint->endpointId = D12_ENDPOINT_CONTROL_OUT;
    pEndpoint->flags = TCD_ENDPOINT_OUT_OK | TCD_ENDPOINT_CTRL_OK;
    pEndpoint->endpointNumMask = 0x0001;    /* endpoint #0 only */
    pEndpoint->ctlMaxPacketSize = D12_MAX_PKT_CONTROL;

    /* default control IN endpoint */

    pEndpoint = &pTarget->pEndpoints [D12_ENDPOINT_CONTROL_IN];

    pEndpoint->endpointId = D12_ENDPOINT_CONTROL_IN;
    pEndpoint->flags = TCD_ENDPOINT_IN_OK | TCD_ENDPOINT_CTRL_OK;
    pEndpoint->endpointNumMask = 0x0001;    /* endpoint #0 only */
    pEndpoint->ctlMaxPacketSize = D12_MAX_PKT_CONTROL;

    /* bulk or interrupt OUT endpoint */

    pEndpoint = &pTarget->pEndpoints [D12_ENDPOINT_1_OUT];

    pEndpoint->endpointId = D12_ENDPOINT_1_OUT;
    pEndpoint->flags = TCD_ENDPOINT_OUT_OK | TCD_ENDPOINT_BULK_OK |
	TCD_ENDPOINT_INT_OK;
    pEndpoint->endpointNumMask = 0x0002;    /* endpoint #1 only */
    pEndpoint->bulkOutMaxPacketSize   = D12_MAX_PKT_ENDPOINT_1;
    pEndpoint->bulkInOutMaxPacketSize = D12_MAX_PKT_ENDPOINT_1;
    pEndpoint->intOutMaxPacketSize    = D12_MAX_PKT_ENDPOINT_1;
    pEndpoint->intInOutMaxPacketSize  = D12_MAX_PKT_ENDPOINT_1;

    /* bulk or interrupt IN endpoint */

    pEndpoint = &pTarget->pEndpoints [D12_ENDPOINT_1_IN];

    pEndpoint->endpointId = D12_ENDPOINT_1_IN;
    pEndpoint->flags = TCD_ENDPOINT_IN_OK | TCD_ENDPOINT_BULK_OK |
	TCD_ENDPOINT_INT_OK;
    pEndpoint->endpointNumMask = 0x0002;    /* endpoint #1 only */
    pEndpoint->bulkInMaxPacketSize    = D12_MAX_PKT_ENDPOINT_1;
    pEndpoint->bulkInOutMaxPacketSize = D12_MAX_PKT_ENDPOINT_1;
    pEndpoint->intInMaxPacketSize     = D12_MAX_PKT_ENDPOINT_1;
    pEndpoint->intInOutMaxPacketSize  = D12_MAX_PKT_ENDPOINT_1;

    /* main OUT endpoint: bulk, interrupt, or isochronous */

    pEndpoint = &pTarget->pEndpoints [D12_ENDPOINT_2_OUT];

    pEndpoint->endpointId = D12_ENDPOINT_2_OUT;
    pEndpoint->flags = TCD_ENDPOINT_OUT_OK | TCD_ENDPOINT_BULK_OK |
	TCD_ENDPOINT_INT_OK | TCD_ENDPOINT_ISOCH_OK;
    pEndpoint->bulkOutMaxPacketSize    = D12_MAX_PKT_ENDPOINT_2_NON_ISO;
    pEndpoint->bulkInOutMaxPacketSize  = D12_MAX_PKT_ENDPOINT_2_NON_ISO;
    pEndpoint->intOutMaxPacketSize     = D12_MAX_PKT_ENDPOINT_2_NON_ISO;
    pEndpoint->intInOutMaxPacketSize   = D12_MAX_PKT_ENDPOINT_2_NON_ISO;
    pEndpoint->isochOutMaxPacketSize   = D12_MAX_PKT_ENDPOINT_2_ISO_OUT;
    pEndpoint->isochInOutMaxPacketSize = D12_MAX_PKT_ENDPOINT_2_ISO_IO;

    /* main IN endpoint: bulk, interrupt, or isochronous */

    pEndpoint = &pTarget->pEndpoints [D12_ENDPOINT_2_IN];

    pEndpoint->endpointId = D12_ENDPOINT_2_IN;
    pEndpoint->flags = TCD_ENDPOINT_IN_OK | TCD_ENDPOINT_BULK_OK |
	TCD_ENDPOINT_INT_OK | TCD_ENDPOINT_ISOCH_OK;
    pEndpoint->bulkInMaxPacketSize     = D12_MAX_PKT_ENDPOINT_2_NON_ISO;
    pEndpoint->bulkInOutMaxPacketSize  = D12_MAX_PKT_ENDPOINT_2_NON_ISO;
    pEndpoint->intInMaxPacketSize      = D12_MAX_PKT_ENDPOINT_2_NON_ISO;
    pEndpoint->intInOutMaxPacketSize   = D12_MAX_PKT_ENDPOINT_2_NON_ISO;
    pEndpoint->isochInMaxPacketSize    = D12_MAX_PKT_ENDPOINT_2_ISO_IN;
    pEndpoint->isochInOutMaxPacketSize = D12_MAX_PKT_ENDPOINT_2_ISO_IO;


    /* Make sure the hardware is present */

    if ((d12ReadChipId (pTarget) & D12_CHIP_ID_MASK) != D12_CHIP_ID)
	{
	pTarget->ioBase = 0;
	destroyTarget (pTarget);
	return S_usbTcdLib_HW_NOT_READY;
	}


    /* Initialize hardware */

    /* disable eval board and address decoding */

    OUT_EVAL_GOUT (0);				/* turn OFF LEDs & interrupts */

    pTarget->deviceAddress = 0;
    pTarget->enableFlag = DISABLE;
    d12SetAddress (pTarget);			/* disable device */

    d12SetEndpointEnable (pTarget, DISABLE);	/* disable endpoints 1 & 2 */


    /* Clear pending status. */

    for (i = 0; i < D12_NUM_ENDPOINTS; i++)
	{
	d12SelectEndpoint (pTarget, i);
	d12ClearBfr (pTarget);
	d12ReadLastTransStatus (pTarget, i);
	d12ReadLastTransStatus (pTarget, i);
	}

    d12ReadIntReg (pTarget);


    /* Hook interrupts */

    if (USB_ISA_INT_CONNECT (usbTcdPdiusbd12IntHandler, pTarget, pTarget->irq) 
	!= OK)
	{
	destroyTarget (pTarget);
	return S_usbTcdLib_INT_HOOK_FAILED;
	}

    pTarget->intInstalled = TRUE;


    /* continue initializing hardware */

    /* set basic operating mode
     *
     * NOTE: Setting the "clock running" bit keeps the chip alive even during
     * suspend in order to facilitate debugging.  In a production environment
     * where power device power consumption may be an issue, this bit should
     * not be set.
     */

    pTarget->configByte = D12_CMD_SM_CFG_NO_LAZYCLOCK |
	D12_CMD_SM_CFG_CLOCK_RUNNING | D12_CMD_SM_CFG_MODE0_NON_ISO;

    pTarget->clkDivByte = D12_CMD_SM_CLK_DIV_DEFAULT | 
	D12_CMD_SM_CLK_SET_TO_ONE;

    d12SetMode (pTarget);


    /* Set default DMA mode 
     *
     * NOTE: Originally when writing this code I set the PDIUSBD12 for
     * single-cycle DMA mode.  However, I noticed that the D12 would stop 
     * asserting DRQ mid-transfer.  In examining the Philips evaluation code,
     * I noticed that they only use "burst 16" DMA mode, and that appears
     * to work correct.  -rcb
     */

    pTarget->dmaByte = D12_CMD_SD_DMA_BURST_16 | 
	D12_CMD_SD_ENDPT_2_OUT_INTRPT | D12_CMD_SD_ENDPT_2_IN_INTRPT;

    d12SetDma (pTarget);


    /* The following command enables interrupts.  Since we're using an evaluation
     * board with some debug LEDs, we also turn on an "LED" to indicate that the
     * board is configured.
     */

    pTarget->goutByte = ATTACH_LED | D12EVAL_GOUT_INTENB;
    OUT_EVAL_GOUT (pTarget->goutByte);


    /* Return target information to caller */

    pTrb->header.handle = (TCD_HANDLE) pTarget;
    pTrb->speed = USB_SPEED_FULL;
    pTrb->numEndpoints = D12_NUM_ENDPOINTS;
    pTrb->pEndpoints = pTarget->pEndpoints;

    return OK;
    }


/***************************************************************************
*
* fncDetach - Executes TCD_FNC_DETACH
*
* This function shuts down a target controller previously enabled by
* calling fncAttach().
*
* RETURNS: OK or S_usbTcdLib_xxxx if an error is detected
*/

LOCAL int fncDetach
    (
    pTRB_DETACH pTrb,
    pTARGET pTarget
    )

    {
    destroyTarget (pTarget);
    return OK;
    }


/***************************************************************************
*
* fncEnable - Enables a target channel
*
* After a TCD has been attached/initialized, it still needs to be enabled
* before the target controller will appear as a device on the USB. 
* Separating the enabling of the device into a separate step allows 
* additional device initialization to be performed before the device is
* required to respond to requests from the host.
*
* RETURNS: OK or S_usbTcdLib_xxxx if an error is detected
*/

LOCAL int fncEnable
    (
    pTRB_ENABLE_DISABLE pTrb,
    pTARGET pTarget
    )

    {
    pTarget->enableFlag = ENABLE;
    d12SetAddress (pTarget);

    pTarget->configByte |= D12_CMD_SM_CFG_SOFTCONNECT;
    d12SetMode (pTarget);

    pTarget->goutByte |= ENABLE_LED;
    OUT_EVAL_GOUT (pTarget->goutByte);

    return OK;
    }


/***************************************************************************
*
* fncDisable - Disables a target channel
*
* This function disables the target controller from responding on the
* USB as a device.  
*
* RETURNS: OK or S_usbTcdLib_xxxx if an error is detected
*/

LOCAL int fncDisable
    (
    pTRB_ENABLE_DISABLE pTrb,
    pTARGET pTarget
    )

    {
    doDisable (pTarget);
    return OK;
    }


/***************************************************************************
*
* fncAddressSet - Sets the device (target) address
*
* After fncAttach() the device defaults to USB address 0.  This function
* is generally invoked in response to a host request to assign a different
* address to this device.
*
* RETURNS: OK or S_usbTcdLib_xxxx if an error is detected
*/

LOCAL int fncAddressSet
    (
    pTRB_ADDRESS_SET pTrb,
    pTARGET pTarget
    )

    {
    pTarget->deviceAddress = pTrb->deviceAddress;
    d12SetAddress (pTarget);
    return OK;
    }


/***************************************************************************
*
* pickPacketSize - chooses packets size based on direction
*
* RETURNS: selected packet size
*/

LOCAL UINT16 pickPacketSize
    (
    UINT16 direction,
    UINT16 inMaxPacketSize,
    UINT16 outMaxPacketSize,
    UINT16 inOutMaxPacketSize
    )

    {
    switch (direction)
	{
	case USB_DIR_IN:    return inMaxPacketSize;
	case USB_DIR_OUT:   return outMaxPacketSize;
	case USB_DIR_INOUT: return inOutMaxPacketSize;
	}

    return 0;
    }		    


/***************************************************************************
*
* fncEndpointAssign - Assigns an endpoint for a specific kind of transfer
*
* This function is called when the usbTargLib or the target application
* wants to create a new pipe.  This function enables the endpoint to handle
* the type of data transfer indicated by the caller.
*
* RETURNS: OK or S_usbTcdLib_xxxx if an error is detected
*/

LOCAL int fncEndpointAssign
    (
    pTRB_ENDPOINT_ASSIGN pTrb,
    pTARGET pTarget
    )

    {
    pUSB_TARG_ENDPOINT_INFO pEndpoint;


    /* NOTE: By convention, usbTargLib guarantees that the parameters in the
     * TRB are valid for this endpoint prior to calling this function.
     */

    pEndpoint = &pTarget->pEndpoints [pTrb->endpointId];

    pEndpoint->endpointNum = pTrb->endpointNum;
    pEndpoint->configuration = pTrb->configuration;
    pEndpoint->interface = pTrb->interface;
    pEndpoint->transferType = pTrb->transferType;
    pEndpoint->direction = pTrb->direction;


    /* Determine the maxPacketSize based on the selected type of transfer */

    switch (pTrb->transferType)
	{
	case USB_XFRTYPE_CONTROL:

	    pEndpoint->maxPacketSize = pEndpoint->ctlMaxPacketSize;
	    break;

	case USB_XFRTYPE_BULK:

	    pEndpoint->maxPacketSize = pickPacketSize (pTrb->direction,
		pEndpoint->bulkInMaxPacketSize, pEndpoint->bulkOutMaxPacketSize,
		pEndpoint->bulkInOutMaxPacketSize);
	    break;

	case USB_XFRTYPE_INTERRUPT:

	    pEndpoint->maxPacketSize = pickPacketSize (pTrb->direction,
		pEndpoint->intInMaxPacketSize, pEndpoint->intOutMaxPacketSize,
		pEndpoint->intInOutMaxPacketSize);
	    break;

	case USB_XFRTYPE_ISOCH:

	    pEndpoint->maxPacketSize = pickPacketSize (pTrb->direction,
		pEndpoint->isochInMaxPacketSize, pEndpoint->isochOutMaxPacketSize,
		pEndpoint->isochInOutMaxPacketSize);
	    break;
	}


    /* Enable the endpoint as necessary. */

    d12SetEndpointStatus (pTarget, pTrb->endpointId, 0);    /* reset to DATA0 */

    switch (pTrb->endpointId)
	{
	case D12_ENDPOINT_CONTROL_OUT:
	case D12_ENDPOINT_CONTROL_IN:

	    break;  /* no additional processing required */

	case D12_ENDPOINT_2_OUT:
	case D12_ENDPOINT_2_IN:

	    /* These comprise the D12's "main" endpoint.  When these endpoints
	     * are enabled, we need to change the endpoint configuration mode
	     * according to the type of transfer.
	     */

	    if (++pTarget->epMainCount == 1 &&
		pTrb->transferType == USB_XFRTYPE_ISOCH)
		{
		pTarget->configByte &= ~D12_CMD_SM_CFG_MODE_MASK;

		switch (pTrb->direction)
		    {
		    case USB_DIR_OUT:
			pTarget->configByte |= D12_CMD_SM_CFG_MODE1_ISO_OUT;
			break;

		    case USB_DIR_IN:
			pTarget->configByte |= D12_CMD_SM_CFG_MODE2_ISO_IN;
			break;
			
		    case USB_DIR_INOUT:
			pTarget->configByte |= D12_CMD_SM_CFG_MODE3_ISO_IO;
			break;
		    }

		d12SetMode (pTarget);
		}

	    /* NOTE: Code falls through into case below intentionally. */

	case D12_ENDPOINT_1_OUT:
	case D12_ENDPOINT_1_IN:

	    /* Whenever the D12's endpoint #1 or #2 is enabled, we need to
	     * issue a Set Endpoint Enable command.
	     */

	    if (++pTarget->epOneAndTwoCount == 1)
		d12SetEndpointEnable (pTarget, ENABLE);

	    break;
	}


    /* Mark the endpoint as "in use" */

    pEndpoint->flags |= TCD_ENDPOINT_IN_USE;

    return OK;
    }


/***************************************************************************
*
* fncEndpointRelease - Releases an endpoint
*
* This function is used to release an endpoint which was previously enabled
* for data transfer by calling fncEndpointAssign().
*
* RETURNS: OK or S_usbTcdLib_xxxx if an error is detected
*/

LOCAL int fncEndpointRelease
    (
    pTRB_ENDPOINT_RELEASE pTrb,
    pTARGET pTarget
    )

    {
    pUSB_TARG_ENDPOINT_INFO pEndpoint;


    /* mark endpoint as no longer "in use" */

    pEndpoint = &pTarget->pEndpoints [pTrb->endpointId];

    pEndpoint->flags &= ~TCD_ENDPOINT_IN_USE;

    pEndpoint->endpointNum = 0;
    pEndpoint->configuration = 0;
    pEndpoint->interface = 0;
    pEndpoint->transferType = 0;
    pEndpoint->direction = 0;
    pEndpoint->maxPacketSize = 0;

    /* We may need to disable certain d12 functions. */

    switch (pEndpoint->endpointId)
	{
	case D12_ENDPOINT_CONTROL_OUT:
	case D12_ENDPOINT_CONTROL_IN:

	    break;  /* no additional processing required */

	case D12_ENDPOINT_2_OUT:
	case D12_ENDPOINT_2_IN:

	    /* These comprise the D12's "main" endpoint.  When these endpoints
	     * are disabled, we may need to change the endpoint configuration.
	     */

	    if (--pTarget->epMainCount == 0)
		{
		pTarget->configByte &= ~D12_CMD_SM_CFG_MODE_MASK;
		pTarget->configByte |= D12_CMD_SM_CFG_MODE0_NON_ISO;

		d12SetMode (pTarget);
		}

	    /* NOTE: Code falls through into case below intentionally. */

	case D12_ENDPOINT_1_OUT:
	case D12_ENDPOINT_1_IN:

	    /* Whenever the D12's endpoint #1 or #2 is disabled, we may need to
	     * issue a Set Endpoint Enable command.
	     */

	    if (--pTarget->epOneAndTwoCount == 0)
		d12SetEndpointEnable (pTarget, DISABLE);

	    break;
	}
    
    return OK;
    }


/***************************************************************************
*
* fncSignalResume - Drives RESUME signalling on USB
*
* If the USB is in the SUSPEND state, the host may wish to drive RESUME
* signalling in order to try to "wake up" the host.
*
* RETURNS: OK or S_usbTcdLib_xxxx if an error is detected
*/

LOCAL int fncSignalResume
    (
    pTRB_SIGNAL_RESUME pTrb,
    pTARGET pTarget
    )

    {
    d12SendResume (pTarget);
    return OK;
    }


/***************************************************************************
*
* fncEndpointStateSet - Sets endpoint as stalled/unstalled or resets data toggle
*
* In response to an error condition, the usbTargLib or an application may
* wish to set one or more endpoints to the "stalled" state.  Using this
* function, the caller can specify whether an endpoint should be "stalled"
* or "unstalled".
*
* Independently, an endpoint's data toggle may be reset to DATA0.  This is 
* necessary when a "configuration event" is detected for the endpoint.
*
* RETURNS: OK, or S_usbTcdLib_xxx if an error is detected.
*/

LOCAL int fncEndpointStateSet
    (
    pTRB_ENDPOINT_STATE_SET pTrb,
    pTARGET pTarget
    )

    {
    if ((pTrb->state & TCD_ENDPOINT_STALL) != 0)
	{
	stallEndpoint (pTarget, pTrb->endpointId);
	}

    if ((pTrb->state & TCD_ENDPOINT_UNSTALL) != 0 ||
	(pTrb->state & TCD_ENDPOINT_DATA0) != 0)
	{
	d12SetEndpointStatus (pTarget, pTrb->endpointId, 0);
	}

    return OK;
    }


/***************************************************************************
*
* fncCurrentFrameGet - Returns current USB frame number
*
* Certain applications, particularly those using isochronous data transfers,
* need to know the current USB frame number.  This function returns the
* most recently decoded USB frame number (as encoded in the USB SOF packet).
*
* RETURNS: OK or S_usbTcdLib_xxxx if an error is detected
*/

LOCAL int fncCurrentFrameGet
    (
    pTRB_CURRENT_FRAME_GET pTrb,
    pTARGET pTarget
    )

    {
    pTrb->frameNo = d12ReadCurrentFrameNumber (pTarget);
    return OK;
    }


/***************************************************************************
*
* fncErpSubmit - Submits an ERP for subsequent data transfer
*
* All data transfers across the USB are managed through ERPs (Endpoint
* Request Packets).  These ERPs are analogous to the IRPs used by the host
* to manage data transfers.
*
* RETURNS: OK or S_usbTcdLib_xxxx if an error is detected
*/

LOCAL int fncErpSubmit
    (
    pTRB_ERP_SUBMIT pTrb,
    pTARGET pTarget
    )

    {
    pUSB_ERP pErp = pTrb->pErp;

    /* Enqueue the ERP for the corresponding endpoint. */

    usbListLink (&pTarget->erps [pErp->endpointId], pErp, &pErp->tcdLink,
	LINK_TAIL);

    pTarget->endpointNeedsService [pErp->endpointId] = TRUE;
    OSS_SEM_GIVE (pTarget->intPending);


    return OK;
    }


/***************************************************************************
*
* fncErpCancel - Cancels an ERP 
*
* This function allows the caller to cancel an ERP which was previously
* submitted for execution by calling fncErpSubmit().  It may or may not
* be possible to cancel the ERP (depending on whether or not the ERP has
* already completed processing).
*
* RETURNS: OK or S_usbTcdLib_xxxx if an error is detected
*/

LOCAL int fncErpCancel
    (
    pTRB_ERP_CANCEL pTrb,
    pTARGET pTarget
    )

    {
    return cancelErp (pTarget, pTrb->pErp);
    }


/***************************************************************************
*
* usbTcdPdiusbd12EvalExec - USB_TCD_EXEC_FUNC entry point for PDIUSBD12 TCD
*
* This is the primary entry point for the Philips PDIUSBD12 (ISA eval version)
* USB TCD (Target Controller Driver).  The function qualifies the TRB passed
* by the caller and fans out to the appropriate TCD function handler.
*
* RETURNS: OK or ERROR if failed to execute TRB passed by caller
*
* ERRNO:
*   S_usbTcdLib_BAD_PARAM
*   S_usbTcdLib_BAD_HANDLE
*   S_usbTcdLib_SHUTDOWN
*/

STATUS usbTcdPdiusbd12EvalExec
    (
    pVOID pTrb			    /* TRB to be executed */
    )

    {
    pTRB_HEADER pHeader = (pTRB_HEADER) pTrb;
    pTARGET pTarget = NULL;
    int status;

    
    /* Validate parameters */

    if (pHeader == NULL || pHeader->trbLength < sizeof (TRB_HEADER))
	return ossStatus (S_usbTcdLib_BAD_PARAM);

    if (pHeader->function != TCD_FNC_ATTACH)
	{
	if ((pTarget = (pTARGET) pHeader->handle) == NULL)
	    return ossStatus (S_usbTcdLib_BAD_HANDLE);

	if (pTarget->shutdown)
	    return ossStatus (S_usbTcdLib_SHUTDOWN);
	}


    /* Guard against other tasks */

    if (pHeader->function != TCD_FNC_ATTACH && 
	pHeader->function != TCD_FNC_DETACH)
	{
	OSS_MUTEX_TAKE (pTarget->tcdMutex, OSS_BLOCK);
	}


    /* Fan-out to appropriate function processor */

    switch (pHeader->function)
	{
	case TCD_FNC_ATTACH:
	    status = fncAttach ((pTRB_ATTACH) pHeader);
	    break;

	case TCD_FNC_DETACH:
	    status = fncDetach ((pTRB_DETACH) pHeader, pTarget);
	    break;

	case TCD_FNC_ENABLE:
	    status = fncEnable ((pTRB_ENABLE_DISABLE) pHeader, pTarget);
	    break;

	case TCD_FNC_DISABLE:
	    status = fncDisable ((pTRB_ENABLE_DISABLE) pHeader, pTarget);
	    break;

	case TCD_FNC_ADDRESS_SET:
	    status = fncAddressSet ((pTRB_ADDRESS_SET) pHeader, pTarget);
	    break;

	case TCD_FNC_ENDPOINT_ASSIGN:
	    status = fncEndpointAssign ((pTRB_ENDPOINT_ASSIGN) pHeader, pTarget);
	    break;

	case TCD_FNC_ENDPOINT_RELEASE:
	    status = fncEndpointRelease ((pTRB_ENDPOINT_RELEASE) pHeader, pTarget);
	    break;

	case TCD_FNC_SIGNAL_RESUME:
	    status = fncSignalResume ((pTRB_SIGNAL_RESUME) pHeader, pTarget);
	    break;

	case TCD_FNC_ENDPOINT_STATE_SET:
	    status = fncEndpointStateSet ((pTRB_ENDPOINT_STATE_SET) pHeader, pTarget);
	    break;

	case TCD_FNC_CURRENT_FRAME_GET:
	    status = fncCurrentFrameGet ((pTRB_CURRENT_FRAME_GET) pHeader, pTarget);
	    break;

	case TCD_FNC_ERP_SUBMIT:
	    status = fncErpSubmit ((pTRB_ERP_SUBMIT) pHeader, pTarget);
	    break;

	case TCD_FNC_ERP_CANCEL:
	    status = fncErpCancel ((pTRB_ERP_CANCEL) pHeader, pTarget);
	    break;

	default:
	    status = S_usbTcdLib_BAD_PARAM;
	    break;
	}

    /* Release guard mutex */

    if (pHeader->function != TCD_FNC_ATTACH && 
	pHeader->function != TCD_FNC_DETACH)
	{
	OSS_MUTEX_RELEASE (pTarget->tcdMutex);
	}


    /* Return status */

    return ossStatus (status);
    }




/* End of file. */

