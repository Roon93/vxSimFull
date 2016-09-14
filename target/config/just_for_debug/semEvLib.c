/* semEvLib.c - VxWorks events support for semaphores */

/* Copyright 2001-2003 Wind River Systems, Inc. */

#include "copyright_wrs.h"

/*
modification history
--------------------
01c,21nov02,bwa  Corrected display of html code in semEvStart documentation
		 (SPR 78019).
01b,18oct01,bwa  Modified comment for options in semEvStart.() Added check for
		 current task in semEvStop().
01a,20sep01,bwa  written
*/

/*
DESCRIPTION

This library is an extension to eventLib, the events library. Its purpose  
is to support events for semaphores.

The functions in this library are used to control registration of tasks on a
semaphore. The routine semEvStart() registers a task and starts the
notification process. The function semEvStop() un-registers the task, which 
stops the notification mechanism.

When a task is registered and the semaphore becomes available, the events
specified are sent to that task. However, if a semTake() is to be done
afterwards, there is no guarantee that the semaphore will still be available. 

INCLUDE FILES: semEvLib.h

SEE ALSO: eventLib, semLib,
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "taskLib.h"
#include "errnoLib.h"
#include "intLib.h"
#include "semLib.h"
#include "taskArchLib.h"
#include "eventLib.h"
#include "private/windLibP.h"
#include "private/semLibP.h"

/* prototypes (local functions) */

LOCAL BOOL semEvBMIsFree (const SEM_ID semId);
LOCAL BOOL semEvCIsFree  (const SEM_ID semId);

/* externs */

extern CLASS_ID semClassId;

/* globals */

FUNCPTR semEvIsFreeTbl [MAX_SEM_TYPE]  = {(FUNCPTR)semEvBMIsFree,
					  (FUNCPTR)semEvBMIsFree,
					  (FUNCPTR)semEvCIsFree,
					  (FUNCPTR)semEvBMIsFree,
					  (FUNCPTR)semInvalid,
					  (FUNCPTR)semInvalid,
					  (FUNCPTR)semInvalid,
					  (FUNCPTR)semInvalid};

/*******************************************************************************
*
* semEvLibInit - dummy lib init function, used to pull lib in kernel
*
* NOMANUAL
*/

void semEvLibInit (void)
    {
    }

/*******************************************************************************
*
* semEvStart - start event notification process for a semaphore
*
* This routine turns on the event notification process for a given semaphore, 
* registering the calling task on that semaphore. When the semaphore becomes
* available but no task is pending on it, the events specified will be sent
* to the registered task. A task can always overwrite its own registration.
*
* The <option> parameter is used for 3 user options:
* .iP
* Specify if the events are to be sent only once or every time the semaphore 
* becomes free until semEvStop() is called.
* .iP
* Specify if another task can subsequently register itself while the calling
* task is still registered. If so specified, the existing task registration
* will be overwritten without any warning.
* .iP
* Specify if events are to be sent at the time of the registration in the
* case the semaphore is free.
* .LP
*
* Here are the respective values to be used to form the options field:
* .iP "EVENTS_SEND_ONCE (0x1)"
* The semaphore will send the events only once.
* .iP "EVENTS_ALLOW_OVERWRITE (0x2)"
* Allows subsequent registrations from other tasks to overwrite the current one.
* .iP "EVENTS_SEND_IF_FREE (0x4)"
* The registration process will send events if the semaphore is free at the
* time semEvStart() is called.
* .iP "EVENTS_OPTIONS_NONE" (0x0)
* Must be passed to the <options> parameter if none of the other three
* options are used.
*
* WARNING: This routine cannot be called from interrupt level.
*
* WARNING: Task preemption can allow a semDelete to be performed between
* the calls to semEvStart and eventReceive. This will prevent
* the task from ever receiving the events wanted from the semaphore.
*
* RETURNS: OK on success, or ERROR.
*
* ERRNO
* .iP "S_objLib_OBJ_ID_ERROR"
* The semaphore ID is invalid.
* .iP "S_eventLib_ALREADY_REGISTERED"
* A task is already registered on the semaphore.
* .iP "S_intLib_NOT_ISR_CALLABLE"
* Routine has been called from interrupt level.
* .iP "S_eventLib_EVENTSEND_FAILED"
* User chose to send events right away and that operation failed.
* .iP "S_eventLib_ZERO_EVENTS"
* User passed in a value of zero to the <events> parameter.
*
* SEE ALSO: eventLib, semLib, semEvStop()
*/

