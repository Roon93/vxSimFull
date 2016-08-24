/* smNetLib.h	- VxWorks specific backplane driver header file */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01m,03may02,mas  made pPkt volatile pointer in loanBuild() (SPR 68334)
01l,21mar02,vvv  added doc for copyFromMbufs (SPR #20787)
01k,02oct01,mas  added support for all 8 NETIF arguments in smNetAttach2 (4547)
01j,20aug96,vin  commented out do_protocol
01i,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01h,22sep92,rrr  added support for c++
01g,27jul92,elh  Added smNetShowInit.
01f,15jun92,elh  changed parameter to smNetInetGet.
01e,02jun92,elh  the tree shuffle
01d,27may92,elh  Incorperated the changes caused from restructuring the
		 shared memory libraries.  Renamed from smVxLib.h
01c,02may92,elh  added mask to smNetInit
01b,10apr92,elh  added startAddr for sequential addressing.
01a,17nov90,elh  written.
*/

#ifndef __INCsmNetLibh
#define __INCsmNetLibh

#ifdef __cplusplus
extern "C" {
#endif

/* includes */

#include "vxWorks.h"
#include "net/mbuf.h"
#include "net/if.h"
#include "netinet/in.h"
#include "netinet/in_var.h"
#include "netinet/if_ether.h"
#include "if_sm.h"
#include "errno.h"
#include "sys/ioctl.h"
#include "etherLib.h"
#include "net/unixLib.h"
#include "net/if_subr.h"
#include "inetLib.h"
#include "stdio.h"

/* defines */

#define	NSM		1

/* macros needed by backplane driver */

IMPORT SM_SOFTC *		sm_softc[];

#define UNIT_TO_SOFTC(unit)	(sm_softc [unit])
#define etherAddrPtr(eaddr)	(eaddr)

/* 
 * copyFromMbufs should not be used since it may cause unaligned memory
 * accesses.
 */

#define copyFromMbufs(pData, pMbuf, len)			\
		copy_from_mbufs ((pData), (pMbuf), (len))

#define copyToMbufs(pData, len, off, pIf) 			\
		copy_to_mbufs ((pData), (len), (off), (pIf))

						/* hooks */
#define outputHook(pIf, pData, bytes)				\
	((etherOutputHookRtn != NULL) && 			\
	((*etherOutputHookRtn) ((pIf), (pData), (bytes)))) ?	\
	(TRUE) : (FALSE)

#define inputHook(pIf, pData, bytes)				\
	((etherInputHookRtn != NULL) &&				\
	((*etherInputHookRtn)((pIf), (pData), (bytes)))) ?	\
	(TRUE) : (FALSE)

#define deviceValid(unit)	((sm_softc [unit] != NULL) ? TRUE : FALSE)

/* function prototypes */

#if defined(__STDC__) || defined(__cplusplus)

extern STATUS smNetInit (SM_ANCHOR * pAnchor, char * pMem, int memSize,
                         int tasType, int cpuMax, int maxPktBytes,
                         u_long startAddr);
extern STATUS smNetAttach (int unit, SM_ANCHOR *pAnchor, int maxInputPkts,
                           int intType, int intArg1, int intArg2, int intArg3);
extern STATUS smNetAttach2 (int unit, SM_ANCHOR *pAnchor, int maxInputPkts,
                            int intType, int intArg1, int intArg2, int intArg3,
                            int masterCpu, int maxTicsWait);
extern STATUS smNetInetGet (char * smName, char * smInet, int cpuNum);
extern void   smNetShowInit (void);
extern STATUS smNetShow (char *	ifName, BOOL zero);
struct mbuf * loanBuild (SM_SOFTC * xs, SM_PKT volatile * pPkt, u_char * pData,
                         int len);

#else

extern STATUS smNetInit ();
extern STATUS smNetAttach ();
extern STATUS smNetInetGet ();
extern void   smNetShowInit ();
extern STATUS smNetShow ();
struct	mbuf * loanBuild ();

#endif	/* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif /* __INCsmNetLibh */

