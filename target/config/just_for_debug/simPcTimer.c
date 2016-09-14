/* simPcTimer.c - Windows simulator timer library */

/* Copyright 1984-2003 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,07jan03,jeg  remove a build warning
01c,16dec02,jeg	 add timestamp timer support (SPR #68971).
01b,13sep01,hbh  Fixed SPR 63532 to be able to modify the system clock rate.
		 Code cleanup.
01a,07jul98,cym  written.
*/

/*
DESCRIPTION
This driver exposes windows timer objects in a vxWorks way, allowing
them to be used as the system and auxiliary clocks.  Windows timers are cyclic,
and can be set to the millisecond.  All timers send the WM_TIMER message/
interrupt, which also passes in a timer ID.  This driver's multiplexer then
calls the appropriate driver function based on the ID.

The macros SYS_CLK_RATE_MIN, SYS_CLK_RATE_MAX, AUX_CLK_RATE_MIN, and
AUX_CLK_RATE_MAX must be defined to provide parameter checking for the
sys[Aux]ClkRateSet() routines.

If INCLUDE_TIMESTAMP is defined, a timestamp driver based on the windows
performance counter is provided.

INCLUDES:
win_Lib.h
intLib.h
*/


/* includes */

#include "win_Lib.h"
#include "intLib.h"

/* defines */

#define TIMER_VEC 0x0113 /* from windows headers */

#ifdef INCLUDE_TIMESTAMP

/* maximum value of the timestamp counter before a rollover */

#define TIMESTAMP_MAX_VALUE	0xFFFFFFFF

#endif /*INCLUDE_TIMESTAMP*/

/* locals */

#ifdef INCLUDE_TIMESTAMP
LOCAL 	FUNCPTR sysTimestampRoutine = NULL;     /* rollover routine          */
LOCAL 	int	sysTimestampArg;		/* rollover routine argument */	
LOCAL   int     sysTimestampRunning = FALSE;	/* timestamp driver 	     */
						/* activated		     */ 
LOCAL 	BOOL	rolloverDetected    = FALSE;	/* a rollover is detected    */
LOCAL   UINT64  timestampRef	    = 0;	/* performance counter value */
						/* when enabling the driver  */
LOCAL 	UINT32 	predTimestampValue  = 0;	/* previous timestamp value  */
LOCAL   UINT32	nbRollOver	    = 0;        /* number of rollover	     */ 
						/* detected		     */ 
#endif /*INCLUDE_TIMESTAMP*/

LOCAL FUNCPTR	clockRoutines[2];		/* Handler for SYS/AUX clock */
LOCAL int 	clockArgs[2];			/* Args for SYS/AUX handler  */
LOCAL int 	clockRate[2] = {60,60};		/* Rate for SYS/AUX clock    */
LOCAL int 	clockEnable[2];			/* Enable flag for SYS/AUX   */

/* forward declarations */

#ifdef INCLUDE_TIMESTAMP

LOCAL void sysTimestampInt ();

#endif /*INCLUDE_TIMESTAMP*/

/***************************************************************************
*
* sysClkInt - interrupt level processing for system clock
*
* This routine handles a clock interrupt.  It demultiplexes the interrupt,
* and calls the routine installed by sysClkConnect() for that timer.
*/

LOCAL void sysClkInt
    (
    int zero,		/* ignore the parameter 	*/
    int timerId		/* 1 = sysClk 2 = sysAuxClk 	*/
    )
    {
    if (timerId == 1) clockRoutines[0](clockArgs[0]);
    if (timerId == 2) clockRoutines[1](clockArgs[1]);

#ifdef INCLUDE_TIMESTAMP
    /*
     * if a rollover is detected, then a ROLL_OVER event is posted and the 
     * rollover counter is incremented.
     */

    if (rolloverDetected)
	{
	rolloverDetected = FALSE;
	sysTimestampInt ();
	nbRollOver ++;
	}
#endif /*INCLUDE_TIMESTAMP*/
    }

/***************************************************************************
*
* sysClkConnect - connect a routine to the system clock interrupt
*
* This routine specifies the interrupt service routine to be called at each
* clock interrupt from the system timer.  Normally, it is called from usrRoot()
* in usrConfig.c to connect usrClock() to the system clock interrupt.
*
* RETURN: OK, or ERROR if the routine cannot be connected to the interrupt.
*
* SEE ALSO: intConnect(), usrClock(), sysClkEnable()
*/

