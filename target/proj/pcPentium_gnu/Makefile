# Makefile generated by the project manager
#


# GENERATED: Tue Oct 01 01:38:44 PDT 2002
# DO NOT EDIT - file is regenerated whenever the project changes


## core information

ifeq ($(wildcard Makefile),)
PRJ_DIR        = ..
else
PRJ_DIR        = .
endif
PRJ_FILE       = pcPentium_gnu.wpj
PRJ_TYPE       = vxWorks
PRJ_OBJS       = sysALib.o sysLib.o usrAppInit.o prjConfig.o linkSyms.o
BOOT_OBJS      = romInit.o romStart.o $(BOOT_EXTRA)
BUILD_SPEC     = default
BSP_DIR        = $(WIND_BASE)/target/config/pcPentium
TGT_DIR        = $(WIND_BASE)/target



## set searching directories for dependencies

vpath %.c $(BSP_DIR)
vpath %.cpp $(BSP_DIR)
vpath %.cxx $(BSP_DIR)


## build-configuration info

ifeq ($(BUILD_SPEC),default)
CPU            = PENTIUM
TOOL           = gnu
TOOL_FAMILY    = gnu
DEFAULT_RULE   = vxWorks
endif

ifeq ($(BUILD_SPEC),default_rom)
CPU            = PENTIUM
TOOL           = gnu
TOOL_FAMILY    = gnu
DEFAULT_RULE   = vxWorks_rom
endif

ifeq ($(BUILD_SPEC),default_romCompress)
CPU            = PENTIUM
TOOL           = gnu
TOOL_FAMILY    = gnu
DEFAULT_RULE   = vxWorks_romCompress
endif

ifeq ($(BUILD_SPEC),default_romResident)
CPU            = PENTIUM
TOOL           = gnu
TOOL_FAMILY    = gnu
DEFAULT_RULE   = vxWorks_romResident
endif


## component-configuration info

COMPONENTS = INCLUDE_ANSI_ASSERT \
	INCLUDE_ANSI_CTYPE \
	INCLUDE_ANSI_LOCALE \
	INCLUDE_ANSI_MATH \
	INCLUDE_ANSI_STDIO \
	INCLUDE_ANSI_STDIO_EXTRA \
	INCLUDE_ANSI_STDLIB \
	INCLUDE_ANSI_STRING \
	INCLUDE_ANSI_TIME \
	INCLUDE_ARP_API \
	INCLUDE_BOOT_LINE_INIT \
	INCLUDE_BSD_SOCKET \
	INCLUDE_BUF_MGR \
	INCLUDE_CACHE_ENABLE \
	INCLUDE_CACHE_SUPPORT \
	INCLUDE_CBIO \
	INCLUDE_CTORS_DTORS \
	INCLUDE_DHCPC_LEASE_CLEAN \
	INCLUDE_DISK_CACHE \
	INCLUDE_DLL \
	INCLUDE_DOSFS \
	INCLUDE_DOSFS_CHKDSK \
	INCLUDE_DOSFS_DIR_FIXED \
	INCLUDE_DOSFS_DIR_VFAT \
	INCLUDE_DOSFS_FAT \
	INCLUDE_DOSFS_FMT \
	INCLUDE_DOSFS_MAIN \
	INCLUDE_END \
	INCLUDE_END_BOOT \
	INCLUDE_ENV_VARS \
	INCLUDE_EXC_HANDLING \
	INCLUDE_EXC_SHOW \
	INCLUDE_EXC_TASK \
	INCLUDE_FD \
	INCLUDE_FLOATING_POINT \
	INCLUDE_FORMATTED_IO \
	INCLUDE_FTP \
	INCLUDE_GNU_INTRINSICS \
	INCLUDE_HASH \
	INCLUDE_HOST_TBL \
	INCLUDE_HW_FP \
	INCLUDE_ICMP \
	INCLUDE_IGMP \
	INCLUDE_IO_SYSTEM \
	INCLUDE_IP \
	INCLUDE_KERNEL \
	INCLUDE_LOGGING \
	INCLUDE_LOOPBACK \
	INCLUDE_LSTLIB \
	INCLUDE_MEMORY_CONFIG \
	INCLUDE_MEM_MGR_BASIC \
	INCLUDE_MEM_MGR_FULL \
	INCLUDE_MMU_BASIC \
	INCLUDE_MSG_Q \
	INCLUDE_MSG_Q_SHOW \
	INCLUDE_MUX \
	INCLUDE_NETDEV_CONFIG \
	INCLUDE_NETDEV_NAMEGET \
	INCLUDE_NETMASK_GET \
	INCLUDE_NETWORK \
	INCLUDE_NET_HOST_SETUP \
	INCLUDE_NET_INIT \
	INCLUDE_NET_LIB \
	INCLUDE_NET_REM_IO \
	INCLUDE_NET_SETUP \
	INCLUDE_PIPES \
	INCLUDE_POSIX_CLOCKS \
	INCLUDE_RNG_BUF \
	INCLUDE_SELECT \
	INCLUDE_SELECT_SUPPORT \
	INCLUDE_SEM_BINARY \
	INCLUDE_SEM_COUNTING \
	INCLUDE_SEM_MUTEX \
	INCLUDE_SIGNALS \
	INCLUDE_SIO \
	INCLUDE_STDIO \
	INCLUDE_SYM_TBL \
	INCLUDE_SYSCLK_INIT \
	INCLUDE_SYSHW_INIT \
	INCLUDE_SYS_START \
	INCLUDE_TASK_HOOKS \
	INCLUDE_TASK_VARS \
	INCLUDE_TCP \
	INCLUDE_TFTP_CLIENT \
	INCLUDE_TIMEX \
	INCLUDE_TTY_DEV \
	INCLUDE_UDP \
	INCLUDE_USER_APPL \
	INCLUDE_VXEVENTS \
	INCLUDE_WATCHDOGS \
	INCLUDE_WDB \
	INCLUDE_WDB_BANNER \
	INCLUDE_WDB_BP \
	INCLUDE_WDB_COMM_END \
	INCLUDE_WDB_CTXT \
	INCLUDE_WDB_DIRECT_CALL \
	INCLUDE_WDB_EVENTPOINTS \
	INCLUDE_WDB_EVENTS \
	INCLUDE_WDB_EXC_NOTIFY \
	INCLUDE_WDB_EXIT_NOTIFY \
	INCLUDE_WDB_FUNC_CALL \
	INCLUDE_WDB_GOPHER \
	INCLUDE_WDB_HW_FP \
	INCLUDE_WDB_MEM \
	INCLUDE_WDB_REG \
	INCLUDE_WDB_START_NOTIFY \
	INCLUDE_WDB_SYS \
	INCLUDE_WDB_SYS_HW_FP \
	INCLUDE_WDB_TASK \
	INCLUDE_WDB_TASK_BP \
	INCLUDE_WDB_TASK_HW_FP \
	INCLUDE_WDB_USER_EVENT \
	INCLUDE_WDB_VIO \
	INCLUDE_WDB_VIO_LIB
