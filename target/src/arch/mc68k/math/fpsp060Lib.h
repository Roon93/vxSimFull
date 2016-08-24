/* fpsp060Lib.h - MC68060 floating point instruction header. */

/* Copyright 1984-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,28jun94,tpr	clean up following code review.
		written.
*/

#ifndef __INCfpsp060Libh
#define __INCfpsp060Libh

#ifdef __cplusplus
extern "C" {
#endif

/* unimplemented floating point instruction call out table */
extern int FPSP_060_CO_TBL [32];

#define FPSP_060_CO_TBL_SIZE	128	/* 128 bytes : 32 * 4 entries */

/* floating point handler entry point offset */

#define	_060_FPSP_SNAN_OFFSET			0x000
#define	_060_FPSP_OPERR_OFFSET			0x008
#define	_060_FPSP_OVFL_OFFSET			0x010
#define	_060_FPSP_UNFL_OFFSET			0x018
#define	_060_FPSP_DZ_OFFSET			0x020
#define	_060_FPSP_INEX_OFFSET			0x028
#define	_060_FPSP_FLINE_OFFSET			0x030
#define	_060_FPSP_UNSUPP_OFFSET			0x038
#define	_060_FPSP_EFFADD_OFFSET			0x040


/* floating point handler entry point */

#define	FPSP_060_BASE		FPSP_060_CO_TBL + FPSP_060_CO_TBL_SIZE

#define	FPSP_060_SNAN		((int ) FPSP_060_BASE + _060_FPSP_SNAN_OFFSET)
#define	FPSP_060_OPERR		((int ) FPSP_060_BASE + _060_FPSP_OPERR_OFFSET)
#define	FPSP_060_OVFL		((int ) FPSP_060_BASE + _060_FPSP_OVFL_OFFSET)
#define	FPSP_060_UNFL		((int ) FPSP_060_BASE + _060_FPSP_UNFL_OFFSET)
#define	FPSP_060_DZ		((int ) FPSP_060_BASE + _060_FPSP_DZ_OFFSET)
#define	FPSP_060_INEX		((int ) FPSP_060_BASE + _060_FPSP_INEX_OFFSET)
#define	FPSP_060_FLINE		((int ) FPSP_060_BASE + _060_FPSP_FLINE_OFFSET)
#define	FPSP_060_UNSUPP		((int ) FPSP_060_BASE + _060_FPSP_UNSUPP_OFFSET)
#define	FPSP_060_EFFADD		((int ) FPSP_060_BASE + _060_FPSP_EFFADD_OFFSET)

#if defined(__STDC__) || defined(__cplusplus)

extern void	fpsp060COTblInit (void);
extern void	_060_fpsp_done (void);
extern void	_060_real_ovfl (void);
extern void	_060_real_unfl (void);
extern void	_060_real_operr (void);
extern void	_060_real_snan (void);
extern void	_060_real_dz (void);
extern void	_060_real_inex (void);
extern void	_060_real_bsun (void);
extern void	_060_real_fline (void);
extern void	_060_real_fpu_disabled (void);
extern void	_060_real_trap (void);

#else   /* __STDC__ */

extern void	fpsp060COTblInit ();
extern void	_060_fpsp_done ();
extern void	_060_real_ovfl ();
extern void	_060_real_unfl ();
extern void	_060_real_operr ();
extern void	_060_real_snan ();
extern void	_060_real_dz ();
extern void	_060_real_inex ();
extern void	_060_real_bsun ();
extern void	_060_real_fline ();
extern void	_060_real_fpu_disabled ();
extern void	_060_real_trap ();

#endif  /* __STDC__ */


#ifdef __cplusplus
}
#endif

#endif /* __INCfpsp060Libh */
