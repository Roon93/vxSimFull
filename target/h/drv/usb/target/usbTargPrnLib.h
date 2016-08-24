/* usbTargPrnLib.h - Defines exported interface from usbTargPrnLib */

/* Copyright 2000 Wind River Systems, Inc. */
 
/*
Modification history
--------------------
01a,18aug99,rcb  First.
*/

#ifndef __INCusbTargPrnLibh
#define __INCusbTargPrnLibh

#ifdef	__cplusplus
extern "C" {
#endif


/* function prototypes */

VOID usbTargPrnCallbackInfo
    (
    pUSB_TARG_CALLBACK_TABLE *ppCallbacks,
    pVOID *pCallbackParam
    );


STATUS usbTargPrnDataInfo
    (
    pUINT8 *ppBfr,
    pUINT16 pActLen
    );


STATUS usbTargPrnDataRestart (void);


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbTargPrnLibh */


/* End of file. */