STATUS sysClkConnect
    (
    FUNCPTR	routine,
    int		arg
    )
    {
    int key = intLock ();		/* INTERRUPTS LOCKED */
 
    clockRoutines[0] = routine;
    clockArgs[0] = arg;
    intUnlock (key);

    intConnect ((VOIDFUNCPTR *)TIMER_VEC, (VOIDFUNCPTR)sysClkInt, 0);

    return(0);
    }

/***************************************************************************
*
* sysClkDisable - turn off system clock interrupts
*
* This routine disables system clock interrupts.
*
* RETURNS: N/A
*
* SEE ALSO: sysClkEnable()
*/

void sysClkDisable(void)
    {
    int key = intLock ();		/* INTERRUPTS LOCKED */
    clockEnable[0] = 0;
    win_KillTimer (1);
    intUnlock (key);
    }

/***************************************************************************
*
* sysClkEnable - turn on system clock interrupts
*
* This routine enables system clock interrupts.
*
* RETURNS: N/A
*
* SEE ALSO: sysClkConnect(), sysClkDisable(), sysClkRateSet()
*/

void sysClkEnable(void)
    {
    int key = intLock();		/* INTERRUPTS LOCKED */
    clockEnable[0] = 1;
    win_SetTimer (1, 1000 / clockRate[0]);
    intUnlock (key);
    }

/***************************************************************************
*
* sysClkRateGet - get the system clock rate
*
* This routine returns the system clock rate.
*
* RETURNS: The number of ticks per second of the system clock.
*
* SEE ALSO: sysClkEnable(), sysClkRateSet()
*/

int sysClkRateGet(void)
    {
    return (clockRate[0]);
    }

/***************************************************************************
*
* sysClkRateSet - set the system clock rate
*
* This routine sets the interrupt rate of the system clock.
* It is called by usrRoot() in usrConfig.c.
*
* RETURNS: OK, or ERROR if the tick rate is invalid or the timer cannot be set.
*
* SEE ALSO: sysClkEnable(), sysClkRateGet()
*/

STATUS sysClkRateSet
    (
    int rate
    )
    {
    if ( (rate < SYS_CLK_RATE_MIN) || (rate > SYS_CLK_RATE_MAX) )
        return (ERROR);
    clockRate[0] = rate;
    if (clockEnable[0])
        {
        win_KillTimer (1);
        win_SetTimer (1, 1000 / clockRate[0]);
        }
    return (OK);
    }

/***************************************************************************
*
* sysAuxClkConnect - connect a routine to the auxiliary clock interrupt
*
* This routine specifies the interrupt service routine to be called at each
* auxiliary clock interrupt.  It does not enable auxiliary clock interrupts.
*
* RETURNS: OK, or ERROR if the routine cannot be connected to the interrupt.
*
* SEE ALSO: intConnect(), sysAuxClkEnable()
*/

STATUS sysAuxClkConnect
    (
    FUNCPTR	routine,
    int 	arg
    )
    {
    int key = intLock();		/* INTERRUPTS LOCKED */

    clockRoutines[1] = routine;
    clockArgs[1] = arg;
    intUnlock (key);

    intConnect ((VOIDFUNCPTR *)TIMER_VEC, (VOIDFUNCPTR)sysClkInt, 0);

    return (0);
    }

/***************************************************************************
*
* sysAuxClkDisable - turn off auxiliary clock interrupts
*
* This routine disables auxiliary clock interrupts.
*
* RETURNS: N/A
*
* SEE ALSO: sysAuxClkEnable()
*/

void sysAuxClkDisable (void)
    {
    int key = intLock();		/* INTERRUPTS LOCKED */
    clockEnable[1] = 0;
    win_KillTimer (2);
    intUnlock (key);
    }

/***************************************************************************
*
* sysAuxClkEnable - turn on auxiliary clock interrupts
*
* This routine enables auxiliary clock interrupts.
*
* RETURNS: N/A
*
* SEE ALSO: sysAuxClkConnect(), sysAuxClkDisable(), sysAuxClkRateSet()
*/

