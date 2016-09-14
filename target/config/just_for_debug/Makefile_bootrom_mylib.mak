####采用自行编译的库文件生成bootrom

VXWORKS_BASE_DIR = /mnt/vxSimFull/target
CC_INCLUDE = -I/h   -I. -I$(VXWORKS_BASE_DIR)/config/all -I$(VXWORKS_BASE_DIR)/h  -I$(VXWORKS_BASE_DIR)/src/config -I$(VXWORKS_BASE_DIR)/src/drv

CC_DEFINES	= -DCPU=PENTIUM \
		  -DTOOL_FAMILY=gnu \
		  -DTOOL=gnu 

CFLAGS		= -march=pentium -ansi -g -gdwarf-2 -gstrict-dwarf -O2 -nostdlib -fno-builtin -fno-defer-pop -Wall $(CC_INCLUDE) $(CC_DEFINES)

# -fvolatile is not available in gcc v4		
CFLAGS_AS	= -march=pentium -ansi -O -g $(CC_INCLUDE) \
		  $(CC_DEFINES)  -P \
		  -xassembler-with-cpp
		  
RAM_HIGH_ADRS      = 00108000	# Boot image entry point
RAM_LOW_ADRS       = 00308000
ROM_SIZE           = 00090000	# number of bytes of ROM space

CASFLAGS	= -march=pentium -E -g -gdwarf-2 -gstrict-dwarf -xassembler-with-cpp $(CC_INCLUDE) \
	          $(CC_DEFINES)

CFLAGS_PIC	= -march=pentium -ansi -g -gdwarf-2 -gstrict-dwarf -O -Wall \
		  $(CC_INCLUDE) $(CC_DEFINES)

LIBS		=  -lcplus  -lgnucplus  -lvxcom  -lvxdcom  -larch  -lcommoncc  -ldrv  -lgcc  -lnet  -los  -lrpc  -ltffs  -lusb  -lvxfusion  -lvxmp  -lvxvmi  -lwdb  -lwind  -lwindview $(VXWORKS_BASE_DIR)/lib/libPENTIUMgnuvx.a
	
#LD_LINK_PATH	= -L$(VXWORKS_BASE_DIR)/lib/pentium/PENTIUM/gnu -L$(VXWORKS_BASE_DIR)/lib/pentium/PENTIUM/common

LD_LINK_PATH	= -L$(VXWORKS_BASE_DIR)/src/os -L$(VXWORKS_BASE_DIR)/src/wind -L$(VXWORKS_BASE_DIR)/src/util -L$(VXWORKS_BASE_DIR)/src/arch/i86 -L$(VXWORKS_BASE_DIR)/src/libc/string -L$(VXWORKS_BASE_DIR)/src/libc/ctype -L$(VXWORKS_BASE_DIR)/src/fs -L$(VXWORKS_BASE_DIR)/src/drv -L$(VXWORKS_BASE_DIR)/src/usr -L$(VXWORKS_BASE_DIR)/src/libc/time -L$(VXWORKS_BASE_DIR)/src/libc/stdlib -L$(VXWORKS_BASE_DIR)/src/netwrs -L$(VXWORKS_BASE_DIR)/src/netinet -L$(VXWORKS_BASE_DIR)/src/bpf -L$(VXWORKS_BASE_DIR)/src/libc/assert -L$(VXWORKS_BASE_DIR)/src/drv/netif -L$(VXWORKS_BASE_DIR)/src/netinet/ppp -L$(VXWORKS_BASE_DIR)/src/libc/stdio -L$(VXWORKS_BASE_DIR)/src/libc/setjmp -L$(VXWORKS_BASE_DIR)/src/ostool
#vpath定义了.a库文件的搜索路径，如果没有定义则会去生成.a库文件
vpath %.a $(subst -L,,$(LD_LINK_PATH))
	
.s.o :
	$(RM) $@
	cc $(CFLAGS_AS) -c -o $@ $<
	
.c.o :
	$(RM) $@
	cc $(CFLAGS) -c $<