COMPONENT_LIBS = 


include $(TGT_DIR)/h/make/defs.project


## build-configuration info

ifeq ($(BUILD_SPEC),default)
AR               = arpentium
AS               = ccpentium
BOOT_EXTRA       = 
CC               = ccpentium
CC_ARCH_SPEC     = -mcpu=pentium -march=pentium
CFLAGS           = -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -I$(PRJ_DIR) -I$(WIND_BASE)/target/config/pcPentium -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/config/comps/src -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu -DPRJ_BUILD 
CFLAGS_AS        = -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -P -xassembler-with-cpp -I$(PRJ_DIR) -I$(WIND_BASE)/target/config/pcPentium -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/config/comps/src -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu -DPRJ_BUILD 
CFLAGS_AS_PROJECT = -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -I/h -I. -I$(WIND_BASE)/target/config/all -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/src/config -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu -P -xassembler-with-cpp
CFLAGS_PROJECT   = -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -I/h -I. -I$(WIND_BASE)/target/config/all -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/src/config -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu
CPP              = ccpentium -E -P
EXTRA_MODULES    = 
LD               = ldpentium
LDFLAGS          = -X -N
LD_LINK_PATH     = -L$(WIND_BASE)/target/lib/pentium/PENTIUM/gnu -L$(WIND_BASE)/target/lib/pentium/PENTIUM/common
LD_PARTIAL       = ccpentium -r -nostdlib -Wl,-X
LD_PARTIAL_FLAGS = -X -r
LIBS             = $(VX_OS_LIBS)
NM               = nmpentium -g
OPTION_DEFINE_MACRO = -D
OPTION_DEPEND    = -M -w
OPTION_GENERATE_DEPENDENCY_FILE = -MD
OPTION_INCLUDE_DIR = -I
OPTION_LANG_C    = -xc
OPTION_UNDEFINE_MACRO = -U
RAM_HIGH_ADRS    = 00108000	# Boot image entry point
RAM_LOW_ADRS     = 00308000	# VxWorks image entry point
SIZE             = sizepentium
TOOL_FAMILY      = gnu
POST_BUILD_RULE  = 
endif

