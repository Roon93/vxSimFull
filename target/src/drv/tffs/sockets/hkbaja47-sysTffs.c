/* sysTffs.c - Heurikon Baja4700 system-dependent TrueFFS library */

/* Copyright 1984-1997 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/* FAT-FTL Lite Software Development Kit
 * Copyright (C) M-Systems Ltd. 1995-1997	*/

/*
modification history
--------------------
01f,31may99,yp   Added comments suggested in SPR #25319
01e,21apr98,yp   added tffs to files included from there
01d,11mar98,yp   made including tffsConfig.c conditional so man page
                 generation does not include it.
01c,09mar98,kbw  made edits to the man pages to fix problems found by QE
01b,04mar98,kbw  made edits to the man pages
01a,23dec97,hdn  written.
*/

/*
DESCRIPTION
This library provides board-specific hardware access routines for TrueFFS.  
In effect, these routines comprise the socket component driver (or drivers)
for your flash device hardware.  At socket registration time, TrueFFS stores 
pointers to the functions of this socket component driver in an 'FLSocket' 
structure.  When TrueFFS needs to access the flash device, it uses these 
functions.  

Because this file is, for the most part, a device driver that exports its 
functionality by registering function pointers with TrueFFS, very few of the 
functions defined here are externally callable.  For the record, these 
external functions are flFitInSocketWindow() and flDelayLoop().  You should 
never have any need to call these functions.  

However, one of the most import functions defined in this file is neither
referenced in an 'FLSocket' structure, nor is it externally callable.  This
function is sysTffsInit().  TrueFFS calls this function at initialization 
time to register socket component drivers for all the flash devices attached 
to your target.  It is this call to sysTffs() that results in assigning 
drive numbers to the flash devices on your target hardware.  Drive numbers 
are assigned by the order in which the socket component drivers are registered.
The first to be registered is drive 0, the second is drive 1, and so on up to 
4.  As shipped, TrueFFS supports up to five flash drives.  

After registering socket component drivers for a flash device, you may 
format the flash medium even though there is not yet a block device driver
associated with the flash (see the reference entry for the tffsDevCreate() 
routine).  To format the flash medium for use with TrueFFS, 
call tffsDevFormat() or, for some BSPs, sysTffsFormat().  

The sysTffsFormat() routine is an optional but BSP-specific externally 
callable helper function.  Internally, it calls tffsDevFormat() with a 
pointer to a 'FormatParams' structure initialized to values that leave a 
space on the flash device for a boot image. This space is outside the 
region managed by TrueFFS.  This special region is necessary for boot 
images because the normal translation and wear-leveling services of TrueFFS 
are incompatible with the needs of the boot program and the boot image it 
relies upon.  To write a boot image (or any other data) into this area, 
use tffsBootImagePut().  

Finally, this file also contains define statements for symbolic constants 
that determine which MTDs, translation layer modules, and other utilities 
are ultimately included in TrueFFS.  These defines are as follows:

.IP "INCLUDE_TL_NFTL"
To include the NAND-based translation layer module.
.IP "INCLUDE_TL_FTL"
To include the NOR-based translation layer module.
.IP "INCLUDE_TL_SSFDC"
To include the SSFDC-appropriate translation layer module.
.IP "INCLUDE_MTD_I28F016"
For Intel 28f016 flash devices.
.IP "INCLUDE_MTD_I28F008"
For Intel 28f008 flash devices.
.IP "INCLUDE_MTD_I28F008_BAJA"
For Intel 28f008 flash devices on the Heurikon Baja 4700.
.IP "INCLUDE_MTD_AMD"
For AMD, Fujitsu: 29F0{40,80,16} 8-bit flash devices.
.IP "INCLUDE_MTD_CDSN"
For Toshiba, Samsung: NAND CDSN flash devices.
.IP "INCLUDE_MTD_DOC2"
For Toshiba, Samsung: NAND DOC flash devices.
.IP "INCLUDE_MTD_CFISCS"
For CFI/SCS flash devices.
.IP "INCLUDE_MTD_WAMD"
For AMD, Fujitsu 29F0{40,80,16} 16-bit flash devices.
.IP "INCLUDE_TFFS_BOOT_IMAGE"
To include tffsBootImagePut() in TrueFFS for Tornado.
.LP
To exclude any of the modules mentioned above, edit sysTffs.c and undefine
its associated symbolic constant.

INCLUDE FILES: flsocket.h, tffsDrv.h
*/


