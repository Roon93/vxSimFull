/* regsP7.h - Pentium4 architecture specific registers */

/* Copyright 2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,02sep02,hdn  written
*/

#ifndef	__INCregsP7h
#define	__INCregsP7h

#ifdef __cplusplus
extern "C" {
#endif


/* MSR, Pentium4 specific */

#define	MSR_EBC_HARD_POWERON	0x002a
#define	MSR_EBC_SOFT_POWERON	0x002b
#define	MSR_EBC_FREQUENCY_ID	0x002c
#define	MSR_LER_FROM_LIP	0x01d7
#define	MSR_LER_TO_LIP		0x01d8
#define	MSR_LASTBRANCH_TOS	0x01da
#define	MSR_LASTBRANCH_0	0x01db
#define	MSR_LASTBRANCH_1	0x01dc
#define	MSR_LASTBRANCH_2	0x01dd
#define	MSR_LASTBRANCH_3	0x01de
#define	MSR_BPU_COUNTER0	0x0300
#define	MSR_BPU_COUNTER1	0x0301
#define	MSR_BPU_COUNTER2	0x0302
#define	MSR_BPU_COUNTER3	0x0303
#define	MSR_MS_COUNTER0		0x0304
#define	MSR_MS_COUNTER1		0x0305
#define	MSR_MS_COUNTER2		0x0306
#define	MSR_MS_COUNTER3		0x0307
#define	MSR_FLAME_COUNTER0	0x0308
#define	MSR_FLAME_COUNTER1	0x0309
#define	MSR_FLAME_COUNTER2	0x030a
#define	MSR_FLAME_COUNTER3	0x030b
#define	MSR_IQ_COUNTER0		0x030c
#define	MSR_IQ_COUNTER1		0x030d
#define	MSR_IQ_COUNTER2		0x030e
#define	MSR_IQ_COUNTER3		0x030f
#define	MSR_IQ_COUNTER4		0x0310
#define	MSR_IQ_COUNTER5		0x0311
#define	MSR_BPU_CCCR0		0x0360
#define	MSR_BPU_CCCR1		0x0361
#define	MSR_BPU_CCCR2		0x0362
#define	MSR_BPU_CCCR3		0x0363
#define	MSR_MS_CCCR0		0x0364
#define	MSR_MS_CCCR1		0x0365
#define	MSR_MS_CCCR2		0x0366
#define	MSR_MS_CCCR3		0x0367
#define	MSR_FLAME_CCCR0		0x0368
#define	MSR_FLAME_CCCR1		0x0369
#define	MSR_FLAME_CCCR2		0x036a
#define	MSR_FLAME_CCCR3		0x036b
#define	MSR_IQ_CCCR0		0x036c
#define	MSR_IQ_CCCR1		0x036d
#define	MSR_IQ_CCCR2		0x036e
#define	MSR_IQ_CCCR3		0x036f
#define	MSR_IQ_CCCR4		0x0370
#define	MSR_IQ_CCCR5		0x0371
#define	MSR_BSU_ESCR0		0x03a0
#define	MSR_BSU_ESCR1		0x03a1
#define	MSR_FSB_ESCR0		0x03a2
#define	MSR_FSB_ESCR1		0x03a3
#define	MSR_FIRM_ESCR0		0x03a4
#define	MSR_FIRM_ESCR1		0x03a5
#define	MSR_FLAME_ESCR0		0x03a6
#define	MSR_FLAME_ESCR1		0x03a7
#define	MSR_DAC_ESCR0		0x03a8
#define	MSR_DAC_ESCR1		0x03a9
#define	MSR_MOB_ESCR0		0x03aa
#define	MSR_MOB_ESCR1		0x03ab
#define	MSR_PMH_ESCR0		0x03ac
#define	MSR_PMH_ESCR1		0x03ad
#define	MSR_SAAT_ESCR0		0x03ae
#define	MSR_SAAT_ESCR1		0x03af
#define	MSR_U2L_ESCR0		0x03b0
#define	MSR_U2L_ESCR1		0x03b1
#define	MSR_BPU_ESCR0		0x03b2
#define	MSR_BPU_ESCR1		0x03b3
#define	MSR_IS_ESCR0		0x03b4
#define	MSR_IS_ESCR1		0x03b5
#define	MSR_ITLB_ESCR0		0x03b6
#define	MSR_ITLB_ESCR1		0x03b7
#define	MSR_CRU_ESCR0		0x03b8
#define	MSR_CRU_ESCR1		0x03b9
#define	MSR_IQ_ESCR0		0x03ba
#define	MSR_IQ_ESCR1		0x03bb
#define	MSR_RAT_ESCR0		0x03bc
#define	MSR_RAT_ESCR1		0x03bd
#define	MSR_SSU_ESCR0		0x03be
#define	MSR_MS_ESCR0		0x03c0
#define	MSR_MS_ESCR1		0x03c1
#define	MSR_TBPU_ESCR0		0x03c2
#define	MSR_TBPU_ESCR1		0x03c3
#define	MSR_TC_ESCR0		0x03c4
#define	MSR_TC_ESCR1		0x03c5
#define	MSR_IX_ESCR0		0x03c8
#define	MSR_IX_ESCR1		0x03c9
#define	MSR_ALF_ESCR0		0x03ca
#define	MSR_ALF_ESCR1		0x03cb
#define	MSR_CRU_ESCR2		0x03cc
#define	MSR_CRU_ESCR3		0x03cd
#define	MSR_CRU_ESCR4		0x03e0
#define	MSR_CRU_ESCR5		0x03e1
#define	MSR_TC_PRECISE_EVENT	0x03f0
#define	MSR_PEBS_MATRIX_VERT	0x03f2


#ifdef __cplusplus
}
#endif

#endif	/* __INCregsP7h */
