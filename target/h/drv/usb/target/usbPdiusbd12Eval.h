/* usbPdiusbd12Eval.h - Definitions for Philips PDIUSBD12 eval board */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01a,05aug99,rcb  First.
*/

/*
DESCRIPTION

Defines constants related to the Philips PDIUSBD12 evaluation board.
*/

#ifndef __INCusbPdiusbd12Evalh
#define __INCusbPdiusbd12Evalh

#ifdef	__cplusplus
extern "C" {
#endif


/* defines */

/* evaluation board I/O address offsets */

#define D12EVAL_D12REG	    0	    /* base of PDIUSBD12 registers */
#define D12EVAL_GIN_REG     2	    /* general input register */
#define D12EVAL_GOUT_REG    3	    /* general output register */


/* default settings for I/O, IRQ, DMA */

#define D12EVAL_DEFAULT_IOBASE	    0x368   /* default base I/O address */
#define D12EVAL_DEFAULT_IRQ	    5	    /* default IRQ channel */
#define D12EVAL_DEFAULT_DMA	    3	    /* default DMA channel */


/* general input port */

#define D12EVAL_GIN_S1		    0x01    /* key S1, '0' for pressed */
#define D12EVAL_GIN_S2		    0x02    /* key S2, '0' for pressed */
#define D12EVAL_GIN_S3		    0x04    /* key S3, '0' for pressed */
#define D12EVAL_GIN_S4		    0x08    /* key S4, '0' for pressed */
#define D12EVAL_GIN_GOODLINK	    0x10    /* D12 GoodLink pin state */
#define D12EVAL_BUS_POWER	    0x20    /* USB bus power state */
					    /* '1' for USB VBUS present */
#define D12EVAL_SUSPEND 	    0x40    /* D12 SUSPEND pin state */
#define D12EVAL_INTN		    0x80    /* D12 INT_N pin state */


/* general output port */

#define D12EVAL_GOUT_LED_D2	    0x01    /* LED D2, '1' lights up LED */
#define D12EVAL_GOUT_LED_D3	    0x02    /* LED D3, '1' lights up LED */
#define D12EVAL_GOUT_LED_D4	    0x04    /* LED D4, '1' lights up LED */
#define D12EVAL_GOUT_LED_D5	    0x08    /* LED D5, '1' lights up LED */
#define D12EVAL_GOUT_SUSPEND	    0x40    /* suspend control */
					    /* '1' forces D12 SUSPEND low */
#define D12EVAL_GOUT_INTENB	    0x80    /* interrupt enable */
					    /* '1' enables interrupt */


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbPdiusbd12Evalh */


/* End of file. */

