/* pc386/config.h - PC [34]86/Pentium/Pentium[234] configuration header */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
04h,12jul02,pai  Changed RAM_HIGH_ADRS, RAM_LOW_ADRS, and
                 LOCAL_MEM_LOCAL_ADRS.
04g,12jul02,dmh  added check for !SYS_WARM_BIOS and set NV_RAM_SIZE
04f,27jun02,hdn  made SYS_CLK_RATE_MAX an arch dependent macro (spr 27449)
04e,05jun02,hdn  added 36Bit MMU support with 2MB page size
04d,09may02,hdn  added Pentium4 Asymmetric Multi Processor configuration
04c,10may02,pai  In an effort to reduce the compiled size of default images,
                 the ATA driver (INCLUDE_ATA) and PC console
                 (INCLUDE_PC_CONSOLE) are no longer configured into a build by
                 default (SPR 77193).
04b,01may02,pai  Remove ATAPI/5 configuration introduced in version 03w.
04a,25apr02,rhe  Added C++ Protection
03z,24apr02,pai  Added a sysHwInit0() prototype, as a generic interface of
                 this name has yet to be defined.
03y,22apr02,pai  Removed configuration constants for netif drivers that have
                 an END equivalent.  Made DEC and GEI END driver config
                 constant names consistent with other BSP END driver config
                 constant names.  Removed INCLUDE_EX and INCLUDE_ENP
                 config constants (SPR 75629).
03x,09apr02,pai  Reworked ATA configuration and documentation (SPR 73848).
                 Remove sysCpuProbe() dependence upon INCLUDE_CACHE_SUPPORT
                 configuration (SPR 74951).  Updated the configuration for
                 components that have a dependency on INCLUDE_PCI (SPR 75634).
                 Added sysInumTbl[] table declaration required for
                 INT_NUM_GET() (SPR 75710).
03w,15apr02,rip  support for ATAPI/5 
03v,01apr02,jkf  Added _WRS_BSP_DEBUG_NULL_ACCESS to generate exception when
                 code accesses to lower page of memory, null ptr, occur
03u,27mar02,pai  No longer explicitly setting WDB_COMM_TYPE to WDB_COMM_END,
                 as this is now done in configAll.h (SPR 73338).
