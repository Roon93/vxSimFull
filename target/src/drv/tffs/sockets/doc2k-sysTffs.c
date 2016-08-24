/* sysTffs.c - PC 386/486 system-dependent TrueFFS library */

/* Copyright 1984-1997 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/* FAT-FTL Lite Software Development Kit
 * Copyright (C) M-Systems Ltd. 1995-1997	*/

/*
modification history
--------------------
01d,31may99,yp   Added comments suggested in SPR #25319
01c,05feb99,yp   fixed SPR #22515. Bad preprocessor define
01b,03jun98,yp   made scan addresses BSP specific
01a,02jun98,yp   derived from pc386-sysTffs.c
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

INCLUDE FILES: flsocket.h

SEE ALSO : tffsDevFormat tffsRawio
*/


/* includes */

#include "vxWorks.h"
#include "config.h"
#include "tffs/flsocket.h"
#include "tffs/pcic.h"


/* defines */

#define	INCLUDE_MTD_DOC2		/* Toshiba, Samsung: NAND, DOC */
#define	INCLUDE_TL_NFTL			/* NFTL translation layer */
#define	INCLUDE_SOCKET_DOC		/* DOC socket interface */
#define INCLUDE_TFFS_BOOT_IMAGE		/* include tffsBootImagePut() */
#define	WINDOW_ID	0		/* PCIC window used (0-4) */
#define	VPP_DELAY_MSEC	100		/* Millisecs to wait for Vpp ramp up */
#ifdef  MBX860_BD_SIZE			/* only way to know if bsp is MBX860 */
#define DOC2_SCAN_ADRS_0 (CPU_PCI_ISA_MEM_BA + 0xc8000)         /* start */
#define DOC2_SCAN_ADRS_1 (CPU_PCI_ISA_MEM_BA + 0xf0000)         /* end */
#else	/* ! MBX860 */
#define DOC2_SCAN_ADRS_0 (0xc8000)                              /* start */
#define DOC2_SCAN_ADRS_1 (0xf0000)                              /* end */
#endif  /* MBX 860 */
#define KILL_TIME_FUNC	 ((iz * iz) / (iz + iz)) / ((iy + iz) / (iy * iz))
#define PC_WINDOW	1		/* PCIC window no. used by TFFS */
#define PC_EXTRAWS	1		/* PCIC wait state used by TFFS */
#define PC_SOCKET_NAME_DOC "DOC"	/* DOC socket name for DOC */


/* externs */

IMPORT unsigned windowBaseAddress (unsigned driveNo, unsigned long startAddr,
                                   unsigned long endAddr);   /* nfdc2148.c */
/* globals */

char pcDriveNo[2] = {NONE, NONE};       /* drive number of the sockets */

/* locals */

LOCAL UINT32 sysTffsMsecLoopCount = 0;	/* loop count to consume milli sec */


/* forward declarations */

#ifdef	INCLUDE_SOCKET_DOC
LOCAL FLStatus		docRegister (void);
LOCAL unsigned		docWindowBaseAddress (unsigned driveNo);
LOCAL FLBoolean		docCardDetected (FLSocket vol);
LOCAL void		docVccOn (FLSocket vol);
LOCAL void		docVccOff (FLSocket vol);
#ifdef	SOCKET_12_VOLTS
LOCAL FLStatus		docVppOn (FLSocket vol);
LOCAL void		docVppOff (FLSocket vol);
#endif	/* SOCKET_12_VOLTS */
LOCAL FLStatus		docInitSocket (FLSocket vol);
LOCAL void		docSetWindow (FLSocket vol);
LOCAL void		docSetMappingContext (FLSocket vol, unsigned page);
LOCAL FLBoolean		docGetAndClearCardChangeIndicator (FLSocket vol);
LOCAL FLBoolean		docWriteProtected (FLSocket vol);
#ifdef	EXIT
LOCAL void		docFreeSocket (FLSocket vol);
#endif	/* EXIT */
#endif	/* INCLUDE_SOCKET_DOC */


#ifndef DOC
#include "tffs/tffsConfig.c"
#endif /* DOC */


/*******************************************************************************
*
* sysTffsInit - board level initialization for TFFS
*
* This routine calls the socket registration routines for the socket component
* drivers that will be used with this BSP. The order of registration signifies
* the logical drive number given to the drive associated with the socket.
*
* RETURNS: N/A
*/

LOCAL void sysTffsInit (void)
    {
    UINT32 ix = 0;
    UINT32 iy = 1;
    UINT32 iz = 2;
    int oldTick;

    /* we assume followings:
     *   - no interrupts except timer is happening.
     *   - the loop count that consumes 1 msec is in 32 bit.
     * it should be done in the early stage of usrRoot() in tffsDrv().  */

    oldTick = tickGet();
    while (oldTick == tickGet())	/* wait for next clock interrupt */
	;

    oldTick = tickGet();
    while (oldTick == tickGet())	/* loop one clock tick */
	{
	iy = KILL_TIME_FUNC;		/* consume time */
	ix++;				/* increment the counter */
	}
    
    sysTffsMsecLoopCount = ix * sysClkRateGet() / 1000;

    (void) docRegister ();			/* Disk On Chip */

    }

#ifdef	INCLUDE_SOCKET_DOC
/*******************************************************************************
*
* docRegister - registration routine for M-Systems Disk On Chip (DOC) 
*		socket component driver
*
* This routine populates the 'vol' structure for a logical drive with the
* socket component driver routines for the M-System DOC. All socket routines
* are referanced through the 'vol' structure and never from here directly
*
* RETURNS: flOK, or flTooManyComponents if there're too many drives,
*                or flAdapterNotFound if there's no controller.
*/

