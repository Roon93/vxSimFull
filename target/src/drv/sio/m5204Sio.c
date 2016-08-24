/* m5204Sio.c - m5204 Serial Communications driver */

/* Copyright 1984-1996 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,05may97,mem  changed baud rate calculation to round rather than
		 truncate when dividing.  Reduces error at high baud rates.
01a,10dec96,mem  written.
*/

/*
DESCRIPTION
This is the driver for the UART contained in the m5204 Microcontroller.

Only asynchronous serial operation is supported by this driver.  The
default serial settings are 8 data bits, 1 stop bit, no parity, 9600
buad, and software flow control.  These default settings can be
overridden by setting the M5204_CHAN options and baudRate fields to
the desired values before calling m5204DevInit.  See sioLib.h for
options values.  The defaults for the module can be changed by
redefining the macros M5204_DEFAULT_OPTIONS and M5204_DEFAULT_BAUD and
recompiling this driver.

This driver uses the system clock as the input to the baud rate
generator.  The clkRate field must be set to the system clock rate in
HZ for the baud rate calculations to work correctly.  The actual range
of supported baud rates depends on the system clock speed.  For example,
with a 25MHz system clock, the lowest baud rate is 24, and the highest
is over 38400.  Because the baud rate values are calculated on request,
there is no checking that the requested baud rate is standard, you can
set the UART to operate at 4357 baud if you wish.

USAGE
A M5204_CHAN structure is used to describe the chip.

The BSP's sysHwInit() routine typically calls sysSerialHwInit()
which initializes all the H/W addresses in the M5204_CHAN structure
before calling m5204DevInit().  This enables the chip to operate in
polled mode, but not interrupt mode.  Calling m5204DevInit2() from
the sysSerialHwInit2() routine allows interrupts to be enabled and
interrupt mode operation to be used.

.CS
i.e.

#include "drv/multi/m5204Sio.h"

M5204_CHAN m5204Uart;			/@ my device structure @/

#define INT_VEC_UART	(24+3)		/@ use single vector, #27 @/

sysSerialHwInit()
    {
    /@ initialize the register pointers/data for uart @/
    m5204Uart.clkRate	= MASTER_CLOCK;
    m5204Uart.intVec	= INT_VEC_UART;
    m5204Uart.mr	= M5204_UART_MR(SIM_BASE);
    m5204Uart.sr	= M5204_UART_SR(SIM_BASE);
    m5204Uart.csr	= M5204_UART_CSR(SIM_BASE);
    m5204Uart.cr	= M5204_UART_CR(SIM_BASE);
    m5204Uart.rb	= M5204_UART_RB(SIM_BASE);
    m5204Uart.tb	= M5204_UART_TB(SIM_BASE);
    m5204Uart.ipcr	= M5204_UART_IPCR(SIM_BASE);
    m5204Uart.acr	= M5204_UART_ACR(SIM_BASE);
    m5204Uart.isr	= M5204_UART_ISR(SIM_BASE);
    m5204Uart.imr	= M5204_UART_IMR(SIM_BASE);
    m5204Uart.bg1	= M5204_UART_BG1(SIM_BASE);
    m5204Uart.bg2	= M5204_UART_BG2(SIM_BASE);
    m5204Uart.ivr	= M5204_UART_IVR(SIM_BASE);
    m5204Uart.ip	= M5204_UART_IP(SIM_BASE);
    m5204Uart.op1	= M5204_UART_OP1(SIM_BASE);
    m5204Uart.op2	= M5204_UART_OP2(SIM_BASE);

    m5204DevInit (&m5204Uart);
    }
.CE

The BSP's sysHwInit2() routine typically calls sysSerialHwInit2() which
connects the chips interrupts via intConnect() to the single
interrupt handler m5204Int.  After the interrupt service routines are 
connected, the user then calls m5204DevInit2() to allow the driver to 
turn on interrupt enable bits.

.CS
i.e.

sysSerialHwInit2 ()
    {
    /@ connect single vector for 5204 @/
    intConnect (INUM_TO_IVEC(MY_VEC), m5204Int, (int)&m5204Uart);

    ...

    /@ allow interrupts to be enabled @/
    m5204DevInit2 (&m5204Uart);
    }
.CE

SPECIAL CONSIDERATIONS:

The CLOCAL hardware option presumes that CTS outputs are wired as
necessary.  If not wired correctly, then the user must not select the
CLOCAL option.  CLOCAL is not one of the default options for this
reason.

As to the output port, this driver does not manipulate the output port,
or it's configuration register in any way.  As stated above, if the user
selects the CLOCAL option then the output port bit must be wired correctly
or the hardware flow control will not function as desired.

INCLUDE FILES: drv/sio/m5204Sio.h
*/

