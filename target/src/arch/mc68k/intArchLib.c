/* intArchLib.c - 680x0 interrupt subroutine library */

/* Copyright 1984-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
03x,14dec00,pai  added stub routines for intEnable and intDisable (SPR #63046).
03w,10feb99,wsl  add comment documenting ERRNO value
03v,23aug95,ms   removed taskSafe/Unsafe calls from intVecSet().
03u,15jun95,ms	 added intRegsLock, intRegsUnlock
03t,17oct94,rhp  remove docn reference to intALib (SPR#3712)
03s,09oct92,rdc  intVecSet checks taskIdCurrent before TASK_{SAFE,UNSAFE}.
03r,16sep92,jdi  deleted mention of intContext() and intCount().
03q,23aug92,jcf  changed cache* to CACHE_*. changed filename.
03p,02aug92,jcf  added vxmIfVecxxx callout for monitor support.
03o,28jul92,rdc  cleaned up intVecSet and made it TASK_SAFE.
03n,27jul92,rdc  added intVecTableWriteProtect.
03m,16jul92,jwt  added 5.1 cache flush and invalidate; cleaned up 03l warnings.
03l,16jul92,rdc  intVecSet checks for write protected vector table.
03k,07jul92,yao  removed intRestrict(), intContext(), intCount(), intCnt.
03j,04jul92,jcf  cleaned up ANSI.
03i,03jul92,jwt  first pass at converting cache calls to 5.1 cache library.
03h,26may92,rrr  the tree shuffle
03g,07jan92,jcf  added i-cache coherence calls.
03g,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed copyright notice
03f,28aug91,shl  added support for MC68040.
03e,05may91,jdi	 documentation changes.
03d,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by jcf.
03c,13feb91,jaa	 documentation cleanup.
03b,28sep90,jcf  documentation.
03a,02aug90,jcf  made intRestrict() nomanual.
02z,21dec21,jcf  moved intLock()/intUnlock() to intALib.s.
02y,13aug89,gae  changed iv68k.h to iv.h.
02x,24may89,jcf  made v02w solution optional.  clean up.
02w,21apr89,jcf  fixed intUnlock () to solve psos priority inversion.
02v,23sep88,gae  documentation touchup.
02u,30aug88,gae  more documentation tweaks.
02t,18aug88,gae  documentation.
02s,12aug88,jcf  changed interrupt wrapper to better accomodate '010 wind.
02r,30may88,dnw  changed to v4 names.
02q,29oct87,gae  fixed intConnect() to use sysKernelTrap() and split
		   routine construction into intHandlerCreate().
		 made intSetVecBase() truly a NOP for 68000's.
02p,10may87,gae  changed intConnect to not rely on structure layout.
02o,27may87,llk  changed intConnect so that UI_ENTER is not included for 68020s.
02n,16mar87,dnw  fixed documentation.
		 added vxLockLevel.
02m,21dec86,dnw  changed to not get include files from default directories.
02l,29nov86,dnw  removed conditional assembly for 68020/68000.
02k,28oct86,llk  conditionally compile out vector base address code
		 for non 68020 based systems.
02j,27oct86,llk  added facilities for using the vector base register.
		 added intSetVec, intGetVec, intSetVecBase, intGetVecBase.
02i,01jul86,jlf  minor documentation
02h,14apr86,rdc  changed memAllocates to mallocs.
02g,18jan86,dnw  added intCount routine to return interrupt nesting depth.
		 included interrupt termination code, formerly in intEnd,
		   directly in driver built by intConnect.
02f,23oct85,jlf  changed stub made by intConnect to not call intEnt, but
		   rather put the same code as intEnt in-line.  This is because
		   of ISR stack switching.
02e,11oct85,dnw  de-linted.
02d,06aug85,jlf  removed include of intLib.h
02c,20jul85,jlf  documentation.
02b,31may85,jlf  added intContext and intCnt.  Made intConnect call
		 intEnt (in intALib.s) instead of trapping to UI_ENTER.
02a,05apr85,rdc  installed modifications for vrtx version 3.
01e,06sep84,ecs  removed intInit, intTask, intTlCall, and intPrintf to dbgLib.
		 got rid of includes of memLib.h, rngLib.h, and semLib.h.
01d,04sep84,dnw  changed call to fioLog to printf.
01c,22aug84,ecs  changed intPrintf to return status.
01b,17aug84,dnw  added interrupt to task-level support routines
01a,02aug84,dnw  extracted from old ubarLib
*/

