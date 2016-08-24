/* sysTffs.c - Motorola 860ads system-dependent TrueFFS library */

/* Copyright 1984-1997 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01l,29jan02,dtr  Changing vxImmrGet to vxImmrIsbGet.
01k,15nov01,dtr  Put in #if defined to retain file dependencies for make.
01j,31may99,yp   Added comments suggested in SPR #25319
01i,21apr98,yp   added tffs subdir to search path
01h,11mar98,yp   made including tffsConfig.c conditional so man page
                 generation does not include it.
01g,09mar98,kbw  fixed a flub I missed in the sysTffsFormat function
01f,09mar98,kbw  made man page edits to fix problems found by QE
01e,04mar98,kbw  made man page edits
01d,18jan98,hdn  added configuration macros for SIMM and PCMCIA.
01c,18dec97,hdn  added comment.  cleaned up.
01b,05dec97,hdn  added flDelayMsecs(), tffsSocket[]. cleaned up.
01a,05nov97,hdn  written.
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

The function sysTffsFormat() is provided so one might seasily format the 
Flash SIMM to have the TFFS file system in one half of it.

INCLUDE FILES: flsocket.h, tffsDrv.h
*/


/* includes */

#include "vxWorks.h"
#include "taskLib.h"
#include "config.h"
#include "tffs/flsocket.h"
#include "tffs/tffsDrv.h"
#include "ads860.h"
#include "arch/ppc/vxPpcLib.h"
#include "drv/multi/ppc860Siu.h"

#if defined(INCLUDE_TFFS)

/* defines */

#define	INCLUDE_MTD_I28F016		/* Intel: 28f016 */
#define	INCLUDE_MTD_I28F008		/* Intel: 28f008 */
#define	INCLUDE_MTD_AMD			/* AMD, Fujitsu: 29f0{40,80,16} 8bit */
#undef	INCLUDE_MTD_CFISCS		/* CFI/SCS */
#undef	INCLUDE_MTD_WAMD		/* AMD, Fujitsu: 29f0{40,80,16} 16bit */
#define	INCLUDE_TL_FTL			/* FTL translation layer */
#undef	INCLUDE_TL_SSFDC		/* SSFDC translation layer */
#define	INCLUDE_SOCKET_SIMM		/* SIMM socket interface */
#define	INCLUDE_SOCKET_PCMCIA		/* PCMCIA socket interface */
#define INCLUDE_TFFS_BOOT_IMAGE		/* include tffsBootImagePut() */
#define	FLASH_BASE_ADRS		0x02800000	/* Flash memory base address */
#define	FLASH_SIZE		0x00200000	/* Flash memory size */
#define VCC_DELAY_MSEC		100	/* Millisecs to wait for Vcc ramp up */
#define VPP_DELAY_MSEC		100	/* Millisecs to wait for Vpp ramp up */
#define BCSR1_PCCVPP_12V	0x00200000
#define KILL_TIME_FUNC	 ((iz * iz) / (iz + iz)) / ((iy + iz) / (iy * iz))


/* locals */

LOCAL UINT32 sysTffsMsecLoopCount = 0;


/* forward declarations */

#ifdef	INCLUDE_SOCKET_SIMM
LOCAL FLBoolean		simmCardDetected (FLSocket vol);
LOCAL void		simmVccOn (FLSocket vol);
LOCAL void		simmVccOff (FLSocket vol);
#ifdef	SOCKET_12_VOLTS
LOCAL FLStatus		simmVppOn (FLSocket vol);
LOCAL void		simmVppOff (FLSocket vol);
#endif	/* SOCKET_12_VOLTS */
LOCAL FLStatus		simmInitSocket (FLSocket vol);
LOCAL void		simmSetWindow (FLSocket vol);
LOCAL void		simmSetMappingContext (FLSocket vol, unsigned page);
LOCAL FLBoolean		simmGetAndClearCardChangeIndicator (FLSocket vol);
LOCAL FLBoolean		simmWriteProtected (FLSocket vol);
LOCAL void		simmRegister (void);
#endif	/* INCLUDE_SOCKET_SIMM */

