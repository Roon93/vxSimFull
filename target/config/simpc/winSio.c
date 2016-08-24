/* winSio.c - win serial driver */

/* Copyright 1984-1997 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,23nov01,jmp  added intLock()/intUnlock() around NT system call.
01d,31oct01,jmp  moved to simpc bsp.
01c,06oct98,cym  added more documentation, removed winIntErr.
01b,13jul98,cym  moved to src/drv from config/winnt updated path the header.
01a,15sep97,cym  written.
*/

/*
DESCRIPTION
This is the console serial driver for the Windows simulator.  It 
recieves character interrupts from Windows and send them off to vxWorks.

Device data structures are defined in the header file winSio.h.
A device data structure, WIN_CHAN, is defined for each channel.

.SH USAGE
The driver is typically only called only by the BSP. The directly callable
routines in this module are winDevInit(), winDevInit2(),
winIntRcv() and winIntTx().

The BSP calls winDevInit() to initialize or reset the device.
It connects the driver's interrupt handlers (winIntRcv and winIntTx)
using intConnect().
After connecting the interrupt handlers, the BSP calls winDevInit2()
to inform the driver that interrupt mode operation is now possible.

.SH BSP
By convention all the BSP-specific serial initialization is performed in
a file called sysSerial.c, which is #include'ed by sysLib.c.
sysSerial.c implements at least four functions, sysSerialHwInit()
sysSerialHwInit2(), sysSerialChanGet(), and sysSerialReset(), which work as follows:

sysSerialHwInit is called by sysHwInit to initialize the serial devices.
This routine will initialize all the board specific fields in the
WIN_CHAN structure (e.g., register I/O addresses, etc) before
calling winDevInit(), which resets the device and installs the driver
function pointers. sysSerialHwInit() should also perform any other processing
which is needed for the serial drivers, such as configuring on-board
interrupt controllers as appropriate.

sysSerialHwInit2 is called by sysHwInit2 to connect the serial driver's
interrupt handlers using intConnect().  After connecting the interrupt
handlers, the call to winDevInit2() is made to permit interrupt
mode operations to begin.

sysSerialChanGet is called by usrRoot to get the serial channel descriptor
associated with a serial channel number. The routine takes a single parameter
which is a channel number ranging between zero and NUM_TTY. It returns
a pointer to the corresponding channel descriptor, SIO_CHAN *, which is
just the address of the WIN_CHAN structure.

sysSerialReset is called from sysToMonitor() and should reset the serial
devices to an inactive state.

.SH INCLUDE FILES:
winSio.h sioLib.h
*/

#include "vxWorks.h"
#include "sioLib.h"
#include "intLib.h"
#include "errno.h"
#include "winSio.h"

#define WIN_BAUD_MIN	75
#define WIN_BAUD_MAX	38400

extern int winCharOut();
extern int winConIntEnable();
extern int winConIntDisable();

/* forward static declarations */

LOCAL	int    winTxStartup (SIO_CHAN * pSioChan);
LOCAL	int    winCallbackInstall (SIO_CHAN *pSioChan, int callbackType,
				    STATUS (*callback)(), void *callbackArg);
LOCAL	int    winPollOutput (SIO_CHAN *pSioChan, char	outChar);
LOCAL	int    winPollInput (SIO_CHAN *pSioChan, char *thisChar);
LOCAL	int    winIoctl (SIO_CHAN *pSioChan, int request, void *arg);
LOCAL	STATUS dummyCallback (void);

/* local variables */

LOCAL	SIO_DRV_FUNCS winSioDrvFuncs =
    {
    winIoctl,
    winTxStartup,
    winCallbackInstall,
    winPollInput,
    winPollOutput
    };

LOCAL BOOL winIntrMode = FALSE;	/* interrupt mode allowed flag */

/******************************************************************************
*
* winDevInit - initialize a WIN_CHAN
*
* This routine initializes the driver
* function pointers and then resets the chip in a quiescent state.
* The BSP must have already initialized all the device addresses and the
* baudFreq fields in the WIN_CHAN structure before passing it to
* this routine.
*
* RETURNS: N/A
*/

