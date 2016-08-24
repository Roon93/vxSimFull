/* dbgStrLib.h - Debug Store mechanism definitions header file */

/* Copyright 2001-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,23jul01,hdn  written
*/

#ifndef	__INCdbgStrLibh
#define	__INCdbgStrLibh

#ifdef __cplusplus
extern "C" {
#endif


#include "arch/i86/regsP7.h"


/* DS buffer : BTS/PEBS minimum/default/threshold size */

#define	BTS_NBYTES_MIN	0x1000		/*  4KB minimum BTS buffer */
#define	BTS_NBYTES_DEF	0x4000		/* 16KB default BTS buffer */
#define	BTS_NBYTES_OFF	0x200		/* .5KB (max - threshold) BTS */
#define	PEBS_NBYTES_MIN	0x1000		/*  4KB minimum PEBS buffer */
#define	PEBS_NBYTES_DEF	0x4000		/* 16KB default PEBS buffer */
#define	PEBS_NBYTES_OFF	0x200		/* .5KB (max - threshold) PEBS */
#define PEBS_DEF_RESET	-1LL		/* default reset counter value */

/* BTS : record bit */

#define BTS_PREDICTED	0x00000010	/* bit4: Branch Predicted */

/* PEBS : events */

#define	PEBS_FRONT_END			0x01
#define	PEBS_EXECUTION			0x02
#define	PEBS_REPLAY			0x03

/* PEBS : metrics (Front End Event) */

#define	PEBS_MEMORY_LOADS		0x01
#define	PEBS_MEMORY_STORES		0x02

/* PEBS : metrics (Execution Event) */

#define	PEBS_PACKED_SP			0x03
#define	PEBS_PACKED_DP			0x04
#define	PEBS_SCALAR_SP			0x05
#define	PEBS_SCALAR_DP			0x06
#define	PEBS_128BIT_MMX			0x07
#define	PEBS_64BIT_MMX			0x08
#define	PEBS_X87_FP			0x09
#define	PEBS_X87_SIMD_MOVES		0x0a

/* PEBS : metrics (Replay Event) */

#define	PEBS_1STL_CACHE_LOAD_MISS	0x0b
#define	PEBS_2NDL_CACHE_LOAD_MISS	0x0c
#define	PEBS_DTLB_LOAD_MISS		0x0d
#define	PEBS_DTLB_STORE_MISS		0x0e
#define	PEBS_DTLB_ALL_MISS		0x0f
#define	PEBS_MOB_LOAD_REPLAY		0x10
#define	PEBS_SPLIT_LOAD			0x11
#define	PEBS_SPLIT_STORE		0x12

/* PEBS : ESCR, CCCR definitions (Front End Event) */

#define ESCR_FRONT_END					\
	(((0x08 << DS_SHIFT_25) & ESCR_EVENT_SELECT) |	\
	 ((DS_BIT_1 << DS_SHIFT_9) & ESCR_EVENT_MASK) |	\
	 ESCR_TAG_ENABLE)

#define CCCR_FRONT_END					\
	(((0x05 << DS_SHIFT_13) & CCCR_ESCR_SELECT) |	\
	 CCCR_RSVD)

#define	ESCR_MEMORY_LOADS				\
	(((0x02 << DS_SHIFT_25) & ESCR_EVENT_SELECT) |	\
	 ((DS_BIT_1 << DS_SHIFT_9) & ESCR_EVENT_MASK) |	\
	 ESCR_TAG_ENABLE)
	
#define	ESCR_MEMORY_STORES				\
	(((0x02 << DS_SHIFT_25) & ESCR_EVENT_SELECT) |	\
	 ((DS_BIT_2 << DS_SHIFT_9) & ESCR_EVENT_MASK) |	\
	 ESCR_TAG_ENABLE)

/* PEBS : ESCR, CCCR definitions (Execution Event) */

#define ESCR_EXECUTION					\
	(((0x0c << DS_SHIFT_25) & ESCR_EVENT_SELECT) |	\
	 ((DS_BIT_0 << DS_SHIFT_9) & ESCR_EVENT_MASK) |	\
	 ((DS_BIT_0 << DS_SHIFT_5) & ESCR_TAG_VALUE) |	\
	 ESCR_TAG_ENABLE)

#define CCCR_EXECUTION					\
	(((0x05 << DS_SHIFT_13) & CCCR_ESCR_SELECT) |	\
	 CCCR_RSVD)

#define ESCR_PACKED_SP					\
	(((0x08 << DS_SHIFT_25) & ESCR_EVENT_SELECT) |	\
	 ((DS_BIT_15 << DS_SHIFT_9) & ESCR_EVENT_MASK) |\
	 ((DS_BIT_0 << DS_SHIFT_5) & ESCR_TAG_VALUE) |	\
	 ESCR_TAG_ENABLE)

#define ESCR_PACKED_DP					\
	(((0x0c << DS_SHIFT_25) & ESCR_EVENT_SELECT) |	\
	 ((DS_BIT_15 << DS_SHIFT_9) & ESCR_EVENT_MASK) |\
	 ((DS_BIT_0 << DS_SHIFT_5) & ESCR_TAG_VALUE) |	\
	 ESCR_TAG_ENABLE)

#define ESCR_SCALAR_SP					\
	(((0x0a << DS_SHIFT_25) & ESCR_EVENT_SELECT) |	\
	 ((DS_BIT_15 << DS_SHIFT_9) & ESCR_EVENT_MASK) |\
	 ((DS_BIT_0 << DS_SHIFT_5) & ESCR_TAG_VALUE) |	\
	 ESCR_TAG_ENABLE)

#define ESCR_SCALAR_DP					\
	(((0x0e << DS_SHIFT_25) & ESCR_EVENT_SELECT) |	\
	 ((DS_BIT_15 << DS_SHIFT_9) & ESCR_EVENT_MASK) |\
	 ((DS_BIT_0 << DS_SHIFT_5) & ESCR_TAG_VALUE) |	\
	 ESCR_TAG_ENABLE)

#define ESCR_64BIT_MMX					\
	(((0x02 << DS_SHIFT_25) & ESCR_EVENT_SELECT) |	\
	 ((DS_BIT_15 << DS_SHIFT_9) & ESCR_EVENT_MASK) |\
	 ((DS_BIT_0 << DS_SHIFT_5) & ESCR_TAG_VALUE) |	\
	 ESCR_TAG_ENABLE)

#define ESCR_128BIT_MMX					\
	(((0x1a << DS_SHIFT_25) & ESCR_EVENT_SELECT) |	\
	 ((DS_BIT_15 << DS_SHIFT_9) & ESCR_EVENT_MASK) |\
	 ((DS_BIT_0 << DS_SHIFT_5) & ESCR_TAG_VALUE) |	\
	 ESCR_TAG_ENABLE)

#define ESCR_X87_FP					\
	(((0x04 << DS_SHIFT_25) & ESCR_EVENT_SELECT) |	\
	 ((DS_BIT_15 << DS_SHIFT_9) & ESCR_EVENT_MASK) |\
	 ((DS_BIT_0 << DS_SHIFT_5) & ESCR_TAG_VALUE) |	\
	 ESCR_TAG_ENABLE)

#define ESCR_X87_SIMD_MOVES				\
	(((0x2e << DS_SHIFT_25) & ESCR_EVENT_SELECT) |	\
	 (((DS_BIT_3 | DS_BIT_4) << DS_SHIFT_9) & ESCR_EVENT_MASK) | \
	 ((DS_BIT_0 << DS_SHIFT_5) & ESCR_TAG_VALUE) |	\
	 ESCR_TAG_ENABLE)

/* PEBS : ESCR, CCCR definitions (Replay Event) */

#define ESCR_REPLAY					\
	(((0x09 << DS_SHIFT_25) & ESCR_EVENT_SELECT) |	\
	 ((DS_BIT_0 << DS_SHIFT_9) & ESCR_EVENT_MASK) |	\
	 ((DS_BIT_0 << DS_SHIFT_5) & ESCR_TAG_VALUE) |	\
	 ESCR_TAG_ENABLE)

#define CCCR_REPLAY					\
	(((0x05 << DS_SHIFT_13) & CCCR_ESCR_SELECT) |	\
	 CCCR_RSVD)

#define ESCR_MOB_LOAD_REPLAY				\
	(((0x03 << DS_SHIFT_25) & ESCR_EVENT_SELECT) |	\
	 (((DS_BIT_4 | DS_BIT_5) << DS_SHIFT_9) & ESCR_EVENT_MASK) |	\
	 ESCR_TAG_ENABLE)

#define ESCR_SPLIT_LOAD					\
	(((0x04 << DS_SHIFT_25) & ESCR_EVENT_SELECT) |	\
	 ((DS_BIT_1 << DS_SHIFT_9) & ESCR_EVENT_MASK) |	\
	 ESCR_TAG_ENABLE)

#define ESCR_SPLIT_STORE				\
	(((0x05 << DS_SHIFT_25) & ESCR_EVENT_SELECT) |	\
	 ((DS_BIT_1 << DS_SHIFT_9) & ESCR_EVENT_MASK) |	\
	 ESCR_TAG_ENABLE)


/* DS : MSR(ESCR) bit definition */

#define	DS_BIT_0			0x00000001
#define	DS_BIT_1			0x00000002
#define	DS_BIT_2			0x00000004
#define	DS_BIT_3			0x00000008
#define	DS_BIT_4			0x00000010
#define	DS_BIT_5			0x00000020
#define	DS_BIT_6			0x00000040
#define	DS_BIT_7			0x00000080
#define	DS_BIT_8			0x00000100
#define	DS_BIT_9			0x00000200
#define	DS_BIT_10			0x00000400
#define	DS_BIT_11			0x00000800
#define	DS_BIT_12			0x00001000
#define	DS_BIT_13			0x00002000
#define	DS_BIT_14			0x00004000
#define	DS_BIT_15			0x00008000
#define	DS_BIT_16			0x00010000
#define	DS_BIT_17			0x00020000
#define	DS_BIT_18			0x00040000
#define	DS_BIT_19			0x00080000
#define	DS_BIT_20			0x00100000
#define	DS_BIT_21			0x00200000
#define	DS_BIT_22			0x00400000
#define	DS_BIT_23			0x00800000
#define	DS_BIT_24			0x01000000
#define	DS_BIT_25			0x02000000
#define	DS_BIT_26			0x04000000
#define	DS_BIT_27			0x08000000
#define	DS_BIT_28			0x10000000
#define	DS_BIT_29			0x20000000
#define	DS_BIT_30			0x40000000
#define	DS_BIT_31			0x80000000

/* DS : MSR(ESCR, CCCR) shift count definition */

#define	DS_SHIFT_1			1
#define	DS_SHIFT_2			2
#define	DS_SHIFT_3			3
#define	DS_SHIFT_4			4
#define	DS_SHIFT_5			5
#define	DS_SHIFT_6			6
#define	DS_SHIFT_7			7
#define	DS_SHIFT_8			8
#define	DS_SHIFT_9			9
#define	DS_SHIFT_10			10
#define	DS_SHIFT_11			11
#define	DS_SHIFT_12			12
#define	DS_SHIFT_13			13
#define	DS_SHIFT_14			14
#define	DS_SHIFT_15			15
#define	DS_SHIFT_16			16
#define	DS_SHIFT_17			17
#define	DS_SHIFT_18			18
#define	DS_SHIFT_19			19
#define	DS_SHIFT_20			20
#define	DS_SHIFT_21			21
#define	DS_SHIFT_22			22
#define	DS_SHIFT_23			23
#define	DS_SHIFT_24			24
#define	DS_SHIFT_25			25
#define	DS_SHIFT_26			26
#define	DS_SHIFT_27			27
#define	DS_SHIFT_28			28
#define	DS_SHIFT_29			29
#define	DS_SHIFT_30			30
#define	DS_SHIFT_31			31


#ifndef	_ASMLANGUAGE


typedef struct btsRec		/* BTS (Branch Trace Store) record */
    {
    UINT32	pcFm;		/* Last Branch From */
    UINT32	pcTo;		/* Last Branch To */
    UINT32	misc;		/* bit4: Branch Predicted */
    } BTS_REC;

typedef struct pebsRec		/* PEBS (Precise Event Based Sampling) rec */
    {
    UINT32	eflags;		/* EFLAGS */
    UINT32	eip;		/* Linear IP */
    UINT32	eax;		/* EAX */
    UINT32	ebx;		/* EBX */
    UINT32	ecx;		/* ECX */
    UINT32	edx;		/* EDX */
    UINT32	esi;		/* ESI */
    UINT32	edi;		/* EDI */
    UINT32	ebp;		/* EBP */
    UINT32	esp;		/* ESP */
    } PEBS_REC;

typedef struct dsBufHeader	/* DS (Debug Store) buffer management */
    {
    BTS_REC *	btsBase;	/* BTS buffer base */
    BTS_REC *	btsIndex;	/* BTS index */
    UINT32	btsMax;		/* BTS absolute maximum */
    UINT32	btsThreshold;	/* BTS interrupt threshold */
    PEBS_REC *	pebsBase;	/* PEBS buffer base */
    PEBS_REC *	pebsIndex;	/* PEBS index */
    UINT32	pebsMax;	/* PEBS absolute maximum */
    UINT32	pebsThreshold;	/* PEBS interrupt threshold */
    LL_INT	pebsCtr;	/* PEBS counter reset value */
    UINT32	reserved[3];	/* reserved */
    } DS_BUF_HEADER;

typedef struct dsConfig		/* DS (Debug Store) configuration */
    {
    UINT32	btsNbytes;	/* BTS buffer size */
    BOOL	btsAvailable;	/* TRUE if BTS is available */
    BOOL	btsEnabled;	/* TRUE if BTS is enabled */
    BOOL	btsIntMode;	/* TRUE for interrupt mode */
    BOOL	btsBufMode;	/* TRUE to store BTMs */
    UINT32	pebsNbytes;	/* PEBS buffer size */
    BOOL	pebsAvailable;	/* TRUE if PEBS is available */
    BOOL	pebsEnabled;	/* TRUE if PEBS is enabled */
    INT32	pebsEvent;	/* PEBS event */
    INT32	pebsMetric;	/* PEBS metric */
    BOOL	pebsOs;		/* PEBS os */
    LL_INT	pebsCtr;	/* PEBS counter */
    DS_BUF_HEADER * pH;		/* DS buffer header */
    } DS_CONFIG;


/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

extern STATUS	dbgStrLibInit		(UINT32 nBytesBts, UINT32 nBytesPebs,
					 BOOL sysMode);
extern DS_BUF_HEADER * dbgStrBufInit	(DS_BUF_HEADER * pH, 
					 BTS_REC * btsBufAddr,
			   		 UINT32 btsMaxOffset, 
					 UINT32 btsIntOffset,
			   		 PEBS_REC * pebsBufAddr,
			   		 UINT32 pebsMaxOffset, 
					 UINT32 pebsIntOffset);
extern STATUS	dbgStrBufAlloc		(WIND_TCB * pTcb);
extern STATUS	dbgStrBufFree		(WIND_TCB * pTcb);
extern STATUS	dbgStrConfig		(WIND_TCB * pTcb,
					 BOOL  btsEnable,  BOOL  pebsEnable, 
					 BOOL  btsIntMode, BOOL  btsBufMode, 
					 INT32 pebsEvent,  INT32 pebsMetric, 
					 BOOL  pebsOs,  LL_INT * pPebsValue);
extern STATUS	dbgStrStart		(WIND_TCB * pTcb);
extern STATUS	dbgStrStop		(WIND_TCB * pTcb);
extern STATUS	dbgStrBtsModeSet	(BOOL intMode, BOOL bufMode);
extern BOOL	dbgStrBtsEnable		(BOOL enable);
extern STATUS	dbgStrPebsModeSet	(INT32 event, INT32 metric, BOOL os,
					 LL_INT * pValue);
extern BOOL	dbgStrPebsEnable	(BOOL enable);

extern void	dbgStrShowInit		(void);
extern void	dbgStrShow		(WIND_TCB * pTcb, int type);

#else	/* __STDC__ */

extern STATUS	dbgStrLibInit		();
extern DS_BUF_HEADER * dbgStrBufInit	();
extern STATUS	dbgStrBufAlloc		();
extern STATUS	dbgStrBufFree		();
extern STATUS	dbgStrConfig		();
extern STATUS	dbgStrStart		();
extern STATUS	dbgStrStop		();
extern STATUS	dbgStrBtsModeSet	();
extern BOOL	dbgStrBtsEnable		();
extern STATUS	dbgStrPebsModeSet	();
extern BOOL	dbgStrPebsEnable	();

extern void	dbgStrShowInit		();
extern void	dbgStrShow		();

#endif	/* __STDC__ */


#endif	/* _ASMLANGUAGE */


#ifdef __cplusplus
}
#endif

#endif	/* __INCdbgStrLibh */