#ifdef	INCLUDE_SOCKET_PCMCIA
LOCAL FLBoolean		pcCardDetected (FLSocket vol);
LOCAL void		pcVccOn (FLSocket vol);
LOCAL void		pcVccOff (FLSocket vol);
#ifdef	SOCKET_12_VOLTS
LOCAL FLStatus		pcVppOn (FLSocket vol);
LOCAL void		pcVppOff (FLSocket vol);
#endif	/* SOCKET_12_VOLTS */
LOCAL FLStatus		pcInitSocket (FLSocket vol);
LOCAL void		pcSetWindow (FLSocket vol);
LOCAL void		pcSetMappingContext (FLSocket vol, unsigned page);
LOCAL FLBoolean		pcGetAndClearCardChangeIndicator (FLSocket vol);
LOCAL FLBoolean		pcWriteProtected (FLSocket vol);
LOCAL void		pcRegister (void);
#endif	/* INCLUDE_SOCKET_PCMCIA */


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
     * it is done in the early stage of usrRoot() in tffsDrv().  */

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

#ifdef	INCLUDE_SOCKET_SIMM
    simmRegister ();			/* SIMM socket interface register */
#endif	/* INCLUDE_SOCKET_SIMM */

#ifdef	INCLUDE_SOCKET_PCMCIA
    pcRegister ();			/* PCMCIA socket interface register */
#endif	/* INCLUDE_SOCKET_PCMCIA */
    }

#ifdef	INCLUDE_SOCKET_SIMM
/*******************************************************************************
*
* simmRegister - install routines for the Flash SIMM
*
* This routine installs necessary functions for the Flash SIMM.
*
* RETURNS: N/A
*/

LOCAL void simmRegister (void)
    {
    FLSocket vol = flSocketOf (noOfDrives);

    tffsSocket[noOfDrives] =	"SIMM";
    vol.window.baseAddress =	FLASH_BASE_ADRS >> 12;
    vol.cardDetected =		simmCardDetected;
    vol.VccOn =			simmVccOn;
    vol.VccOff =		simmVccOff;
#ifdef SOCKET_12_VOLTS
    vol.VppOn =			simmVppOn;
    vol.VppOff =		simmVppOff;
#endif
    vol.initSocket =		simmInitSocket;
    vol.setWindow =		simmSetWindow;
    vol.setMappingContext =	simmSetMappingContext;
    vol.getAndClearCardChangeIndicator = simmGetAndClearCardChangeIndicator;
    vol.writeProtected =	simmWriteProtected;
    noOfDrives++;
    }

/*******************************************************************************
*
* simmCardDetected - detect if a card is present (inserted)
*
* This routine detects if a card is present (inserted).
*
* RETURNS: TRUE, or FALSE if the card is not present.
*/

LOCAL FLBoolean simmCardDetected
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    return (TRUE);
    }

/*******************************************************************************
*
* simmVccOn - turn on Vcc (3.3/5 Volts)
*
* This routine turns on Vcc (3.3/5 Volts).  Vcc must be known to be good
* on exit.
*
* RETURNS: N/A
*/

LOCAL void simmVccOn
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    }

/*******************************************************************************
*
* simmVccOff - turn off Vcc (3.3/5 Volts)
*
* This routine turns off Vcc (3.3/5 Volts). 
*
* RETURNS: N/A
*/

LOCAL void simmVccOff
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    }

#ifdef SOCKET_12_VOLTS

/*******************************************************************************
*
* simmVppOn - turns on Vpp (12 Volts)
*
* This routine turns on Vpp (12 Volts). Vpp must be known to be good on exit.
*
* RETURNS: flOK always.
*/

LOCAL FLStatus simmVppOn
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    return (flOK);
    }

/*******************************************************************************
*
* simmVppOff - turns off Vpp (12 Volts)
*
* This routine turns off Vpp (12 Volts).
*
* RETURNS: N/A
*/

LOCAL void simmVppOff
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    }

#endif	/* SOCKET_12_VOLTS */

/*******************************************************************************
*
* simmInitSocket - perform all necessary initializations of the socket
*
* This routine performs all necessary initializations of the socket.
*
* RETURNS: flOK always.
*/

LOCAL FLStatus simmInitSocket
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    *BCSR1 &= ~(BCSR1_FLASH_EN_L);		/* enable the flash */
    return (flOK);
    }

/*******************************************************************************
*
* simmSetWindow - set current window attributes, Base address, size, etc
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

LOCAL void simmSetWindow
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
* simmSetMappingContext - sets the window mapping register to a card address
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

LOCAL void simmSetMappingContext
    (
    FLSocket vol,		/* pointer identifying drive */
    unsigned page		/* page to be mapped */
    )
    {
    }