void winDevInit
    (
    WIN_CHAN * pChan
    )
    {
    /* initialize each channel's driver function pointers */

    pChan->sio.pDrvFuncs	= &winSioDrvFuncs;

    /* install dummy driver callbacks */

    pChan->getTxChar    = dummyCallback;
    pChan->putRcvChar	= dummyCallback;
    pChan->getTxChar    = dummyCallback;
    pChan->putRcvChar	= dummyCallback;
    pChan->transmitting = FALSE;
    
    /* setting polled mode is one way to make the device quiet */
    winConIntDisable();

    }

/******************************************************************************
*
* winDevInit2 - initialize a WIN_CHAN, part 2
*
* This routine is called by the BSP after interrupts have been connected.
* The driver can now operate in interrupt mode.  Before this routine is
* called only polled mode operations should be allowed.
*
* RETURNS: N/A
* ARGSUSED
*/

void winDevInit2
    (
    WIN_CHAN * pChan	/* device to initialize */
    )
    {

    winConIntEnable();
    /* Interrupt mode is allowed */

    winIntrMode = TRUE;
    }

/******************************************************************************
*
* winIntRcv - handle a channel's receive-character interrupt
*
* This function is attached to the simulator's interrupt handler, and
* passes the character received in the message to the callback.
*
* RETURNS: N/A
*/ 

void winIntRcv
    (
    WIN_CHAN *	pChan,		/* channel generating the interrupt */
    UINT16 	wparam		/* message args get passed if you look */
    )
    {

    /* hand off the char */
    (*pChan->putRcvChar) (pChan->putRcvArg, (char)wparam); 
    }

/******************************************************************************
*
* winIntTx - transmit a single character.
*
* This displays a single character to the simulator's window.
*
* RETURNS: N/A
*/ 

void winIntTx
    (
    WIN_CHAN *	pChan		/* channel generating the interrupt */
    )
    {
    char outChar;

    if((*pChan->getTxChar) (pChan->getTxArg,&outChar) != ERROR)
        winCharOut(outChar);
    else
        pChan->transmitting = FALSE;
    }

/******************************************************************************
*
* winTxStartup - start the interrupt transmitter
*
* RETURNS: OK on success, ENOSYS if the device is polled-only, or
* EIO on hardware error.
*/

static int winTxStartup
    (
    SIO_CHAN * pSioChan                 /* channel to start */
    )
    {
    WIN_CHAN * pChan = (WIN_CHAN *)pSioChan;
    int key;

    pChan->transmitting = TRUE;

    /*
     * all system calls need to be protected from being interrupted,
     * winIntTx() indirectly use winOut() which is a system call.
     */
    key = intLock ();

    while(pChan->transmitting)
        winIntTx(pChan);
    winIntTx(pChan);

    intUnlock (key);	/* unlock system call */

    return (OK);
    }

/******************************************************************************
*
* winCallbackInstall - install ISR callbacks to get/put chars
*
* This driver allows interrupt callbacks for transmitting characters
* and receiving characters. In general, drivers may support other
* types of callbacks too.
*
* RETURNS: OK on success, or ENOSYS for an unsupported callback type.
*/ 

static int winCallbackInstall
    (
    SIO_CHAN *	pSioChan,               /* channel */
    int		callbackType,           /* type of callback */
    STATUS	(*callback)(),          /* callback */
    void *      callbackArg             /* parameter to callback */
    )
    {
    WIN_CHAN * pChan = (WIN_CHAN *)pSioChan;

    switch (callbackType)
	{
	case SIO_CALLBACK_GET_TX_CHAR:
	    pChan->getTxChar	= callback;
	    pChan->getTxArg	= callbackArg;
	    return (OK);
	case SIO_CALLBACK_PUT_RCV_CHAR:
	    pChan->putRcvChar	= callback;
	    pChan->putRcvArg	= callbackArg;
	    return (OK);
	default:
	    return (ENOSYS);
	}

    }

