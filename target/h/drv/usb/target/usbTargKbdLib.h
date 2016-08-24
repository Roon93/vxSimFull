/* usbTargKbdLib.h - Defines exported interface from usbTargKbdLib */

/* Copyright 2000 Wind River Systems, Inc. */
 
/*
Modification history
--------------------
01a,18aug99,rcb  First.
*/

#ifndef __INCusbTargKbdLibh
#define __INCusbTargKbdLibh

#ifdef	__cplusplus
extern "C" {
#endif


/* function prototypes */

VOID usbTargKbdCallbackInfo
    (
    pUSB_TARG_CALLBACK_TABLE *ppCallbacks,
    pVOID *pCallbackParam
    );


STATUS usbTargKbdInjectReport
    (
    pHID_KBD_BOOT_REPORT pReport,
    UINT16 reportLen
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbTargKbdLibh */


/* End of file. */

