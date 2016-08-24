/* intI86Lib.h - I86-specific interrupt library header file */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,09nov01,hdn  added intConnectCode offset macros
01a,29aug01,hdn  taken from T31 ver 01b.
*/

#ifndef __INCintI86Libh
#define __INCintI86Libh

#ifdef __cplusplus
extern "C" {
#endif


/* offset macro for stuff in the intConnect code */

#define	ICC_INT_ENT		1	/* intConnectCode[1] */
#define	ICC_BOI_PUSH		8	/* intConnectCode[8] */
#define	ICC_BOI_PARAM		9	/* intConnectCode[9] */
#define	ICC_BOI_ROUTN		14	/* intConnectCode[14] */
#define	ICC_INT_PARAM		19	/* intConnectCode[19] */
#define	ICC_INT_ROUTN		24	/* intConnectCode[24] */
#define	ICC_EOI_PARAM		29	/* intConnectCode[29] */
#define	ICC_EOI_CALL		33	/* intConnectCode[33] */
#define	ICC_EOI_ROUTN		34	/* intConnectCode[34] */
#define	ICC_ADD_N		40	/* intConnectCode[40] */
#define	ICC_INT_EXIT		45	/* intConnectCode[45] */


#if defined(__STDC__) || defined(__cplusplus)

extern FUNCPTR 	intHandlerCreateI86 (FUNCPTR routine, int parameter,
				     FUNCPTR routineBoi, int parameterBoi,
				     FUNCPTR routineEoi, int parameterEoi);
extern void 	intVecSet2 (FUNCPTR * vector, FUNCPTR function,
			    int idtGate, int idtSelector);
extern void	intVecGet2 (FUNCPTR * vector, FUNCPTR * pFunction, 
			    int * pIdtGate, int * pIdtSelector);

#else /* defined(__STDC__) || defined(__cplusplus) */

extern FUNCPTR 	intHandlerCreateI86 ();
extern void 	intVecSet2 ();
extern void	intVecGet2 ();

#endif /* defined(__STDC__) || defined(__cplusplus) */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INCintI86Libh */
