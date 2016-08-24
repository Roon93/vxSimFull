/* vxI86Lib.h - header for arch/board dependent routines for the I80X86 */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,04sep02,hdn  added vxIdleHookAdd(), vxIdleShow() prototype.
01e,11apr02,pai  Added vxCpuShow(), vxDrShow(), and vxSseShow() prototypes.
01d,26mar02,pai  Added vxShowInit() prototype (SPR 74103).
01c,13nov01,ahm  Added rudimentary support for power management (SPR#32599)
01b,26oct01,hdn  added vxDr[SG]et(), vxTss[GS]et() prototype.
01a,22aug01,hdn  written.
*/

#ifndef __INCvxI86Libh
#define __INCvxI86Libh

#ifdef __cplusplus
extern "C" {
#endif


/* power management mode definitions - used by vxPowerModeSet/Get */

#define VX_POWER_MODE_DISABLE      (0x1)     /* power mgt disable: loop when idle*/
#define VX_POWER_MODE_AUTOHALT     (0x4)     /* AutoHalt power management mode */

/* function declarations */

#ifndef	_ASMLANGUAGE

#if defined(__STDC__) || defined(__cplusplus)

extern void	vxMemProbeTrap		(void);
extern STATUS	vxMemProbeSup		(int length, char * adrs, char * pVal);
extern int	vxCr0Get		(void);
extern void	vxCr0Set		(int cr0);
extern int	vxCr2Get		(void);
extern void	vxCr2Set		(int cr2);
extern int	vxCr3Get		(void);
extern void	vxCr3Set		(int cr3);
extern int	vxCr4Get		(void);
extern void	vxCr4Set		(int cr4);
extern int	vxEflagsGet		(void);
extern void	vxEflagsSet		(int eflags);
extern void	vxDrGet			(int * pDr0, int * pDr1, int * pDr2, 
					 int * pDr3, int * pDr4, int * pDr5, 
					 int * pDr6, int * pDr7);
extern void	vxDrSet			(int dr0, int dr1, int dr2, int dr3, 
					 int dr4, int dr5, int dr6, int dr7);
extern int	vxTssGet		(void);
extern void	vxTssSet		(int value);
extern void	vxGdtrGet		(long long int * pGdtr);
extern void	vxIdtrGet		(long long int * pIdtr);
extern void	vxLdtrGet		(long long int * pLdtr);
IMPORT STATUS	vxPowerModeSet		(UINT32 mode);
IMPORT UINT32	vxPowerModeGet		(void);
extern STATUS	vxIdleHookAdd		(FUNCPTR entHook, FUNCPTR exitHook);
extern VOID	vxIdleEntHookRtn	(void);
extern VOID	vxIdleExtHookRtn	(void);
extern UINT32	vxIdleUtilGet		(void);

extern void	vxShowInit		(void);
extern void	vxCpuShow		(void);
extern void	vxDrShow		(void);
extern void	vxSseShow		(int);
extern void	vxIdleShow		(BOOL debug);

#else	/* __STDC__ */

extern void	vxMemProbeTrap		();
extern STATUS	vxMemProbeSup		();
extern int	vxCr0Get		();
extern void	vxCr0Set		();
extern int	vxCr2Get		();
extern void	vxCr2Set		();
extern int	vxCr3Get		();
extern void	vxCr3Set		();
extern int	vxCr4Get		();
extern void	vxCr4Set		();
extern int	vxEflagsGet		();
extern void	vxEflagsSet		();
extern void	vxDrGet			();
extern void	vxDrSet			();
extern int	vxTssGet		();
extern void	vxTssSet		();
extern void	vxGdtrGet		();
extern void	vxIdtrGet		();
extern void	vxLdtrGet		();
IMPORT STATUS	vxPowerModeSet		();
IMPORT UINT32	vxPowerModeGet		();
extern STATUS	vxIdleHookAdd		();
extern VOID	vxIdleEntHookRtn	();
extern VOID	vxIdleExtHookRtn	();
extern UINT32	vxIdleUtilGet		();

extern void	vxShowInit		();
extern void	vxCpuShow		();
extern void	vxDrShow		();
extern void	vxSseShow		();
extern void	vxIdleShow		();

#endif	/* __STDC__ */

#endif  /* _ASMLANGUAGE */


#ifdef __cplusplus
}
#endif

#endif /* __INCvxI86Libh */
