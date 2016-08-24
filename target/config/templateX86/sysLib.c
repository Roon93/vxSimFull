/* sysLib.c - templateX86 system-dependent library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
TODO -	Remove the template modification history and begin a new history
	starting with version 01a and growing the history upward with
	each revision.

modification history
--------------------
01l,08dec01,dat  Update for T2.2, chg to Pentium
01k,15oct01,dat  70855, adding sysNanoDelay
01j,13oct99,dat  SPR 22676 fixed declaration of end
01i,08apr99,dat  SPR 26491, PCI macro names
01h,02feb99,tm   added PCI AutoConfig support to template BSPs (SPR 24733)
01g,28jan99,dat  added MMU mappings for VME and PCI.
01f,14jan99,mas  added use of USER_D_CACHE_MODE in sysPhysMemDesc[] (SPR 24319)
01e,21jul98,db   added sysLanIntEnable(), sysLanIntDisable() and
		 sysLanEnetAddrGet().
01d,27aug97,dat  code review comments, added comments, removed
		 sysDelay().
01c,10jul97,dat  made more generic, but still uses i8259Pic.c
01b,14mar97,dat  moved PC_CONSOLE support to sysSerial.c
01a,27jan97,dat  written (from pc386/sysLib.c, ver 02u)
*/

/*
TODO - Update this documentation.

DESCRIPTION
This library provides board-specific routines.  The chip drivers included are:

    i8259Pic.c - Intel 8259 Programmable Interrupt Controller (PIC) library
    templateTimer.c - Intel 8253 timer driver
    nullNvRam.c - null NVRAM library
    templateVme.c - template VMEbus library

It #includes the following BSP stub files:
    sysSerial.c - serial device initialization routines
    sysNet.c - network subsystem routines
    sysScsi.c -	SCSI subsystem routines

X86 SPECIFICS
Because the great majority of X86 implementations use the 8259 PIC interrupt
controller chips, we have included the driver for that in this generic
template.  No other specific drivers are included, only template or null
drivers.

INCLUDE FILES: sysLib.h

SEE ALSO:
.pG "Configuration"
*/

/* includes */

#include "vxWorks.h"
#include "config.h"
#include "vme.h"
#include "memLib.h"
#include "sysLib.h"
#include "string.h"
#include "intLib.h"
#include "logLib.h"
#include "taskLib.h"
#include "vxLib.h"
#include "errnoLib.h"
#include "dosFsLib.h"
#include "stdio.h"
#include "cacheLib.h"
#include "private/vmLibP.h"
#include "arch/i86/mmuI86Lib.h"

/* imports */

IMPORT char end[];		  /* end of system, created by ld */
IMPORT GDT sysGdt[];		  /* the global descriptor table */
IMPORT VOIDFUNCPTR intEOI;	  /* pointer to a function sysIntEOI() */

/* globals */

/*
 * Decode USER_D_CACHE_MODE to determine main memory cache mode.
 * The macro MEM_STATE will be used to specify the cache state for
 * the sysPhysMemDesc table entry that describes the main memory
 * region.
 */

#if (USER_D_CACHE_MODE & CACHE_COPYBACK)
#   define MEM_STATE VM_STATE_CACHEABLE
#elif (USER_D_CACHE_MODE & CACHE_WRITETHOUGH)
#   define MEM_STATE VM_STATE_CACHEABLE_WRITETHROUGH
#else
#   define MEM_STATE VM_STATE_CACHEABLE_NOT
#endif