/*******************************************************************************
*
* simmGetAndClearCardChangeIndicator - return the hardware card-change indicator
*
* This routine returns the hardware card-change indicator and clears it if set.
*
* RETURNS: FALSE, or TRUE if the card has been changed
*/

LOCAL FLBoolean simmGetAndClearCardChangeIndicator
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    return (FALSE);
    }

/*******************************************************************************
*
* simmWriteProtected - return the write-protect state of the media
*
* This routine returns the write-protect state of the media
*
* RETURNS: FALSE, or TRUE if the card is write-protected
*/

LOCAL FLBoolean simmWriteProtected
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    return (FALSE);
    }
#endif	/* INCLUDE_SOCKET_SIMM */

#ifdef	INCLUDE_SOCKET_PCMCIA
/*******************************************************************************
*
* pcRegister - install routines for the PCMCIA Flash Card
*
* This routine installs necessary functions for the PCMCIA Flash Card.
*
* RETURNS: N/A
*/

LOCAL void pcRegister (void)
    {
    FLSocket vol = flSocketOf (noOfDrives);

    tffsSocket[noOfDrives] =	"PCMCIA";
    vol.window.baseAddress =	(PC_BASE_ADRS_0) >> 12;
    vol.cardDetected =		pcCardDetected;
    vol.VccOn =			pcVccOn;
    vol.VccOff =		pcVccOff;
#ifdef SOCKET_12_VOLTS
    vol.VppOn =			pcVppOn;
    vol.VppOff =		pcVppOff;
#endif
    vol.initSocket =		pcInitSocket;
    vol.setWindow =		pcSetWindow;
    vol.setMappingContext =	pcSetMappingContext;
    vol.getAndClearCardChangeIndicator = pcGetAndClearCardChangeIndicator;
    vol.writeProtected =	pcWriteProtected;
    noOfDrives++;
    }

/*******************************************************************************
*
* pcCardDetected - detect if a card is present (inserted)
*
* This routine detects if a card is present (inserted).
*
* RETURNS: TRUE, or FALSE if the card is not present.
*/

LOCAL FLBoolean pcCardDetected
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    int	immrVal = vxImmrIsbGet();
    
    if ((*PIPR (immrVal) & 0x18000000) == 0)
	return (TRUE);
    else
	return (FALSE);
    }

/*******************************************************************************
*
* pcVccOn - turn on Vcc (3.3/5 Volts)
*
* This routine turns on Vcc (3.3/5 Volts). Vcc must be known to be good 
* on exit.
*
* RETURNS: N/A
*/

LOCAL void pcVccOn
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    *BCSR1 &= ~(BCSR1_PCCARD_VCCON_L);	/* on PC card VCC */
    flDelayMsecs (VCC_DELAY_MSEC);	/* wait for Vcc to ramp up */
    }

/*******************************************************************************
*
* pcVccOn - turn off Vcc (3.3/5 Volts)
*
* This routine turns off Vcc (3.3/5 Volts).
*
* RETURNS: N/A
*/

LOCAL void pcVccOff
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    *BCSR1 |= (BCSR1_PCCARD_VCCON_L);	/* off PC card VCC */
    }

#ifdef SOCKET_12_VOLTS

/*******************************************************************************
*
* pcVppOn - turn on Vpp (12 Volts)
*
* This routine turns on Vpp (12 Volts). Vpp must be known to be good on exit.
*
* RETURNS: flOK always
*/

LOCAL FLStatus pcVppOn
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    *BCSR1 = (*BCSR1 & ~BCSR1_PCCVPP_MSK) | BCSR1_PCCVPP_12V;
    flDelayMsecs (VPP_DELAY_MSEC);	/* wait for Vpp to ramp up */
    return (flOK);
    }

/*******************************************************************************
*
* pcVppOff - turn off Vpp (12 Volts)
*
* This routine turns off Vpp (12 Volts).
*
* RETURNS: N/A
*/

LOCAL void pcVppOff
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    *BCSR1 = (*BCSR1 & ~BCSR1_PCCVPP_MSK) | BCSR1_PCCVPP_MSK;
    }

#endif	/* SOCKET_12_VOLTS */

/*******************************************************************************
*
* pcInitSocket - perform all necessary initializations of the socket
*
* This routine performs all necessary initializations of the socket.
*
* RETURNS: flOK always
*/

LOCAL FLStatus pcInitSocket
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    int	immrVal = vxImmrIsbGet();

