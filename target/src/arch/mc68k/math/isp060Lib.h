/* isp060Lib.h - unimplemented integer instruction header for MC68060. */

/* Copyright 1984-1991 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,27jun94,tpr	clean up following code review.
		written.
*/

#ifndef __INCisp060Libh
#define __INCisp060Libh

#ifdef __cplusplus
extern "C" {
#endif

/* unimplemented integer instruction call out table */
extern int ISP_060_CO_TBL [32];

#define ISP_060_CO_TBL_SIZE	128	/* 128 bytes : 32 * 4 entries */

/* unimplemented integer instruction library entry point offset */

#define   _060_ISP_UNIMP_OFFSET			0x000
#define   _060_ISP_CAS_OFFSET			0x008
#define   _060_ISP_CAS2_OFFSET			0x010
#define   _060_ISP_CAS_FINISH_OFFSET		0x018
#define   _060_ISP_CAS2_FINISH_OFFSET		0x020
#define   _060_ISP_CAS_INRANGE_OFFSET		0x028
#define   _060_ISP_CAS_TERMINATE_OFFSET		0x030
#define   _060_ISP_CAS_RESTART_OFFSET		0x038

/* unimplemented integer instruction library entry point */

#define	ISP_060_BASE		ISP_060_CO_TBL + ISP_060_CO_TBL_SIZE

#define ISP_060_UNIMP		((int ) ISP_060_BASE + _060_ISP_UNIMP_OFFSET)
#define ISP_060_CAS		((int ) ISP_060_BASE + _060_ISP_CAS_OFFSET)
#define ISP_060_CAS2		((int ) ISP_060_BASE + _060_ISP_CAS2_OFFSET)
#define ISP_060_CAS_FINISH	((int ) ISP_060_BASE + _060_ISP_CAS_FINISH_OFFSET)
#define ISP_060_CAS2_FINISH	((int ) ISP_060_BASE + _060_ISP_CAS2_FINISH_OFFSET)
#define ISP_060_CAS_INRANGE	((int ) ISP_060_BASE + _060_ISP_CAS_INRANGE_OFFSET)
#define ISP_060_CAS_TERMINATE	((int ) ISP_060_BASE + _060_ISP_CAS_TERMINATE_OFFSET)
#define ISP_060_CAS_RESTART	((int ) ISP_060_BASE + _060_ISP_CAS_RESTART_OFFSET)

#if defined(__STDC__) || defined(__cplusplus)

extern void	isp060COTblInit (void);
extern void	_060_isp_done (void);
extern void	_060_real_chk (void);
extern void	_060_real_divbyzero (void);
extern void	_060_real_cas (void);
extern void	_060_real_cas2 (void);
extern void	_060_real_lock_page (void);
extern void	_060_real_unlock_page (void);

#else   /* __STDC__ */

extern void	isp060COTblInit ();
extern void	_060_isp_done ();
extern void	_060_real_chk ();
extern void	_060_real_divbyzero ();
extern void	_060_real_cas ();
extern void	_060_real_cas2 ();
extern void	_060_real_lock_page ();
extern void	_060_real_unlock_page ();

#endif  /* __STDC__ */


#ifdef __cplusplus
}
#endif

#endif /* __INCisp060Libh */