#include "vxWorks.h"
#include "sioLib.h"
#include "intLib.h"
#include "errno.h"
#include "drv/multi/m5204.h"
#include "drv/sio/m5204Sio.h"

/* forward static declarations */

LOCAL STATUS m5204ModeSet (M5204_CHAN *, UINT);
LOCAL STATUS m5204BaudSet (M5204_CHAN *, UINT);
LOCAL STATUS m5204OptsSet (M5204_CHAN *, UINT);

LOCAL int    m5204Ioctl (M5204_CHAN *, int, void *);
LOCAL int    m5204TxStartup (M5204_CHAN *);
LOCAL int    m5204CallbackInstall (M5204_CHAN *, int, STATUS (*)(), void *);
LOCAL int    m5204PollInput (M5204_CHAN *, char *);
LOCAL int    m5204PollOutput (M5204_CHAN*, char);
LOCAL STATUS m5204DummyCallback (void);
      void m5204AcrSetClr (M5204_CHAN * pChan, UCHAR setBits, UCHAR clearBits);
      void m5204ImrSetClr (M5204_CHAN * pChan, UCHAR setBits, UCHAR clearBits);

/* driver functions */

LOCAL SIO_DRV_FUNCS m5204SioDrvFuncs =
    {
    (int (*)(SIO_CHAN *, int, void *))m5204Ioctl,
    (int (*)(SIO_CHAN *))m5204TxStartup,
    (int (*)())m5204CallbackInstall,
    (int (*)(SIO_CHAN *, char*))m5204PollInput,
    (int (*)(SIO_CHAN *, char))m5204PollOutput
    };

/* typedefs */

/* defines */

#ifndef M5204_DEFAULT_BAUD
#   define M5204_DEFAULT_BAUD  9600
#endif

#ifndef M5204_DEFAULT_OPTIONS
    /* no handshake, rcvr enabled, 8 data bits, 1 stop bit, no parity */
#   define M5204_DEFAULT_OPTIONS (CREAD | CS8)
#endif

/* Hardware read/write routines.  Can be redefined to create drivers
 * for boards with special i/o access procedures. The reg pointer
 * arguments are the register pointers from the M5204_CHAN structure.
 */

#ifndef M5204_READ
#   define M5204_READ(x)	(*x)	/* argument is register pointer */
#endif

#ifndef M5204_WRITE
#   define M5204_WRITE(x,y)	(*x = y) /* args are reg ptr and data */
#endif

#define MAX_OPTIONS	(0xff)

/******************************************************************************
*
* m5204DevInit - intialize a M5204_CHAN
*
* The BSP must have already initialized all the device addresses, etc in
* m5204_CHAN structure. This routine initializes some transmitter &
* receiver status values to be used in the interrupt mask register and then
* resets the chip to a quiescent state.
*
* RETURNS: N/A
*/

void m5204DevInit
    (
    M5204_CHAN * pChan
    )
    {
    int oldlevel;
    
    pChan->pDrvFuncs	= &m5204SioDrvFuncs;
    pChan->getTxChar	= m5204DummyCallback;
    pChan->putRcvChar	= m5204DummyCallback;

    pChan->mode		= 0; /* undefined */
    pChan->intEnable	= FALSE;
    pChan->acrCopy	= 0;
    pChan->imrCopy	= 0;
    pChan->oprCopy	= 0;

    if (pChan->options == 0 || pChan->options > MAX_OPTIONS)
	pChan->options = M5204_DEFAULT_OPTIONS;

    /* baud rate range is determined by calculation. */
    if (pChan->baudRate)
	{
	UINT val;

	val = ((pChan->clkRate / pChan->baudRate) + 16) / 32;
	if ((val < 2) || (val > 0xffff))
	    pChan->baudRate = M5204_DEFAULT_BAUD;
	}
    else
	pChan->baudRate = M5204_DEFAULT_BAUD;

    oldlevel =  intLock ();

    m5204ImrSetClr (pChan, 0, ~0);
    m5204AcrSetClr (pChan, 0, M5204_UART_ACR_IEC);

    /* Use internal timer */
    M5204_WRITE (pChan->csr,
		 M5204_UART_CSR_TIMER_RX | M5204_UART_CSR_TIMER_TX);

    /* Reset the transmitters  & receivers  */

    M5204_WRITE (pChan->cr, M5204_UART_CR_RESET_RX);
    M5204_WRITE (pChan->cr, M5204_UART_CR_RESET_TX);
    M5204_WRITE (pChan->cr, M5204_UART_CR_RESET_ERR);

    /* TX is disabled */
    m5204OptsSet (pChan, pChan->options);
    m5204BaudSet (pChan, pChan->baudRate);

    intUnlock (oldlevel);
    }


