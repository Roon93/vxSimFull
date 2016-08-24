/* usbPdiusbd12.h - Definitions for Philips PDIUSBD12 USB target controller */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01a,05aug99,rcb  First.
*/

/*
DESCRIPTION

Defines constants related to the Philips PDIUSBD12 USB device (target) IC.
*/


#ifndef __INCusbPdiusbd12h
#define __INCusbPdiusbd12h

#ifdef	__cplusplus
extern "C" {
#endif


/* defines */

/* PDIUSBD12 signature (read through Read Chip Id command) 
 *
 * NOTE: The value of the signature has been determined empirically.  The
 * official data sheet for the PDIUSBD12 does not discuss the Read Chip ID
 * command, but sample code from Philips reveals the existence of the 
 * command.
 */

#define D12_CHIP_ID_MASK		0xffff
#define D12_CHIP_ID			0x1012


/* PDIUSBD12 I/O registers */

#define D12_DATA_REG			0	/* data register (r/w) */
#define D12_CMD_REG			1	/* command register (write only) */


/* PDIUSBD12 endpoints */

#define D12_NUM_ENDPOINTS		6	/* number of endpoints */

#define D12_ENDPOINT_CONTROL_OUT	0	/* control OUT endpoint */
#define D12_ENDPOINT_CONTROL_IN 	1	/* control IN endpoint */
#define D12_ENDPOINT_1_OUT		2	/* endpoint 1 OUT */
#define D12_ENDPOINT_1_IN		3	/* endpoint 1 IN */
#define D12_ENDPOINT_2_OUT		4	/* endpoint 2 OUT */
#define D12_ENDPOINT_2_IN		5	/* endpoint 2 IN */


/* max endpoint packet lengths */

#define D12_MAX_PKT_CONTROL		16
#define D12_MAX_PKT_ENDPOINT_1		16
#define D12_MAX_PKT_ENDPOINT_2_NON_ISO	64	/* mode 0 */
#define D12_MAX_PKT_ENDPOINT_2_ISO_OUT	128	/* mode 1 */
#define D12_MAX_PKT_ENDPOINT_2_ISO_IN	128	/* mode 2 */
#define D12_MAX_PKT_ENDPOINT_2_ISO_IO	64	/* mode 3 */


/* PDIUSBD12 commands */
						/* data phase */

#define D12_CMD_SET_ADDRESS		0xd0	/* write 1 byte */
#define D12_CMD_SET_ENDPOINT_ENABLE	0xd8	/* write 1 byte */
#define D12_CMD_SET_MODE		0xf3	/* write 2 bytes */
#define D12_CMD_SET_DMA 		0xfb	/* write/read 1 byte */
#define D12_CMD_READ_INTERRUPT_REG	0xf4	/* read 2 bytes */
#define D12_CMD_READ_BUFFER		0xf0	/* read n bytes */
#define D12_CMD_WRITE_BUFFER		0xf0	/* write n bytes */
#define D12_CMD_ACK_SETUP		0xf1	/* none */
#define D12_CMD_CLEAR_BUFFER		0xf2	/* none */
#define D12_CMD_VALIDATE_BUFFER 	0xfa	/* none */
#define D12_CMD_SEND_RESUME		0xf6	/* none */
#define D12_CMD_READ_CURRENT_FRAME_NO	0xf5	/* read 1 or 2 bytes */
#define D12_CMD_READ_CHIP_ID		0xfd	/* read 2 bytes */

/* following cmds must be OR'd with endpoint selector: D12_ENDPOINT_xxxx */

#define D12_CMD_SELECT_ENDPOINT 	0x00	/* read 1 byte (optional) */
#define D12_CMD_READ_LAST_TRANS_STATUS	0x40	/* read 1 byte */
#define D12_CMD_SET_ENDPOINT_STATUS	0x40	/* write 1 byte */
#define D12_CMD_READ_ENDPOINT_STATUS	0x80	/* read 1 byte */


/* set address command (write 1 byte) */

#define D12_CMD_SA_ADRS_MASK		0x7f	/* address */
#define D12_CMD_SA_ENABLE		0x80	/* enable */


/* set endpoint enable command (write 1 byte) */

#define D12_CMD_SEE_ENABLE		0x01	/* generic/isoch endpt enable */


/* set mode command (write 2 bytes) */

/* first byte... */

#define D12_CMD_SM_CFG_NO_LAZYCLOCK	0x02	/* no lazyclock */
#define D12_CMD_SM_CFG_CLOCK_RUNNING	0x04	/* clock running */
#define D12_CMD_SM_CFG_INTERRUPT_MODE	0x08	/* interrupt mode */
#define D12_CMD_SM_CFG_SOFTCONNECT	0x10	/* SoftConnect */

#define D12_CMD_SM_CFG_MODE_MASK	0xc0
#define D12_CMD_SM_CFG_MODE0_NON_ISO	0x00	/* mode 0, non-iso */
#define D12_CMD_SM_CFG_MODE1_ISO_OUT	0x40	/* mode 1, iso-out */
#define D12_CMD_SM_CFG_MODE2_ISO_IN	0x80	/* mode 2, iso-in */
#define D12_CMD_SM_CFG_MODE3_ISO_IO	0xc0	/* mode 3, iso-io */

/* second byte... */

#define D12_CMD_SM_CLK_DIV_FACTOR_MASK	0x0f	/* clock division factor */
#define D12_CMD_SM_CLK_DIV_DEFAULT	0x0b	/* default = 11 */
#define D12_CMD_SM_CLK_SET_TO_ONE	0x40	/* bit must be set to 1 */
#define D12_CMD_SM_CLK_SOF_ONLY_INTRPT	0x80	/* SOF-only interrupt */


/* set dma command (read/write 1 byte) */

#define D12_CMD_SD_DMA_SINGLE_CYCLE	0x00	/* single cycle DMA */
#define D12_CMD_SD_DMA_BURST_4		0x01	/* burst (4 cycle) DMA */
#define D12_CMD_SD_DMA_BURST_8		0x02	/* burst (8 cycle) DMA */
#define D12_CMD_SD_DMA_BURST_16 	0x03	/* burst (16 cycle) DMA */
#define D12_CMD_SD_DMA_ENABLE		0x04	/* dma enable */
#define D12_CMD_SD_DMA_DIRECTION_MASK	0x08	/* mask for direction */
#define D12_CMD_SD_DMA_DIRECTION_WRITE	0x08	/* '1' = dma write */
#define D12_CMD_SD_DMA_DIRECTION_READ	0x00	/* '0' = dma read */
#define D12_CMD_SD_AUTO_RELOAD		0x10	/* dma auto restart */
#define D12_CMD_SD_SOF_INTRPT		0x20	/* enable SOF interrupt */
#define D12_CMD_SD_ENDPT_2_OUT_INTRPT	0x40	/* intrpt when valid packet */
#define D12_CMD_SD_ENDPT_2_IN_INTRPT	0x80	/* intrpt when valid packet */


/* read interrupt register (read 2 bytes) */

/* first byte... */

#define D12_CMD_RIR_CONTROL_OUT 	0x0001	/* control out endpoint */
#define D12_CMD_RIR_CONTROL_IN		0x0002	/* control in endpoint */
#define D12_CMD_RIR_ENDPOINT_1_OUT	0x0004	/* endpoint 1 OUT */
#define D12_CMD_RIR_ENDPOINT_1_IN	0x0008	/* endpoint 1 IN */
#define D12_CMD_RIR_ENDPOINT_2_OUT	0x0010	/* endpoint 2 OUT (main OUT) */
#define D12_CMD_RIR_ENDPOINT_2_IN	0x0020	/* endpoint 2 IN (main IN) */
#define D12_CMD_RIR_BUS_RESET		0x0040	/* bus reset */
#define D12_CMD_RIR_SUSPEND		0x0080	/* suspend change */

/* second byte... */

#define D12_CMD_RIR_DMA_EOT		0x0100	/* DMA EOT */


/* select endpoint command (optional read 1 byte) */

#define D12_CMD_SE_FULL_EMPTY		0x01	/* '1' = bfr full, '0' = empty */
#define D12_CMD_SE_STALL		0x02	/* '1' = endpoint stall */


/* read last transaction status command (read 1 byte) */

#define D12_CMD_RLTS_DATA_SUCCESS	0x01	/* xmit/rcv data success */
#define D12_CMD_RLTS_SETUP_PACKET	0x20	/* Setup packet */
#define D12_CMD_RLTS_DATA1		0x40	/* DATA0/DATA1 indicator */
#define D12_CMD_RLTS_STATUS_SKIPPED	0x80	/* previous status not read */

#define D12_CMD_RLTS_ERROR_CODE_MASK	0x1e	/* error code */
#define D12_CMD_RLTS_ERROR_CODE_SHIFT	1

#define D12_CMD_RLTS_ERROR_CODE(b)  \
    (((b) & D12_CMD_RLTS_ERROR_CODE_MASK) >> D12_CMD_RLTS_ERROR_CODE_SHIFT)

#define D12_CMD_RLTS_ERROR_NONE 	0x0	/* no error */
#define D12_CMD_RLTS_ERROR_PID_ENCODE	0x1	/* PID encoding error */
#define D12_CMD_RLTS_ERROR_PID_UNKNOWN	0x2	/* unknown PID */
#define D12_CMD_RLTS_ERROR_EXPECTED_PKT 0x3	/* unexpected packet */
#define D12_CMD_RLTS_ERROR_TOKEN_CRC	0x4	/* token CRC error */
#define D12_CMD_RLTS_ERROR_DATA_CRC	0x5	/* data CRC error */
#define D12_CMD_RLTS_ERROR_TIME_OUT	0x6	/* time out error */
#define D12_CMD_RLTS_ERROR_END_OF_PKT	0x8	/* unexpected end of packet */
#define D12_CMD_RLTS_ERROR_NAK		0x9	/* sent or received NAK */
#define D12_CMD_RLTS_ERROR_STALL	0xa	/* sent stall */
#define D12_CMD_RLTS_ERROR_OVERFLOW	0xb	/* packet too long for bfr */
#define D12_CMD_RLTS_ERROR_BITSTUFF	0xd	/* bitstuff error */
#define D12_CMD_RLTS_ERROR_DATA_TOGGLE	0xf	/* wrong data toggle */


/* set endpoint status command (write 1 byte) */

#define D12_CMD_SES_STALLED		0x01	/* stalled */


/* read current frame number command (read 1 or 2 bytes) */

/* LSB is in first byte (8 bits) , MSB is in second byte (3 bits).  
 * 11 bits total. */

#define D12_CMD_RCFN_MSB_MASK		0x07	/* mask for MSB */


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbPdiusbd12h */


/* End of file. */

