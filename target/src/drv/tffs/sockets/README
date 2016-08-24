The socket drivers provided in this directory are intended to be informational. They were used to test the TrueFFS core on each of the respective BSPs and will
work in their default environments. It is highly likely that you will have to 
modify them to suite your BSP and it's implementation of Flash support. The 
following instructions describe the modifications you will need to make to the
BSP to build vxWorks with TrueFFS support in each of the respective BSPs. The
current BSP REV Level is 1.1. Future revisions of BSPs might have these changes
incorporated in them.

The names of each of the socket drivers, sysTffs.c,is prepended by the name
of the BSP that it belongs to. Example: mv177-sysTffs.c contains the socket
drivers used for the mv177 BSP. If you were using the mv177 BSP you would
copy mv177-sysTffs.c to target/config/mv177/sysTffs.c and follow the 
instructions in this document in the section mv177:. You can then rebuild
your vxWorks image and boot it on the target and have TrueFFS support for
the Flash part.

The driver pc386-sysTffs.c and the pc386 BSP modification instructions were
tested on the pc486 BSP as well. Similarly, the pid7t driver and BSP 
modifications were tested on pid7t_t as well.

-------------------------------------------------------------------------------
ads860:
-------------------------------------------------------------------------------

Makefile modifications:
  - Add MACH_EXTRA configuration macro as follows.
      MACH_EXTRA = sysTffs.o
config.h modifications:
  - Define INCLUDE_TFFS to link in tffsDrv() which is called in 
    usrConfig.c or bootConfig.c to initialize the TFFS.
  - Define INCLUDE_SHOW_ROUTINES to link in tffsShow() which shows 
    a relationship between a drive number and a socket interface.
  - Define INCLUDE_DOSFS to link in DOS File System.
  - Change ROM_SIZE from 1MB to 2MB.
      #define ROM_SIZE		0x00200000 	/* 2M ROM space */
