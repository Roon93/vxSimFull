/* regsGv3.h - Thermal Monitor and Geyserville III Technology registers */

/* Copyright 2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,26aug02,hdn  written
*/

#ifndef	__INCregsGv3h
#define	__INCregsGv3h

#ifdef __cplusplus
extern "C" {
#endif


/* gv3Lib.c initialization status flags */

#define	GV3_INIT_NOT_DONE	0x0001	/* gv3Lib initialization not done */
#define	GV3_ERR_NO_TM		0x0002	/* no Thermal Monitor exist */
#define	GV3_OK_NO_GV		0x0004	/* no GV1+ or GV3 exist */
#define	GV3_OK_GV3		0x0008	/* GV3 exist */
#define	GV3_OK_NO_MATCH		0x0010	/* no matching FREQ_VID table */
#define	GV3_OK_TM2		0x0020	/* GV3 TM2 exist */
#define	GV3_OK_TM1		0x0040	/* TM1 exist */
#define	GV3_OK_AC		0x0080	/* power source is AC */
#define	GV3_OK_BATT		0x0100	/* power source is Battery */

/* User Thermal Monitor Mode Configuration */

#define	GV3_MAX_PERF		0x1	/* max performance */
#define	GV3_OPT_BATT		0x2	/* battery optimized  performance */
#define	GV3_MAX_BATT		0x3	/* max battery */
#define	GV3_AUTO		0x4	/* automatic */
#define	GV3_DEFAULT		0x5	/* default */

/* GV3 Thermal Monitor Mode Used */

#define	GV3_TM_1		0x1	/* Thermal Monitor 1 */
#define	GV3_TM_2		0x2	/* Thermal Monitor 2 */

/* GV3 State Transition parameters default value */

#define GV3_UP_UTIL		95	/* def utilization(%) to go 1 up */
#define GV3_DOWN_UTIL		95	/* def utilization(%) to go 1 down */
#define GV3_UP_TIME		300	/* def time(millisec) to go 1 up */
#define GV3_DOWN_TIME		1000	/* def time(millisec) to go 1 down */

/* Thermal Throttle parameters default value */

#define GV3_DUTY_CYCLE		0x8	/* throttle duty cycle : 50.0% */
#define GV3_BUS_RATIO		0x8	/* throttle bus ratio : 800Mhz */


/* Geyserville III specific MSRs */

#define	MSR_EBL_CR_POWERON	0x002a	/* Power On Configuration */
#define	MSR_BBL_CR_CTL		0x0119	/* Control Register */
#define	MSR_BBL_CR_CTL3		0x011e	/* Control Register 3 */
#define	MSR_CLOCK_FLEX_MAX	0x0194	/* Flexible Bus Ratio/VID Control */
#define	MSR_PERF_STS		0x0198	/* Performance (Geyserville) Status */
#define	MSR_PERF_CTL		0x0199	/* Performance (Geyserville) Control */
#define	MSR_GV_THERM		0x019d	/* Geyserville Thermal Control */
#define	MSR_GV1P_REG_PTR	0x019e	/* Geyserville 1+ Pointer */

/* MSR_EBL_CR_POWERON bit difinition */

#define	EBL_RATIO	0x07c00000	/* RO Core Clock to System Bus Ratio */
#define	EBL_AGENT_ID	0x00300000	/* RO Agent ID */
#define	EBL_BUS_FREQ	0x00040000	/* RO System Bus Frequency */
#define	EBL_APIC_ID	0x00030000	/* RO APIC Cluster ID */
#define	EBL_PO_RESET	0x00004000	/* RO 1M Power On Reset */
#define	EBL_IOQ_DEP	0x00002000	/* RO In Order Queue Depth */
#define	EBL_BINI_OBS_EN	0x00001000	/* RO BINIT# Observation Enabled */
#define	EBL_AERR_OBS_EN	0x00000400	/* RO AERR# Observation Enabled */
#define	EBL_BIST	0x00000200	/* RO Execute BIST */
#define	EBL_TRI_STATE	0x00000100	/* RO Output Tri-state Enable */
#define	EBL_BINI_EN	0x00000080	/* RW BINIT# Enable */
#define	EBL_BERR_IE_EN	0x00000040	/* RW BERR# Enable, Internal Err */
#define	EBL_QUICK	0x00000020	/* RO Quickstart Mode */
#define	EBL_BERR_BR_EN	0x00000010	/* RW BERR# Enable, Bus Request */
#define	EBL_AERR_EN	0x00000008	/* RW AERR# Enable */
#define	EBL_RESP_CHK_EN	0x00000004	/* RW Response Error Check Enable */
#define	EBL_DATA_CHK_EN	0x00000002	/* RW Data Error Check Enable */

/* MSR_BBL_CR_CTL3 bit difinition */

#define	BBL_L2_NOT	0x00800000	/* RO L2 Not Present */
#define	BBL_L2_RANGE	0x00700000	/* RO L2 Physical Address Range */
#define	BBL_L2_EN	0x00000100	/* RW L2 Enabled */
#define	BBL_L2_ECC_EN	0x00000020	/* RO L2 ECC Check Enabled */
#define	BBL_L2_HARD_EN	0x00000001	/* RO L2 Hardware Enabled */

/* MSR_CLOCK_FLEX_MAX bit difinition */

#define	FLEX_EN		0x00010000	/* RW Flex Enable */
#define	FLEX_RATIO	0x00001f00	/* RW Flex Ratio */
#define	FLEX_VID	0x0000003f	/* RW Flex VID */

/* MSR_GV1P_REG_PTR bit difinition */

#define	GV1P_REG_PTR	0x0000ffff	/* RW GV1P pointer register */

/* MSR_PERF_CTL bit difinition */

#define	BUS_RATIO_SEL	0x00001f00	/* RW Bus Ratio Selection */
#define	GV_VID_SEL	0x0000003f	/* RW VID Selection */

/* MSR_PERF_STS bit difinition (upper 32) */

#define	BUS_RATIO_BOOT	0x1f000000	/* RO Boot Bus Ratio */
#define	GV_VID_BOOT	0x003f0000	/* RO Boot VID */
#define	BUS_RATIO_MAX	0x00001f00	/* RO Max Bus Ratio */
#define	GV_VID_MAX	0x0000003f	/* RO Max VID */

/* MSR_PERF_STS bit difinition (lower 32) */

#define	BUS_RATIO_MIN	0x1f000000	/* RO Min Bus Ratio */
#define	GV_TS		0x00200000	/* RO GV3 Transition Started */
#define	GV_CMD_SEEK	0x00100000	/* RO GV3 Transition Performing */
#define	GV_THERM_THROT	0x00080000	/* RO GV3 Thermal Throttle */
#define	GV_TT		0x00040000	/* RO Thermal Trip Active */
#define	GV_VIP		0x00020000	/* RO Voltage Transition Pending */
#define	GV_FIP		0x00010000	/* RO Frequency Change Pending */
#define	BUS_RATIO_STS	0x00001f00	/* RO Current Bus Ratio */
#define	GV_VID_STS	0x0000003f	/* RO Current VID */

/* MSR_GV_THERM bit difinition */

#define	GV_THROT_SEL	0x00010000	/* RW Thermal Management Mode */
#define	BUS_RATIO_THROT	0x00001f00	/* RW Throttling Bus Ratio */
#define	GV_VID_THROT	0x0000003f	/* RW Throttling VID */


#ifndef	_ASMLANGUAGE

/* frequency - VID table : state entry */

typedef struct freqVidState {
    UINT16	no;		/* state no. */
    UINT16	vid;		/* VID_SEL value in MSR_PERF_CTL */
    UINT16	ratio;		/* BUS_RATIO_SEL value in MSR_PERF_CTL */
    UINT16	power;		/* power(mW) consumed by CPU in this state */
    } FREQ_VID_STATE;

/* frequency - VID table : header */

typedef struct freqVidHeader {
    UINT32	cpuId;		/* CPU family-model-stepping value */
    UINT8	maxVid;		/* VID_MAX value in MSR_PERF_STS */
    UINT8	maxRatio;	/* BUS_RATIO_MAX value in MSR_PERF_STS */
    UINT8	nState;		/* no of states supported by this CPU */
    FREQ_VID_STATE * pState;
    } FREQ_VID_HEADER;

/* Thermal Monitor interrupt routines and arguments */

typedef struct sysTherm {
    FUNCPTR hotRoutine;		/* rtn to call on TM Hot int */
    INT32   hotArg;		/* its argument */
    BOOL    hotConnected;
    FUNCPTR coldRoutine;	/* rtn to call on TM Cold int */
    INT32   coldArg;		/* its argument */
    BOOL    coldConnected;
    } SYS_THERM;
    

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

extern STATUS	gv3LibInit	(UINT32 mode, UINT32 upUtil, UINT32 downUtil, 
				 UINT32 upTime, UINT32 downTime, 
				 FUNCPTR acCheckRtn);
extern STATUS	gv3StateSet	(UINT32 state);
extern STATUS	gv3DutyCycleSet	(UINT32 dutyCycleIx);
extern STATUS	gv3DutyCycleEnable (BOOL enable);
extern BOOL	gv3DutyCycleCheck (void);
extern BOOL	gv3HotCheck	(void);
extern BOOL	gv3AcCheck	(void);
extern STATUS	gv3AutoEnable	(BOOL enable);
extern STATUS	gv3AutoSet	(UINT32 upUtil, UINT32 downUtil,
				 UINT32 upTime, UINT32 downTime);
extern STATUS	gv3AutoGet	(UINT32 * pUpUtil, UINT32 * pDownUtil,
				 UINT32 * pUpTime, UINT32 * pDownTime);
extern VOID	gv3ShowInit	(void);
extern VOID	gv3Show		(void);

#else	/* __STDC__ */

extern STATUS	gv3LibInit	();
extern STATUS	gv3StateSet	();
extern STATUS	gv3DutyCycleSet	();
extern STATUS	gv3DutyCycleEnable ();
extern BOOL	gv3DutyCycleCheck ();
extern BOOL	gv3HotCheck	();
extern BOOL	gv3AcCheck	();
extern STATUS	gv3AutoEnable	();
extern STATUS	gv3AutoSet	();
extern STATUS	gv3AutoGet	();
extern VOID	gv3ShowInit	();
extern VOID	gv3Show		();

#endif	/* __STDC__ */


#endif	/* _ASMLANGUAGE */


#ifdef __cplusplus
}
#endif

#endif	/* __INCregsGv3h */
