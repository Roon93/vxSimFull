/* usbIsaLib.h - System-specific ISA Functions */

/* Copyright 2000-2001 Wind River Systems, Inc. */

/*
Modification history
--------------------
01c,21aug01,hdn  added PENTIUM2/3/4 support
01b,17mar00,rcb  Change name of #include from "i8237dma.h" to
		 "i8237Dma.h" to reflect case-sensitive nature of
		 filenames on unix build platforms.
01a,11aug99,rcb  First.
*/

/*
DESCRIPTION

This file defines platform-independent functions accessing ISA bus 
capabilities.  These functions allow ISA device drivers to be written 
independent of the underlying O/S's ISA access mechanisms.
*/

#ifndef __INCusbIsaLibh
#define __INCusbIsaLibh


/* Includes */

#include "ioLib.h"		    /* defines O_RDONLY/O_WRONLY */

#include "usb/usbPciLib.h"


/* defines */

/* Define USE_PCI_INT_FUNCS to map USB_ISA_INT_CONNECT() and 
 * USB_ISA_INT_RESTORE() to the corresponding functions in usbPciLib.c.
 * The PCI functions have the advantage that they define both an
 * "intConnect()" as well as an "intDisconnect()" function, allowing
 * interrupts to be unhooked effectively.
 */

#define USE_PCI_INT_FUNCS


/* platform-dependent definitions */

#if	(CPU_FAMILY==I80X86)

/* Pentium (e.g, PC Pentium) */

/* The 8237 DMA controller is enabled using a function called dmaSetup().
 * A prototype for this function exists in the vxWorks header file below.
 */

#include "drv/dma/i8237Dma.h"

#define DMA_SETUP(direction, pAddr, nBytes, chan) \
    dmaSetup (direction, pAddr, nBytes, chan)

/* Unknown platform */

#else

#warning    "Unknown platform.	Must create platform mapping in usbIsaLib.h."

#define DMA_SETUP(direction, pAddr, nBytes, chan)   (OK)

#endif	/* #if CPU == xxxx */


#ifdef	__cplusplus
extern "C" {
#endif


/* typedefs */

/* I/O functions */

#define USB_ISA_BYTE_IN(address)	    USB_PCI_BYTE_IN(address)
#define USB_ISA_BYTE_OUT(address, value)    USB_PCI_BYTE_OUT(address, value)


/* memory mapping functions */

#define USB_MEM_TO_ISA(pMem)		    USB_MEM_TO_PCI(pMem)
#define USB_ISA_TO_MEM(isaAdrs) 	    USB_PCI_TO_MEM(isaAdrs)


/* cache functions */

#define USB_ISA_MEM_FLUSH(pMem, size)	    USB_PCI_MEM_FLUSH(pMem, size)
#define USB_ISA_MEM_INVALIDATE(pMem, size)  USB_PCI_MEM_INVALIDATE(pMem, size)


/* interrupt functions */

#ifdef	USE_PCI_INT_FUNCS

#define USB_ISA_INT_CONNECT(func, param, intNo) \
    usbPciIntConnect (func, param, intNo)
#define USB_ISA_INT_RESTORE(func, param, intNo)     \
    usbPciIntRestore (func, param, intNo)

#else

#define USB_ISA_INT_CONNECT(func, param, intNo) \
    usbIsaIntConnect (func, param, intNo)
#define USB_ISA_INT_RESTORE(func, param, intNo)     \
    usbIsaIntRestore (func, param, intNo)

#endif	/* #ifdef USE_PCI_INT_FUNCS */


/* DMA functions 
 *
 * NOTE: WRS convention for DMA transfer is that a "read from memory"
 * by the DMAC (what we call DMA_MEM_READ) is actually the direction
 * O_WRONLY. 
 */

#define DMA_MEM_READ	0
#define DMA_MEM_WRITE	1

#define USB_ISA_DMA_SETUP(direction, isaAdrs, bfrLen, channel) \
    DMA_SETUP ((direction == DMA_MEM_READ) ? O_WRONLY : O_RDONLY, \
	(void *) isaAdrs, bfrLen, channel)


#ifndef USE_PCI_INT_FUNCS

/* function prototypes */

STATUS usbIsaIntConnect
    (
    INT_HANDLER_PROTOTYPE func,     /* new interrupt handler */
    pVOID param,		    /* parameter for int handler */
    UINT16 intNo		    /* interrupt vector number */
    );


VOID usbIsaIntRestore
    (
    INT_HANDLER_PROTOTYPE func,     /* int handler to be removed */
    pVOID param,		    /* parameter for int handler */
    UINT16 intNo		    /* interrupt vector number */
    );

#endif	/* #ifndef USE_PCI_INT_FUNCS */


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbIsaLibh */


/* End of file. */