/*
DESCRIPTION
This library provides architecture-dependent routines to manipulate
and connect to hardware interrupts.  Any C language routine can be
connected to any interrupt by calling intConnect().  Vectors can be
accessed directly by intVecSet() and intVecGet().  
On MC680x0 processors other than the 68000, the vector base register can be
accessed by the routines intVecBaseSet() and intVecBaseGet().

Tasks can lock and unlock interrupts by calling the routines intLock() and
intUnlock().  The lock level can be set and reported by intLockLevelSet()
and intLockLevelGet(); the default interrupt mask level is set to 7 by
kernelInit() when VxWorks is initialized.

WARNING
Do not call VxWorks system routines with interrupts locked.
Violating this rule may re-enable interrupts unpredictably.

INTERRUPT VECTORS AND NUMBERS
Most of the routines in this library take an interrupt vector as a
parameter, which is the byte offset into the vector table.  Macros are
provided to convert between interrupt vectors and interrupt numbers:
.iP IVEC_TO_INUM(intVector) 10
changes a vector to a number.
.iP INUM_TO_IVEC(intNumber)
turns a number into a vector.
.iP TRAPNUM_TO_IVEC(trapNumber)
converts a trap number to a vector.

EXAMPLE
To switch between one of several routines for a particular interrupt,
the following code fragment is one alternative:
.CS
    vector  = INUM_TO_IVEC(some_int_vec_num);
    oldfunc = intVecGet (vector);
    newfunc = intHandlerCreate (routine, parameter);
    intVecSet (vector, newfunc);
    ...
    intVecSet (vector, oldfunc);    /@ use original routine @/
    ...
    intVecSet (vector, newfunc);    /@ reconnect new routine @/
.CE

INCLUDE FILE: iv.h
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "cacheLib.h"
#include "errno.h"
#include "intLib.h"
#include "memLib.h"
#include "sysLib.h"
#include "taskLib.h"
#include "string.h"
#include "stdlib.h"
#include "private/vxmIfLibP.h"
#include "private/vmLibP.h"

IMPORT void intVBRSet (FUNCPTR *baseAddr);

IMPORT void intEnt ();		/* interrupt entrance stub */
IMPORT void intExit ();		/* interrupt exit stub */

#define	HI_WORD(w)		(short)(((int)(w) & 0xffff0000) >> 16)
#define	LO_WORD(w)		(short)((int)(w) & 0x0000ffff)


/* globals */

/* The routine intLock(), found in intALib.s uses intLockMask to construct a
 * new SR with the correct interrupt lock-out level.  The difficulty is
 * intLock() may be called from either interrupt level, or task level, so
 * simply reserving a SR suchas 0x3700 does not work because such a SR would
 * assume task-level code.
 */

USHORT intLockMask = 0x0700;	/* interrupt lock mask - default level 7 */

/* The kernel also locks interrupts but unlike intLock() it knows which stack
 * is being used so intLockIntSR is a status register to lock interrupts from
 * the interrupt stack, and intLockTaskSR is a status register to lock
 * interrupts from the task stack.  These SRs are updated by
 * intLockLevelSet().  It is faster to move these SRs into the
 * SR, then to 'or' in the intLockMask, because there is no: or.w <ea>,SR.
 */

USHORT intLockIntSR  = 0x2700;	/* SR for locking interrupts from int. level */
USHORT intLockTaskSR = 0x3700;	/* SR for locking interrupts from task level */

LOCAL int  dummy (void) { return ERROR; }  /* dummy, returns ERROR   */
FUNCPTR    sysIntLvlEnableRtn  = dummy;    /* enable a single level  */
FUNCPTR    sysIntLvlDisableRtn = dummy;    /* disable a single level */


/* locals */

LOCAL FUNCPTR *intVecBase = 0;		/* vector base address */

LOCAL USHORT intConnectCode []	=	/* intConnect stub */
    {
/*
*0x00  4EB9 kkkk kkkk	jsr	_intEnt  	  * tell kernel
*0x06  48E7 E0C0	movem.l	d0-d2/a0-a1,-(a7) * save regs
*0x0a  2F3C pppp pppp	move.l	#parameter,-(a7)  * push param
*0x10  4EB9 rrrr rrrr	jsr	routine		  * call C routine
*0x16  588F		addq.l  #4,a7             * pop param
*0x18  4CDF 0307	movem.l (a7)+,d0-d2/a0-a1 * restore regs
*0x1c  4EF9 kkkk kkkk	jmp     _intExit          * exit via kernel
*/
     0x4eb9, 0x0000, 0x0000,	/* _intEnt filled in at runtime */
     0x48e7, 0xe0c0,
     0x2f3c, 0x0000, 0x0000,	/* parameter filled in at runtime */
     0x4eb9, 0x0000, 0x0000,	/* routine to be called filled in at runtime */
     0x588f,
     0x4cdf, 0x0307,
     0x4ef9, 0x0000, 0x0000,	/* _intExit filled in at runtime */
    };