LOCAL FLStatus docRegister (void)
    {
    FLSocket vol;

    if (noOfDrives >= DRIVES)
        return (flTooManyComponents);

    pVol = flSocketOf (noOfDrives);

    vol.window.baseAddress =	docWindowBaseAddress (vol.volNo);
    if (vol.window.baseAddress == 0)
        return (flAdapterNotFound);

    vol.cardDetected =		docCardDetected;
    vol.VccOn =			docVccOn;
    vol.VccOff =		docVccOff;
#ifdef SOCKET_12_VOLTS
    vol.VppOn =			docVppOn;
    vol.VppOff =		docVppOff;
#endif
    vol.initSocket =		docInitSocket;
    vol.setWindow =		docSetWindow;
    vol.setMappingContext =	docSetMappingContext;
    vol.getAndClearCardChangeIndicator = docGetAndClearCardChangeIndicator;
    vol.writeProtected =	docWriteProtected;
#ifdef EXIT
    vol.freeSocket =		docFreeSocket;
#endif

    tffsSocket[noOfDrives] = PC_SOCKET_NAME_DOC;
    noOfDrives++;

    return (flOK);
    }
 
/*******************************************************************************
*
* docWindowBaseAddress - Return the host base address of the DOC2 window
*
* This routine Return the host base address of the window.
* It scans the host address range from DOC2_SCAN_ADRS_0 to DOC2_SCAN_ADRS_1
* (inclusive) attempting to identify DiskOnChip 2000 memory window.
*
* RETURNS: Host physical address of window divided by 4 KB
*/

LOCAL unsigned docWindowBaseAddress
    (
    unsigned driveNo		/* drive number */
    )
    {

    return (windowBaseAddress (driveNo, DOC2_SCAN_ADRS_0, DOC2_SCAN_ADRS_1));
    }

/*******************************************************************************
*
* docCardDetected - detect if a card is present (inserted)
*
* This routine detects if a card is present (inserted).
*
* RETURNS: TRUE, or FALSE if the card is not present.
*/

LOCAL FLBoolean docCardDetected
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    return (TRUE);
    }

/*******************************************************************************
*
* docVccOn - turn on Vcc (3.3/5 Volts)
*
* This routine turns on Vcc (3.3/5 Volts).  Vcc must be known to be good
* on exit.
*
* RETURNS: N/A
*/

LOCAL void docVccOn
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    }

/*******************************************************************************
*
* docVccOff - turn off Vcc (3.3/5 Volts)
*
* This routine turns off Vcc (3.3/5 Volts). 
*
* RETURNS: N/A
*/

LOCAL void docVccOff
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    }

#ifdef SOCKET_12_VOLTS

/*******************************************************************************
*
* docVppOn - turns on Vpp (12 Volts)
*
* This routine turns on Vpp (12 Volts). Vpp must be known to be good on exit.
*
* RETURNS: flOK always
*/

LOCAL FLStatus docVppOn
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    return (flOK);
    }

/*******************************************************************************
*
* docVppOff - turns off Vpp (12 Volts)
*
* This routine turns off Vpp (12 Volts).
*
* RETURNS: N/A
*/

LOCAL void docVppOff
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    }

#endif	/* SOCKET_12_VOLTS */

/*******************************************************************************
*
* docInitSocket - perform all necessary initializations of the socket
*
* This routine performs all necessary initializations of the socket.
*
* RETURNS: flOK always
*/

LOCAL FLStatus docInitSocket
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    return (flOK);
    }

/*******************************************************************************
*
* docSetWindow - set current window attributes, Base address, size, etc
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

LOCAL void docSetWindow
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    }

/*******************************************************************************
*
* docSetMappingContext - sets the window mapping register to a card address
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

LOCAL void docSetMappingContext
    (
    FLSocket vol,		/* pointer identifying drive */
    unsigned page		/* page to be mapped */
    )
    {
    }

/*******************************************************************************
*
* docGetAndClearCardChangeIndicator - return the hardware card-change indicator
*
* This routine returns the hardware card-change indicator and clears it if set.
*
* RETURNS: FALSE, or TRUE if the card has been changed
*/

LOCAL FLBoolean docGetAndClearCardChangeIndicator
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    return (FALSE);
    }

/*******************************************************************************
*
* docWriteProtected - return the write-protect state of the media
*
* This routine returns the write-protect state of the media
*
* RETURNS: FALSE, or TRUE if the card is write-protected
*/

LOCAL FLBoolean docWriteProtected
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    return (FALSE);
    }

#ifdef EXIT

/*******************************************************************************
*
* docFreeSocket - free resources that were allocated for this socket.
*
* This routine free resources that were allocated for this socket.
* This function is called when FLite exits.
*
* RETURNS: N/A
*/

LOCAL void docFreeSocket
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    }

#endif  /* EXIT */
#endif	/* INCLUDE_SOCKET_DOC */
/*******************************************************************************
*
* flFitInSocketWindow - check whether the flash array fits in the socket window
*
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
    /* x86 architectures use sliding windows for flash arrays */
    /* so this check is irrelevant for them                   */

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
* flDelayMsecs - wait for specified number of milliseconds
*
* This routine waits for specified number of milliseconds.
*
* RETURNS: N/A
* 
* NOMANUAL
*/

void flDelayMsecs
    (
    unsigned milliseconds       /* milliseconds to wait */
    )
    {
    UINT32 ix;
    UINT32 iy = 1;
    UINT32 iz = 2;

    /* it doesn't count time consumed in interrupt level */

    for (ix = 0; ix < milliseconds; ix++)
        for (ix = 0; ix < sysTffsMsecLoopCount; ix++)
	    {
	    tickGet ();			/* dummy */
	    iy = KILL_TIME_FUNC;	/* consume time */
	    }
    }

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
    int  cycles
    )
    {
    }