#include "vxWorks.h"
#include "config.h"
#include "tffs/flsocket.h"
#include "tffs/tffsDrv.h"


/* defines */

#undef	INCLUDE_MTD_I28F016		/* Intel: 28f016 */
#undef	INCLUDE_MTD_I28F008		/* Intel: 28f008 */
#define	INCLUDE_MTD_I28F008_BAJA	/* Intel: 28f008 for Baja4700 */
#undef	INCLUDE_MTD_AMD			/* AMD, Fujitsu: 29f0{40,80,16} 8bit */
#undef	INCLUDE_MTD_CDSN		/* Toshiba, Samsung: NAND, CDSN */
#undef	INCLUDE_MTD_DOC2		/* Toshiba, Samsung: NAND, DOC */
#undef	INCLUDE_MTD_CFISCS		/* CFI/SCS */
#undef	INCLUDE_MTD_WAMD		/* AMD, Fujitsu: 29f0{40,80,16} 16bit */
#undef	INCLUDE_TL_NFTL			/* NFTL translation layer */
#define	INCLUDE_TL_FTL			/* FTL translation layer */
#undef	INCLUDE_TL_SSFDC		/* SSFDC translation layer */
#define INCLUDE_TFFS_BOOT_IMAGE		/* include tffsBootImagePut() */
#define	FLASH_BASE_ADRS		0x1d400000 /* Flash ROM 1 */
#if	FALSE
#define	FLASH_BASE_ADRS		0x1d800000 /* Flash ROM 2 */
#define	FLASH_BASE_ADRS		0x1dc00000 /* Flash ROM 3 */
#endif	/* FALSE */
#define	FLASH_SIZE		0x100000
#define SETUP_WRITE		0x40


/* externs */

extern void sysWbFlush ();


/* locals */


/* forward declarations */

LOCAL FLBoolean		rfaCardDetected (FLSocket vol);
LOCAL void		rfaVccOn (FLSocket vol);
LOCAL void		rfaVccOff (FLSocket vol);
#ifdef	SOCKET_12_VOLTS
LOCAL FLStatus		rfaVppOn (FLSocket vol);
LOCAL void		rfaVppOff (FLSocket vol);
#endif	/* SOCKET_12_VOLTS */
LOCAL FLBoolean		rfaGetAndClearCardChangeIndicator (FLSocket vol);
LOCAL FLBoolean		rfaWriteProtected (FLSocket vol);
LOCAL void		rfaSetWindow (FLSocket vol);
LOCAL void		rfaSetMappingContext (FLSocket vol, unsigned page);
LOCAL FLStatus		rfaSocketInit (FLSocket vol);
LOCAL FLStatus		rfaRegister (void);

#ifndef DOC
#include "tffs/tffsConfig.c"
#endif /* DOC */

/*******************************************************************************
*
* sysTffsInit - initialize TFFS flash socket interface routine.
*
* This is the TFFS flash socket interface initialization routine.
*
* RETURNS: N/A
*/

LOCAL void sysTffsInit (void)
    {

    rfaRegister ();
    }

/*******************************************************************************
*
* rfaRegister - install routines for the RFA
*
* This routine installs necessary functions for the RFA.
*
* RETURNS: flOK, or flTooManyComponents if there're too many drives
*/

