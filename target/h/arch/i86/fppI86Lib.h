/* fppLib.h - floating-point coprocessor support library header */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,25mar02,hdn  added two APIs to detect illegal FPU usage (spr 70187)
01f,21nov01,hdn  derived xmm[] from the reserved res2[] in FPX_CONTEXT
01e,21aug01,hdn  imported SSE support from T31 ver 01g
01d,02sep93,hdn  deleted a macro FPX.
01c,08jun93,hdn  added support for c++. updated to 5.1.
01b,29sep92,hdn  changed UTINY to UCHAR.
01a,07apr92,hdn  written based on TRON version.
*/

#ifndef	__INCfppI86Libh
#define	__INCfppI86Libh

#ifdef __cplusplus
extern "C" {
#endif


/* control register: defines for control word */

#define	FPCR_IM			0x0001	/* IM exc-mask invalid operation */
#define	FPCR_DM			0x0002	/* DM exc-mask denormalized operand */
#define	FPCR_ZM			0x0004	/* ZM exc-mask zero divide */
#define	FPCR_OM			0x0008	/* OM exc-mask overflow */
#define	FPCR_UM			0x0010	/* UM exc-mask underflow */
#define	FPCR_PM			0x0020	/* PM exc-mask precision */
#define	FPCR_PC_SINGLE		0x0000	/* PC single precision(24 bits) */
#define	FPCR_PC_DOUBLE		0x0200	/* PC double precision(53 bits) */
#define	FPCR_PC_EXTENDED	0x0300	/* PC extended precision(64 bits) */
#define	FPCR_PC_MASK		0xfcff	/* PC extended precision(64 bits) */
#define	FPCR_RC_NEAREST		0x0000	/* RC round to nearest */
#define	FPCR_RC_DOWN		0x0400	/* RC round down */
#define	FPCR_RC_UP		0x0800	/* RC round up */
#define	FPCR_RC_ZERO		0x0c00	/* RC round to zero */
#define	FPCR_RC_MASK		0xf3ff	/* RC round bits mask */
#define	FPCR_X			0x1000	/* X infinity control */

/* status register: defines for status word */

#define FPSR_IE			0x0001	/* IE exc-flag invalid operation */
#define FPSR_DE			0x0002	/* DE exc-flag denormalized operand */
#define FPSR_ZE			0x0004	/* ZE exc-flag zero divide */
#define FPSR_OE			0x0008	/* OE exc-flag overflow */
#define FPSR_UE			0x0010	/* UE exc-flag underflow */
#define FPSR_PE			0x0020	/* PE exc-flag precision */
#define FPSR_SF			0x0040	/* SF stack fault */
#define FPSR_ES			0x0080	/* ES error summary status */
#define FPSR_C0			0x0100	/* C0 condition code */
#define FPSR_C1			0x0200	/* C1 condition code */
#define FPSR_C2			0x0400	/* C2 condition code */
#define FPSR_TOP		0x3800	/* TOP top of stack pointer */
#define FPSR_C3			0x4000	/* C3 condition code */
#define FPSR_B			0x8000	/* FPU busy */

/* number of FP/MM and XMM registers on coprocessor */

#define	FP_NUM_REGS		8	/* number of FP/MM registers */
#define	XMM_NUM_REGS		8	/* number of XMM registers */
#define	FP_NUM_RESERVED		14	/* reserved area in FPX_CONTEXT */

/* maximum size of floating-point coprocessor state frame */

#define FPO_STATE_FRAME_SIZE	108	/* nBytes of FPO_CONTEXT */
#define FPX_STATE_FRAME_SIZE	512	/* nBytes of FPX_CONTEXT */

/* FPREG_SET structure offsets */

#define	FPREG_FPCR	0x00	/* offset to FPCR in FPREG_SET */
#define	FPREG_FPSR	0x04	/* offset to FPSR in FPREG_SET */
#define	FPREG_FPTAG	0x08	/* offset to FPTAG in FPREG_SET */
#define	FPREG_OP	0x0c	/* offset to OP in FPREG_SET */
#define	FPREG_IP	0x10	/* offset to IP in FPREG_SET */
#define	FPREG_CS	0x14	/* offset to CS in FPREG_SET */
#define	FPREG_DP	0x18	/* offset to DP in FPREG_SET */
#define	FPREG_DS	0x1c	/* offset to DS in FPREG_SET */
#define FPREG_FPX(n)	(0x20 + (n)*sizeof(DOUBLEX)) /* offset to FPX(n) */


#ifndef	_ASMLANGUAGE

/* DOUBLEX - double extended precision */

typedef struct
    {
    UCHAR f[10];			/* ST[0-7] or MM[0-7] */
    } DOUBLEX;

/* DOUBLEX_SSE - double extended precision used in FPX_CONTEXT for SSE */

typedef struct
    {
    UCHAR f[10];			/* ST[0-7] or MM[0-7] */
    UCHAR r[6];				/* reserved */
    } DOUBLEX_SSE;

/* FPREG_SET - FP register set that is different from FP_CONTEXT */

typedef struct fpregSet
    {
    int		fpcr;			/* control word */
    int		fpsr;			/* status word */
    int		fptag;			/* tag word */
    int		op;			/* last FP instruction op code */
    int		ip;			/* instruction pointer */
    int		cs;			/* instruction pointer selector */
    int		dp;			/* data pointer */
    int		ds;			/* data pointer selector */
    DOUBLEX	fpx[FP_NUM_REGS];	/* FR[0-7] non-TOS rel. order */
    } FPREG_SET;

/* FPO_CONTEXT - Old FP context used by fsave/frstor instruction */

typedef struct fpOcontext
    {
    int		fpcr;			/* 4    control word */
    int		fpsr;			/* 4    status word */
    int		fptag;			/* 4    tag word */
    int		ip;			/* 4    instruction pointer */
    short	cs;			/* 2    instruction pointer selector */
    short	op;			/* 2    last FP instruction op code */
    int		dp;			/* 4    data pointer */
    int		ds;			/* 4    data pointer selector */
    DOUBLEX	fpx[FP_NUM_REGS];	/* 8*10 FR[0-7] non-TOS rel. order */
    } FPO_CONTEXT;			/* 108  bytes total */

/* FPX_CONTEXT - New FP context used by fxsave/fxrstor instruction */

typedef struct fpXcontext
    {
    short	fpcr;			/* 2     control word */
    short	fpsr;			/* 2     status word */
    short	fptag;			/* 2     tag word */
    short	op;			/* 2     last FP instruction op code */
    int		ip;			/* 4     instruction pointer */
    int		cs;			/* 4     instruction pointer selector */
    int		dp;			/* 4     data pointer */
    int		ds;			/* 4     data pointer selector */
    int		reserved0;		/* 4     reserved */
    int		reserved1;		/* 4     reserved */
    DOUBLEX_SSE	fpx[FP_NUM_REGS];	/* 8*16  FR[0-7] non-TOS rel. order */
    DOUBLEX_SSE	xmm[XMM_NUM_REGS];	/* 8*16  XMM[0-7] */
    DOUBLEX_SSE	res2[FP_NUM_RESERVED];	/* 14*16 reserved */
    } FPX_CONTEXT;			/* 512   bytes total */

/* FP_CONTEXT - Common FP context */

typedef struct fpContext
    {
    union u
	{
	FPO_CONTEXT o;			/* old FPO_CONTEXT for fsave/frstor */
	FPX_CONTEXT x;			/* new FPX_CONTEXT for fxsave/fxrstor */
	} u;
    } FP_CONTEXT;


/* variable declarations */

extern REG_INDEX   fpRegName[];		/* f-point data register table */
extern REG_INDEX   fpCtlRegName[];	/* f-point control register table */
extern WIND_TCB    *pFppTaskIdPrevious;	/* task id for deferred exceptions */
extern FUNCPTR	   fppCreateHookRtn;	/* arch dependent create hook routine */
extern FUNCPTR	   fppDisplayHookRtn;	/* arch dependent display routine */
extern VOIDFUNCPTR _func_fppSaveRtn;	/* fppSave or fppXsave */
extern VOIDFUNCPTR _func_fppRestoreRtn;	/* fppRestore or fppXrestore */

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

extern	void	fppArchInit (void);
extern	void	fppArchTaskCreateInit (FP_CONTEXT *pFpContext);
extern	STATUS  fppProbeSup (void);
extern	void	fppProbeTrap (void);
extern	void	fppDtoDx (DOUBLEX *pDx, double *pDouble);
extern	void	fppDxtoD (double *pDouble, DOUBLEX *pDx);
extern	void	fppXrestore (FP_CONTEXT *pFpContext);
extern	void 	fppXsave (FP_CONTEXT *pFpContext);
extern	void	fppXregsToCtx (FPREG_SET *pFpRegSet, FP_CONTEXT *pFpContext);
extern	void	fppXctxToRegs (FP_CONTEXT *pFpContext, FPREG_SET *pFpRegSet);
extern	void	fppArchSwitchHook (WIND_TCB * pOldTcb, WIND_TCB * pNewTcb);
extern	STATUS	fppArchSwitchHookEnable (BOOL enable);

#else

extern	void	fppArchInit ();
extern	void	fppArchTaskCreateInit ();
extern	STATUS	fppProbeSup ();
extern	void	fppProbeTrap ();
extern	void	fppDtoDx ();
extern	void	fppDxtoD ();
extern	void	fppXrestore ();
extern	void 	fppXsave ();
extern	void	fppXregsToCtx ();
extern	void	fppXctxToRegs ();
extern	void	fppArchSwitchHook ();
extern	STATUS	fppArchSwitchHookEnable ();

#endif	/* __STDC__ */

#endif	/* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __INCfppI86Libh */
