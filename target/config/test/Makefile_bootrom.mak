CC_INCLUDE = -I/h   -I. -Id:\Tornado2.2\target\config\all -Id:\Tornado2.2\target/h  -Id:\Tornado2.2\target/src/config -Id:\Tornado2.2\target/src/drv

CC_DEFINES	= -DCPU=PENTIUM \
		  -DTOOL_FAMILY=gnu \
		  -DTOOL=gnu 

CFLAGS		= -mcpu=pentium -march=pentium -ansi  -O2 -fvolatile -nostdlib -fno-builtin -fno-defer-pop -Wall $(CC_INCLUDE) $(CC_DEFINES)
		
CFLAGS_AS	= -mcpu=pentium -march=pentium -ansi -O -fvolatile $(CC_INCLUDE) \
		  $(CC_DEFINES)  -P \
		  -xassembler-with-cpp
		  
RAM_HIGH_ADRS      = 00108000	# Boot image entry point
RAM_LOW_ADRS       = 00308000
ROM_SIZE           = 00090000	# number of bytes of ROM space

CASFLAGS	= -mcpu=pentium -march=pentium -E -xassembler-with-cpp $(CC_INCLUDE) \
	          $(CC_DEFINES)

CFLAGS_PIC	= -mcpu=pentium -march=pentium -ansi -O -fvolatile -Wall \
		  $(CC_INCLUDE) $(CC_DEFINES)

LIBS		=  -lcplus  -lgnucplus  -lvxcom  -lvxdcom  -larch  -lcommoncc  -ldrv  -lgcc  -lnet  -los  -lrpc  -ltffs  -lusb  -lvxfusion  -lvxmp  -lvxvmi  -lwdb  -lwind  -lwindview d:\Tornado2.2\target/lib/libpentiumgnuvx.a

WIND_HOST_TYPE=x86-win32
WIND_BASE=d:\Tornado2.2
PATH=$(WIND_BASE)\host\$(WIND_HOST_TYPE)\bin
	
LD_LINK_PATH	= -Ld:\Tornado2.2\target/lib/pentium/PENTIUM/gnu -Ld:\Tornado2.2\target/lib/pentium/PENTIUM/common

#vpath定义了.a库文件的搜索路径，如果没有定义则会去生成.a库文件
vpath %.a $(subst -L,,$(LD_LINK_PATH))
	
.s.o :
	vxrm $@
	ccpentium $(CFLAGS_AS) -c -o $@ $< 
	
.c.o :
	vxrm $@
	ccpentium $(CFLAGS) -c $<

bootConfig.o : depend.VirtualBoxBSP d:\Tornado2.2\target\config\all\bootConfig.c
	vxrm $@
	ccpentium -c $(CFLAGS) d:\Tornado2.2\target\config\all\bootConfig.c
	
usrConfig.o : depend.VirtualBoxBSP d:\Tornado2.2\target\config\all\usrConfig.c
	vxrm $@
	ccpentium -c $(CFLAGS) d:\Tornado2.2\target\config\all\usrConfig.c -o $@

dataSegPad.o: depend.VirtualBoxBSP d:\Tornado2.2\target\config\all\dataSegPad.c d:\Tornado2.2\target\config\all\configAll.h config.h
	vxrm $@
	ccpentium -c $(CFLAGS) d:\Tornado2.2\target\config\all\dataSegPad.c -o $@

bootrom.Z.s : depend.VirtualBoxBSP bootConfig.o sysALib.o sysLib.o  \
			$(patsubst -l%,lib%.a,$(LIBS))
	vxrm $@ 
	vxrm tmp.o 
	vxrm tmp.out
	vxrm tmp.Z
	vxrm version.o
	ccpentium -c $(CFLAGS) -o version.o \
	    d:\Tornado2.2\target\config\all/version.c
	ldpentium -o tmp.o -X -N -e _usrInit \
	    -Ttext $(RAM_HIGH_ADRS) bootConfig.o version.o sysALib.o sysLib.o \
	    --start-group $(LD_LINK_PATH) $(LIBS) --end-group \
	    -T d:\Tornado2.2\target\h/tool/gnu/ldscripts/link.RAM
	@rem tmp.o
	d:\Tornado2.2\host\x86-win32\bin\objcopypentium -O binary --binary-without-bss tmp.o tmp.out
	d:\Tornado2.2\host\x86-win32\bin\deflate < tmp.out > tmp.Z
	d:\Tornado2.2\host\x86-win32\bin\binToAsm tmp.Z >bootrom.Z.s
	vxrm tmp.o 
	vxrm tmp.out
	vxrm tmp.Z 
		