bootConfig.o : depend.VirtualBoxBSP $(VXWORKS_BASE_DIR)/config/all/bootConfig.c
	$(RM) $@
	cc -c $(CFLAGS) $(VXWORKS_BASE_DIR)/config/all/bootConfig.c
	
usrConfig.o : depend.VirtualBoxBSP $(VXWORKS_BASE_DIR)/config/all/usrConfig.c
	$(RM) $@
	ccpentium -c $(CFLAGS) $(VXWORKS_BASE_DIR)/config/all/usrConfig.c -o $@

dataSegPad.o: depend.VirtualBoxBSP $(VXWORKS_BASE_DIR)/config/all/dataSegPad.c $(VXWORKS_BASE_DIR)/config/all/configAll.h config.h
	$(RM) $@
	ccpentium -c $(CFLAGS) $(VXWORKS_BASE_DIR)/config/all/dataSegPad.c -o $@

bootrom.Z.s : depend.VirtualBoxBSP bootConfig.o sysALib.o sysLib.o  \
			$(patsubst -l%,lib%.a,$(LIBS))
	vxrm $@ 
	vxrm tmp.o 
	vxrm tmp.out
	vxrm tmp.Z
	vxrm version.o
	ccpentium -c $(CFLAGS) -o version.o \
	    $(VXWORKS_BASE_DIR)/config/all/version.c
	ldpentium -o tmp.o -X -N -e _usrInit \
	    -Ttext $(RAM_HIGH_ADRS) bootConfig.o version.o sysALib.o sysLib.o \
	    --start-group $(LD_LINK_PATH) $(LIBS) --end-group \
	    -T $(VXWORKS_BASE_DIR)/h/tool/gnu/ldscripts/link.RAM
	@rem tmp.o
	objcopy -O binary --binary-without-bss tmp.o tmp.out
	d:\Tornado2.2\host\x86-win32\bin\deflate < tmp.out > tmp.Z
	d:\Tornado2.2\host\x86-win32\bin\binToAsm tmp.Z >bootrom.Z.s
	vxrm tmp.o 
	vxrm tmp.out
	vxrm tmp.Z 
		
depend.VirtualBoxBSP:
	ccpentium -M -MG -w $(CFLAGS) mkboot.c pciCfgIntStub.c pciCfgStub.c sysDec21x40End.c sysEl3c90xEnd.c sysElt3c509End.c sysFei82557End.c sysGei82543End.c sysLib.c sysLn97xEnd.c sysNe2000End.c sysNet.c sysNetif.c sysNvRam.c sysScsi.c sysSerial.c sysTffs.c sysUltraEnd.c sysWindML.c usbPciStub.c $(VXWORKS_BASE_DIR)/config/all/bootConfig.c $(VXWORKS_BASE_DIR)/config/all/bootInit.c $(VXWORKS_BASE_DIR)/config/all/dataSegPad.c $(VXWORKS_BASE_DIR)/config/all/usrConfig.c $(VXWORKS_BASE_DIR)/config/all/version.c >>$@
	ccpentium -E -P -M -w $(CASFLAGS) romInit.s >> $@
	ccpentium -E -P -M -w $(CASFLAGS) sysALib.s >> $@
#下面这句的作用是将	depend.VirtualBoxBSP中绝对路径替换成$(WIND_BASE)
#	wtxtcl d:\Tornado2.2\host/src/hutils/bspDepend.tcl $@

##################################################################make rominit###########################################################
depend.rominit:
	$(CC) -E -P -M -w $(CASFLAGS) romInit.s >> $@
romInit.bin: romInit.o
	$(RM) $@
	$(RM) $@.dasm
	ld -Ttext 0x0 -e romInit --oformat binary -o $@ romInit.o
	objdump -D -b binary -m i386 $@ > $@.dasm
##########################################################################################################################################

bootInit.o : depend.VirtualBoxBSP $(VXWORKS_BASE_DIR)/config/all/bootInit.c
	$(RM) $@
	cc -c $(CFLAGS_PIC) $(VXWORKS_BASE_DIR)/config/all/bootInit.c

