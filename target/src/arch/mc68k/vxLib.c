/* vxLib.c - MC680X0 miscellaneous support routines */

/* Copyright 1984-1997 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02d,04jun97,dat  added _func_vxMemProbeHook and vxMemArchProbe, SPR 8658.
		 removed sysMemProbe().
02c,04jul92,jcf  ansi cleanup.
02b,01jul92,jcf  restored RCS.
02a,22jun92,yao  moved from util/vxLib.c ver01u.
01w,26jun92,rrr  fixed ifdef MIPS so it wouldn't take out every architecture
01v,24jun92,ajm  got rid of mips stuff and use vxLib.c in arch/mips.  This
		  should be cleaned up soon!
01u,26may92,rrr  the tree shuffle
01t,22apr92,jwt  converted CPU==SPARC to CPU_FAMILY==SPARC; copyright.
01s,16dec91,gae  added includes for ANSI.
01r,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -fixed #else and #endif
		  -changed TINY and UTINY to INT8 and UINT8
		  -changed READ, WRITE to VX_READ, VX_WRITE
		  -changed VOID to void
		  -changed copyright notice
01q,25sep91,yao  added CPU32.
01p,09aug91,ajm  many changes for MIPS R3000.
            rtp  moved vxTas, vxMemProbeTrap here.
02o,25may91,gae  moved vxLsBitIndexTable & vxMsBitIndexTable here from sysLib.c.
01o,12jul91,gae  reworked i960 vxMemProbe(); defined sysBErrVec here.
01n,09jul91,del  fixed vxMemProbe (I960) to use sysBErrVec global as bus
		 error vector. Installed new vxTas that will work on
		 non-aligned bytes.
01m,01jun91,gae  fixed return of 960's vxTas().
01l,24may91,jwt  added ASI parameters to create vxMemProbeAsi.
01n,29apr91,hdn  added defines and macros for TRON architecture.
01m,29apr91,del  reference sysMemProbe trap for board dependent reasons.
01l,28apr91,del  I960 integration. added  vxMemProbeSup and vxTas here,
		 as callouts to system (board) dependent functions because
		 some i960 implementations require different interfaces
		 to hardware.
01k,30mar91,jdi  documentation cleanup; doc review by dnw.
01j,19mar90,jdi  documentation cleanup.
01i,22aug89,jcf  fixed vxMemProbe for 68020/68030.
01h,29apr89,mcl  vxMemProbe for SPARC; merged versions.
01g,01sep88,gae  documentation.
01f,22jun88,dnw  removed include of ioLib.h.
01e,05jun88,dnw  changed from kLib to vxLib.
		 removed taskRegsShow(), exit(), and breakpoint rtns to taskLib.
01d,30may88,dnw  changed to v4 names.
01c,28may88,dnw  removed reboot to rebootLib.
01b,21apr88,gae  added include of ioLib.h for O_RDONLY/O_WRONLY/O_RDWR.
01a,28jan88,jcf	 written and soon to be redistributed.
*/

/*
DESCRIPTION
This module contains miscellaneous VxWorks support routines for Motorola 680X0.

SEE ALSO: vxALib
*/

#include "vxWorks.h"
#include "taskLib.h"
#include "intLib.h"
#include "iv.h"
#include "esf.h"

/* externals */

IMPORT vxMemProbeTrap ();
IMPORT vxMemProbeSup (int length, char *adrs, char *pVal);

FUNCPTR	_func_vxMemProbeHook = NULL;	/* hook for BSP vxMemProbe */

/*******************************************************************************
*
* vxMemArchProbe - architecture specific probe routine (mc68k)
*
* This is the routine implementing the architecture specific part of the
* vxMemProbe routine.  It traps the relevant
* exceptions while accessing the specified address.
*
* RETURNS: OK or ERROR if an exception occurred during access.
*
* INTERNAL
* This routine functions by setting the machine check, data access and
* alignment exception vector to vxMemProbeTrap and then trying to read/write 
* the specified byte. If the address doesn't exist, or access error occurs,
* vxMemProbeTrap will return ERROR.  Note that this routine saves and restores 
* the exception vectors that were there prior to this call.  The entire 
* procedure is done with interrupts locked out.
*/

