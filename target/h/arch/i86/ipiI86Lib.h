/* ipiI86Lib.h - I80X86 IPI library header */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,28feb02,hdn  took IPI_MAX_XXX macros from ipiArchLib.c
01b,27feb02,hdn  renamed ipiStubShutdown() to ipiShutdownSup()
01a,20feb02,hdn  written
*/

#ifndef __INCipiI86Libh
#define __INCipiI86Libh

#ifdef __cplusplus
extern "C" {
#endif


/* defines */

#define	IPI_MAX_HANDLERS	8	/* max number of handlers */
#define	IPI_MAX_RETRIES		3	/* max number of retries */


#ifndef	_ASMLANGUAGE

/* variable declarations */

IMPORT char	ipiCallTbl [];		/* table of IPI stub calls */


/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

extern void	ipiVecInit (UINT32 intNum);
extern void	ipiHandler (UINT32 index);
extern STATUS	ipiConnect (UINT32 intNum, VOIDFUNCPTR routine);
extern STATUS	ipiStartup (UINT32 apicId, UINT32 vector, UINT32 nTimes);
extern STATUS	ipiShutdown (UINT32 apicId, UINT32 vector);
extern void	ipiStub (void);
extern void	ipiShutdownSup (void);
extern void	ipiHandlerTlbFlush (void);
extern void	ipiHandlerTscReset (void);
extern void	ipiHandlerShutdown (void);

#else	/* __STDC__ */

extern void	ipiVecInit ();
extern void	ipiHandler ();
extern STATUS	ipiConnect ();
extern STATUS	ipiStartup ();
extern STATUS	ipiShutdown ();
extern void	ipiStub ();
extern void	ipiShutdownSup ();
extern void	ipiHandlerTlbFlush ();
extern void	ipiHandlerTscReset ();
extern void	ipiHandlerShutdown ();

#endif	/* __STDC__ */


#endif	/* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __INCipiI86Libh */