/* forward declarations */

FUNCPTR	intHandlerCreate ();
FUNCPTR	*intVecBaseGet ();
FUNCPTR	intVecGet ();


/*******************************************************************************
*
* intConnect - connect a C routine to a hardware interrupt
*
* This routine connects a specified C routine to a specified interrupt
* vector.  The address of <routine> is stored at <vector> so that <routine>
* is called with <parameters> when the interrupt occurs.  The routine is
* invoked in supervisor mode at interrupt level.  A proper C environment
* is established, the necessary registers saved, and the stack set up.
*
* The routine can be any normal C code, except that it must not invoke
* certain operating system functions that may block or perform I/O
* operations.
*
* This routine simply calls intHandlerCreate() and intVecSet().  The address
* of the handler returned by intHandlerCreate() is what actually goes in
* the interrupt vector.
*
* RETURNS:
* OK, or
* ERROR if the interrupt handler cannot be built.
*
* SEE ALSO: intHandlerCreate(), intVecSet()
*/

STATUS intConnect
    (
    VOIDFUNCPTR *vector,            /* interrupt vector to attach to */
    VOIDFUNCPTR routine,            /* routine to be called */
    int parameter               /* parameter to be passed to routine */
    )
    {
    FUNCPTR intDrvRtn = intHandlerCreate ((FUNCPTR) routine, parameter);

    if (intDrvRtn == NULL)
	return (ERROR);

    /* make vector point to synthesized code */

    intVecSet ((FUNCPTR *) vector, (FUNCPTR) intDrvRtn);

    return (OK);
    }

/*******************************************************************************
*
* intEnable - enable a specific interrupt level
*
* Enable a specific interrupt level.  For each interrupt level to be used,
* there must be a call to this routine before it will be
* allowed to interrupt.
*
* RETURNS:
* OK or ERROR for invalid arguments.
*/
int intEnable
    (
    int level   /* level to be enabled */
    )
    {
    return (*sysIntLvlEnableRtn) (level);
    }

/*******************************************************************************
*
* intDisable - disable a particular interrupt level
*
* This call disables a particular interrupt level, regardless of the current
* interrupt mask level.
*
* RETURNS:
* OK or ERROR for invalid arguments.
*/
int intDisable
    (
    int level   /* level to be disabled */
    )
    {
    return (*sysIntLvlDisableRtn) (level);
    }

/*******************************************************************************
*
* intHandlerCreate - construct an interrupt handler for a C routine
*
* This routine builds an interrupt handler around the specified C routine.
* This interrupt handler is then suitable for connecting to a specific
* vector address with intVecSet().  The routine will be invoked in
* supervisor mode at interrupt level.  A proper C environment is
* established, the necessary registers saved, and the stack set up.
*
* The routine can be any normal C code, except that it must not invoke
* certain operating system functions that may block or perform I/O
* operations.
*
* IMPLEMENTATION:
* This routine builds an interrupt handler of the following form in
* allocated memory.
*
* .CS
*     0x00  4EB9 kkkk kkkk jsr     _intEnt           * tell kernel
*     0x06  48E7 E0C0      movem.l d0-d2/a0-a1,-(a7) * save regs
*     0x0a  2F3C pppp pppp move.l  #parameter,-(a7)  * push param
*     0x10  4EB9 rrrr rrrr jsr     routine           * call C routine
*     0x16  588F           addq.l  #4,a7             * pop param
*     0x18  4CDF 0307      movem.l (a7)+,d0-d2/a0-a1 * restore regs
*     0x1c  4EF9 kkkk kkkk jmp     _intExit          * exit via kernel
* .CE
*
* RETURNS: A pointer to the new interrupt handler,
* or NULL if the routine runs out of memory.
*/