STATUS semEvStart
    (
    SEM_ID semId,	/* semaphore on which to register events */
    UINT32 events, 	/* 32 possible events to register        */
    UINT8 options	/* event-related semaphore options	 */
    )
    {
    FUNCPTR fcn; /* fcn that finds if the sem is free, passed to eventStart */

    if (events == 0x0)
	{
	errnoSet (S_eventLib_ZERO_EVENTS);
	return (ERROR);
	}

    if (INT_RESTRICT () != OK)
	return (ERROR);    /* errno set by INT_RESTRICT() */

    TASK_LOCK (); /* to prevent sem from being deleted */

    if (OBJ_VERIFY(semId,semClassId) != OK)
	{
	TASK_UNLOCK ();
	return (ERROR);    /* errno is set by OBJ_VERIFY */
	}

    fcn = semEvIsFreeTbl [semId->semType & SEM_TYPE_MASK];

    /* TASK_UNLOCK() will be done by eventStart() */

    return (eventStart ((OBJ_ID)semId, &semId->events, fcn, events, options) );
    }


/*******************************************************************************
*
* semEvCIsFree - verify if a counting semaphore is free
*
* RETURNS: TRUE if semaphore is free, FALSE if not
*
* NOTE: the semId has to be validated before calling this function since it
*       doesn't validate it itself before dereferencing the pointer.
*
* NOMANUAL
*/

LOCAL BOOL semEvCIsFree
    (
    const SEM_ID semId
    )
    {
    if (SEMC_IS_FREE (semId))
	return (TRUE);

    return (FALSE);
    }

/*******************************************************************************
*
* semEvBMIsFree - verify if a mutex or binary semaphore is free
*
* RETURNS: TRUE if semaphore is free, FALSE if not
*
* NOTE: the semId has to be validated before calling this function since it
*       doesn't validate it itself before dereferencing the pointer.
*
* NOMANUAL
*/

LOCAL BOOL semEvBMIsFree
    (
    const SEM_ID semId
    )
    {
    if (SEMBM_IS_FREE (semId))
	return (TRUE);

    return (FALSE);
    }

/*******************************************************************************
*
* semEvStop - stop event notification process for a semaphore
*
* This routine turns off the event notification process for a given semaphore.
* It thus allows another task to register itself for event notification on
* that particular semaphore. It has to be called from the task that is
* already registered on that particular semaphore.
* 
* RETURNS: OK on success, or ERROR.
*
* ERRNO
* .iP "S_objLib_OBJ_ID_ERROR"
* The semaphore ID is invalid.
* .iP "S_intLib_NOT_ISR_CALLABLE"
* Routine has been called at interrupt level.
* .iP "S_eventLib_TASK_NOT_REGISTERED"
* Routine has not been called by the registered task.
*
* SEE ALSO: eventLib, semLib, semEvStart()
*/

STATUS semEvStop
    (
    SEM_ID semId
    )
    {
    int level;

    if (INT_RESTRICT () != OK)
	return (ERROR);
    /*
     * intLock is used instead of TASK_LOCK because it is faster (since
     * TASK_UNLOCK calls intLock anyway) and the time the ints are locked is
     * really short
     */

    level = intLock ();

    if (OBJ_VERIFY (semId,semClassId) != OK)
	{
	intUnlock (level);
	return (ERROR);    /* errno is set by OBJ_VERIFY */
	}

    if (semId->events.taskId != (int)taskIdCurrent)
	{
	intUnlock (level);
	errnoSet (S_eventLib_TASK_NOT_REGISTERED);
	return (ERROR);
	}

    semId->events.taskId = (int)NULL;

    intUnlock (level);

    return (OK);
    }

