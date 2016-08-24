/* pentiumLib.h - system dependent routines header */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,15jun02,hdn  added LL_UNION typedef for 36bit MMU support
01c,01nov01,hdn  added PENTIUM_MSR, pentiumMsrInit/Show's prototype
01b,21aug01,hdn  included regs.h for MTRR_ID.
		 added P5/P6 PMC routines.
01a,09jul97,hdn  written
*/

#ifndef __INCpentiumLibh
#define __INCpentiumLibh

#ifdef __cplusplus
extern "C" {
#endif

#ifndef	_ASMLANGUAGE

#include "regs.h"


/* typedefs */

typedef long long int   LL_INT;
typedef union llUnion
    {
    long long int i64;	/* 64bit integer */
    int        i32[2];	/* 32bit integer * 2 */
    } LL_UNION;

typedef struct pentiumMsr
    {
    INT32       addr;   /* address of the MSR */
    INT8 *      name;   /* name of the MSR */
    } PENTIUM_MSR;


/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

extern int	pentiumCr4Get		(void);
extern void	pentiumCr4Set		(int cr4);
extern STATUS   pentiumPmcStart         (int pmcEvtSel0, int pmcEvtSel1);
extern STATUS   pentiumPmcStart0        (int pmcEvtSel0);
extern STATUS   pentiumPmcStart1        (int pmcEvtSel1);
extern void     pentiumPmcStop          (void);
extern void     pentiumPmcStop0         (void);
extern void     pentiumPmcStop1         (void);
extern void     pentiumPmcGet           (long long int * pPmc0,
                                         long long int * pPmc1);
extern void     pentiumPmcGet0          (long long int * pPmc0);
extern void     pentiumPmcGet1          (long long int * pPmc1);
extern void     pentiumPmcReset         (void);
extern void     pentiumPmcReset0        (void);
extern void     pentiumPmcReset1        (void);
extern STATUS   pentiumP6PmcStart       (int pmcEvtSel0, int pmcEvtSel1);
extern void     pentiumP6PmcStop        (void);
extern void     pentiumP6PmcStop1       (void);
extern void     pentiumP6PmcGet         (long long int * pPmc0,
					 long long int * pPmc1);
extern void     pentiumP6PmcGet0        (long long int * pPmc0);
extern void     pentiumP6PmcGet1        (long long int * pPmc1);
extern void     pentiumP6PmcReset       (void);
extern void     pentiumP6PmcReset0      (void);
extern void     pentiumP6PmcReset1      (void);
extern STATUS   pentiumP5PmcStart0      (int cesr);
extern STATUS   pentiumP5PmcStart1      (int cesr);
extern void     pentiumP5PmcStop0       (void);
extern void     pentiumP5PmcStop1       (void);
extern void     pentiumP5PmcGet         (long long int * pPmc0,
					 long long int * pPmc1);
extern void     pentiumP5PmcGet0        (long long int * pPmc0);
extern void     pentiumP5PmcGet1        (long long int * pPmc1);
extern void     pentiumP5PmcReset       (void);
extern void     pentiumP5PmcReset0      (void);
extern void     pentiumP5PmcReset1      (void);
extern void	pentiumTscGet64		(long long int * pTsc);
extern UINT32	pentiumTscGet32		(void);
extern void	pentiumTscReset		(void);
extern void	pentiumMsrGet		(int addr, long long int * pData);
extern void	pentiumMsrSet		(int addr, long long int * pData);
extern void	pentiumTlbFlush		(void);
extern void	pentiumSerialize	(void);
extern STATUS	pentiumBts		(char * pFlag);
extern STATUS	pentiumBtc		(char * pFlag);
extern void	pentiumMtrrEnable	(void);
extern void	pentiumMtrrDisable	(void);
extern STATUS	pentiumMtrrGet		(MTRR_ID pMtrr);
extern STATUS	pentiumMtrrSet		(MTRR_ID pMtrr);
extern void	pentiumPmcShow		(BOOL zap);
extern STATUS	pentiumMsrInit		(void);
extern void	pentiumMsrShow		(void);
extern void	pentiumMcaEnable	(BOOL enable);
extern void	pentiumMcaShow		(void);

#else	/* __STDC__ */

extern int	pentiumCr4Get		();
extern void	pentiumCr4Set		();
extern STATUS   pentiumPmcStart         ();
extern STATUS   pentiumPmcStart0        ();
extern STATUS   pentiumPmcStart1        ();
extern void     pentiumPmcStop          ();
extern void     pentiumPmcStop0         ();
extern void     pentiumPmcStop1         ();
extern void     pentiumPmcGet           ();
extern void     pentiumPmcGet0          ();
extern void     pentiumPmcGet1          ();
extern void     pentiumPmcReset         ();
extern void     pentiumPmcReset0        ();
extern void     pentiumPmcReset1        ();
extern STATUS   pentiumP6PmcStart       ();
extern void     pentiumP6PmcStop        ();
extern void     pentiumP6PmcStop1       ();
extern void     pentiumP6PmcGet         ();
extern void     pentiumP6PmcGet0        ();
extern void     pentiumP6PmcGet1        ();
extern void     pentiumP6PmcReset       ();
extern void     pentiumP6PmcReset0      ();
extern void     pentiumP6PmcReset1      ();
extern STATUS   pentiumP5PmcStart0      ();
extern STATUS   pentiumP5PmcStart1      ();
extern void     pentiumP5PmcStop0       ();
extern void     pentiumP5PmcStop1       ();
extern void     pentiumP5PmcGet         ();
extern void     pentiumP5PmcGet0        ();
extern void     pentiumP5PmcGet0        ();
extern void     pentiumP5PmcGet1        ();
extern void     pentiumP5PmcReset       ();
extern void     pentiumP5PmcReset0      ();
extern void	pentiumTscGet64		();
extern UINT32	pentiumTscGet32		();
extern void	pentiumTscReset		();
extern void	pentiumMsrGet		();
extern void	pentiumMsrSet		();
extern void	pentiumTlbFlush		();
extern void	pentiumSerialize	();
extern STATUS	pentiumBts		();
extern STATUS	pentiumBtc		();
extern void	pentiumMtrrEnable	();
extern void	pentiumMtrrDisable	();
extern STATUS	pentiumMtrrGet		();
extern STATUS	pentiumMtrrSet		();
extern void	pentiumPmcShow		();
extern STATUS	pentiumMsrInit		();
extern void	pentiumMsrShow		();
extern void	pentiumMcaEnable	();
extern void	pentiumMcaShow		();

#endif	/* __STDC__ */

#endif	/* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __INCpentiumLibh */