/*******************************************************************************
*
* winPollOutput - output a character in polled mode
*
* RETURNS: OK if a character arrived, EIO on device error, EAGAIN
* if the output buffer if full. ENOSYS if the device is
* interrupt-only.
*/

static int winPollOutput
    (
    SIO_CHAN *	pSioChan,
    char	outChar
    )
    {
    winCharOut(outChar);
    return (OK);
    }

/******************************************************************************
*
* winPollInput - poll the device for input
*
* RETURNS: OK if a character arrived, EIO on device error, EAGAIN
* if the input buffer if empty, ENOSYS if the device is
* interrupt-only.
*/

static int winPollInput
    (
    SIO_CHAN *	pSioChan,
    char *	thisChar
    )
    {
    return (ENOSYS);
    }

/******************************************************************************
*
* winModeSet - toggle between interrupt and polled mode
*
* RETURNS: OK on success, EIO on unsupported mode.
*/

static int winModeSet
    (
    WIN_CHAN * pChan,		/* channel */
    uint_t	    newMode          	/* new mode */
    )
    {
    if(newMode != SIO_MODE_INT) return (EIO);
    return(OK);
    }

/******************************************************************************
*
* winOptSet - set hardware options
*
* This routine sets up the hardware according to the specified option
* argument.  If the hardware cannot support a particular option value, then
* it should ignore that portion of the request.
*
* RETURNS: OK upon success, or EIO for invalid arguments.
*/

static int winOptSet
    (
    WIN_CHAN * pChan,		/* channel */
    uint_t	    newOpts          	/* new options */
    )
    {
    return (OK);
    }

/*******************************************************************************
*
* winIoctl - special device control
*
* This routine handles the IOCTL messages from the user.
*
* RETURNS: OK on success, ENOSYS on unsupported request, EIO on failed
* request.
*/

static int winIoctl
    (
    SIO_CHAN *	pSioChan,		/* device to control */
    int		request,		/* request code */
    void *	someArg			/* some argument */
    )
    {
    WIN_CHAN *pChan = (WIN_CHAN *) pSioChan;
    int     arg = (int)someArg;

    switch (request)
	{
	case SIO_BAUD_SET:

	    if (arg < WIN_BAUD_MIN || arg > WIN_BAUD_MAX)
	        {
		return (EIO);
	        }

	    return (OK);

	case SIO_BAUD_GET:

	    /* Get the baud rate and return OK */
	    return (OK);

	case SIO_MODE_SET:

	    /*
	     * Set the mode (e.g., to interrupt or polled). Return OK
	     * or EIO for an unknown or unsupported mode.
	     */

	    return (winModeSet (pChan, arg));

	case SIO_MODE_GET:

	    /*
	     * Get the current mode and return OK.
	     */

	    *(int *)arg = pChan->mode;
	    return (OK);

	case SIO_AVAIL_MODES_GET:

	    /*
	     * Get the available modes and return OK.
	     */

	    *(int *)arg = SIO_MODE_INT;
	    return (OK);

	case SIO_HW_OPTS_SET:

	    /*
	     * Optional command to set the hardware options (as defined
	     * in sioLib.h).
	     * Return OK, or ENOSYS if this command is not implemented.
	     * Note: several hardware options are specified at once.
	     * This routine should set as many as it can and then return
	     * OK. The SIO_HW_OPTS_GET is used to find out which options
	     * were actually set.
	     */

	    return (winOptSet (pChan, arg));

	case SIO_HW_OPTS_GET:

	    /*
	     * Optional command to get the hardware options (as defined
	     * in sioLib.h). Return OK or ENOSYS if this command is not
	     * implemented.  Note: if this command is unimplemented, it
	     * will be assumed that the driver options are CREAD | CS8
	     * (e.g., eight data bits, one stop bit, no parity, ints enabled).
	     */

	    *(int *)arg = pChan->options;
	    return (OK);

	default:
	    return (ENOSYS);
	}
    }

/*******************************************************************************
*
* dummyCallback - dummy callback routine
*
* RETURNS: ERROR.
*/

STATUS dummyCallback (void)
    {
    return (ERROR);
    }