LOCAL FLStatus rfaRegister (void)
    {
    FLSocket vol = flSocketOf (noOfDrives);

    if (noOfDrives >= DRIVES)
        return (flTooManyComponents);

    tffsSocket[noOfDrives] = "RFA";
    noOfDrives++;

    vol.serialNo = 0;
    vol.window.baseAddress = FLASH_BASE_ADRS >> 12;

    /* fill in function pointers */

    vol.cardDetected      = rfaCardDetected;
    vol.VccOn             = rfaVccOn;
    vol.VccOff            = rfaVccOff;
#ifdef SOCKET_12_VOLTS
    vol.VppOn             = rfaVppOn;
    vol.VppOff            = rfaVppOff;
#endif
    vol.initSocket        = rfaSocketInit;
    vol.setWindow         = rfaSetWindow;
    vol.setMappingContext = rfaSetMappingContext;
    vol.getAndClearCardChangeIndicator =
                          rfaGetAndClearCardChangeIndicator;
    vol.writeProtected    = rfaWriteProtected;

    return (flOK);
    }

/*******************************************************************************
*
* rfaCardDetected - detect if a card is present (inserted)
*
* This routine detects if a card is present (inserted).
*
* RETURNS: TRUE, or FALSE if the card is not present.
*/

LOCAL FLBoolean rfaCardDetected
    (
    FLSocket vol
    )
    {
    return (TRUE);
    }

/*******************************************************************************
*
* rfaVccOn - turn on Vcc (3.3/5 Volts)
*
* This routine turns on Vcc (3.3/5 Volts).  Vcc must be known to be good
* on exit.
*
* RETURNS: N/A
*/

LOCAL void rfaVccOn 
    (
    FLSocket vol
    )
    {
    }

/*******************************************************************************
*
* rfaVccOff - turn off Vcc (3.3/5 Volts)
*
* This routine turns off Vcc (3.3/5 Volts). 
*
* RETURNS: N/A
*/

LOCAL void rfaVccOff 
    (
    FLSocket vol
    )
    {
    }

#ifdef SOCKET_12_VOLTS

/*******************************************************************************
*
* rfaVppOn - turns on Vpp (12 Volts)
*
* This routine turns on Vpp (12 Volts). Vpp must be known to be good on exit.
*
* RETURNS: flOK always
*/

LOCAL FLStatus rfaVppOn
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    return (flOK);
    }

/*******************************************************************************
*
* rfaVppOff - turns off Vpp (12 Volts)
*
* This routine turns off Vpp (12 Volts).
*
* RETURNS: N/A
*/

LOCAL void rfaVppOff 
    (
    FLSocket vol		/* pointer identifying drive */
    ) 
    {
    }

#endif	/* SOCKET_12_VOLTS */


/*******************************************************************************
*
* rfaSocketInit - perform all necessary initializations of the socket
*
* This routine performs all necessary initializations of the socket.
*
* RETURNS: flOK always
*/

LOCAL FLStatus rfaSocketInit
    (
    FLSocket vol		/* pointer identifying drive */
    ) 
    {
    vol.cardChanged = FALSE;

    /* enable memory window and map it at address 0 */
    rfaSetWindow (&vol);

    return (flOK);
    }

/*******************************************************************************
*
* rfaSetWindow - set current window attributes, Base address, size, etc
*
* This routine sets current window hardware attributes: Base address, size,
* speed and bus width.  The requested settings are given in the 'vol.window' 
* structure.  If it is not possible to set the window size requested in
* 'vol.window.size', the window size should be set to a larger value, 
* if possible. In any case, 'vol.window.size' should contain the 
* actual window size (in 4 KB units) on exit.
*
* RETURNS: N/A
*/

LOCAL void rfaSetWindow
    (
    FLSocket vol		/* pointer identifying drive */
    ) 
    {
    /* Physical base as a 4K page */
    vol.window.baseAddress = FLASH_BASE_ADRS >> 12;

    flSetWindowSize (&vol, FLASH_SIZE >> 12);
    }

