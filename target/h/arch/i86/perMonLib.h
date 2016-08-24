/* perMonLib.h - Performance Monitoring mechanism definitions header file */

/* Copyright 2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,30jul01,hdn  written
*/

#ifndef	__INCperMonLibh
#define	__INCperMonLibh

#ifdef __cplusplus
extern "C" {
#endif


/* performance monitoring, number of counters, etc */

#define	PM_N_COUNTER			18
#define	EVENT_N_ESCR			2
#define EVENT_N_COUNTER			3
#define	MSR_N_ESCR			8

/* performance monitoring event name/no */

#define	PM_BRANCH_RETIRED		0
#define	PM_MISPRED_BRANCH_RETIRED	1
#define	PM_TC_DELIVER_MODE		2
#define	PM_BPU_FETCH_REQUEST		3
#define	PM_ITLB_REFERENCE		4
#define	PM_MEMORY_CANCEL		5
#define	PM_MEMORY_COMPLETE		6
#define	PM_LOAD_PORT_REPLAY		7
#define	PM_STORE_PORT_REPLAY		8
#define	PM_MOB_LOAD_REPLAY		9
#define	PM_PAGE_WALK_TYPE		10
#define	PM_BSQ_2NDL_CACHE_REF		11
#define	PM_IOQ_ALLOCATION		12
#define	PM_FSB_DATA_ACTIVITY		13
#define	PM_BSQ_ALLOCATION		14
#define	PM_X87_ASSIST			15
#define	PM_SSE_INPUT_ASSIST		16
#define	PM_PACKED_SP_UOP		17
#define	PM_PACKED_DP_UOP		18
#define	PM_SCALAR_SP_UOP		19
#define	PM_SCALAR_DP_UOP		20
#define	PM_64BIT_MMX_UOP		21
#define	PM_128BIT_MMX_UOP		22
#define	PM_X87_FP_UOP			23
#define	PM_X87_SIMD_MOVES_UOP		24
#define	PM_MACHINE_CLEAR		25
#define	PM_FRONT_END_EVENT		26
#define	PM_EXECUTION_EVENT		27
#define	PM_REPLAY_EVENT			28
#define	PM_INSTR_RETIRED		29
#define	PM_UOPS_RETIRED			30

/* metrics available for Front_end Tagging */

#define	FRONT_MEMORY_LOADS		0
#define	FRONT_MEMORY_STORES		1

/* metrics available for Execution Tagging */

#define	EXEC_PACKED_SP_RETIRED		0
#define	EXEC_PACKED_DP_RETIRED		1
#define	EXEC_SCALAR_SP_RETIRED		2
#define	EXEC_SCALAR_DP_RETIRED		3
#define	EXEC_128BIT_MMX_RETIRED		4
#define	EXEC_64BIT_MMX_RETIRED		5
#define	EXEC_X87_FP_RETIRED		6
#define	EXEC_X87_SIMD_MM_RETIRED	7

/* metrics available for Replay Tagging */

#define	REPLAY_1CACHE_LOAD_MISS_RETIRED	0
#define	REPLAY_2CACHE_LOAD_MISS_RETIRED	1
#define	REPLAY_DTLB_LOAD_MISS_RETIRED	2
#define	REPLAY_DTLB_STORE_MISS_RETIRED	3
#define	REPLAY_DTLB_ALL_MISS_RETIRED	4
#define	REPLAY_MOB_LOAD_REPLAY_RETIRED	5
#define	REPLAY_SPLIT_LOAD_RETIRED	6
#define	REPLAY_SPLIT_STORE_RETIRED	7

/* event mask available for Replay Tagging */

#define	PARTIAL_DATA			0x10
#define	UNALIGN_ADDR			0x20
#define	SPLIT_LD			0x02
#define	SPLIT_ST			0x02

/* CCCR bit definitions */

#define	CCCR_ENABLE			0x00001000
#define	CCCR_ENABLE_NOT			0xffffefff
#define	CCCR_ESCR_SELECT		0x0000e000
#define	CCCR_RSVD			0x00030000
#define	CCCR_COMPARE			0x00040000
#define	CCCR_COMPLIMENT			0x00080000
#define	CCCR_THRESHOLD			0x00f00000
#define	CCCR_EDGE			0x01000000
#define	CCCR_FORCE_OVF			0x02000000
#define	CCCR_OVF_PMI			0x04000000
#define	CCCR_CASCADE			0x40000000
#define	CCCR_OVF			0x80000000

/* ESCR bit definitions */

#define	ESCR_USR			0x00000004
#define	ESCR_OS				0x00000008
#define	ESCR_TAG_ENABLE			0x00000010
#define	ESCR_TAG_VALUE			0x000001e0
#define	ESCR_EVENT_MASK			0x01fffe00
#define	ESCR_EVENT_SELECT		0x7e000000

/* Replay Event Mask definitions */

#define	REPLAY_NBOGUS			0x00000001
#define	REPLAY_BOGUS			0x00000002

/* Execution Event Mask definitions */

#define	EXEC_NBOGUS0			0x00000001
#define	EXEC_NBOGUS1			0x00000002
#define	EXEC_NBOGUS2			0x00000004
#define	EXEC_NBOGUS3			0x00000008
#define	EXEC_BOGUS0			0x00000010
#define	EXEC_BOGUS1			0x00000020
#define	EXEC_BOGUS2			0x00000040
#define	EXEC_BOGUS3			0x00000080


#ifndef	_ASMLANGUAGE

typedef struct perMonMsr		/* PM CCCR-ESCR table */
    {
    short	counter;
    short	cccr;
    short	escr[MSR_N_ESCR];
    } PER_MON_MSR;

typedef struct perMonMsrUse		/* PM MSRs selected and its value */
    {
    BOOL	used;
    int		escrMsr;
    int		counterValue;
    int		cccrValue;
    int		escrValue;
    int		pebsEnable;
    int		pebsMatrix;
    } PER_MON_MSR_USE;

typedef struct perMonEvent		/* PM event table */
    {
    char *	name;
    short	escr[EVENT_N_ESCR];
    char	counterNo[EVENT_N_ESCR][EVENT_N_COUNTER];
    int		eventSelect;
    int		eventMask;
    int		cccrSelect;
    BOOL	pebs;
    } PER_MON_EVENT;

typedef struct perMonReplayTag		/* Replay Tagging Metrics table */
    {
    char * 	name;
    int		pebsEnable;
    int		pebsMatrix;
    int		upEvent;
    int		upEventMask;
    int		eventMask;
    } PER_MON_REPLAY_TAG;

typedef struct perMonExecTag		/* Execution Tagging Metrics table */
    {
    char * 	name;
    int		upEvent;
    int		upEventMask;
    int		upTagValue;
    int		eventMask;
    } PER_MON_EXEC_TAG;



/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

extern void	perMonLibInit	(void);
extern STATUS	perMonInt	(void);
extern int	perMonCget32	(int counterNo);
extern long long int perMonCget40 (int counterNo);
extern void	perMonCenable	(int cccr);
extern void	perMonCdisable	(int cccr);

extern void	perMonIntEnable	(void);
extern void	perMonIntDisable (void);
extern int	perMonCccrGet	(int counterNo, PER_MON_MSR_USE * pMsrUse);
extern void	perMonCccrSet	(int counterNo, int value, 
				 PER_MON_MSR_USE *pMsrUse);
extern int	perMonEscrGet	(int counterNo, PER_MON_MSR_USE *pMsrUse);
extern void	perMonEscrSet	(int counterNo, int value, 
				 PER_MON_MSR_USE *pMsrUse);
extern void	perMonOsSet	(int counterNo, PER_MON_MSR_USE * pMsrUse);
extern void	perMonUsrSet	(int counterNo, PER_MON_MSR_USE * pMsrUse);
extern void	perMonIntSet	(int counterNo, PER_MON_MSR_USE * pMsrUse);
extern void	perMonIntClear	(int counterNo, PER_MON_MSR_USE * pMsrUse);
extern void	perMonCset	(int counterNo, long long int * pValue);
extern void	perMonCstart	(int counterNo);
extern void	perMonCstop	(int counterNo);
extern STATUS	perMonEventSelect (int event, int escrMsr, int counterNo,
				 int eventMask, int * pCounterMsr,
				 int * pCccrMsr, int * pCccrValue,
				 int * pEscrValue);
extern STATUS	perMonReplaySelect (int metric, int * pUpEvent, int * pUpEventM,
				 int * pPebsEnable, int * pPebsMatrix,
				 int * pEventMask);
extern STATUS	perMonExecSelect (int metric, int * pUpEvent, int * pUpEventM,
				 int * pUpTagValue, int * pEventMask);
extern STATUS	perMonEventSet	(int event, int escrMsr, int counterNo,
				 int eventMask, int metric, int upEventNo,
				 int upEscrMsr, int upCounterNo, 
				 PER_MON_MSR_USE * pMsrUse);
extern STATUS	perMonMsrSet	(PER_MON_MSR_USE * pMsrUse);

#else	/* __STDC__ */

extern void	perMonLibInit	();
extern STATUS	perMonInt	();
extern int	perMonCget32	();
extern long long int perMonCget40 ();
extern void	perMonCenable	();
extern void	perMonCdisable	();

extern void	perMonIntEnable	();
extern void	perMonIntDisable ();
extern int	perMonCccrGet	();
extern void	perMonCccrSet	();
extern int	perMonEscrGet	();
extern void	perMonEscrSet	();
extern void	perMonOsSet	();
extern void	perMonUsrSet	();
extern void	perMonIntSet	();
extern void	perMonIntClear	();
extern void	perMonCset	();
extern void	perMonCstart	();
extern void	perMonCstop	();
extern STATUS	perMonEventSelect ();
extern STATUS	perMonReplaySelect ();
extern STATUS	perMonExecSelect ();
extern STATUS	perMonEventSet	();
extern STATUS	perMonMsrSet	();

#endif	/* __STDC__ */


#endif	/* _ASMLANGUAGE */


#ifdef __cplusplus
}
#endif

#endif	/* __INCperMonLibh */