ifeq ($(BUILD_SPEC),default_rom)
AR               = arpentium
AS               = ccpentium
BOOT_EXTRA       = 
CC               = ccpentium
CC_ARCH_SPEC     = -mcpu=pentium -march=pentium
CFLAGS           = -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -I$(PRJ_DIR) -I$(WIND_BASE)/target/config/pcPentium -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/config/comps/src -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu -DPRJ_BUILD 
CFLAGS_AS        = -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -P -xassembler-with-cpp -I$(PRJ_DIR) -I$(WIND_BASE)/target/config/pcPentium -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/config/comps/src -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu -DPRJ_BUILD 
CFLAGS_AS_PROJECT = -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -I/h -I. -I$(WIND_BASE)/target/config/all -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/src/config -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu -P -xassembler-with-cpp
CFLAGS_PROJECT   = -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -I/h -I. -I$(WIND_BASE)/target/config/all -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/src/config -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu
CPP              = ccpentium -E -P
EXTRA_MODULES    = 
HEX_FLAGS        = 
LD               = ldpentium
LDFLAGS          = -X -N
LD_LINK_PATH     = -L$(WIND_BASE)/target/lib/pentium/PENTIUM/gnu -L$(WIND_BASE)/target/lib/pentium/PENTIUM/common
LD_PARTIAL       = ccpentium -r -nostdlib -Wl,-X
LD_PARTIAL_FLAGS = -X -r
LIBS             = $(VX_OS_LIBS)
NM               = nmpentium -g
OPTION_DEFINE_MACRO = -D
OPTION_DEPEND    = -M -w
OPTION_GENERATE_DEPENDENCY_FILE = -MD
OPTION_INCLUDE_DIR = -I
OPTION_LANG_C    = -xc
OPTION_UNDEFINE_MACRO = -U
RAM_HIGH_ADRS    = 00108000	# Boot image entry point
RAM_LOW_ADRS     = 00308000	# VxWorks image entry point
ROM_SIZE         = 00090000	# number of bytes of ROM space
ROM_TEXT_ADRS    = 00008000	# ROM entry address - A: or C:
SIZE             = sizepentium
TOOL_FAMILY      = gnu
POST_BUILD_RULE  = 
endif

ifeq ($(BUILD_SPEC),default_romCompress)
AR               = arpentium
AS               = ccpentium
BOOT_EXTRA       = 
CC               = ccpentium
CC_ARCH_SPEC     = -mcpu=pentium -march=pentium
CFLAGS           = -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -I$(PRJ_DIR) -I$(WIND_BASE)/target/config/pcPentium -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/config/comps/src -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu -DPRJ_BUILD 
CFLAGS_AS        = -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -P -xassembler-with-cpp -I$(PRJ_DIR) -I$(WIND_BASE)/target/config/pcPentium -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/config/comps/src -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu -DPRJ_BUILD 
CFLAGS_AS_PROJECT = -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -I/h -I. -I$(WIND_BASE)/target/config/all -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/src/config -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu -P -xassembler-with-cpp
CFLAGS_PROJECT   = -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -I/h -I. -I$(WIND_BASE)/target/config/all -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/src/config -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu
CPP              = ccpentium -E -P
EXTRA_MODULES    = 
HEX_FLAGS        = 
LD               = ldpentium
LDFLAGS          = -X -N
LD_LINK_PATH     = -L$(WIND_BASE)/target/lib/pentium/PENTIUM/gnu -L$(WIND_BASE)/target/lib/pentium/PENTIUM/common
LD_PARTIAL       = ccpentium -r -nostdlib -Wl,-X
LD_PARTIAL_FLAGS = -X -r
LIBS             = $(VX_OS_LIBS)
NM               = nmpentium -g
OPTION_DEFINE_MACRO = -D
OPTION_DEPEND    = -M -w
OPTION_GENERATE_DEPENDENCY_FILE = -MD
OPTION_INCLUDE_DIR = -I
OPTION_LANG_C    = -xc
OPTION_UNDEFINE_MACRO = -U
RAM_HIGH_ADRS    = 00108000	# Boot image entry point
RAM_LOW_ADRS     = 00308000	# VxWorks image entry point
ROM_SIZE         = 00090000	# number of bytes of ROM space
ROM_TEXT_ADRS    = 00008000	# ROM entry address - A: or C:
SIZE             = sizepentium
TOOL_FAMILY      = gnu
POST_BUILD_RULE  = 
endif

