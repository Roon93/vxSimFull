/* usbTargPhilipsD12EvalLib.h - Defines usbTargPhilipsD12EvalLib interface */

/* Copyright 2000 Wind River Systems, Inc. */
 
/*
Modification history
--------------------
01a,03sep99,rcb  First.
*/

#ifndef __INCusbTargPhilipsD12EvalLibh
#define __INCusbTargPhilipsD12EvalLibh

#ifdef	__cplusplus
extern "C" {
#endif


/* function prototypes */

VOID usbTargPhilipsD12EvalCallbackInfo
    (
    pUSB_TARG_CALLBACK_TABLE *ppCallbacks,
    pVOID *pCallbackParam
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbTargPhilipsD12EvalLibh */


/* End of file. */