PHYS_MEM_DESC sysPhysMemDesc [] =
    {
    /* adrs and length parameters must be page-aligned (multiples of 0x1000) */

    /* lower memory */
    {
    (void *) LOCAL_MEM_LOCAL_ADRS,
    (void *) LOCAL_MEM_LOCAL_ADRS,
    0xa0000,	/* 640 KB */
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | MEM_STATE
    },

    /* video ram, etc */
    {
    (void *) 0xa0000,
    (void *) 0xa0000,
    0x60000,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    /* upper memory */
    {
    (void *) 0x100000,
    (void *) 0x100000,
    LOCAL_MEM_SIZE - 0x100000,	/* it is changed in sysMemTop() */
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | MEM_STATE
    },

    /* TODO -The default ROM is CACHEABLE, change if necessary. */

    {
    (void *) ROM_BASE_ADRS,
    (void *) ROM_BASE_ADRS,
    ROM_SIZE,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE_NOT  | VM_STATE_CACHEABLE
    },

    /* TODO - local I/O devices & NVRAM */

    /* TODO- Add additional entries for VME space, special I/O devices, etc */

#ifdef INCLUDE_FLASH
#endif

#ifdef INCLUDE_VME
    /* TODO - one-time setup of control registers for master window mapping */

    /* VME A16 space */

    {
    (void *) VME_A16_MSTR_LOCAL,
    (void *) VME_A16_MSTR_LOCAL,
    VME_A16_MSTR_SIZE,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    /* VME A24 space */

    {
    (void *) VME_A24_MSTR_LOCAL,
    (void *) VME_A24_MSTR_LOCAL,
    VME_A24_MSTR_SIZE,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    /* VME A32 space */

    {
    (void *) VME_A32_MSTR_LOCAL,
    (void *) VME_A32_MSTR_LOCAL,
    VME_A32_MSTR_SIZE,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },
#endif

#ifdef INCLUDE_PCI

    /* PCI I/O space */

    {
    (void *) PCI_MSTR_IO_LOCAL,
    (void *) PCI_MSTR_IO_LOCAL,
    PCI_MSTR_IO_SIZE,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    /* PCI MEM space */

    {
    (void *) PCI_MSTR_MEM_LOCAL,
    (void *) PCI_MSTR_MEM_LOCAL,
    PCI_MSTR_MEM_SIZE,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    /* PCI MEMIO (non-prefetch) space */

    {
    (void *) PCI_MSTR_MEMIO_LOCAL,
    (void *) PCI_MSTR_MEMIO_LOCAL,
    PCI_MSTR_MEMIO_SIZE,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    /*
     * TODO: Add additional mappings as needed to support special PCI
     * functions like memory mapped configuration space, or special IACK
     * register space.
     */

#endif
    };

int sysPhysMemDescNumEnt = NELEMENTS (sysPhysMemDesc);


int	sysCpu		= CPU;		/* system cpu type (MC680x0) */
char	*sysBootLine	= BOOT_LINE_ADRS; /* address of boot line */
char	*sysExcMsg	= EXC_MSG_ADRS;	/* catastrophic message area */
int	sysProcNum;			/* processor number of this cpu */
int	sysFlags;			/* boot flags */
char	sysBootHost [BOOT_FIELD_LEN];	/* name of host from which we booted */
char	sysBootFile [BOOT_FIELD_LEN];	/* name of file from which we booted */
UINT	sysIntIdtType	= 0x0000ef00;	/* trap gate, 0x0000ee00=int gate */
UINT	sysProcessor	= NONE;		/* 0=386, 1=486, 2=Pentium */
UINT	sysCoprocessor	= 0;		/* 0=non, 1=387, 2=487 */
UINT    sysVectorIRQ0	= INT_NUM_IRQ0;	/* vector number for IRQ0 */
GDT	*pSysGdt	= (GDT *)(LOCAL_MEM_LOCAL_ADRS + GDT_BASE_OFFSET);
CPUID	sysCpuId	= {0,{0},0,0,0,0,0,0,0,0,{0},{0}}; /* CPUID struct. */


/* locals */

LOCAL short *sysRomBase[] =
    {
    (short *)0xce000, (short *)0xce800, (short *)0xcf000, (short *)0xcf800
    };
#define ROM_SIGNATURE_SIZE	16

LOCAL char sysRomSignature[ROM_SIGNATURE_SIZE] =
    {
    0x55,0xaa,0x01,0x90,0x90,0x90,0x90,0x90,
    0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90
    };


/* Device drivers and driver sub systems */

#include "intrCtl/i8259Pic.c"
#include "mem/nullNvRam.c"
#include "vme/templateVme.c"
#include "timer/templateTimer.c"

#ifdef INCLUDE_PCI
#   include "pci/pciConfigLib.c"
#   include "pci/pciIntLib.c"
#   ifdef INCLUDE_PCI_AUTOCONF
#       include "pci/pciAutoConfigLib.c"
#       include "./sysBusPci.c"
#   endif
#endif

#include "sysSerial.c"
#include "sysNet.c"		/* network driver support */
#include "sysScsi.c"		/* scsi driver support */

/*******************************************************************************
*
* sysModel - return the model name of the CPU board
*
* This routine returns the model name of the CPU board.
*
* RETURNS: A pointer to the string "Template X86".
*/

char *sysModel (void)

    {
    return ("Template X86");
    }

/*******************************************************************************
*
* sysBspRev - return the BSP version and revision number
*
* This routine returns a pointer to a BSP version and revision number, for
* example, 1.1/0. BSP_REV is concatenated to BSP_VERSION and returned.
*
* RETURNS: A pointer to the BSP version/revision string.
*/

char * sysBspRev (void)
    {
    return (BSP_VERSION BSP_REV);
    }

/*******************************************************************************
*
* sysHwInit - initialize the system hardware
*
* This routine initializes various features of the board.
* It is the first board-specific C code executed, and runs with
* interrupts masked in the processor.
* This routine resets all devices to a quiescent state, typically
* by calling driver initialization routines.
*
* NOTE: Because this routine will be called from sysToMonitor, it must
* shutdown all potential DMA master devices.  If a device is doing DMA
* at reboot time, the DMA could interfere with the reboot. For devices
* on non-local busses, this is easy if the bus reset or sysFail line can
* be asserted.
*
* NOTE: This routine should not be called directly by the user application.
*
* RETURNS: N/A
*/

void sysHwInit (void)
    {
    /*
     * TODO - add initialization code for all devices and
     * for the interrupt controller.
     * Note: the interrupt controller should mask all interrupts here.
     * Interrupts are unmasked later on a per-device basis:
     *     device                     where unmasked
     *     ------                     --------------
     * abort switch, clock            sysHwInit2()
     * serial                         sysSerialHwInit2()
     * SCSI                           sysScsiInit()
     * LAN                            sysLanIntEnable()
     */

    /* initialize the PIC (Programmable Interrupt Controller) */

    sysIntInitPIC ();
    intEOI		= sysIntEOI;	/* system EOI ack procedure */

#if defined(INCLUDE_MMU)
    /* run-time update of the MMU entry for main RAM */

    sysPhysMemDesc[2].len = (UINT)(sysPhysMemTop () -
				sysPhysMemDesc[2].physicalAddr);
#endif

#ifdef INCLUDE_NETWORK
    sysNetHwInit ();		/* network interface */
#endif

#ifdef INCLUDE_SERIAL
    sysSerialHwInit ();		/* serial devices */
#endif

#ifdef INCLUDE_VME
    /* TODO - any VME hardware setup */
#endif

#ifdef INCLUDE_PCI
    /* TODO - any PCI hardware setup */
#endif

#ifdef INCLUDE_FLASH
    /* TODO - any Flash ROM hardware setup */
#endif
    }

/*******************************************************************************
*
* sysHwInit2 - initialize additional system hardware
*
* This routine connects system interrupt vectors and configures any
* required features not configured by sysHwInit. It is called from usrRoot()
* in usrConfig.c after the multitasking kernel has started.
*
* RETURNS: N/A
*/

void sysHwInit2 (void)
    {
    static int	initialized;	/* must protect against double call! */

    if (!initialized)
	{
	initialized = TRUE;

	/*
         * TODO - connect timer and abort vector interrupts and enable
	 * those interrupts as needed.
         */

#ifdef INCLUDE_NETWORK
	sysNetHwInit2 ();	/* network interface */
#endif

#ifdef INCLUDE_SERIAL
	sysSerialHwInit2 ();	/* connect serial interrupts */
#endif

#ifdef INCLUDE_VME
	/* TODO - any secondary VME setup */
#endif

#ifdef INCLUDE_PCI
        /* TODO - any secondary PCI setup */
#ifdef INCLUDE_PCI_AUTOCONF
        sysPciAutoConfig();
#if defined(INCLUDE_NETWORK) && defined(INCLUDE_END)
        /* TODO - update load string routine */
#endif /* defined(INCLUDE_NETWORK) && defined(INCLUDE_END) */
#endif /* INCLUDE_PCI_AUTOCONF */
#endif /* INCLUDE_PCI */

	}
    }

/*******************************************************************************
*
* sysPhysMemTop - get the address of the physical top of memory
*
* This routine returns the address of the first missing byte of memory,
* which indicates the top of memory.
*
* Memory probing begins at the end of BSS; at every 4K boundary a byte
* is read until it finds one that cannot be read, or
* 4MB have been probed, whichever is first.
*
* RETURNS: The address of the top of memory.
*
* INTERNAL
* This routine is used by sysHwInit() to differentiate between models.
* It is highly questionable whether vxMemProbe() can be used during the
* initial stage of booting.
*/

char* sysPhysMemTop (void)

    {
    static char *memTop = NULL;			/* top of memory */
    char gdtr[6];

    if (memTop == NULL)
	{
#ifdef LOCAL_MEM_AUTOSIZE
#	define TEST_PATTERN	0x12345678
#	define SYS_PAGE_SIZE	0x10000
#	define N_TIMES		3

	int delta		= SYS_PAGE_SIZE;
	BOOL found		= FALSE;
	int temp[N_TIMES];
	char *p;
	int ix;

	if (memTop != NULL)
	    return (memTop);

	/*
	 * if (end) is in upper memory, we assume it is VxWorks image.
	 * if not, it is Boot image
	 */

	if ((int)end > 0x100000)
	    p = (char *)(((int)end + (delta - 1)) & (~ (delta - 1)));
	else
	    p = (char *)0x100000;

	/* find out the actual size of the memory (max 1GB) */

	for (; (int)p < 0x40000000; p += delta)
	    {
	    for (ix = 0; ix < N_TIMES; ix++)		/* save and write */
		{
		temp[ix] = *((int *)p + ix);
		*((int *)p + ix) = TEST_PATTERN;
		}
	    cacheFlush (DATA_CACHE, p, 4 * sizeof(int));/* for 486, Pentium */

	    if (*(int *)p != TEST_PATTERN)		/* compare */
		{
		p -= delta;
		if (delta == VM_PAGE_SIZE)
		    {
		    memTop = p;
		    found = TRUE;
		    break;
		    }
		delta = VM_PAGE_SIZE;
		}

	    for (ix = 0; ix < N_TIMES; ix++)		/* restore */
		*((int *)p + ix) = temp[ix];
	    }

	if (!found)		/* we are fooled by write-back external cache */
	    memTop = (char *)(LOCAL_MEM_LOCAL_ADRS + LOCAL_MEM_SIZE);
#else
	memTop = (char *)(LOCAL_MEM_LOCAL_ADRS + LOCAL_MEM_SIZE);
#endif /* LOCAL_MEM_AUTOSIZE */

	/* copy the gdt from RAM/ROM to RAM, update, load */

	bcopy ((char *)sysGdt, (char *)pSysGdt, GDT_ENTRIES * sizeof(GDT));
	*(short *)&gdtr[0]	= GDT_ENTRIES * sizeof(GDT) - 1;
	*(int *)&gdtr[2]	= (int)pSysGdt;
	sysLoadGdt (gdtr);
	}

    return (memTop);
    }

/*******************************************************************************
*
* sysMemTop - get the address of the top of VxWorks memory
*
* This routine returns the address of the first byte of memory not controlled
* or used by VxWorks.
*
* The user can reserve memory space by defining the
* macro USER_RESERVED_MEM.  This
* routine returns the address of the reserved memory
* area.  The value of USER_RESERVED_MEM is specified in byte units.
* When used with any MMU options, USER_RESERVED_MEM should be a multiple
* of the MMU page size.  This will insure that the sysPhysMemDesc[] entry
* for main ram is also a multiple of the page size.
*
* For X86 architecture, if the image is being loaded into low memory (below
* 1 Mbyte) then the top of memory is forced to be 0xa0000 (640K) for
* DOS compatibility. This protects the I/O area that starts at that address.
*
* RETURNS: The address of the top of VxWorks memory.
*/

char* sysMemTop (void)
    {
    static char * memTop = NULL;

    if (memTop == NULL)
	{
	memTop = sysPhysMemTop () - USER_RESERVED_MEM;

	if ((int)end < 0x100000)		/* this is for bootrom */
	    memTop = (char *)0xa0000;
	}

    return memTop;
    }


/*******************************************************************************
*
* sysToMonitor - transfer control to the ROM monitor
*
* This routine transfers control to the ROM monitor.  It is usually called
* only by reboot() -- which services ^X -- and by bus errors at interrupt
* level.  However, in some circumstances, the user may wish to introduce a
* new <startType> to enable special boot ROM facilities.
*
* For X86, the startType argument is not used.  The board is reset by
* pulsing the reset line and pointing the global descriptor table to 0x0.
*
* RETURNS: Does not return.
*/

STATUS sysToMonitor
    (
    int startType   /* passed to ROM to tell it how to boot */
    )
    {
    FUNCPTR pEntry;
    int ix;
    int iy;
    int iz;
    char buf[ROM_SIGNATURE_SIZE];
    short *pSrc;
    short *pDst;

    VM_ENABLE (FALSE);			/* disable MMU */

    /* determine destination RAM address and the entry point */

    if ((int)end > 0x100000)
	{
	pDst = (short *)RAM_HIGH_ADRS;	/* copy it in lower mem */
	pEntry = (FUNCPTR)(RAM_HIGH_ADRS + ROM_WARM_HIGH);
	}
    else
	{
	pDst = (short *)RAM_LOW_ADRS;	/* copy it in upper mem */
	pEntry = (FUNCPTR)(RAM_LOW_ADRS + ROM_WARM_LOW);
	}

    /* copy EPROM to RAM and jump, if there is a VxWorks EPROM */

    for (ix = 0; ix < NELEMENTS(sysRomBase); ix++)
	{
	bcopyBytes ((char *)sysRomBase[ix], buf, ROM_SIGNATURE_SIZE);
	if (strncmp (sysRomSignature, buf, ROM_SIGNATURE_SIZE) == 0)
	    {
	    for (iy = 0; iy < 1024; iy++)
		{
		*sysRomBase[ix] = iy;		/* map the moveable window */
		pSrc = (short *)((int)sysRomBase[ix] + 0x200);
	        for (iz = 0; iz < 256; iz++)
		    *pDst++ = *pSrc++;
		}
	    (*pEntry) (startType);
	    }
	}

    intLock ();


    sysHwInit ();		/* disable all sub systems to a quiet state */

    sysClkDisable ();

    /* TODO - This assumes the keyboard controller */

    sysWait ();
    sysOutByte (COMMAND_8042, 0xfe);	/* assert SYSRESET */
    sysWait ();
    sysOutByte (COMMAND_8042, 0xff);	/* NULL command */

    sysReboot ();			/* crash the global descriptor table */

    return (OK);	/* in case we ever continue from ROM monitor */
    }


/******************************************************************************
*
* sysProcNumGet - get the processor number
*
* This routine returns the processor number for the CPU board, which is
* set with sysProcNumSet().
*
* RETURNS: The processor number for the CPU board.
*
* SEE ALSO: sysProcNumSet()
*/

int sysProcNumGet (void)
    {
    return (sysProcNum);
    }

/******************************************************************************
*
* sysProcNumSet - set the processor number
*
* This routine sets the processor number for the CPU board.  Processor numbers
* should be unique on a single backplane.
*
* For bus systems, it is assumes that processor 0 is the bus master and
* exports its memory to the bus.
*
* RETURNS: N/A
*
* SEE ALSO: sysProcNumGet()
*/

void sysProcNumSet
    (
    int procNum			/* processor number */
    )
    {
    sysProcNum = procNum;

    if (procNum == 0)
        {
#ifdef INCLUDE_VME
	/* TODO - one-time setup of slave window control registers */
#endif

#ifdef INCLUDE_PCI
	/* TODO - Enable/Initialize the interface as bus slave */
#endif
	}
    }

/******************************************************************************
*
* sysLanIntEnable - enable the LAN interrupt
*
* This routine enables interrupts at a specified level for the on-board LAN
* chip.
*
* RETURNS: OK, or ERROR if network support not included.
*
* SEE ALSO: sysLanIntDisable()
*/

STATUS sysLanIntEnable
    (
    int intLevel 		/* interrupt level to enable */
    )
    {
#ifdef INCLUDE_NETWORK

    /* TODO - enable lan interrupt */

    return (OK);

#else

    return (ERROR);

#endif /* INCLUDE_NETWORK */
    }


/******************************************************************************
*
* sysLanIntDisable - disable the LAN interrupt
*
* This routine disables interrupts for the on-board LAN chip.
*
* RETURNS: OK, or ERROR if network support not included.
*
* SEE ALSO: sysLanIntEnable()
*/

STATUS sysLanIntDisable
    (
    int intLevel 		/* interrupt level to enable */
    )
    {
#ifdef INCLUDE_NETWORK

    /* TODO - disable lan interrupt */

    return (OK);

#else

    return (ERROR);

#endif /* INCLUDE_NETWORK */
    }

/******************************************************************************
*
* sysLanEnetAddrGet - retrieve ethernet address.
*
* This routine returns a six-byte ethernet address for a given ethernet unit.
* The template END driver uses this routine to obtain the ethernet address if
* indicated by a user-flag in END_LOAD_STRING in configNet.h; or if the
* reading the ethernet address ROM is unsuccessful.
*
* RETURNS: OK or ERROR if ROM has valid standard ethernet address
*/

STATUS sysLanEnetAddrGet
    (
    int		unit,
    char *	enetAdrs
    )
    {
    /* TODO -The default return code is ERROR, change function if necessary. */

    return (ERROR);
    }

volatile static int sysNanoDummy = 1; /* dummy variable for spin loop */

/******************************************************************************
*
* sysNanoDelay - delay for specified number of nanoseconds
*
* This function implements a spin loop type delay for at
* least the specified number of nanoseconds.  This is not a task delay,
* control of the processor is not given up to another task.  The actual delay
* will be equal to or greater than the requested number of nanoseconds.
*
* The purpose of this function is to provide a reasonably accurate time delay
* of very short duration.  It should not be used for any delays that are much
* greater than two system clock ticks in length.  For delays of a full clock
* tick, or more, the use of taskDelay() is recommended.
*
* This routine is interrupt safe.
*
* .nT
*
* RETURNS: N/A.
*
* SEE ALSO:
* .sA
*/ 

void sysNanoDelay
    (
    UINT32 nanoseconds	/* nanoseconds to delay */
    )
    {
    int i;

    /*
     * TODO - setup a calibrated spin loop.
     *
     * In this example code, we assume that one pass of the spin
     * loop takes 1 microsecond for this BSP and processor.  Be sure
     * to use 'volatile' as appropriate to force the compiler to 
     * not optimize your code.
     *
     * Must be interrupt safe.
     *
     * To calibrate, set SYS_LOOP_NANO to 1.  Measure the elapsed time
     * for sysNanoDelay(1000000) to execute.  Set SYS_NANO_LOOP to the
     * the number of milliseconds measured (round up to integer!).
     * (or measure time for sysNanoDelay(1000) and set SYS_NANO_LOOP
     * to the measured number of microseconds, rounded up to an integer).
     */

#define SYS_LOOP_NANO	1000	/* loop time in nanoseconds */

    sysNanoDummy = 1;
    for (i = 0; i < (nanoseconds/SYS_LOOP_NANO) + 1; i++)
	{
	sysNanoDummy += 1;
	sysNanoDummy *= 2;
	}
    }