ifeq ($(BUILD_SPEC),default_romResident)
AR               = arpentium
AS               = ccpentium
BOOT_EXTRA       = 
CC               = ccpentium
CC_ARCH_SPEC     = -mcpu=pentium -march=pentium
CFLAGS           = -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -I$(PRJ_DIR) -I$(WIND_BASE)/target/config/pcPentium -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/config/comps/src -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu -DPRJ_BUILD 
CFLAGS_AS        = -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -P -xassembler-with-cpp -I$(PRJ_DIR) -I$(WIND_BASE)/target/config/pcPentium -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/config/comps/src -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu -DPRJ_BUILD 
CFLAGS_AS_PROJECT = -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -I/h -I. -I$(WIND_BASE)/target/config/all -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/src/config -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu -P -xassembler-with-cpp
CFLAGS_PROJECT   = -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -I/h -I. -I$(WIND_BASE)/target/config/all -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/src/config -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu
CPP              = ccpentium -E -P
EXTRA_MODULES    = 
HEX_FLAGS        = 
LD               = ldpentium
LDFLAGS          = -X -N
LD_LINK_PATH     = -L$(WIND_BASE)/target/lib/pentium/PENTIUM/gnu -L$(WIND_BASE)/target/lib/pentium/PENTIUM/common
LD_PARTIAL       = ccpentium -r -nostdlib -Wl,-X
LD_PARTIAL_FLAGS = -X -r
LIBS             = $(VX_OS_LIBS)
NM               = nmpentium -g
OPTION_DEFINE_MACRO = -D
OPTION_DEPEND    = -M -w
OPTION_GENERATE_DEPENDENCY_FILE = -MD
OPTION_INCLUDE_DIR = -I
OPTION_LANG_C    = -xc
OPTION_UNDEFINE_MACRO = -U
RAM_HIGH_ADRS    = 00108000	# Boot image entry point
RAM_LOW_ADRS     = 00308000	# VxWorks image entry point
ROM_SIZE         = 00090000	# number of bytes of ROM space
ROM_TEXT_ADRS    = 00008000	# ROM entry address - A: or C:
SIZE             = sizepentium
TOOL_FAMILY      = gnu
POST_BUILD_RULE  = 
endif

# override make definitions only below this line


# override make definitions only above this line

include $(TGT_DIR)/h/make/rules.project


## build-configuration info

sysALib.o: $(PRJ_DIR)/prjParams.h

ifeq ($(BUILD_SPEC),default)

usrAppInit.o:
	$(CC) -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -I$(PRJ_DIR) -I$(WIND_BASE)/target/config/pcPentium -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/config/comps/src -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu -DPRJ_BUILD  -c $(PRJ_DIR)/usrAppInit.c
endif

ifeq ($(BUILD_SPEC),default_rom)

usrAppInit.o:
	$(CC) -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -I$(PRJ_DIR) -I$(WIND_BASE)/target/config/pcPentium -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/config/comps/src -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu -DPRJ_BUILD  -c $(PRJ_DIR)/usrAppInit.c
endif

ifeq ($(BUILD_SPEC),default_romCompress)

usrAppInit.o:
	$(CC) -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -I$(PRJ_DIR) -I$(WIND_BASE)/target/config/pcPentium -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/config/comps/src -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu -DPRJ_BUILD  -c $(PRJ_DIR)/usrAppInit.c
endif

ifeq ($(BUILD_SPEC),default_romResident)

usrAppInit.o:
	$(CC) -g -mcpu=pentium -march=pentium -ansi -nostdlib -fno-builtin -fno-defer-pop -I$(PRJ_DIR) -I$(WIND_BASE)/target/config/pcPentium -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/config/comps/src -I$(WIND_BASE)/target/src/drv -DCPU=PENTIUM -DTOOL_FAMILY=gnu -DTOOL=gnu -DPRJ_BUILD  -c $(PRJ_DIR)/usrAppInit.c
endif


## dependencies


sysALib.o: $(WIND_BASE)/target/config/pcPentium/sysALib.s \
	$(PRJ_DIR)/prjComps.h \
	$(PRJ_DIR)/prjParams.h


sysLib.o: $(WIND_BASE)/target/config/pcPentium/sysLib.c \
	$(WIND_BASE)/host/$(WIND_HOST_TYPE)/lib/gcc-lib/i586-wrs-vxworks/2.9-PentiumIII-010221/include/stddef.h \
	$(PRJ_DIR)/prjComps.h \
	$(PRJ_DIR)/prjParams.h \
	$(WIND_BASE)/host/$(WIND_HOST_TYPE)/lib/gcc-lib/i586-wrs-vxworks/2.9-PentiumIII-010221/include/stdarg.h