/******************************************************************************
*
* m5204DevInit2 - intialize a M5204_CHAN, part 2
*
* This routine is called as part of sysSerialHwInit2() and tells
* the driver that interrupt vectors are connected and that it is
* safe to allow interrupts to be enabled.
*
* RETURNS: N/A
*/

void m5204DevInit2
    (
    M5204_CHAN * pChan
    )
    {
    /* Allow interrupt mode */

    pChan->intEnable = TRUE;
    M5204_WRITE(pChan->ivr, pChan->intVec);
    m5204ModeSet (pChan, pChan->mode);
    }


/******************************************************************************
*
* m5204ImrSetClr - set and clear bits in the UART's interrupt mask register
*
* This routine sets and clears bits in the UART's IMR.
*
* This routine sets and clears bits in a local copy of the IMR, then
* writes that local copy to the UART.  This means that all changes to
* the IMR must go through this routine.  Otherwise, any direct changes
* to the IMR would be lost the next time this routine is called.
*
* Set has priority over clear.  Thus you can use this routine to update
* multiple bit fields by specifying the field mask as the clear bits.
*
* RETURNS: N/A
*/

void m5204ImrSetClr
    (
    M5204_CHAN * pChan,
    UCHAR setBits,       /* which bits to set in the IMR   */
    UCHAR clearBits      /* which bits to clear in the IMR */
    )
    {
    pChan->imrCopy = ((pChan->imrCopy & ~clearBits) | setBits);
    M5204_WRITE (pChan->imr, pChan->imrCopy);
    }


/******************************************************************************
*
* m5204Imr - return current interrupt mask register contents
*
* This routine returns the interrupt mask register contents.  The imr
* is not directly readable, a copy of the last value written is kept
* in the UART data structure.
*
* RETURNS: Returns interrupt mask register contents.
*/

UCHAR m5204Imr
    (
    M5204_CHAN * pChan
    )
    {
    return pChan->imrCopy;
    }


/******************************************************************************
*
* m5204AcrSetClr - set and clear bits in the UART's aux control register
*
* This routine sets and clears bits in the UART's ACR.
*
* This routine sets and clears bits in a local copy of the ACR, then
* writes that local copy to the UART.  This means that all changes to
* the ACR must go through this routine.  Otherwise, any direct changes
* to the ACR would be lost the next time this routine is called.
*
* Set has priority over clear.  Thus you can use this routine to update
* multiple bit fields by specifying the field mask as the clear bits.
*
* RETURNS: N/A
*/

void m5204AcrSetClr
    (
    M5204_CHAN * pChan,
    UCHAR setBits,       /* which bits to set in the ACR   */
    UCHAR clearBits      /* which bits to clear in the ACR */
    )
    {
    pChan->acrCopy = ((pChan->acrCopy &  ~clearBits) | setBits);
    M5204_WRITE (pChan->acr, pChan->acrCopy);
    }


/******************************************************************************
*
* m5204Acr - return aux control register contents
*
* This routine returns the auxilliary control register contents.  The acr
* is not directly readable, a copy of the last value written is kept
* in the UART data structure.
*
* RETURNS: Returns auxilliary control register (acr) contents.
*/

UCHAR m5204Acr
    (
    M5204_CHAN * pChan
    )
    {
    return pChan->acrCopy;
    }