void sysAuxClkEnable (void)
    {
    int key = intLock();		/* INTERRUPTS LOCKED */
    clockEnable[1] = 1;
    win_SetTimer (2, 1000 / clockRate[1]);
    intUnlock (key);
    }

/***************************************************************************
*
* sysAuxClkRateGet - get the auxiliary clock rate
*
* This routine returns the interrupt rate of the auxiliary clock.
*
* RETURNS: The number of ticks per second of the auxiliary clock.
*
* SEE ALSO: sysAuxClkEnable(), sysAuxClkRateSet()
*/

int sysAuxClkRateGet (void)
    {
    return (clockRate[1]);
    }

/***************************************************************************
*
* sysAuxClkRateSet - set the auxiliary clock rate
*
* This routine sets the interrupt rate of the auxiliary clock.  It does not
* enable auxiliary clock interrupts.
*
* RETURNS: OK, or ERROR if the tick rate is invalid or the timer cannot be set.
*
* SEE ALSO: sysAuxClkEnable(), sysAuxClkRateGet()
*/

STATUS sysAuxClkRateSet
    (
    int rate
    )
    {
    if ( (rate < AUX_CLK_RATE_MIN) || (rate > AUX_CLK_RATE_MAX) )
        return(ERROR);
    clockRate[1] = rate;
    if (clockEnable[1])
        {
        win_KillTimer (2);
        win_SetTimer (2, 1000 / clockRate[1]);
        }

    return (OK);
    }

#ifdef  INCLUDE_TIMESTAMP

/*******************************************************************************
*
* sysTimestampInt - timestamp timer interrupt handler
*
* This rountine handles the timestamp timer interrupt.  A user routine is
* called, if one was connected by sysTimestampConnect().
*
* RETURNS: N/A
*
* SEE ALSO: sysTimestampConnect()
*/

LOCAL void sysTimestampInt (void)
    {

    if (sysTimestampRoutine != NULL)    /* call user connected routine */
        (*sysTimestampRoutine) (sysTimestampArg);
    }

/*******************************************************************************
*
* sysTimestampConnect - connect a user routine to the timestamp timer interrupt
*
* This routine specifies the user interrupt routine to be called at each
* time the timestamp counter rolls over.
*
* RETURNS: OK always.
*/

STATUS sysTimestampConnect
    (
    FUNCPTR routine,    /* routine called at each timestamp timer rollover */
    int arg             /* argument with which to call routine 		   */
    )
    {
    sysTimestampRoutine = routine;
    sysTimestampArg = arg;

    return (OK);
    }

/*******************************************************************************
*
* sysTimestampEnable - initialize and enable the timestamp timer
*
* If the timer is not aleady enabled, this routine performs all the necessary
* initialization. Furthermore, it checks the validity of windows performance
* counter routines call. Else, an OK status is returned.
* updated.
*
* RETURNS: OK, or ERROR if windows performance counter routines check failed.
*/
STATUS sysTimestampEnable (void)
    {
    BOOL 	curValueIsValid 	= FALSE; /* performance counter      */
						 /* current value query      */
						 /* validity 		     */  
    BOOL 	freqValueIsValid 	= FALSE; /* performance counter      */
						 /* frequency query validity */
    UINT64	tmpVar; 			 /* tmp var for windows      */
						 /* routine call	     */ 
    int 	key;				 /* lock-out key	     */

    if (sysTimestampRunning)
	{
	return (OK);
	}
    else
	{
	/* initialize timastamp driver global variables */

	predTimestampValue = 0;
	rolloverDetected = FALSE;	
	nbRollOver = 0;

	/* 
         * Initialize the timestamp reference value and check the values 
	 * returned by wind_QueryPerformanceFrequency and
	 * wind_QueryPerformanceCounter. If the values returned are correct,
	 * then enabling the timastamp driver, else an ERROR status is returned
	 */

	key = intLock ();               /* INTERRUPTS LOCKED */
	freqValueIsValid = wind_QueryPerformanceFrequency(&tmpVar);
	curValueIsValid = wind_QueryPerformanceCounter (&timestampRef);
	intUnlock (key);		/* INTERRUPTS UNLOCKED */

	if (curValueIsValid && freqValueIsValid)
	    {
	    sysTimestampRunning = TRUE;
	    return (OK);
	    }
	else
	    return (ERROR);
	}
    }