/*******************************************************************************
*
* rfaSetMappingContext - sets the window mapping register to a card address
*
* This routine sets the window mapping register to a card address.
* The window should be set to the value of 'vol.window.currentPage',
* which is the card address divided by 4 KB. An address over 128MB,
* (page over 32K) specifies an attribute-space address. On entry to this 
* routine vol.window.currentPage is the page already mapped into the window.
* (In otherwords the page that was mapped by the last call to this routine.)
*
* The page to map is guaranteed to be on a full window-size boundary.
*
* RETURNS: N/A
*/

LOCAL void rfaSetMappingContext 
    (
    FLSocket vol,		/* pointer identifying drive */
    unsigned page		/* page to be mapped */
    )
    {
    }

/*******************************************************************************
*
* rfaGetAndClearCardChangeIndicator - return the hardware card-change indicator
*
* This routine returns the hardware card-change indicator and clears it if set.
*
* RETURNS: FALSE, or TRUE if the card has been changed
*/

LOCAL FLBoolean rfaGetAndClearCardChangeIndicator
    (
    FLSocket vol		/* pointer identifying drive */
    ) 
    {
    return (FALSE);
    }

/*******************************************************************************
*
* rfaWriteProtected - return the write-protect state of the media
*
* This routine returns the write-protect state of the media
*
* RETURNS: FALSE, or TRUE if the card is write-protected
*/

LOCAL FLBoolean rfaWriteProtected 
    (
    FLSocket vol		/* pointer identifying drive */
    ) 
    {
    return (FALSE);
    }

/*******************************************************************************
*
* flFitInSocketWindow - check whether the flash array fits in the socket window
f
* This routine checks whether the flash array fits in the socket window.
*
* RETURNS: A chip size guaranteed to fit in the socket window.
*/

long int flFitInSocketWindow
    (
    long int chipSize,		/* size of single physical chip in bytes */
    int      interleaving,	/* flash chip interleaving (1,2,4 etc) */
    long int windowSize		/* socket window size in bytes */
    )
    {
    return (chipSize);
    }

#if	FALSE
/*******************************************************************************
*
* sysTffsCpy - copy memory from one location to another
*
* This routine copies <size> characters from the object pointed
* to by <source> into the object pointed to by <destination>. If copying
* takes place between objects that overlap, the behavior is undefined.
*
* INCLUDE FILES: string.h
*
* RETURNS: A pointer to <destination>.
*
* NOMANUAL
*/

void * sysTffsCpy
    (
    void *       destination,   /* destination of copy */
    const void * source,        /* source of copy */
    size_t       size           /* size of memory to copy */
    )
    {
    bcopy ((char *) source, (char *) destination, (size_t) size);
    return (destination);
    }

/*******************************************************************************
*
* sysTffsSet - set a block of memory
*
* This routine stores <c> converted to an `unsigned char' in each of the
* elements of the array of `unsigned char' beginning at <m>, with size <size>.
*
* INCLUDE FILES: string.h
*
* RETURNS: A pointer to <m>.
*
* NOMANUAL
*/

void * sysTffsSet
    (
    void * m,                   /* block of memory */
    int    c,                   /* character to store */
    size_t size                 /* size of memory */
    )
    {
    bfill ((char *) m, (int) size, c);
    return (m);
    }
#endif	/* FALSE */

/*******************************************************************************
*
* flDelayLoop - consume the specified time
*
* This routine consumes the specified time.
*
* RETURNS: N/A
*/

void flDelayLoop 
    (
    int cycles			/* loop count to be consumed */
    )
    {
    }

/*******************************************************************************
*
* sysTffsByteWrite - write byte to the flash memory
*
* This routine write byte to the flash memory
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sysTffsByteWrite 
    (
    char *pAddr,		/* address */
    char value			/* value */
    )
    {
    volatile char * pAligned = (volatile char *)((int)pAddr & ~3);
    unsigned long offset = ((int)pAddr & 3);

    /* setup pig control register for byte offset */

    *PIG_CONTROL = (*PIG_CONTROL & ~0x700) | (offset << 8);

    /* do the write */

    *pAligned = SETUP_WRITE;
    *pAligned = value;

#if	(CPU == R4000)
    sysWbFlush();
#endif	/* (CPU == R4000) */
    }