/******************************************************************************
*
* m5204OprSetClr - set and clear bits in the output port register
*
* This routine sets and clears bits in the UART's OPR.
*
* A copy of the current opr contents is kept in the UART data structure.
*
* RETURNS: N/A
*/

void m5204OprSetClr
    (
    M5204_CHAN * pChan,
    UCHAR setBits,       /* which bits to set in the OPR   */
    UCHAR clearBits      /* which bits to clear in the OPR */
    )
    {
    pChan->oprCopy = ((pChan->oprCopy &  ~clearBits) | setBits);
    M5204_WRITE (pChan->op2, clearBits);
    M5204_WRITE (pChan->op1, setBits);
    }


/******************************************************************************
*
* m5204Opr - return the current state of the output register
*
* This routine returns the current state of the output register from
* the saved copy in the UART data structure.  The actual opr contents
* are not directly readable.
*
* RETURNS: Returns the current output register state.
*/

UCHAR m5204Opr
    (
    M5204_CHAN * pChan
    )
    {
    return pChan->oprCopy;
    }


/******************************************************************************
*
* m5204DummyCallback - dummy callback routine.
*
* RETURNS:
* Always returns ERROR
*/

LOCAL STATUS m5204DummyCallback (void)
    {
    return (ERROR);
    }


/******************************************************************************
*
* m5204TxStartup - start the interrupt transmitter.
*
* This routine enables the transmitters for the specified UART channel so that
* the UART channel will interrupt the CPU when TxRDY bit in the status
* register is set.
*
* Just enable the transmitter, don't output.  The driver may have just
* switched from POLL mode and may still be outputting a character.
*
* RETURNS:
* Returns OK on success, or EIO on hardware error.
*/

LOCAL int m5204TxStartup
    (
    M5204_CHAN * pChan
    )
    {
    M5204_WRITE (pChan->cr, M5204_UART_CR_TX_ENABLE);
    return (OK);
    }


/******************************************************************************
*
* m5204CallbackInstall - install ISR callbacks to get/put chars.
*
* This driver allows interrupt callbacks, for transmitting characters
* and receiving characters. In general, drivers may support other types
* of callbacks too.
*
* RETURNS:
* Returns OK on success, or ENOSYS for an unsupported callback type.
*/

