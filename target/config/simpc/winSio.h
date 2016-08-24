/* winSio.h - header file for template serial driver */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,13nov01,jmp  moved from target/src/drv to simpc bsp.
01a,12sep97,cym  written based on templateSio.h.
*/

#ifndef __INCwinSioh
#define __INCwinSioh

#include "sioLib.h"

/* device and channel structures */

typedef struct
    {
    /* must be first */

    SIO_CHAN		sio;		/* standard SIO_CHAN element */

    /* callbacks */

    STATUS	        (*getTxChar) ();
    STATUS	        (*putRcvChar) ();
    void *	        getTxArg;
    void *	        putRcvArg;

    /* register addresses */

    /* misc */

    int                 mode;           /* current mode (interrupt or poll) */
    int                 baudFreq;       /* input clock frequency */
    int			options;	/* Hardware options */
    int			transmitting;   /* used to mock transmit interrupts */
    } WIN_CHAN;

/* function prototypes */

#if defined(__STDC__)

extern void 	winDevInit	(WIN_CHAN *pChan); 
extern void 	winDevInit2	(WIN_CHAN *pChan); 
extern void	winIntRcv	(WIN_CHAN *pChan, UINT16 inChar);
extern void	winIntTx	(WIN_CHAN *pChan);
extern void	winIntErr	(WIN_CHAN *pChan);

#else   /* __STDC__ */

extern void 	winDevInit	();
extern void 	winDevInit2	();
extern void	winIntRcv	();
extern void	winIntTx	();
extern void	winIntErr	();

#endif  /* __STDC__ */

#endif  /* __INCwinSioh */