FUNCPTR intHandlerCreate
    (
    FUNCPTR routine,            /* routine to be called */
    int parameter               /* parameter to be passed to routine */
    )
    {
    FAST USHORT *pCode;		/* pointer to newly synthesized code */

    pCode = (USHORT *)malloc (sizeof (intConnectCode));

    if (pCode != NULL)
	{
	/* copy intConnectCode into new code area */

	bcopy ((char *)intConnectCode, (char *)pCode, sizeof (intConnectCode));

	/* set the addresses & instructions */

	pCode [1]  = HI_WORD (intEnt);
	pCode [2]  = LO_WORD (intEnt);
	pCode [6]  = HI_WORD (parameter);
	pCode [7]  = LO_WORD (parameter);
	pCode [9]  = HI_WORD (routine);
	pCode [10] = LO_WORD (routine);
	pCode [15] = HI_WORD (intExit);
	pCode [16] = LO_WORD (intExit);
	}

    CACHE_TEXT_UPDATE ((void *) pCode, sizeof (intConnectCode));

    return ((FUNCPTR) (int) pCode);
    }
/*******************************************************************************
*
* intLockLevelSet - set the current interrupt lock-out level
*
* This routine sets the current interrupt lock-out level and stores it in
* the globally accessible variable `intLockMask'.  The specified
* interrupt level will be masked when interrupts are locked by intLock().
* The default lock-out level is 7 for MC680x0 processors, and is initially
* set by kernelInit() when VxWorks is initialized.
*
* RETURNS: N/A
*
* SEE ALSO: intLockLevelGet()
*/

void intLockLevelSet
    (
    int newLevel                /* new interrupt level */
    )
    {
    int oldSR = intLock ();			/* LOCK INTERRUPTS */

    intLockMask    = (USHORT) (newLevel << 8);
    intLockIntSR  &= 0xf8ff;
    intLockTaskSR &= 0xf8ff;
    intLockIntSR  |= intLockMask;
    intLockTaskSR |= intLockMask;

    intUnlock (oldSR);				/* UNLOCK INTERRUPTS */
    }
/*******************************************************************************
*
* intLockLevelGet - get the current interrupt lock-out level
*
* This routine returns the current interrupt lock-out level, which is
* set by intLockLevelSet() and stored in the globally accessible
* variable `intLockMask'.  This is the interrupt level currently
* masked when interrupts are locked out by intLock().  The default
* lock-out level is 7 for 68K processors, and is initially set by
* kernelInit() when VxWorks is initialized.
*
* RETURNS:
* The interrupt level currently stored in the interrupt lock-out mask.
*
* SEE ALSO: intLockLevelSet() 
*/

int intLockLevelGet (void)
    {
    return ((int)(intLockMask >> 8));
    }
/*******************************************************************************
*
* intVecBaseSet - set the vector base address
*
* This routine sets the vector base address.  The CPU's vector base register
* is set to the specified value, and subsequent calls to intVecGet() or
* intVecSet() will use this base address.  The vector base address is
* initially 0, until modified by calls to this routine.
*
* NOTE:
* The 68000 has no vector base register;
* thus, this routine is thus a no-op for 68000 systems.
*
* RETURNS: N/A
*
* SEE ALSO: intVecBaseGet(), intVecGet(), intVecSet()
*/

void intVecBaseSet
    (
    FUNCPTR *baseAddr       /* new vector base address */
    )
    {
    if (sysCpu == MC68000)
	return;

    intVecBase = baseAddr;	/* keep the base address in a static variable */

    intVBRSet (baseAddr);	/* set the actual vector base register */

    CACHE_TEXT_UPDATE ((void *) baseAddr, 256  * sizeof (FUNCPTR));
    }
/*******************************************************************************
*
* intVecBaseGet - get the vector base address
*
* This routine returns the current vector base address, which is set
* with intVecBaseSet().
*
* RETURNS: The current vector base address.
*
* SEE ALSO: intVecBaseSet()
*/

FUNCPTR *intVecBaseGet (void)
    {
    return (intVecBase);
    }
/******************************************************************************
*
* intVecSet - set a CPU vector
*
* This routine attaches an exception/interrupt handler to a specified vector.
* The vector is specified as an offset into the CPU's vector table.  This
* vector table starts, by default, at address 0.  However on 68010 and 68020
* CPUs, the vector table may be set to start at any address with the
* intVecBaseSet() routine.  The vector table is set up in usrInit() and
* starts at the lowest available memory address.
*
* RETURNS: N/A
*
* SEE ALSO: intVecBaseSet(), intVecGet()
*/