/*******************************************************************************
*
* sysTimestampDisable - disable the timestamp timer
*
* This routine disables the timestamp timer.
*
* RETURNS: ERROR , the timestamp timer cannot be disabled.
*
* SEE ALSO: sysTimestampEnable()
*/

STATUS sysTimestampDisable (void)
    {
    sysTimestampRunning = FALSE;

    return (ERROR);
    }

/*******************************************************************************
*
* sysTimestampPeriod - get the period of a timestamp timer 
*
* This routine gets the period of the timestamp timer, in ticks.  The
* period, or terminal count, is the number of ticks to which the timestamp
* timer counts before rolling over.
*
* RETURNS: The period of the timestamp timer in counter ticks.
*/
UINT32 sysTimestampPeriod (void)
    {
    return (TIMESTAMP_MAX_VALUE);
    }

/*******************************************************************************
*
* sysTimestampFreq - get timestamp timer clock frequency
*
* This routine gets the frequency of the timer clock, in ticks per 
* second.  The rate of the timestamp timer is set explicitly by the 
* hardware and typically cannot be altered.
*
* RETURNS: The timestamp timer clock frequency, in ticks per second.
*/

UINT32 sysTimestampFreq (void)
    {
    UINT64 QueryPerfCountFreq64;	/* timestamp frequency */

    int key = intLock ();               /* INTERRUPTS LOCKED */
    wind_QueryPerformanceFrequency (&QueryPerfCountFreq64);
    intUnlock (key);			/* INTERRUPTS UNLOCKED */

    return ((UINT32) QueryPerfCountFreq64); 
    }

/*******************************************************************************
*
* sysTimestamp - get a timestamp timer tick count
*
* This routine returns the current value of the timestamp timer tick counter.
* The tick count can be converted to seconds by dividing it by the return of
* sysTimestampFreq().
*
* This routine should be called with interrupts locked.  If interrupts are
* not locked, sysTimestampLock() should be used instead.
*
* RETURNS: The current timestamp timer tick count.
*
* SEE ALSO: sysTimestampFreq(), sysTimestampLock()
*/

UINT32 sysTimestamp (void)
    {
    UINT64 perfCurVal64;	/* performance counter current value - 64 bit*/
    UINT32 perfCurVal32; 	/* performance counter current value - 32 bit*/

    /* get the current performance counter value */

    int key = intLock ();       /* INTERRUPTS LOCKED */
    wind_QueryPerformanceCounter (&perfCurVal64);
    intUnlock (key);		/* INTERRUPTS UNLOCKED */

    /*
     * check if the rollover happend. If the rollover is detected, a 	 
     * ROLL_OVER event will be posted by the next system clock interrupt
     */

    perfCurVal32 = (UINT32)perfCurVal64 - (UINT32)timestampRef ;


    if (perfCurVal32 < predTimestampValue)
	{
	rolloverDetected = TRUE;	
	}

    predTimestampValue = perfCurVal32;

    /*
     * after each rollover, a +1 increment must be added to the timestamp value.
     * This is due to sysTimestampPeriod function which return a 32 bit value.
     * The real period value is 0x10000000000000000 and not 0xffffffffffffffff.
     */

    return (perfCurVal32 + nbRollOver);
    }

/*******************************************************************************
*
* sysTimestampLock - lock interrupts and get the timestamp timer tick count
*
* This routine locks interrupts when the tick counter must be stopped 
* in order to read it or when two independent counters must be read.  
* It then returns the current value of the timestamp timer tick
* counter.
* 
* The tick count can be converted to seconds by dividing it by the return of
* sysTimestampFreq().
*
* If interrupts are already locked, sysTimestamp() should be
* used instead.
*
* RETURNS: The current timestamp timer tick count.
*
* SEE ALSO: sysTimestampFreq(), sysTimestamp()
*/

UINT32 sysTimestampLock (void)
    {
    return (sysTimestamp ());
    }

#endif  /* INCLUDE_TIMESTAMP */