#if	FALSE
    printf ("-- pcInitSocket --\n");
    printf ("immrVal=0x%x\n", immrVal);
    printf ("BR0=0x%x, OR0=0x%x\n", *BR0(immrVal), *OR0(immrVal));
    printf ("BR1=0x%x, OR1=0x%x\n", *BR1(immrVal), *OR1(immrVal));
    printf ("BR2=0x%x, OR2=0x%x\n", *BR2(immrVal), *OR2(immrVal));
    printf ("BR3=0x%x, OR3=0x%x\n", *BR3(immrVal), *OR3(immrVal));
    printf ("BR4=0x%x, OR4=0x%x\n", *BR4(immrVal), *OR4(immrVal));
    printf ("BR5=0x%x, OR5=0x%x\n", *BR5(immrVal), *OR5(immrVal));
    printf ("BR6=0x%x, OR6=0x%x\n", *BR6(immrVal), *OR6(immrVal));
    printf ("BR7=0x%x, OR7=0x%x\n", *BR7(immrVal), *OR7(immrVal));
#endif	/* FALSE */

    *BR4(immrVal) = (PC_BASE_ADRS_1 & BR_BA_MSK) | BR_V;
    *OR4(immrVal) = 0xfc000000 | OR_BI | OR_SCY_8_CLK | OR_TRLX | 0x02;

    /* clear PCMCIA base registers */

    *PBR0 (immrVal) = PC_BASE_ADRS_0;
    *PBR1 (immrVal) = PC_BASE_ADRS_1;
    *PBR2 (immrVal) = 0x0;
    *PBR3 (immrVal) = 0x0;
    *PBR4 (immrVal) = 0x0;
    *PBR5 (immrVal) = 0x0;
    *PBR6 (immrVal) = 0x0;
    *PBR7 (immrVal) = 0x0;

    /* clear PCMCIA option registers */

    *POR0 (immrVal) = 0xf0000000 |		/* bank size:     1MB */
		      0x00020000 |		/* strobe hold:   2 clocks */
		      0x00001000 |		/* strobe setup:  1 clock */
		      0x00000300 |		/* strobe length: 6 clocks */
		      0x00000040 |		/* port size:     2 byte */
		      0x00000010 |		/* region select: attribute */
		      0x00000000 |		/* slot ID:       slot-A */
		      0x00000000 |		/* write protect: off */
		      0x00000001;		/* PCMCIA valid:  1 */
    *POR1 (immrVal) = 0xa8000000 |		/* bank size:     32MB */
		      0x00020000 |		/* strobe hold:   2 clocks */
		      0x00001000 |		/* strobe setup:  1 clock */
		      0x00000300 |		/* strobe length: 6 clocks */
		      0x00000040 |		/* port size:     2 byte */
		      0x00000000 |		/* region select: common mem */
		      0x00000000 |		/* slot ID:       slot-A */
		      0x00000000 |		/* write protect: off */
		      0x00000001;		/* PCMCIA valid:  1 */
    *POR2 (immrVal) = 0x0;
    *POR3 (immrVal) = 0x0;
    *POR4 (immrVal) = 0x0;
    *POR5 (immrVal) = 0x0;
    *POR6 (immrVal) = 0x0;
    *POR7 (immrVal) = 0x0;

    *PSCR (immrVal) = 0x0;

    *PGCRA (immrVal) = 0x0;			/* XXX disable interupt */
    *PGCRB (immrVal) = 0x0;			/* XXX disable interupt */

    *BCSR1 &= ~(BCSR1_PCCARD_EN_L);		/* enable the PCMCIA */
    *BCSR1 |= BCSR1_PCCVPP_MSK;			/* setup PC card Vpp to Hi-Z */

    *BCSR1 &= ~(BCSR1_PCCARD_VCCON_L);		/* on PC card VCC */
    flDelayMsecs (VCC_DELAY_MSEC);		/* wait for Vcc to ramp up */
    *PGCRA (immrVal) |= 0x00000040;		/* reset the card A */
    flDelayMsecs (10);				/* wait for the reset */
    *PGCRA (immrVal) &= ~0x00000040;

#if	FALSE
    printf ("PIPR=0x%x\n", *PIPR(immrVal));
    printf ("PSCR=0x%x\n", *PSCR(immrVal));
    printf ("PER=0x%x\n", *PER(immrVal));
    printf ("PGCRA=0x%x\n", *PGCRA(immrVal));
    printf ("PGCRB=0x%x\n", *PGCRB(immrVal));