bootInit_uncmp.o : depend.VirtualBoxBSP $(VXWORKS_BASE_DIR)/config/all/bootInit.c bootInit.o
	$(RM) $@
	cp $(VXWORKS_BASE_DIR)/config/all/bootInit.c bootInit_uncmp.c
	$(CC) -c $(CFLAGS_PIC) -DUNCOMPRESS bootInit_uncmp.c
	$(RM) bootInit_uncmp.c

bootInit.i : depend.VirtualBoxBSP $(VXWORKS_BASE_DIR)/config/all/bootInit.c
	$(RM) $@
	cc -E $(CFLAGS_PIC) $(VXWORKS_BASE_DIR)/config/all/bootInit.c >> $@

#vxrm == rm - @==hidden $@==current object
bootrom_uncmp.elf : depend.VirtualBoxBSP bootInit_uncmp.o romInit.o \
			bootConfig.o sysALib.o sysLib.o 
	$(RM) $@ 
	$(RM) *.dump
	$(RM) $@.nm
	$(RM) version.o
	$(CC) -c $(CFLAGS) -o version.o $(VXWORKS_BASE_DIR)/config/all/version.c
	$(LD) -X -N -e romInit \
	    -Ttext 0x8000 -o $@ romInit.o bootInit_uncmp.o version.o \
	    bootConfig.o sysALib.o sysLib.o --start-group $(LD_LINK_PATH) -los  -lwind -lutil -larch  -lstring -lctype -lfs -ldrv -lusr -ltime -lstdlib -lnetwrs -lnetinet  -lbpf -lassert -lnetif  -lppp -lstdio -lsetjmp -lostool $(VXWORKS_BASE_DIR)/lib/pentium/PENTIUM/common/libgcc.a --end-group\
	    -T $(VXWORKS_BASE_DIR)/h/tool/gnu/ldscripts/link.RAM
	nm $@ > $@.nm 
	objdump -D -j .text $@ > $@.dump

bootrom_uncmp.iso :  bootrom_uncmp.elf
	$(RM) $@
	grub-mkrescue -o $@ bootrom_uncmp.elf
################################## vxWorks #####################################
#
# vxWorks     - normal vxWorks system
# vxWorks.sym - symbol table of vxWorks

vxWorks vxWorks.sym : depend.VirtualBoxBSP usrConfig.o dataSegPad.o \
		sysALib.o sysLib.o   $(patsubst -l%,lib%.a,$(LIBS)) 
	vxrm vxWorks vxWorks.sym
	vxrm version.o
	vxrm vxWorks.tmp ctdt.c ctdt.o
	ccpentium -c $(CFLAGS) -o version.o $(VXWORKS_BASE_DIR)/config/all/version.c
	ccpentium -r -nostdlib -Wl,-X \
	    -o vxWorks.tmp sysALib.o sysLib.o  usrConfig.o version.o \
	    -Wl,--start-group $(LD_LINK_PATH) $(LIBS) \
	    -Wl,--end-group 
	nmpentium vxWorks.tmp | wtxtcl d:\Tornado2.2/host/src/hutils/munch.tcl -c pentium > ctdt.c
	make CC_COMPILER="-fdollars-in-identifiers" ctdt.o
	ldpentium -X -N -e _sysInit -Ttext $(RAM_LOW_ADRS) \
	    -o vxWorks dataSegPad.o vxWorks.tmp ctdt.o -T $(VXWORKS_BASE_DIR)/h/tool/gnu/ldscripts/link.RAM
	vxrm vxWorks.tmp
	@rem vxWorks
	@rem vxWorks
	objcopy --extract-symbol vxWorks vxWorks.sym
	@rem vxWorks.sym
	d:\Tornado2.2\host\x86-win32\bin\vxsize pentium -v $(RAM_HIGH_ADRS) $(RAM_LOW_ADRS) vxWorks


test :
	rem $@