romInit.o: $(WIND_BASE)/target/config/pcPentium/romInit.s \
	$(PRJ_DIR)/prjComps.h \
	$(PRJ_DIR)/prjParams.h


romStart.o: $(WIND_BASE)/target/config/comps/src/romStart.c \
	$(WIND_BASE)/host/$(WIND_HOST_TYPE)/lib/gcc-lib/i586-wrs-vxworks/2.9-PentiumIII-010221/include/stddef.h \
	$(PRJ_DIR)/prjComps.h \
	$(PRJ_DIR)/prjParams.h \
	$(WIND_BASE)/host/$(WIND_HOST_TYPE)/lib/gcc-lib/i586-wrs-vxworks/2.9-PentiumIII-010221/include/stdarg.h


usrAppInit.o: $(PRJ_DIR)/usrAppInit.c


prjConfig.o: $(PRJ_DIR)/prjConfig.c \
	$(WIND_BASE)/host/$(WIND_HOST_TYPE)/lib/gcc-lib/i586-wrs-vxworks/2.9-PentiumIII-010221/include/stddef.h \
	$(PRJ_DIR)/prjComps.h \
	$(PRJ_DIR)/prjParams.h \
	$(WIND_BASE)/host/$(WIND_HOST_TYPE)/lib/gcc-lib/i586-wrs-vxworks/2.9-PentiumIII-010221/include/stdarg.h


linkSyms.o: $(PRJ_DIR)/linkSyms.c


## user defined rules

romStart.o : 
	$(CC) $(OPTION_OBJECT_ONLY) $(CFLAGS) $(PROJECT_BSP_FLAGS_EXTRA) $(ROM_FLAGS_EXTRA) $< -o $@

romcard_bootrom_512.hex: depend.$(BSP_NAME) romcard.o bootrom_uncmp
	- @ $(RM) $@
	$(LD) -o romcard -X -N -e _romcard romcard.o
	$(ROMSIZEPROG) -b 7fe00 bootrom_uncmp
	$(BINHEX) -a 0 bootrom_uncmp | $(SED) -e "/S9/d" > $@
	$(BINHEX) -a 7fe00 -b romcard >> $@
	- @ $(RM) romcard.o romcard

romInit.o : 
	$(CC) $(OPTION_OBJECT_ONLY) $(CFLAGS_AS) $(PROJECT_BSP_FLAGS_EXTRA) $(ROM_FLAGS_EXTRA) $< -o $@

linkSyms.o : 
	$(CC) $(OPTION_OBJECT_ONLY) $(CFLAGS) $(PROJECT_BSP_FLAGS_EXTRA) $(PRJ_DIR)/linkSyms.c -o $@

romcard_bootrom_256.hex: depend.$(BSP_NAME) romcard.o bootrom_uncmp
	- @ $(RM) $@
	$(LD) -o romcard -X -N -e _romcard romcard.o
	$(ROMSIZEPROG) -b 3fe00 bootrom_uncmp
	$(BINHEX) -a 0 bootrom_uncmp | $(SED) -e "/S9/d" > $@
	$(BINHEX) -a 3fe00 -b romcard >> $@
	- @ $(RM) romcard.o romcard

sysLib.o : 
	$(CC) $(OPTION_OBJECT_ONLY) $(CFLAGS) $(PROJECT_BSP_FLAGS_EXTRA) $(WIND_BASE)/target/config/pcPentium/sysLib.c -o $@

romcard_vxWorks_st_512.hex: depend.$(BSP_NAME) romcard.o vxWorks.st_rom
	- @ $(RM) $@
	$(LD) -o romcard -X -N -e _romcard romcard.o
	$(ROMSIZEPROG) -b 7fe00 vxWorks.st_rom
	$(BINHEX) -a 0 vxWorks.st_rom | $(SED) -e "/S9/d" > $@
	$(BINHEX) -a 7fe00 -b romcard >> $@
	- @ $(RM) romcard.o romcard

sysALib.o : 
	$(AS) $(OPTION_OBJECT_ONLY) $(CFLAGS_AS) $(PROJECT_BSP_FLAGS_EXTRA) $(WIND_BASE)/target/config/pcPentium/sysALib.s -o $@

prjConfig.o : 
	$(CC) $(OPTION_OBJECT_ONLY) $(CFLAGS) $(PROJECT_BSP_FLAGS_EXTRA) $(PRJ_DIR)/prjConfig.c -o $@