sysLib.c modifications:
  - Add two entries in sysPhysMemDesc[] for a flash memory area as follows.
    PHYS_MEM_DESC sysPhysMemDesc [] =
    {
    {
    (void *) LOCAL_MEM_LOCAL_ADRS,
    (void *) LOCAL_MEM_LOCAL_ADRS,
    LOCAL_MEM_SIZE ,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE
    },

    {
    (void *) PC_BASE_ADRS_0,
    (void *) PC_BASE_ADRS_0,
    PC_SIZE_0,				/* 1 m - PCMCIA window 0 */	
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    :

    {
    (void *) PC_BASE_ADRS_1,
    (void *) PC_BASE_ADRS_1,
    PC_SIZE_1,				/* 32 m - PCMCIA window 1 */	
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    }
    };
ads860.h modifications:
  - Define macros for PCMCIA memory mapping as follows.
     #define	PC_BASE_ADRS_0		0x02000000	/* PCMCIA base address */
     #define	PC_SIZE_0		0x00100000	/* PCMCIA mapping size */
     #define	PC_BASE_ADRS_1		0x04000000	/* PCMCIA base address */
     #define	PC_SIZE_1		0x02000000	/* PCMCIA mapping size */
tffsBootImagePut:
  - Second parameter "offset" is (ROM_TEXT_ADRS - ROM_BASE_ADRS), 
    so it is 0x100 for the default configuration.
  - elfToBin should be used to get the boot-image for tffsBootImagePut.
    elfToBin < bootrom > bootrom.bin

-------------------------------------------------------------------------------
hkbaja47:
-------------------------------------------------------------------------------
Makefile modifications:
  - Add MACH_EXTRA configuration macro as follows.
      MACH_EXTRA = vxLib.obj sysTffs.o
config.h modifications:
  - Define INCLUDE_TFFS to link in tffsDrv() which is called in 
    usrConfig.c or bootConfig.c to initialize the TFFS.
  - Define INCLUDE_SHOW_ROUTINES to link in tffsShow() which shows 
    a relationship between a drive number and a socket interface.
  - Define INCLUDE_DOSFS to link in DOS File System.

-------------------------------------------------------------------------------
iq960rp:
-------------------------------------------------------------------------------
The patch provided by WRS for IQ960RD must be installed before
making any changes.

Makefile modifications:
  - Add MACH_EXTRA configuration macro as follows.
      MACH_EXTRA = sysTffs.o
config.h modifications:
  - Define INCLUDE_TFFS to link in tffsDrv() which is called in 
    usrConfig.c or bootConfig.c to initialize the TFFS.
  - Define INCLUDE_SHOW_ROUTINES to link in tffsShow() which shows 
    a relationship between a drive number and a socket interface.
  - Define INCLUDE_DOSFS to link in DOS File System.

-------------------------------------------------------------------------------
mv177:
-------------------------------------------------------------------------------
Makefile modifications:
  - Add MACH_EXTRA configuration macro as follows.
      MACH_EXTRA = sysTffs.o
config.h modifications:
  - Define INCLUDE_TFFS to link in tffsDrv() which is called in 
    usrConfig.c or bootConfig.c to initialize the TFFS.
  - Define INCLUDE_SHOW_ROUTINES to link in tffsShow() which shows 
    a relationship between a drive number and a socket interface.
  - Define INCLUDE_DOSFS to link in DOS File System.
sysLib.c modifications:
  - Make ROM area writable, as follows:
    /* ROM */
    {
    (void *) ROM_BASE_ADRS,
    (void *) ROM_BASE_ADRS,
    0x400000,
    VM_STATE_MASK_VALID	| VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	| VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

-------------------------------------------------------------------------------
pc386/pc486:
-------------------------------------------------------------------------------
Makefile modifications:
  - Add MACH_EXTRA configuration macro as follows.
      MACH_EXTRA = sysTffs.o
config.h modifications:
  - Define INCLUDE_TFFS or INCLUDE_PCMCIA configuration macro as follows.
      #define INCLUDE_TFFS
      or
      #define INCLUDE_PCMCIA
    INCLUDE_TFFS pulls in and uses its own PCIC(PCMCIA controller chip) driver.
    INCLUDE_PCMCIA uses a driver provided by PCMCIA package.  
    The TFFS's PCIC driver doesn't check a type of PC card and it assumes the
    accessing PC card is a flash PC card.  However, the size of the driver is
    small.  Thus it might be suitable for people who have memory constraint.
    On the other hand, the PCMCIA package checks a type of PC card and allows
    TFFS driver to access the PC card only if it is a flash PC card. 

    If you wish to use the TrueFFS services with the PCMCIA services you
    must add it to the list of services that PCMCIA will include in to 
    vxWorks.

	#ifdef  INCLUDE_PCMCIA
	#define INCLUDE_TFFS		/* include TFFS driver */  
	#define INCLUDE_ATA             /* include ATA driver */
	#define INCLUDE_SRAM            /* include SRAM driver */
	#ifdef  INCLUDE_NETWORK
	#define INCLUDE_ELT             /* include 3COM EtherLink III driver */
	#endif  /* INCLUDE_NETWORK */
	#endif  /* INCLUDE_PCMCIA */
	
  - Redefine  CIS macros to avoid address conflicts with DiskOnChip 2000. 
    If you are using the M-System DiskOnCHip2000 product you might run in
    to address conflicts with the vxWorks PCMICA product. To resolve this 
    you can add the following to config.h
    
	#ifdef INCLUDE_TFFS
	#undef CIS_MEM_START
	#undef CIS_MEM_STOP
	#undef CIS_REG_START
	#undef CIS_REG_STOP
	#define CIS_MEM_START	0xc8000	/* mapping addr for CIS tuple */
	#define CIS_MEM_STOP	0xcbfff
	#define CIS_REG_START	0xcc000 /* mapping addr for config reg */
	#define CIS_REG_STOP	0xccfff 
	#endif /* INCLUDE_TFFS */

  - Define the warm boot parameter for reboot
        #define SYS_WARM_TFFS_DRIVE     0       /* 0 = c: (DOC) */
    

  - Define INCLUDE_SHOW_ROUTINES to link in tffsShow() which shows 
    a relationship between a drive number and a socket interface.
sysLib.c
  - If your C: drive is Disk On Chip and you are booting from it instead of 
    floppy disk, set sysWarmType global variable to 3 for warm start from 
    the Disk On Chip.

  - The following changes need to be made to sysLib.c if you wish to reboot
    from the Truffs Flash drive.

	Look for the declaration of the variable sysWarmAtaDrive and add 
	the following line after it.

int	sysWarmTffsDrive= SYS_WARM_TFFS_DRIVE; /* TFFS drive 0 (DOC) */

	In the function sysToMonitor you will find the clause that defines
	the reboot process for the reboot device ATA. Add the the following 
	after the #endif statement for INCLUDE_ATA :

#ifdef	INCLUDE_TFFS
    if (sysWarmType == 3)
	{
	IMPORT int dosFsDrvNum;
 
         tffsDrv ();				/* initialize TFFS */
 	if (dosFsDrvNum == ERROR)
     	    dosFsInit (NUM_DOSFS_FILES);	/* initialize DOS-FS */
 
 	if (usrTffsConfig (sysWarmTffsDrive, FALSE, "/vxboot/") == ERROR)
 	    {
 	    printErr ("usrTffsConfig failed.\n");
 	    return (ERROR);
 	    }
	}
#endif	/* INCLUDE_TFFS */
 

	Search for the lines that appear as follows :

#if	(defined (INCLUDE_FD) || defined (INCLUDE_ATA))
    if ((sysWarmType == 1) || (sysWarmType == 2))

	replace them with the following :

#if	(defined(INCLUDE_FD) || defined(INCLUDE_ATA) || defined(INCLUDE_TFFS))
    if ((sysWarmType == 1) || (sysWarmType == 2) || (sysWarmType == 3))

	Search for the line that appears as follows :

#endif	/* defined (INCLUDE_FD) || defined (INCLUDE_ATA) */

	replace them with the following :

#endif	/* (INCLUDE_FD) || (INCLUDE_ATA) || (INCLUDE_TFFS) */

-------------------------------------------------------------------------------
pid7t/pid7t_t:
-------------------------------------------------------------------------------
Makefile modifications:
  - Add MACH_EXTRA configuration macro as follows.
      MACH_EXTRA = if_oli.obj sysTffs.o
config.h modifications:
  - Define INCLUDE_TFFS to link in tffsDrv() which is called in 
    usrConfig.c or bootConfig.c to initialize the TFFS.
  - Define INCLUDE_SHOW_ROUTINES to link in tffsShow() which shows 
    a relationship between a drive number and a socket interface.
  - Define INCLUDE_DOSFS to link in DOS File System.
tffsBootImagePut:
  - Second parameter "offset" is (ROM_TEXT_ADRS - ROM_BASE_ADRS), 
    so it is 0 for the default configuration.
  - coffArmToBin should be used to get the boot-image for tffsBootImagePut.
    coffArmToBin < bootrom > bootrom.bin

-------------------------------------------------------------------------------
sony4520:
-------------------------------------------------------------------------------
Makefile modifications:
  - Add MACH_EXTRA configuration macro as follows.
      MACH_EXTRA = sysTffs.o
config.h modifications:
  - Define INCLUDE_TFFS to link in tffsDrv() which is called in 
    usrConfig.c or bootConfig.c to initialize the TFFS.
  - Define INCLUDE_SHOW_ROUTINES to link in tffsShow() which shows 
    a relationship between a drive number and a socket interface.
  - Define INCLUDE_DOSFS to link in DOS File System.

-------------------------------------------------------------------------------
ss5:
-------------------------------------------------------------------------------
Makefile modifications:
  - Add MACH_EXTRA configuration macro as follows.
      MACH_EXTRA = memDesc.o sysTffs.o
config.h modifications:
  - Define INCLUDE_TFFS to link in tffsDrv() which is called in 
    usrConfig.c or bootConfig.c to initialize the TFFS.
  - Define INCLUDE_SHOW_ROUTINES to link in tffsShow() which shows 
    a relationship between a drive number and a socket interface.
  - Define INCLUDE_DOSFS to link in DOS File System.
  - Undefine USER_D_CACHE_ENABLE to disable the data cache since TFFS assumes
    that it is directly accessing the flash memory.
  - Define INCLUDE_FLASH to make the flash memory writable.

-------------------------------------------------------------------------------
Disk On Chip 2000
-------------------------------------------------------------------------------
The socket driver doc2k-sysTffs.c is specific to a system that has only a Disk
On Chip 2000 component from M-Systems.  The socket driver has been tested with
pc386 and pc486 BSPs and the mbx860 BSP but is believed to be portable to any
BSP. Since the DOC2_SCAN addresses are board specific they have been placed in
conditional compilation constants. To compile this file you must make the 
following change to the Makefile in the BSP directory. 

If the Makefile already has a variable called EXTRA_DEFINE then add
-D$(BSP_NAME) to it. If not add the line

EXTRA_DEFINE	= -D$(BSP_NAME)