STATUS vxMemArchProbe 
    (
    FAST void *adrs,    /* address to be probed          */
    int mode,           /* VX_READ or VX_WRITE                 */
    int length,         /* 1, 2, or 4                    */
    FAST void *pVal     /* where to return value,        */
                        /* or ptr to value to be written */
    )
    {
    STATUS status;
    int oldLevel;
    FUNCPTR oldVec1;

    switch (length)
        {
        case (1):
	    break;

        case (2):
#if     (CPU==MC68000 || CPU==MC68010 || CPU== CPU32)
            if (((int) adrs & 0x1) || ((int) pVal & 0x1))
                return (ERROR);
#endif	/* CPU==MC68000 || CPU==MC68010 || CPU==CPU32 */
            break;

        case (4):
#if     (CPU==MC68000 || CPU==MC68010 || CPU==CPU32)
            if (((int) adrs & 0x1) || ((int) pVal & 0x1))
                return (ERROR);
#endif	/* CPU==MC68000 || CPU==MC68010 || CPU==CPU32 */
            break;

        default:
            return (ERROR);
        }

    oldLevel = intLock ();			/* lock out CPU */

    oldVec1 = intVecGet ((FUNCPTR *)IV_BUS_ERROR);	/* save bus error vec */
    intVecSet ((FUNCPTR *)IV_BUS_ERROR, vxMemProbeTrap);/* replace berr vec */

    /* do probe */

    if (mode == VX_READ)
	status = vxMemProbeSup (length, adrs, pVal);
    else
	status = vxMemProbeSup (length, pVal, adrs);

    /* restore original vector(s) and unlock */

    intVecSet ((FUNCPTR *)IV_BUS_ERROR, oldVec1);

    intUnlock (oldLevel);

    return (status);

    }

/*******************************************************************************
*
* vxMemProbe - probe an address for a bus error
*
* This routine probes a specified address to see if it is readable or
* writable, as specified by <mode>.  The address will be read or written as
* 1, 2, or 4 bytes as specified by <length> (values other than 1, 2, or 4
* yield unpredictable results).  If the probe is a O_RDONLY, the value read will
* be copied to the location pointed to by <pVal>.  If the probe is a O_WRONLY,
* the value written will be taken from the location pointed to by <pVal>.
* In either case, <pVal> should point to a value of 1, 2, or 4 bytes, as
* specified by <length>.
*
* Note that only data bus errors (machine check exception,  data access 
* exception) are trapped during the probe, and that the access must be 
* otherwise valid (i.e., not generate an address error).
*
* EXAMPLE
* .CS
* testMem (adrs)
*    char *adrs;
*    {
*    char testW = 1;
*    char testR;
*
*    if (vxMemProbe (adrs, VX_WRITE, 1, &testW) == OK)
*        printf ("value %d written to adrs %x\en", testW, adrs);
*
*    if (vxMemProbe (adrs, VX_READ, 1, &testR) == OK)
*        printf ("value %d read from adrs %x\en", testR, adrs);
*    }
* .CE
*
* MODIFICATION
* The BSP can modify the behaviour of this routine by supplying an alternate
* routine and placing the address of the routine in the global variable
* _func_vxMemProbeHook.  The BSP routine will be called instead of the
* architecture specific routine vxMemArchProbe().
*
* RETURNS:
* OK if the probe is successful, or
* ERROR if the probe caused a bus error.
*
* SEE ALSO: vxMemArchProbe()
*/

STATUS vxMemProbe
    (
    FAST char *adrs,	/* address to be probed */
    int mode,		/* VX_READ or VX_WRITE */
    int length,		/* 1, 2, or 4 */
    char *pVal 		/* where to return value, */
			/* or ptr to value to be written */
    )
    {
    STATUS status;

    if (_func_vxMemProbeHook != NULL)

	/* BSP specific probe routine */

	status = (* _func_vxMemProbeHook) (adrs, mode, length, pVal);
    else

	/* architecture specific probe routine */

	status = vxMemArchProbe (adrs, mode, length, pVal);
    
    return status;
    }
