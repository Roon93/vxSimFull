/*
Copyright 1999 Wind River Systems, Inc.


modification history
--------------------
01f,26feb02,yp   adding INCLUDE_MTD_CFIAMD component
01e,05dec01,nrv  add INCLUDE_TFFS_SHOW component
01d,13nov01,nrv  re-written to configure MTDs and TL through project
01c,22apr99,pr   added dependency on DOSFS
01b,20apr99,pr   removed reference to ftllite.h msyscard.h ssfdc.h (SPR 26910)
01a,03feb99,yp   written


DESCRIPTION
  This file contains descriptions for the TrueFFS Flash file system
components. This file describes the "generic" component which is available
to all CPU architectures and BSPs.  MTD and "Socket" configuration must
be done in sysTffs.c.

*/

Component INCLUDE_TFFS {
	NAME		TrueFFS Flash File System
	SYNOPSIS	Allows a DOS file system to placed in Flash memory
	INIT_RTN	tffsDrv ();
	CONFIGLETTES	usrTffs.c
        BSP_STUBS       sysTffs.c   
        MODULES         backgrnd.o \
                        dosformt.o \
                        fatlite.o \
                        flbase.o \
                        flflash.o \
                        flparse.o \
                        flsocket.o \
                        fltl.o \
                        reedsol.o \
                        tffsDrv.o \
                        tffsLib.o
	HDR_FILES	tffs/backgrnd.h tffs/dosformt.h tffs/fatlite.h \
			tffs/flbase.h tffs/flbuffer.h tffs/flcustom.h  \
			tffs/flflash.h tffs/flsocket.h tffs/flsysfun.h \
			tffs/flsystem.h tffs/fltl.h tffs/pcic.h \
			tffs/reedsol.h tffs/stdcomp.h tffs/tffsDrv.h
	REQUIRES	INCLUDE_DOSFS
}

Component INCLUDE_TFFS_SHOW {
        NAME                 TrueFFS Show Routines
        SYNOPSIS             TrueFFS show routines: tffsShow and tffsShowAll
}

Component INCLUDE_MTD_AMD {
        NAME            amdmtd
        SYNOPSIS        AMD, Fujitsu: 29F0{40,80,16} 8-bit devices
        MODULES         amdmtd.o
        HDR_FILES       tffs/flflash.h tffs/backgrnd.h
        REQUIRES        INCLUDE_TFFS \
                        INCLUDE_TL_FTL
}

Component INCLUDE_MTD_CFIAMD {
        NAME            cfiamdmtd
        SYNOPSIS        CFI driver for AMD Flash Part
        MODULES         cfiamd.o
        HDR_FILES       tffs/flflash.h tffs/backgrnd.h
        REQUIRES        INCLUDE_TFFS \
                        INCLUDE_TL_FTL
}

Component INCLUDE_MTD_I28F008 {
        NAME            i28f008
        SYNOPSIS        Intel 28f008 devices
        MODULES         i28f008.o
        HDR_FILES       tffs/flflash.h tffs/backgrnd.h
        REQUIRES        INCLUDE_TFFS \
                        INCLUDE_TL_FTL
}

Component INCLUDE_MTD_I28F008BAJA {
        NAME            i28f008Baja
        SYNOPSIS        Intel 28f008 on the Heurikon Baja 4000
        MODULES         i28f008Baja.o
        HDR_FILES       tffs/flflash.h tffs/backgrnd.h
        REQUIRES        INCLUDE_TFFS \
                        INCLUDE_TL_FTL
}

Component INCLUDE_MTD_I28F016 {
        NAME            i28f016
        SYNOPSIS        Intel 28f016 devices
        MODULES         i28f016.o
        HDR_FILES       tffs/flflash.h tffs/backgrnd.h
        REQUIRES        INCLUDE_TFFS \
                        INCLUDE_TL_FTL
}

Component INCLUDE_MTD_WAMDMTD {
        NAME            wamdmtd
        SYNOPSIS        AMD, Fujitsu 29F0{40,80,16} 16-bit devices
        MODULES         wamdmtd.o
        HDR_FILES       tffs/flflash.h tffs/backgrnd.h
        REQUIRES        INCLUDE_TFFS \
                        INCLUDE_TL_FTL
}

Component INCLUDE_MTD_CFISCS {
        NAME            cfiscs
        SYNOPSIS        CFI/SCS devices
        MODULES         cfiscs.o
        HDR_FILES       tffs/flflash.h tffs/backgrnd.h
        REQUIRES        INCLUDE_TFFS \
                        INCLUDE_TL_FTL
}

Component INCLUDE_TL_FTL {
        NAME            ftllite
        SYNOPSIS        NOR Logic Translation Layer
        MODULES         ftllite.o
        HDR_FILES       tffs/fltl.h
        REQUIRES        INCLUDE_TFFS
}


Component INCLUDE_TL_SSFDC {
        NAME            ssfdc
        SYNOPSIS        Translation Layer Solid State Floppy Disk Card
        MODULES         ssfdc.o
        HDR_FILES       tffs/fltl.h
        REQUIRES        INCLUDE_TFFS
}