LOCAL int m5204CallbackInstall
    (
    M5204_CHAN *	pChan,
    int		callbackType,
    STATUS	(*callback)(),
    void *      callbackArg
    )
    {
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

/******************************************************************************
*
* m5204PollOutput - output a character in polled mode.
*
* This routine polls the status register to see if the TxRDY bit has been set.
* This signals that the transmit holding register is empty and that the
* UART is ready for transmission.
*
* RETURNS:
* Returns OK if a character sent, EIO on device error, EAGAIN
* if the output buffer is full.
*/

LOCAL int m5204PollOutput
    (
    M5204_CHAN *pChan,
    char	outChar
    )
    {
    char statusReg;

    statusReg = M5204_READ (pChan->sr);

    /* is the transitter ready to accept a character? */

    if ((statusReg & M5204_UART_SR_TXRDY) == 0x00)
	return (EAGAIN);

    /* write out the character */

    M5204_WRITE (pChan->tb, outChar);

    return (OK);
    }


/******************************************************************************
*
* m5204PollInput - poll the device for input.
*
* This routine polls the status register to see if the RxRDY bit is set.
* This gets set when the UART receives a character and signals the
* pressence of a character in the receive buffer.
*
* RETURNS:
* Returns OK if a character arrived, EIO on device error, EAGAIN
* if the input buffer if empty.
*/

LOCAL int m5204PollInput
    (
    M5204_CHAN *pChan,
    char *	thisChar
    )
    {
    char statusReg;

    statusReg = M5204_READ (pChan->sr);

    if ((statusReg & M5204_UART_SR_RXRDY) == 0x00)
	return (EAGAIN);

    *thisChar = M5204_READ (pChan->rb);

    return (OK);
    }


/******************************************************************************
*
* m5204ModeSet - change channel mode setting
*
* This driver supports both polled and interrupt modes and is capable of
* switching between modes dynamically. If interrupt mode is desired this
* routine enables the channels receiver, transmitter interrupt and the
* received break condition interrupt. If polled mode is desired the xmitrs for
* the specified channel is enabled.
*
* RETURNS:
* Returns a status of OK if the mode was set else ERROR.
*/

LOCAL STATUS m5204ModeSet
    (
    M5204_CHAN * pChan,
    UINT	newMode
    )
    {
    int oldlevel;

    if ((newMode != SIO_MODE_POLL) && (newMode != SIO_MODE_INT))
	return (ERROR);

    oldlevel = intLock ();

    if (newMode == SIO_MODE_INT && pChan->intEnable)
        {
	/* Enable the interrupts for receiver/transmitter conditions */

	m5204ImrSetClr (pChan, M5204_UART_IMR_TXRDY | M5204_UART_IMR_RXRDY, 0);
        }
    else
        {
	/* Disable interrupts and enable the transmitter for the channel */

	m5204ImrSetClr (pChan, 0, M5204_UART_IMR_TXRDY | M5204_UART_IMR_RXRDY);
	M5204_WRITE (pChan->cr, M5204_UART_CR_TX_ENABLE);
        }

    intUnlock (oldlevel);
    pChan->mode = newMode;

    return (OK);
    }


/******************************************************************************
*
* m5204BaudSet - change baud rate for channel
*
* This routine sets the baud rate for the UART. The interrupts are disabled
* during chip access.
*
* RETURNS:
* Returns a status of OK if the baud rate was set else ERROR.
*/

LOCAL STATUS  m5204BaudSet
    (
    M5204_CHAN * pChan,
    UINT	baud
    )
    {
    UINT val;
    int oldlevel;

    if (!baud)
	return (ERROR);
    val = ((pChan->clkRate / baud) + 16) / 32;
    if ((val < 2) || (val > 0xffff))
	return (ERROR);

    oldlevel = intLock ();
    M5204_WRITE (pChan->bg1, val >> 8);
    M5204_WRITE (pChan->bg2, val);
    intUnlock (oldlevel);

    return (OK);
    }


/******************************************************************************
*
* m5204OptsSet - set the serial options
*
* Set the channel operating mode to that specified.  All sioLib options
* are supported: CLOCAL, CREAD, CSIZE, PARENB, and PARODD.
*
* Note, this routine disables the transmitter.  The calling routine
* may have to re-enable it.
*
* RETURNS:
* Returns OK to indicate success, otherwise ERROR is returned
*/

LOCAL STATUS m5204OptsSet
    (
    M5204_CHAN * pChan, /* ptr to channel */
    UINT options	/* new hardware options */
    )
    {
    int mr1Value;
    int mr2Value;
    int lvl;

    if (pChan == NULL || options & 0xffffff00)
	return (ERROR);

    /* Reset the transmitters  & receivers  */

    switch (options & CSIZE)
	{
	case CS5:
	    mr1Value = M5204_UART_MR1_BITS_CHAR_5;
	    break;
	case CS6:
	    mr1Value = M5204_UART_MR1_BITS_CHAR_6;
	    break;
	case CS7:
	    mr1Value = M5204_UART_MR1_BITS_CHAR_7;
	    break;
	default:
	case CS8:
	    mr1Value = M5204_UART_MR1_BITS_CHAR_8;
	    break;
	}

    if (options & STOPB)
	mr2Value = M5204_UART_MR2_STOP_BITS_2;
    else
	mr2Value = M5204_UART_MR2_STOP_BITS_1;

    switch (options & (PARENB|PARODD))
	{
	case PARENB|PARODD:
	    mr1Value |= M5204_UART_MR1_ODD_PARITY;
	    break;
	case PARENB:
	    mr1Value |= M5204_UART_MR1_EVEN_PARITY;
	    break;
	case PARODD:
	    mr1Value |= M5204_UART_MR1_PAR_MODE_MULTI;
	    break;
	default:
	case 0:
	    mr1Value |= M5204_UART_MR1_NO_PARITY; break;
	}

    if (options & CLOCAL)
	{
	/* clocal enables hardware flow control */
	mr1Value |= M5204_UART_MR1_RX_RTS;
	mr2Value |= M5204_UART_MR2_TX_CTS;
	}

    lvl = intLock ();

    /* now reset the channel mode registers */

    M5204_WRITE (pChan->cr, M5204_UART_CR_RESET_MODE_PTR
		 | M5204_UART_CR_RX_DISABLE
		 | M5204_UART_CR_TX_DISABLE);
    M5204_WRITE (pChan->cr, M5204_UART_CR_RESET_TX);
    M5204_WRITE (pChan->cr, M5204_UART_CR_RESET_RX);

    M5204_WRITE (pChan->mr, mr1Value);  /* mode register 1  */
    M5204_WRITE (pChan->mr, mr2Value);  /* mode register 2  */

    if (options & CREAD)
	M5204_WRITE (pChan->cr, M5204_UART_CR_RX_ENABLE);

    intUnlock (lvl);

    pChan->options = options;

    return (OK);
    }


/******************************************************************************
*
* m5204Ioctl - special device control
*
* RETURNS:
* Returns OK on success, EIO on device error, ENOSYS on unsupported
* request.
*/

LOCAL int m5204Ioctl
    (
    M5204_CHAN *	pChan,		/* device to control */
    int		request,		/* request code */
    void *	someArg			/* some argument */
    )
    {
    STATUS result;
    int     arg = (int)someArg;

    switch (request)
	{
	case SIO_BAUD_SET:
	    return (m5204BaudSet (pChan, arg) == OK ? OK : EIO);

	case SIO_BAUD_GET:
	    *(int *)arg = pChan->baudRate;
	    return (OK);

	case SIO_MODE_SET:
	    return (m5204ModeSet (pChan, arg) == OK ? OK : EIO);

	case SIO_MODE_GET:
	    *(int *)arg = pChan->mode;
	    return (OK);

	case SIO_AVAIL_MODES_GET:
	    *(int *)arg = SIO_MODE_INT | SIO_MODE_POLL;
	    return (OK);

	case SIO_HW_OPTS_SET:
	    /* change options, then set mode to restart chip correctly */
	    result = m5204OptsSet (pChan, arg);
	    m5204ModeSet (pChan, pChan->mode);
	    return result;

	case SIO_HW_OPTS_GET:
	    return pChan->options;

	default:
	    return (ENOSYS);
	}
    }


/******************************************************************************
*
* m5204IntWr - handle a transmitter interrupt
*
* This routine handles write interrupts from the UART. This isr is invoked
* when the TxRDY bit in the interrupt status register has been set. If there
* is no character to transmit the transmitter is disabled.
*
* RETURNS: N/A
*/

LOCAL void m5204IntWr
    (
    M5204_CHAN * pChan
    )
    {
    char            outChar;

    if ((*pChan->getTxChar) (pChan->getTxArg, &outChar) != ERROR)
	{
	/* if char available, tx it */
	M5204_WRITE (pChan->tb, outChar);
	}
    else
        {
	M5204_WRITE (pChan->cr, M5204_UART_CR_TX_DISABLE);
        }
    }


/*****************************************************************************
*
* m5204IntRd - handle a reciever interrupt
*
* This routine handles read interrupts from the UART.
* The UART has been programmed to generate read interrupts when the RXRDY
* status bit has been set in the interrupt status register. When a character
* has been received it is removed from the receive buffer.
*
* RETURNS: N/A
*/

LOCAL void m5204IntRd
    (
    M5204_CHAN * pChan
    )
    {
    UCHAR	inchar;

    while (M5204_READ(pChan->sr) & M5204_UART_SR_RXRDY)
        {
	inchar = M5204_READ (pChan->rb);
	(*pChan->putRcvChar) (pChan->putRcvArg, inchar);
	if (pChan->mode != SIO_MODE_INT)
	    break;
        }
    }


/*******************************************************************************
*
* m5204Int - handle all interrupts in one vector
*
* All interrupts share a single interrupt vector.
* We identify each interrupting source and service it.
* We must service all interrupt sources for those systems with edge-
* sensitive interrupt controllers.
*
* RETURNS: N/A
*/

void m5204Int
    (
    M5204_CHAN * pChan
    )
    {
    UCHAR        intStatus;

    /* loop until all sources have been handled */
    while (intStatus = (M5204_READ(pChan->isr) & pChan->imrCopy))
	{
	if (intStatus & M5204_UART_ISR_TXRDY)
	    m5204IntWr (pChan);
	if (intStatus & M5204_UART_ISR_RXRDY)
	    m5204IntRd (pChan);
	if (pChan->mode != SIO_MODE_INT)
	    break;
	}
    }
