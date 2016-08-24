# Makefile - templateX86 makefile
#
# Copyright 1984-2002 Wind River Systems, Inc.
#
# TODO - Remove the template modification history and begin a new history
#        starting with version 01a and growing the history upward with
#	 each revision.
#
# modification history
# --------------------
# 01g,30may02,dat  Use driver archive, not .obj module, 77135
# 01f,30mar02,dat  Remove old def for RELEASE
# 01e,08dec01,dat  Adding templateEnd.obj to MACH_EXTRA, CPU changed to PENTIUM
# 01d,30oct01,tpw  Standardize kernel links and bootrom conversions.
# 01c,11oct01,dat  Update for T2.2
# 01b,08apr98,ms   removed vxWorks.st, mkboot.o, and vxsys.o from RELEASE.
#		   removed unneeded objcopy modhists.
# 01a,27jan97,dat  written (from pc386/Makefile, 01o)
#
# DESCRIPTION
# This file contains rules for building VxWorks for the
# PC 386.
#*/

CPU		= PENTIUM
TOOL		= gnu

TGT_DIR = $(WIND_BASE)/target
include $(TGT_DIR)/h/make/defs.bsp

# TODO - default files are sysLib.c, sysTffs.c and tycoDrv.c. Specify here the
# files to be documented bye make man. If the default files are OK, remove the
# DOC_FILES variable below

DOC_FILES	=

## Only redefine make definitions below this point, or your definitions will
## be overwritten by the makefile stubs above.


TARGET_DIR	= templateX86
VENDOR		= Vendor
BOARD		= Board

#
# The constants ROM_TEXT_ADRS, ROM_SIZE, and RAM_HIGH_ADRS are defined
# in config.h, and Makefile.
# All definitions for these constants must be identical (equivalent).
#

# for BOOTCODE_IN_RAM
ROM_TEXT_ADRS	= 00008000	# ROM entry address - A: or C:
ROM_SIZE	= 00090000	# number of bytes of ROM space

# for not BOOTCODE_IN_RAM
# ROM_TEXT_ADRS	= fff20000	# ROM entry address - EPROM
# ROM_SIZE	= 0007fe00	# number of bytes of ROM space

RAM_LOW_ADRS	= 00108000	# VxWorks image entry point
RAM_HIGH_ADRS	= 00008000	# Boot image entry point

# TODO - Remove the following line, it is for internal WRS use only.
RELEASE_PRE	= mk.templateDrvrs.a

# TODO - The following is only for this template BSP, it will usually be
# removed.
# For template BSPs, the templateEnd driver module is in a BSP specific
# archive, not the normal archive.
LIB_EXTRA	= templateDrvrs.a

## Only redefine make definitions above this point, or the expansion of
## makefile target dependencies may be incorrect.

include $(TGT_DIR)/h/make/rules.bsp


# Following bootable vxWorks should be placed in bootable diskette
# by "mkboot" or "mkbootFd, mkbootAta" utility.
# vxWorks_low should be downloaded by bootrom_high boot image.
#   vxWorks_rom		- bootable vxWorks: 		   upper mem
#   vxWorks_rom_low	- bootable vxWorks:		   lower mem
#   vxWorks.st_rom	- bootable vxWorks.st(compressed): upper mem
#   bootrom		- bootrom(compressed):		   lower mem
#   bootrom_uncmp	- bootrom:			   lower mem
#   bootrom_high	- bootrom(compressed):		   upper mem

vxWorks_low vxWorks_low.sym :
	- @ $(RM) vxWorks_low vxWorks_low.sym
	$(MAKE) vxWorks LD_LOW_FLAGS="$(LD_HIGH_FLAGS)"
	$(CP) vxWorks vxWorks_low
	$(CP) vxWorks.sym vxWorks_low.sym
	- @ $(RM) vxWorks vxWorks.sym

vxWorks_rom_low :
	- @ $(RM) $@
	$(MAKE) vxWorks_rom LD_LOW_FLAGS="$(LD_HIGH_FLAGS)"
	$(CP) vxWorks_rom $@
	- @ $(RM) vxWorks_rom

bootrom_high :
	$(CC) -c $(CFLAGS) -DRAM_DST_ADRS=0x$(RAM_LOW_ADRS) $(BOOTINIT)
	- @ $(RM) bootrom.Z.s $@
	$(MAKE) LD_HIGH_FLAGS="-Ttext $(RAM_LOW_ADRS)" bootrom.Z.s
	$(MAKE) bootrom LD_LOW_FLAGS="$(LD_HIGH_FLAGS)"
	$(CP) bootrom $@
	- @ $(RM) bootrom.Z.* bootrom bootInit.o

# The boot image is either bootrom_uncmp or vxWorks.st_rom.
# If the EPROM is 27020 (256 Kbytes)
#	$(BINHEX) -a 3fe00 -b romcard >> $@
# If the EPROM is 27040 (512 Kbytes)
#	$(BINHEX) -a 7fe00 -b romcard >> $@

SED	= sed

romcard_bootrom_512.hex:	depend.$(BSP_NAME) romcard.o bootrom_uncmp
	- @ $(RM) $@
	$(LD) -o romcard -X -N -e _romcard romcard.o
	$(ROMSIZEPROG) -b 7fe00 bootrom_uncmp
	$(BINHEX) -a 0 bootrom_uncmp | $(SED) -e "/S9/d" > $@
	$(BINHEX) -a 7fe00 -b romcard >> $@
	- @ $(RM) romcard.o romcard

romcard_bootrom_256.hex:	depend.$(BSP_NAME) romcard.o bootrom_uncmp
	- @ $(RM) $@
	$(LD) -o romcard -X -N -e _romcard romcard.o
	$(ROMSIZEPROG) -b 3fe00 bootrom_uncmp
	$(BINHEX) -a 0 bootrom_uncmp | $(SED) -e "/S9/d" > $@
	$(BINHEX) -a 3fe00 -b romcard >> $@
	- @ $(RM) romcard.o romcard

romcard_vxWorks_st_512.hex:	depend.$(BSP_NAME) romcard.o vxWorks.st_rom
	- @ $(RM) $@
	$(LD) -o romcard -X -N -e _romcard romcard.o
	$(ROMSIZEPROG) -b 7fe00 vxWorks.st_rom
	$(BINHEX) -a 0 vxWorks.st_rom | $(SED) -e "/S9/d" > $@
	$(BINHEX) -a 7fe00 -b romcard >> $@
	- @ $(RM) romcard.o romcard

# TODO - Remove this special make target.
# Rather than put the template driver in the real archive, we put it here in
# the template BSP.  This is not the usual way we link to a network driver.

mk.templateDrvrs.a: $(TGT_DIR)/src/drv/end/templateEnd.c
	$(CC) $(CFLAGS) -o templateEnd.o $(OPTION_OBJECT_ONLY) \
		$(TGT_DIR)/src/drv/end/templateEnd.c
	$(AR) crus templateDrvrs.a templateEnd.o
	$(RM) templateEnd.o