void intVecSet
    (
    FUNCPTR *	vector,		/* vector offset */
    FUNCPTR	function	/* address to place in vector */
    )
    {
    FUNCPTR *	newVector;
    UINT	state;
    BOOL	writeProtected = FALSE;
    int		pageSize = 0;
    char *	pageAddr = 0;

    if (VXM_IF_VEC_SET (vector, function) == OK)	/* can monitor do it? */
	return;

    /* vector is offset by the vector base address */

    newVector = (FUNCPTR *) ((int) vector + (int) intVecBaseGet ());

    /* see if we need to write enable the memory */

    if (vmLibInfo.vmLibInstalled)
	{
	pageSize = VM_PAGE_SIZE_GET();

	pageAddr = (char *) ((UINT) newVector / pageSize * pageSize);

	if (VM_STATE_GET (NULL, (void *) pageAddr, &state) != ERROR)
	    if ((state & VM_STATE_MASK_WRITABLE) == VM_STATE_WRITABLE_NOT)
		{
		writeProtected = TRUE;
		VM_STATE_SET (NULL, pageAddr, pageSize, VM_STATE_MASK_WRITABLE,
			      VM_STATE_WRITABLE);
		}

	}

    *newVector = function;

    if (writeProtected)
	{
	VM_STATE_SET (NULL, pageAddr, pageSize, 
		      VM_STATE_MASK_WRITABLE, VM_STATE_WRITABLE_NOT);
	}

    CACHE_TEXT_UPDATE ((void *) newVector, sizeof (FUNCPTR));
    }
/*******************************************************************************
*
* intVecGet - get a vector
* 
* This routine returns a pointer to the exception/interrupt handler
* attached to a specified vector.  The vector is specified as an
* offset into the CPU's vector table.  This vector table starts, by
* default, at address 0.  However, on 68010 and 68020 CPUs, the vector
* table may be set to start at any address with intVecBaseSet().
*
* RETURNS
* A pointer to the exception/interrupt handler attached to the specified vector.
*
* SEE ALSO: intVecSet(), intVecBaseSet()
*/

FUNCPTR intVecGet
    (
    FUNCPTR *vector     /* vector offset */
    )
    {
    FUNCPTR vec;

    if ((vec = VXM_IF_VEC_GET (vector)) != NULL)	/* can monitor do it? */
	return (vec);

    /* vector is offset by vector base address */

    return (* (FUNCPTR *) ((int) vector + (int) intVecBaseGet ()));
    }

/*******************************************************************************
*
* intVecTableWriteProtect - write protect exception vector table
*
* If the unbundled Memory Management Unit (MMU) support package (VxVMI) is
* present, this routine write-protects the exception vector table to
* protect it from being accidentally corrupted.
*
* Note that other data structures contained in the page will also be
* write-protected.  In the default VxWorks configuration, the exception
* vector table is located at location 0 in memory.  Write-protecting this
* affects the backplane anchor, boot configuration information, and potentially
* the text segment (assuming the default text location of 0x1000.)  All code
* that manipulates these structures has been modified to write-enable the 
* memory for the duration of the operation.  If you select a different 
* address for the exception vector table, be sure it resides 
* in a page separate from other writable data structures.
* 
* RETURNS: OK, or ERROR if unable to write protect memory.
*
* ERRNO: S_intLib_VEC_TABLE_WP_UNAVAILABLE
*/

STATUS intVecTableWriteProtect
    (
    void
    )
    {
    int pageSize;
    UINT vectorPage;

    if (!vmLibInfo.vmLibInstalled)
	{
	errno = S_intLib_VEC_TABLE_WP_UNAVAILABLE;
	return (ERROR);
	}

    pageSize = VM_PAGE_SIZE_GET();

    vectorPage = (UINT) intVecBaseGet () / pageSize * pageSize;

    return (VM_STATE_SET (0, (void *) vectorPage, pageSize, 
			  VM_STATE_MASK_WRITABLE, VM_STATE_WRITABLE_NOT));
    }

/******************************************************************************
*
* intRegsLock - modify a REG_SET to have interrupts locked.
*/ 

int intRegsLock
    (
    REG_SET *pRegs			/* register set to modify */
    )
    {
    int oldSr = pRegs->sr;
    pRegs->sr |= intLockMask;
    return (oldSr);
    }

/******************************************************************************
*
* intRegsUnlock - restore an REG_SET's interrupt lockout level.
*/ 

void intRegsUnlock
    (
    REG_SET *	pRegs,			/* register set to modify */
    int		oldSr			/* sr with int lock level to restore */
    )
    {
    pRegs->sr &= (~0x0700);
    pRegs->sr |= (oldSr & 0x0700);
    }