#endif	/* FALSE */

    return (flOK);
    }

/*******************************************************************************
*
* pcSetWindow - set current window attributes, Base address, size, etc
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

LOCAL void pcSetWindow
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    /* Physical base as a 4K page */
    vol.window.baseAddress = PC_BASE_ADRS_1 >> 12;

    flSetWindowSize (&vol, PC_BASE_ADRS_1 >> 12);
    }

/*******************************************************************************
*
* pcSetMappingContext - sets the window mapping register to a card address
*
* This routine sets the window mapping register to a card address.
* The window should be set to the value of 'vol.window.currentPage',
* which is the card address divided by 4 KB. An address over 128MB,
* (page over 32K) specifies an attribute-space address.
*
* The page to map is guaranteed to be on a full window-size boundary.
*
* RETURNS: N/A
*/

LOCAL void pcSetMappingContext
    (
    FLSocket vol,		/* pointer identifying drive */
    unsigned page		/* page to be mapped */
    )
    {
    int	immrVal = vxImmrIsbGet();

    if (page & ATTRIBUTE_SPACE_MAPPED)
        *POR1 (immrVal) |= 0x00000010;		/* attribute mem */
    else
        *POR1 (immrVal) &= ~0x00000010;		/* common mem */
    }

/*******************************************************************************
*
* pcGetAndClearCardChangeIndicator - return the hardware card-change indicator
*
* This routine returns the hardware card-change indicator and clears it if set.
*
* RETURNS: FALSE, or TRUE if the card has been changed
*/

LOCAL FLBoolean pcGetAndClearCardChangeIndicator
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    return (FALSE);
    }

/*******************************************************************************
*
* pcWriteProtected - return the write-protect state of the media
*
* This routine returns the write-protect state of the media
*
* RETURNS: FALSE, or TRUE if the card is write-protected
*/

LOCAL FLBoolean pcWriteProtected
    (
    FLSocket vol		/* pointer identifying drive */
    )
    {
    return (FALSE);
    }
#endif	/* INCLUDE_SOCKET_PCMCIA */

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
    if (chipSize*interleaving > windowSize) /* doesn't fit in socket window */
        {
        int  roundedSizeBits;

        /* fit chip in the socket window */
        chipSize = windowSize / interleaving;

        /* round chip size at powers of 2 */
        for (roundedSizeBits = 0; (0x1L << roundedSizeBits) <= chipSize;
             roundedSizeBits++)
	    ;

        chipSize = (0x1L << (roundedSizeBits - 1));
        }

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
    while (--cycles)
	;
    }

/*******************************************************************************
*
* sysTffsFormat - format the flash memory above an offset
*
* This routine formats the flash memory.  Because this function defines 
* the symbolic constant, HALF_FORMAT, the lower half of the specified flash 
* memory is left unformatted.  If the lower half of the flash memory was
* previously formated by TrueFFS, and you are trying to format the upper half,
* you need to erase the lower half of the flash memory before you format the
* upper half.  To do this, you could use:
* .CS
* tffsRawio(0, 3, 0, 8)  
* .CE
* The first argument in the tffsRawio() command shown above is the TrueFFS 
* drive number, 0.  The second argument, 3, is the function number (also 
* known as TFFS_PHYSICAL_ERASE).  The third argument, 0, specifies the unit 
* number of the first erase unit you want to erase.  The fourth argument, 8,
* specifies how many erase units you want to erase.  
*
* RETURNS: OK, or ERROR if it fails.
*/

STATUS sysTffsFormat (void)
    {
    STATUS status;
    tffsDevFormatParams params = 
	{
#define	HALF_FORMAT	/* lower 0.5MB for bootimage, upper 1.5MB for TFFS */
#ifdef	HALF_FORMAT
	{0x80000l, 99, 1, 0x10000l, NULL, {0,0,0,0}, NULL, 2, 0, NULL},
#else
	{0x000000l, 99, 1, 0x10000l, NULL, {0,0,0,0}, NULL, 2, 0, NULL},
#endif	/* HALF_FORMAT */
	FTL_FORMAT_IF_NEEDED
	};

    /* we assume that the drive number 0 is SIMM */

    status = tffsDevFormat (0, (int)&params);
    return (status);
    }

#endif /*INCLUDE_TFFS */