depend.VirtualBoxBSP:
	ccpentium -M -MG -w $(CFLAGS) mkboot.c pciCfgIntStub.c pciCfgStub.c sysDec21x40End.c sysEl3c90xEnd.c sysElt3c509End.c sysFei82557End.c sysGei82543End.c sysLib.c sysLn97xEnd.c sysNe2000End.c sysNet.c sysNetif.c sysNvRam.c sysScsi.c sysSerial.c sysTffs.c sysUltraEnd.c sysWindML.c usbPciStub.c d:\Tornado2.2\target\config\all/bootConfig.c d:\Tornado2.2\target\config\all/bootInit.c d:\Tornado2.2\target\config\all/dataSegPad.c d:\Tornado2.2\target\config\all/usrConfig.c d:\Tornado2.2\target\config\all/version.c >>$@
	ccpentium -E -P -M -w $(CASFLAGS) romInit.s >> $@
	ccpentium -E -P -M -w $(CASFLAGS) sysALib.s >> $@
#下面这句的作用是将	depend.VirtualBoxBSP中绝对路径替换成$(WIND_BASE)
#	wtxtcl d:\Tornado2.2\host/src/hutils/bspDepend.tcl $@


bootInit.o : depend.VirtualBoxBSP d:\Tornado2.2\target\config\all\bootInit.c
	vxrm $@
	ccpentium -c $(CFLAGS_PIC) d:\Tornado2.2\target\config\all\bootInit.c

#vxrm == rm - @==hidden $@==current object
bootrom : depend.VirtualBoxBSP bootInit.o romInit.o bootrom.Z.o \
		 libcplus.a libgnucplus.a libvxcom.a libvxdcom.a libarch.a libcommoncc.a libdrv.a libgcc.a libnet.a libos.a librpc.a libtffs.a libusb.a libvxfusion.a libvxmp.a libvxvmi.a libwdb.a libwind.a libwindview.a d:\Tornado2.2\target/lib/libPENTIUMgnuvx.a 
	vxrm $@ 
#vxrm ldpentium -o $@ -Ttext  -Tdata  
	vxrm version.o
	ccpentium -c $(CFLAGS) -o version.o \
		d:\Tornado2.2\target\config\all/version.c
	ldpentium -X -N -e romInit -Ttext $(RAM_LOW_ADRS) \
		-o $@ romInit.o bootInit.o version.o \
		bootrom.Z.o  \
		--start-group -Ld:\Tornado2.2\target/lib/pentium/PENTIUM/gnu -Ld:\Tornado2.2\target/lib/pentium/PENTIUM/common $(LIBS) --end-group \
		-T d:\Tornado2.2\target/h/tool/gnu/ldscripts/link.RAM
	d:\Tornado2.2\host\x86-win32\bin\romsize pentium -b $(ROM_SIZE) $@
	@rem $@
	
################################## vxWorks #####################################
#
# vxWorks     - normal vxWorks system
# vxWorks.sym - symbol table of vxWorks

vxWorks vxWorks.sym : depend.VirtualBoxBSP usrConfig.o dataSegPad.o \
		sysALib.o sysLib.o   $(patsubst -l%,lib%.a,$(LIBS)) 
	vxrm vxWorks vxWorks.sym
	vxrm version.o
	vxrm vxWorks.tmp ctdt.c ctdt.o
	ccpentium -c $(CFLAGS) -o version.o d:\Tornado2.2\target\config\all/version.c
	ccpentium -r -nostdlib -Wl,-X \
	    -o vxWorks.tmp sysALib.o sysLib.o  usrConfig.o version.o \
	    -Wl,--start-group $(LD_LINK_PATH) $(LIBS) \
	    -Wl,--end-group 
	nmpentium vxWorks.tmp | wtxtcl d:\Tornado2.2/host/src/hutils/munch.tcl -c pentium > ctdt.c
	make CC_COMPILER="-fdollars-in-identifiers" ctdt.o
	ldpentium -X -N -e _sysInit -Ttext $(RAM_LOW_ADRS) \
	    -o vxWorks dataSegPad.o vxWorks.tmp ctdt.o -T d:\Tornado2.2\target/h/tool/gnu/ldscripts/link.RAM
	vxrm vxWorks.tmp
	@rem vxWorks
	@rem vxWorks
	d:\Tornado2.2\host\x86-win32\bin\objcopypentium --extract-symbol vxWorks vxWorks.sym
	@rem vxWorks.sym
	d:\Tornado2.2\host\x86-win32\bin\vxsize pentium -v $(RAM_HIGH_ADRS) $(RAM_LOW_ADRS) vxWorks


test :
	rem $@