03t,22mar02,pai  Halt in idle mode is now the default for Pentium-based BSPs
                 (SPR# 73324).
03s,13mar02,hdn  updated INT_NUM_GET(), added IPI/SM for HTT (spr 73738)
03r,12mar02,pai  Removed '#ifdef TRUE' around INCLUDE_PC_CONSOLE (SPR# 73325).
03q,07mar02,hdn  added SNOOP_ENABLE to USER_D_CACHE_MODE for P5 (spr 73938)
         undefed PMC, removed MTRR_GET for P5 (spr 73939)
03q,04feb02,mrs  Allow console selection from command line.
03p,04dec01,jkf  adding macros for reboot device strings
                 changed to #undef INCLUDE_LPT by default.
03o,27nov01,dmh  move include of iacsfl.h from sysLib.c to config.h
03n,16nov01,ahm  added power management support (SPR# 32599)
03m,13nov01,hdn  added comment for IOAPIC configuration.
03l,25oct01,pai  Removed INT_NUM_FEIx and FEIx_INT_VEC constants.  Added
                 support for gei82543End driver (jln).  Added missing
                 INT_NUM_MSE definition.
03k,24oct01,hdn  added the mother board and ICH definitions for PENTIUM4.
         enabled TSC timestamp counter for P5.  cleaned up.
03j,24oct01,dmh  add default macros for COM3 and COM4
03i,23oct01,pai  Removed obsolete INCLUDE_SLIP configuration constant.
03h,01oct01,hdn  fixed typo LPT_INT_LVL0 to LPT0_INT_LVL.
03g,26sep01,pai  Added support for dec21x40End driver.  Added INT_NUM_MSE
                 and MSE_INT_VEC backward compatibility values for WindML.
03f,14sep01,hdn  replaced INT_VEC_GET() with INT_NUM_GET() (spr 69775)
         made TIMER_CLOCK_HZ configurable for APIC TIMER
         incremented BSP_REV to 2, incremented SYS_CLK_RATE_MAX
         added LPT[012]_xxx macros (spr 30067) on behalf of pai
03e,16aug01,hdn  added PENTIUM2/3/4 support
03d,15aug01,hdn  moved INCLUDE_SCSI2 in the previous FALSE clause.
03c,14oct99,jk   added defines for sound support.
03b,15mar99,cn   corrected cross-dependencies between INCLUDE_PCI and
         INCLUDE_LN_97X_END, INCLUDE_EL_3C90X_END (SPR# 25680).
03a,12mar99,cn   added support for el3c90xEnd driver (SPR# 25327).
02z,08mar99,sbs  added support for SMC Elite Ultra ethernet card.(SPR #25234)
                 moved INCLUDE_LN_97X_END into INCLUDE_END loop.
                 added support for ne2000End driver (SPR #25398)
02y,26feb99,dat     removed FEI from NETIF_USR_ENTRIES (23818)
02x,25feb99,hdn  added PentiumPro's MESI bus snoop.
         removed GLOBAL bit from VM_STATE_FOR_MEM_OS.
02w,24feb99,pr   removed CONSOLE_TTY define for PC_CONSOLE (SPR#23075)
02v,01feb99,jkf  added END support for AMD 7997x PCI PCNet-FAST card.
                 made FEI_END the default.
02u,26nov98,ms_  add elt3c509 END support
02t,26jan99,jkf  INCLUDE_ADD_BOOTMEM added.  INCLUDE_END
                 is again made the default.  SPR#21338
02s,12nov98,dat  END drivers are not selected by default (See SPR xxxxx)
02r,28aug98,sbs  changed WDB_COMM_TYPE to WDB_COMM_END.
02q,03aug98,cn   new BSP revision id
02o,07jul98,db   changed BSP_VERSION to "1.2" and BSP_REVISION to "/0".
         added BSP_VER_1_2 macro (Tornado 2.0 release).
02n,28may98,hdn  added support for APIC.
02m,12may98,hdn  merged with pcPentium/config.h. obsolete INCLUDE_IDE.
         changed LOCAL_MEM_SIZE to 8MB. cleaned up.
02l,24apr98,yp   added defines for TFFS.
02k,31mar98,cn   Added Enhanced 


Network Driver support.
02j,24mar98,sbs  corrected SYS_WARM_FD to be default SYS_WARM_TYPE.
02i,18mar98,sbs  added SYS_WARM_BIOS, SYS_WARM_FD, SYS_WARM_ATA.
                 added SYS_INT_TRAPGATE, SYS_INT_INTGATE.
02h,16jan98,gnn  Removed documentation and support for END,
         not supported yet.
02g,28apr97,gnn  Added documentation for END_OVERRIDE and INCLUDE_END.
02f,25apr97,gnn  Added Enhanced Network Driver support.
02h,05aug97,dds  added INCLUDE_AIC_7880.
02g,11jul97,dds  #undef INCLUDE_CDROMFS.
02f,10jul97,dds  added SCSI-2 support. REV level 2
02e,26feb97,mas  added defs of USER_RESERVED_MEM and LOCAL_MEM_AUTOSIZE; added
         warm boot parameters (SPR 7806, 7850).
02d,22nov96,dat  fixed warning from NETIF_USR_ENTRIES.
02c,20nov96,hdn  added support for PRO100B.
02b,01nov96,hdn  added support for PCMCIA.
02a,10oct96,dat  release 1.1/1, (Tornado 1.0.1)
01z,03sep96,hdn  added the compression support. removed BOOTABLE macro.
01y,09aug96,hdn  renamed INT_VEC_IRQ0 to INT_NUM_IRQ0.
01x,05aug96,hdn  changed INT_LVL_ENE 0x0b to 0x05(default int level).
01w,19jul96,hdn  added support for ATA driver.
01v,25jun96,hdn  added support for TIMESTAMP timer.
01u,14jun96,hdn  added support for PCI bus.
01t,13jun96,hdn  added INCLUDE_ESMC for SMC91c9x Ethernet driver.
01s,28sep95,dat  new BSP revision id
01r,14jun95,hdn  added INCLUDE_SW_FP for FPP software emulation library.
01q,08jun95,ms   changed PC_CONSOLE defines.
01p,12jan95,hdn  changed SYS_CLK_RATE_MAX to a safe number.
01o,08dec94,hdn  changed EEROM to EEPROM.
01n,15oct94,hdn  changed CONFIG_ELC and CONFIG_ULTRA.
         added INCLUDE_LPT for LPT driver.
         added INCLUDE_EEX32 for Intel EtherExpress32.
         changed the default boot line.
         moved INT_VEC_IRQ0 to pc.h.
01m,03jun94,hdn  deleted shared memory network related macros.
01l,28apr94,hdn  changed ROM_SIZE to 0x7fe00.
01k,22apr94,hdn  added macros INT_VEC_IRQ0, FD_DMA_BUF, FD_DMA_BUF_SIZE.
         moved a macro PC_KBD_TYPE from pc.h.
         added SLIP driver with 9600 baudrate.
01j,15mar94,hdn  changed ULTRA configuration.
         changed CONSOLE_TTY number from 0 to 2.
01i,09feb94,hdn  added 3COM EtherlinkIII driver and Eagle NE2000 driver.
                 changed RAM_HIGH_ADRS and RAM_LOW_ADRS.
                 changed LOCAL_MEM_SIZE to 4MB.
01h,27jan94,hdn  changed RAM_HIGH_ADRS 0x110000 to 0x00108000.
         changed RAM_ENTRY 0x10000 to 0x00008000.
01g,17dec93,hdn  added support for Intel EtherExpress driver.
01f,24nov93,hdn  added INCLUDE_MMU_BASIC.
01e,08nov93,vin  added support for pc console drivers.
01d,03aug93,hdn  changed network board's address and vector.
01c,22apr93,hdn  changed default boot line.
01b,07apr93,hdn  renamed compaq to pc.
01a,15may92,hdn  written based on frc386.
*/

/*
This module contains the configuration parameters for the
PC [34]86/Pentium/Pentium[234].
*/

#ifndef    INCconfigh
#define    INCconfigh

#ifdef __cplusplus
extern "C" {
#endif

/* BSP version/revision identification, before configAll.h */

#define BSP_VER_1_1    1    /* 1.2 is backward compatible with 1.1 */
#define BSP_VER_1_2    1
#define BSP_VERSION    "1.2"
#define BSP_REV        "/2"    /* 2 for Tornado 2.2 */

#include "configAll.h"
#include "pc.h"


/* BSP specific prototypes that must be in config.h */

#ifndef _ASMLANGUAGE
    IMPORT void sysHwInit0 (void);
    IMPORT UINT8 sysInumTbl[];        /* IRQ vs intNum table */
#endif

/* BSP specific initialisation (before cacheLibInit() is called) */

#define INCLUDE_SYS_HW_INIT_0
#define SYS_HW_INIT_0()         (sysHwInit0())


/* Default boot line */

#if    (CPU == I80386)
#define DEFAULT_BOOT_LINE \
    "fd=0,0(0,0)host:/fd0/vxWorks.st h=90.0.0.3 e=90.0.0.50 u=target"
#elif    (CPU == I80486)
#define DEFAULT_BOOT_LINE \
    "fd=0,0(0,0)host:/fd0/vxWorks.st h=90.0.0.3 e=90.0.0.50 u=target"
#elif    (CPU == PENTIUM)
#define DEFAULT_BOOT_LINE \
    "lnPci(0,0)host:d:\\vxWorks h=192.168.102.1 e=192.168.102.88 u=target pw=target tn=target"
#elif    (CPU == PENTIUM2)

#define DEFAULT_BOOT_LINE \
    "fd=0,0(0,0)host:/fd0/vxWorks.st h=90.0.0.3 e=90.0.0.50 u=target"
#elif    (CPU == PENTIUM3)
#define DEFAULT_BOOT_LINE \
    "fd=0,0(0,0)host:/fd0/vxWorks.st h=90.0.0.3 e=90.0.0.50 u=target"
#elif    (CPU == PENTIUM4)
#define DEFAULT_BOOT_LINE \
    "fd=0,0(0,0)host:/fd0/vxWorks.st h=90.0.0.3 e=90.0.0.50 u=target"
#endif    /* (CPU == I80386) */

/* Warm boot (reboot) devices and parameters */

#define SYS_WARM_BIOS         0     /* warm start from BIOS */
#define SYS_WARM_FD           1     /* warm start from FD */
#define SYS_WARM_ATA          2    /* warm start from ATA */
#define SYS_WARM_TFFS          3    /* warm start from DiskOnChip */

#define SYS_WARM_TYPE        SYS_WARM_FD /* warm start device */
#define SYS_WARM_FD_DRIVE       0       /* 0 = drive a:, 1 = b: */
#define SYS_WARM_FD_TYPE        0       /* 0 = 3.5" 2HD, 1 = 5.25" 2HD */
#define SYS_WARM_ATA_CTRL       0       /* controller 0 */
#define SYS_WARM_ATA_DRIVE      0       /* 0 = c:, 1 = d: */
#define SYS_WARM_TFFS_DRIVE     0       /* 0 = c: (DOC) */

/* Warm boot (reboot) device and filename strings */

/* 
 * BOOTROM_DIR is the device name for the device containing
 * the bootrom file. This string is used in sysToMonitor, sysLib.c 
 * in dosFsDevCreate().
 */

#define BOOTROM_DIR  "/vxboot/"

/* 
 * BOOTROM_BIN is the default path and file name to either a binary 
 * bootrom file or an A.OUT file with its 32 byte header stripped.
 * Note that the first part of this string must match BOOTROM_DIR
 * The "bootrom.sys" file name will work with VxLd 1.5.
 */

#define BOOTROM_BIN  "/vxboot/bootrom.sys"

/* 
 * BOOTROM_AOUT is that default path and file name of an A.OUT bootrom
 * _still containing_ its 32byte A.OUT header.   This is legacy code.
 * Note that the first part of this string must match BOOTROM_DIR
 * The "bootrom.dat" file name does not work with VxLd 1.5.
 */

#define BOOTROM_AOUT "/vxboot/bootrom.dat"

/* IDT entry type options */

#define SYS_INT_TRAPGATE     0x0000ef00     /* trap gate */
#define SYS_INT_INTGATE      0x0000ee00     /* int gate */

/* driver and file system options */

#define    INCLUDE_DOSFS        /* include dosFs file system */
#define    INCLUDE_FD        /* include floppy disk driver */
#undef    INCLUDE_ATA        /* include IDE/EIDE(ATA) hard disk driver */
#undef    INCLUDE_LPT        /* include parallel port driver */
#undef    INCLUDE_TIMESTAMP    /* include TIMESTAMP timer for Wind View */
#undef    INCLUDE_TFFS        /* include TrueFFS driver for Flash */
#undef    INCLUDE_PCMCIA        /* include PCMCIA driver */

/* TFFS driver options */

#ifdef    INCLUDE_TFFS
#   define INCLUDE_SHOW_ROUTINES
#endif    /* INCLUDE_TFFS */

/* SCSI driver options */

#undef    INCLUDE_SCSI            /* include SCSI driver */
#undef    INCLUDE_AIC_7880        /* include AIC 7880 SCSI driver */
#undef    INCLUDE_SCSI_BOOT       /* include ability to boot from SCSI */
#undef    INCLUDE_CDROMFS         /* file system to be used */
#undef    INCLUDE_TAPEFS          /* file system to be used */
#undef    INCLUDE_SCSI2           /* select SCSI2 not SCSI1 */

/* Network driver options */

#define INCLUDE_END             /* Enhanced Network Driver Support */

#undef  INCLUDE_DEC21X40_END    /* (END) DEC 21x4x PCI interface */
#undef  INCLUDE_EL_3C90X_END    /* (END) 3Com Fast EtherLink XL PCI */
#undef  INCLUDE_ELT_3C509_END   /* (END) 3Com EtherLink III interface */
#undef  INCLUDE_ENE_END         /* (END) Eagle/Novell NE2000 interface */
#undef    INCLUDE_FEI_END         /* (END) Intel 8255[7/8/9] PCI interface */
#undef    INCLUDE_GEI8254X_END    /* (END) Intel 82543/82544 PCI interface */
#define  INCLUDE_LN_97X_END      /* (END) AMD 79C97x PCI interface */
#undef  INCLUDE_ULTRA_END       /* (END) SMC Elite16 Ultra interface */

#undef  INCLUDE_BSD             /* BSD / Netif Driver Support (Deprecated) */

#undef  INCLUDE_EEX             /* (BSD) Intel EtherExpress interface */
#undef  INCLUDE_EEX32           /* (BSD) Intel EtherExpress flash 32 */
#undef  INCLUDE_ELC             /* (BSD) SMC Elite16 interface */
#undef  INCLUDE_ESMC            /* (BSD) SMC 91c9x Ethernet interface */

/* PCMCIA driver options */

#ifdef  INCLUDE_PCMCIA

#   define INCLUDE_ATA          /* include ATA driver */
#   define INCLUDE_SRAM         /* include SRAM driver */
#   undef INCLUDE_TFFS          /* include TFFS driver */

#   ifdef INCLUDE_NETWORK
#       define INCLUDE_BSD      /* include BSD / Netif Driver Support */
#       define INCLUDE_ELT      /* (BSD) 3Com EtherLink III interface */
#   endif /* INCLUDE_NETWORK */

#endif  /* INCLUDE_PCMCIA */


/* Include PCI support for drivers & libraries that require it. */

#if defined (INCLUDE_LN_97X_END)   || defined (INCLUDE_EL_3C90X_END) || \
    defined (INCLUDE_FEI_END)      || defined (INCLUDE_DEC21X40_END) || \
    defined (INCLUDE_GEI8254X_END) || defined (INCLUDE_AIC_7880)     || \
    defined (INCLUDE_WINDML)       || defined (INCLUDE_USB)

#   define INCLUDE_PCI

#endif


/* default MMU options and PHYS_MEM_DESC type state constants */

#define INCLUDE_MMU_BASIC       /* bundled MMU support */

#define VM_STATE_MASK_FOR_ALL \
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE
#define VM_STATE_FOR_IO \
    VM_STATE_VALID | VM_STATE_WRITABLE | VM_STATE_CACHEABLE_NOT
#define VM_STATE_FOR_MEM_OS \
    VM_STATE_VALID | VM_STATE_WRITABLE | VM_STATE_CACHEABLE
#define VM_STATE_FOR_MEM_APPLICATION \
    VM_STATE_VALID | VM_STATE_WRITABLE | VM_STATE_CACHEABLE
#define VM_STATE_FOR_PCI \
    VM_STATE_VALID | VM_STATE_WRITABLE | VM_STATE_CACHEABLE_NOT


/* default system and auxiliary clock constants
 *
 * Among other things, SYS_CLK_RATE_MAX depends upon the CPU and application
 * work load.  The default value, chosen in order to pass the internal test
 * suite, could go up to PIT_CLOCK.
 */

#define SYS_CLK_RATE_MIN    (19)           /* minimum system clock rate */
#define AUX_CLK_RATE_MIN    (2)            /* minimum auxiliary clock rate */
#define AUX_CLK_RATE_MAX    (8192)         /* maximum auxiliary clock rate */


/* CPU family/type-specific macros and options */

#if    (CPU == I80386) || (CPU == I80486) /* [34]86 specific macros */

/*
 * software floating point emulation support. DO NOT undefine hardware fp
 * support in configAll.h as it is required for software fp emulation.
 */

#define INCLUDE_SW_FP        /* enable emulator if there is no FPU */
#define SYS_CLK_RATE_MAX    (PIT_CLOCK/32) /* max system clock rate */

#ifdef    INCLUDE_TIMESTAMP
#   define INCLUDE_TIMESTAMP_PIT2 /* include PIT2 for timestamp */
#endif    /* INCLUDE_TIMESTAMP */

#elif    (CPU == PENTIUM)    /* P5 specific macros */

#undef    INCLUDE_SW_FP        /* Pentium has hardware FPP */
#undef    USER_D_CACHE_MODE    /* Pentium write-back data cache support */
#define    USER_D_CACHE_MODE    (CACHE_COPYBACK | CACHE_SNOOP_ENABLE)
#undef    INCLUDE_PMC        /* include PMC */
#define SYS_CLK_RATE_MAX    (PIT_CLOCK/32) /* max system clock rate */

#ifdef    INCLUDE_TIMESTAMP    /* select TSC(default) or PIT2 */
#   undef  INCLUDE_TIMESTAMP_PIT2 /* include PIT2 for timestamp */
#   define INCLUDE_TIMESTAMP_TSC  /* include TSC for timestamp */
#   define PENTIUMPRO_TSC_FREQ    0 /* TSC freq, 0 for auto detect */
#endif    /* INCLUDE_TIMESTAMP */

#elif    (CPU == PENTIUM2) || (CPU == PENTIUM3) || (CPU == PENTIUM4) /* P6,P7 */

#undef    INCLUDE_SW_FP        /* Pentium[234] has hardware FPP */
#undef    USER_D_CACHE_MODE    /* Pentium[234] write-back data cache support */
#define    USER_D_CACHE_MODE    (CACHE_COPYBACK | CACHE_SNOOP_ENABLE)
#define    INCLUDE_MTRR_GET    /* get MTRR to sysMtrr[] */
#define    INCLUDE_PMC        /* include PMC */
#undef    VIRTUAL_WIRE_MODE    /* Interrupt Mode: Virtual Wire Mode */
#undef    SYMMETRIC_IO_MODE    /* Interrupt Mode: Symmetric IO Mode */
#define SYS_CLK_RATE_MAX    (PIT_CLOCK/16) /* max system clock rate */

#ifdef    INCLUDE_TIMESTAMP         /* select TSC(default) or PIT2 */
#   undef  INCLUDE_TIMESTAMP_PIT2 /* include PIT2 for timestamp */
#   define INCLUDE_TIMESTAMP_TSC  /* include TSC for timestamp */
#   define PENTIUMPRO_TSC_FREQ    0 /* TSC freq, 0 for auto detect */
#endif    /* INCLUDE_TIMESTAMP */

#define    INCLUDE_MMU_P6_32BIT    /* include 32bit MMU for Pentium[234] */
#ifdef    INCLUDE_MMU_P6_32BIT
#   undef  VM_PAGE_SIZE        /* page size could be 4KB, 4MB */
#   define VM_PAGE_SIZE        PAGE_SIZE_4KB    /* PAGE_SIZE_4MB */
#endif    /* INCLUDE_MMU_P6_32BIT */
#ifdef    INCLUDE_MMU_P6_36BIT
#   undef  VM_PAGE_SIZE        /* page size could be 4KB, 2MB */
#   define VM_PAGE_SIZE        PAGE_SIZE_4KB    /* PAGE_SIZE_2MB */
#endif    /* INCLUDE_MMU_P6_32BIT */

#if    defined (INCLUDE_MMU_P6_32BIT) || defined (INCLUDE_MMU_P6_36BIT) 
#   undef  VM_STATE_MASK_FOR_ALL
#   undef  VM_STATE_FOR_IO
#   undef  VM_STATE_FOR_MEM_OS
#   undef  VM_STATE_FOR_MEM_APPLICATION
#   undef  VM_STATE_FOR_PCI
#   define VM_STATE_MASK_FOR_ALL \
       VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | \
       VM_STATE_MASK_CACHEABLE | VM_STATE_MASK_WBACK | VM_STATE_MASK_GLOBAL
#   define VM_STATE_FOR_IO \
       VM_STATE_VALID | VM_STATE_WRITABLE | \
       VM_STATE_CACHEABLE_NOT | VM_STATE_WBACK_NOT | VM_STATE_GLOBAL_NOT
#   define VM_STATE_FOR_MEM_OS \
       VM_STATE_VALID | VM_STATE_WRITABLE | \
       VM_STATE_CACHEABLE | VM_STATE_WBACK | VM_STATE_GLOBAL_NOT
#   define VM_STATE_FOR_MEM_APPLICATION \
       VM_STATE_VALID | VM_STATE_WRITABLE | \
       VM_STATE_CACHEABLE | VM_STATE_WBACK | VM_STATE_GLOBAL_NOT
#   define VM_STATE_FOR_PCI \
       VM_STATE_VALID | VM_STATE_WRITABLE | \
       VM_STATE_CACHEABLE_NOT | VM_STATE_WBACK_NOT | VM_STATE_GLOBAL_NOT
#endif    /* defined (INCLUDE_MMU_P6_32BIT) || defined (INCLUDE_MMU_P6_36BIT) */

/* 
 * To enable the IOAPIC, define the mother board from the following list.
 * If the IOAPIC is already enabled, defining the mother board is not 
 * needed.  Related code locates in pciCfgIntStub.c.
 *   D815EEA = Pentium3 + i815e + ICH2(i82801BA)
 *   D850GB  = Pentium4 + i850  + ICH2(i82801BA)
 * The PIRQ[n] is directly handled by IOAPIC in the SYMMETRIC_IO_MODE.
 */

#undef    INCLUDE_D815EEA        /* Pentium3 + i815e + ICH2 */
#undef    INCLUDE_D850GB        /* Pentium4 + i850  + ICH2 */

#if    defined (INCLUDE_D815EEA) || defined (INCLUDE_D850GB)
#   define INCLUDE_ICH2        /* ICH2 IO controller hub */
#else
#   if    (CPU == PENTIUM4)
#       define INCLUDE_ICH3    /* set ICH3 as default */
#   endif /* (CPU == PENTIUM4) */
#endif    /* defined (INCLUDE_D815EEA) || defined (INCLUDE_D850GB) */

#endif    /* (CPU == I80386) || (CPU == I80486) */


#define IO_ADRS_ELC    0x240
#define INT_LVL_ELC    0x0b
#define MEM_ADRS_ELC    0xc8000
#define MEM_SIZE_ELC    0x4000
#define CONFIG_ELC    0    /* 0=EEPROM 1=RJ45+AUI 2=RJ45+BNC */

#define IO_ADRS_ULTRA    0x240
#define INT_LVL_ULTRA    0x0b
#define MEM_ADRS_ULTRA    0xc8000
#define MEM_SIZE_ULTRA    0x4000
#define CONFIG_ULTRA    0    /* 0=EEPROM 1=RJ45+AUI 2=RJ45+BNC */

#define IO_ADRS_EEX    0x240
#define INT_LVL_EEX    0x0b
#define NTFDS_EEX    0x00
#define CONFIG_EEX    0    /* 0=EEPROM  1=AUI  2=BNC  3=RJ45 */
                /* Auto-detect is not supported, so choose */
                /* the right one you're going to use */

#define IO_ADRS_ELT    0x240
#define INT_LVL_ELT    0x0b
#define NRF_ELT        0x00
#define CONFIG_ELT    0    /* 0=EEPROM 1=AUI  2=BNC  3=RJ45 */

#define IO_ADRS_ENE    0x300
#define INT_LVL_ENE    0x05
                /* Hardware jumper is used to set */
                /* RJ45(Twisted Pair) AUI(Thick) BNC(Thin) */

#define IO_ADRS_ESMC    0x300
#define INT_LVL_ESMC    0x0b
#define CONFIG_ESMC    0    /* 0=EEPROM 1=AUI  2=BNC 3=RJ45 */
#define RX_MODE_ESMC    0    /* 0=interrupt level 1=task level */

#ifdef    INCLUDE_EEX32
#   define INCLUDE_EI        /* include 82596 driver */
#   define INT_LVL_EI    0x0b
#   define EI_SYSBUS    0x44    /* 82596 SYSBUS value */
#   define EI_POOL_ADRS    NONE    /* memory allocated from system memory */
#endif    /* INCLUDE_EEX32 */


/*
 * ATA_TYPE <ataTypes[][]> ATA_GEO_FORCE parameters 
 *
 * ATA_TYPE is defined in h/drv/hdisk/ataDrv.h.  The <ataTypes[][]> table
 * is declared in sysLib.c.
 */

/* controller zero device zero */

#define ATA_CTRL0_DRV0_CYL  (761)   /* ATA 0, device 0 cylinders */
#define ATA_CTRL0_DRV0_HDS  (8)     /* ATA 0, device 0 heads */
#define ATA_CTRL0_DRV0_SPT  (39)    /* ATA 0, device 0 sectors per track */
#define ATA_CTRL0_DRV0_BPS  (512)   /* ATA 0, device 0 bytes per sector */
#define ATA_CTRL0_DRV0_WPC  (0xff)  /* ATA 0, device 0 write pre-compensation */

/* controller zero device one */

#define ATA_CTRL0_DRV1_CYL  (761)   /* ATA 0, device 1 cylinders */
#define ATA_CTRL0_DRV1_HDS  (8)     /* ATA 0, device 1 heads */
#define ATA_CTRL0_DRV1_SPT  (39)    /* ATA 0, device 1 sectors per track */
#define ATA_CTRL0_DRV1_BPS  (512)   /* ATA 0, device 1 bytes per sector */
#define ATA_CTRL0_DRV1_WPC  (0xff)  /* ATA 0, device 1 write pre-compensation */

/* controller one device zero */

#define ATA_CTRL1_DRV0_CYL  (761)   /* ATA 1, device 0 cylinders */
#define ATA_CTRL1_DRV0_HDS  (8)     /* ATA 1, device 0 heads */
#define ATA_CTRL1_DRV0_SPT  (39)    /* ATA 1, device 0 sectors per track */
#define ATA_CTRL1_DRV0_BPS  (512)   /* ATA 1, device 0 bytes per sector */
#define ATA_CTRL1_DRV0_WPC  (0xff)  /* ATA 1, device 0 write pre-compensation */

/* controller one device one */

#define ATA_CTRL1_DRV1_CYL  (761)   /* ATA 1, device 1 cylinders */
#define ATA_CTRL1_DRV1_HDS  (8)     /* ATA 1, device 1 heads */
#define ATA_CTRL1_DRV1_SPT  (39)    /* ATA 1, device 1 sectors per track */
#define ATA_CTRL1_DRV1_BPS  (512)   /* ATA 1, device 1 bytes per sector */
#define ATA_CTRL1_DRV1_WPC  (0xff)  /* ATA 1, device 1 write pre-compensation */

/*
 * ATA_RESOURCE <ataResources[]> parameters 
 *
 * ATA_RESOURCES is defined in h/drv/pcmcia/pccardLib.h.  The <ataResources[]>
 * table is declared in sysLib.c.  Defaults are based on the pcPentium BSP.
 */

/* ATA controller zero ataResources[] parameters */

#define ATA0_VCC          (5)          /* ATA 0 Vcc (3 or 5 volts) */
#define ATA0_VPP          (0)          /* ATA 0 Vpp (5 or 12 volts or 0) */
#define ATA0_IO_START0    (0x1f0)      /* Start I/O Address 0 for ATA 0 */
#define ATA0_IO_START1    (0x3f6)      /* Start I/O Address 1 for ATA 0 */
#define ATA0_IO_STOP0     (0x1f7)      /* Stop I/O Address for ATA 0 */
#define ATA0_IO_STOP1     (0x3f7)      /* Stop I/O Address for ATA 0 */
#define ATA0_EXTRA_WAITS  (0)          /* ATA 0 extra wait states (0-2) */
#define ATA0_MEM_START    (0)          /* ATA 0 memory start address */
#define ATA0_MEM_STOP     (0)          /* ATA 0 memory start address */
#define ATA0_MEM_WAITS    (0)          /* ATA 0 memory extra wait states */
#define ATA0_MEM_OFFSET   (0)          /* ATA 0 memory offset */
#define ATA0_MEM_LENGTH   (0)          /* ATA 0 memory offset */
#define ATA0_CTRL_TYPE    (IDE_LOCAL)  /* ATA 0 logical type */
#define ATA0_NUM_DRIVES   (1)          /* ATA 0 number drives present */
#define ATA0_INT_LVL      (0x0e)       /* ATA 0 interrupt level */

#define ATA0_CONFIG       (ATA_GEO_CURRENT | ATA_PIO_AUTO | \
                           ATA_BITS_16     | ATA_PIO_MULTI)

#define ATA0_SEM_TIMEOUT  (5)          /* ATA 0 sync. semaphore timeout */
#define ATA0_WDG_TIMEOUT  (5)          /* ATA 0 watchdog timer timeout */
#define ATA0_SOCKET_TWIN  (0)          /* Socket number (TWIN PCMCIA Card) */
#define ATA0_POWER_DOWN   (0)          /* ATA power down mode */

/* ATA controller one ataResources[] parameters */

#define ATA1_VCC          (5)          /* ATA 1 Vcc (3 or 5 volts) */
#define ATA1_VPP          (0)          /* ATA 1 Vpp (5 or 12 volts or 0) */
#define ATA1_IO_START0    (0x170)      /* Start I/O Address 0 for ATA 1 */
#define ATA1_IO_START1    (0x376)      /* Start I/O Address 1 for ATA 1 */
#define ATA1_IO_STOP0     (0x177)      /* Stop I/O Address 0 for ATA 1 */
#define ATA1_IO_STOP1     (0x377)      /* Stop I/O Address 1 for ATA 1 */
#define ATA1_EXTRA_WAITS  (0)          /* ATA 1 extra wait states (0-2) */
#define ATA1_MEM_START    (0)          /* ATA 1 memory start address */
#define ATA1_MEM_STOP     (0)          /* ATA 1 memory start address */
#define ATA1_MEM_WAITS    (0)          /* ATA 1 memory extra wait states */
#define ATA1_MEM_OFFSET   (0)          /* ATA 1 memory offset */
#define ATA1_MEM_LENGTH   (0)          /* ATA 1 memory offset */
#define ATA1_CTRL_TYPE    (ATA_PCMCIA)  /* ATA 1 logical type */
#define ATA1_NUM_DRIVES   (1)          /* ATA 1 number drives present */
#define ATA1_INT_LVL      (0x09)       /* ATA 1 interrupt level */

#define ATA1_CONFIG       (ATA_GEO_CURRENT | ATA_PIO_AUTO | \
                           ATA_BITS_16     | ATA_PIO_MULTI)

#define ATA1_SEM_TIMEOUT  (5)          /* ATA 1 sync. semaphore timeout */
#define ATA1_WDG_TIMEOUT  (5)          /* ATA 1 watchdog timer timeout */
#define ATA1_SOCKET_TWIN  (0)          /* Socket number (TWIN PCMCIA Card) */
#define ATA1_POWER_DOWN   (0)          /* ATA 1 power down mode */


/* console definitions  */

#undef    NUM_TTY
#define NUM_TTY       (N_UART_CHANNELS)  /* number of tty channels */

#define INCLUDE_PC_CONSOLE                /* PC keyboard and VGA console */

#ifdef INCLUDE_PC_CONSOLE
#   define PC_CONSOLE           (0)      /* console number */
#   define N_VIRTUAL_CONSOLES   (2)      /* shell / application */
#endif /* INCLUDE_PC_CONSOLE */

/* PS/2 101-key default keyboard type (use PC_XT_83_KBD for 83-key) */

#define PC_KBD_TYPE   (PC_PS2_101_KBD)


/* memory addresses, offsets, and size constants */

#if (SYS_WARM_TYPE == SYS_WARM_BIOS)            /* non-volatile RAM size */
#   define NV_RAM_SIZE          (NONE)
#else
#   define NV_RAM_SIZE          (0x1000)
#endif

#define USER_RESERVED_MEM       (0)             /* user reserved memory */
#define LOCAL_MEM_LOCAL_ADRS    (0x00100000)    /* on-board memory base */

/*
 * LOCAL_MEM_SIZE is the offset from the start of on-board memory to the
 * top of memory.  If the page size is 2MB or 4MB, write-protected pages
 * for the MMU directory tables and <globalPageBlock> array are also a
 * multiple of 2MB or 4MB.  Thus, LOCAL_MEM_SIZE should be big enough to
 * hold them.
 */

#if  (VM_PAGE_SIZE == PAGE_SIZE_4KB)            /* 4KB page */
#   define SYSTEM_RAM_SIZE      (0x00800000)    /* minimum 8MB system RAM */
#else   /* PAGE_SIZE_[2/4]MB */                 /* [2/4]MB page */
#   define SYSTEM_RAM_SIZE      (0x02000000)    /* minimum 32MB system RAM */
#endif  /* (VM_PAGE_SIZE == PAGE_SIZE_4KB) */

#define LOCAL_MEM_SIZE          (SYSTEM_RAM_SIZE - LOCAL_MEM_LOCAL_ADRS)


/*
 * Memory auto-sizing is supported when this option is defined.
 * See sysyPhysMemTop() in the BSP sysLib.c file.
 */

#ifdef    INCLUDE_MMU_P6_36BIT
#   undef  LOCAL_MEM_AUTOSIZE
#else
#   define LOCAL_MEM_AUTOSIZE
#endif    /* INCLUDE_MMU_P6_36BIT */

/*
 * The following parameters are defined here and in the BSP Makefile.
 * They must be kept synchronized.  Any changes made here must be made
 * in the Makefile and vice versa.
 */

#ifdef    BOOTCODE_IN_RAM
#   undef  ROMSTART_BOOT_CLEAR
#   define ROM_BASE_ADRS        (0x00008000)    /* base address of ROM */
#   define ROM_TEXT_ADRS        (ROM_BASE_ADRS) /* booting from A: or C: */
#   define ROM_SIZE             (0x00090000)    /* size of ROM */
#else
#   define ROM_BASE_ADRS        (0xfff20000)    /* base address of ROM */
#   define ROM_TEXT_ADRS        (ROM_BASE_ADRS) /* booting from EPROM */
#   define ROM_SIZE             (0x0007fe00)    /* size of ROM */
#endif

#define RAM_LOW_ADRS            (0x00308000)    /* VxWorks image entry point */
#define RAM_HIGH_ADRS           (0x00108000)    /* Boot image entry point */

/*
 * The INCLUDE_ADD_BOOTMEM configuration option enables runtime code which
 * will add a specified amount of upper memory (memory above physical address
 * 0x100000) to the memory pool of an image in lower memory.  This option
 * cannot be used on systems with less than 4MB of memory.
 *
 * The default value for ADDED_BOOTMEM_SIZE, the amount of memory to add
 * to a lower memory image's memory pool, is 2MB.  This value may be increased,
 * but one must ensure that the pool does not overlap with the downloaded
 * vxWorks image.  If there is an overlap, then loading the vxWorks runtime
 * image will corrupt the added memory pool.   The calculation for determining
 * the ADDED_BOOTMEM_SIZE value is:
 *
 *     (RAM_LOW_ADRS + vxWorks image size) < (memTopPhys - ADDED_BOOTMEM_SIZE)
 *
 * Where <memTopPhys> is calculated in the BSP sysLib.c file.  This
 * configuration option corrects SPR 21338.
 */

#define INCLUDE_ADD_BOOTMEM     /* Add upper memory to low memory bootrom */
#define ADDED_BOOTMEM_SIZE      (0x00200000)     /* 2MB additional memory */

/* power management definitions */

#define    VX_POWER_MANAGEMENT    /* define to enable */
#define VX_POWER_MODE_DEFAULT   VX_POWER_MODE_AUTOHALT  /* set mode */

/* AMP (asymmetric multi processor) definitions */

#ifdef    TGT_CPU
#   include "configAmp.h"
#endif    /* TGT_CPU */

/* interrupt mode/number definitions */

#include "configInum.h"

#ifdef    INCLUDE_IACSFL
#   include "iacsfl.h"
#endif    /* INCLUDE_IACSFL - iacsfl.h overrides some macros in config.h */

/*
 * defining _WRS_BSP_DEBUG_NULL_ACCESS will disable access to lower 
 * page in MMU, see sysPhysMemDesc [] and sysPhysMemTop() in sysLib.c 
 * for more details.  This causes the CPU to generate an exception for 
 * any NULL pointer access, or any access to lower page of memory. 
 * VxWorks will suspend the task which made the access.
 * Note that the MMU must be enabled for this to work.
 */

#define _WRS_BSP_VM_PAGE_OFFSET    (VM_PAGE_SIZE)

#undef _WRS_BSP_DEBUG_NULL_ACCESS

#ifdef _WRS_BSP_DEBUG_NULL_ACCESS  /* protect NULL access with MMU */

#   if (VM_PAGE_SIZE != PAGE_SIZE_4KB) /* works when page size is 4KB */
#       error PAGE_SIZE_4KB required to use _WRS_BSP_DEBUG_NULL_ACCESS
#   endif /* (VM_PAGE_SIZE == PAGE_SIZE_4KB) */

#   if (LOCAL_MEM_LOCAL_ADRS == 0x0)
#       undef VEC_BASE_ADRS
#       define VEC_BASE_ADRS    ((char *) (_WRS_BSP_VM_PAGE_OFFSET * 2))

#       undef GDT_BASE_OFFSET
#       define GDT_BASE_OFFSET    (0x0800 + (_WRS_BSP_VM_PAGE_OFFSET * 2))

#       undef SM_ANCHOR_OFFSET
#       define SM_ANCHOR_OFFSET    (0x1100 + (_WRS_BSP_VM_PAGE_OFFSET * 2))

#       undef EXC_MSG_OFFSET
#       define EXC_MSG_OFFSET    (0x1300 + (_WRS_BSP_VM_PAGE_OFFSET * 2))

#       undef FD_DMA_BUF_ADDR
#       define FD_DMA_BUF_ADDR    (0x2000 + (_WRS_BSP_VM_PAGE_OFFSET * 2))

#       undef FD_DMA_BUF_SIZE
#       define FD_DMA_BUF_SIZE    (0x1000)

#       undef BOOT_LINE_ADRS
#       define BOOT_LINE_ADRS    ((char *) (0x1200))
#   endif /* (LOCAL_MEM_LOCAL_ADRS == 0x0) */

#endif /* _WRS_BSP_DEBUG_NULL_ACCESS */

#ifdef __cplusplus
}
#endif

#endif    /* INCconfigh */

#if defined(PRJ_BUILD)
#   include "prjParams.h"
#endif