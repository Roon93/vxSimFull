OBJS=	arpLib.o bootpLib.o ipProto.o bsdSockLib.o \
        dhcpcShow.o dhcpcBootLib.o dhcpcCommonLib.o dhcpcLib.o \
        rarpLib.o rdiscLib.o dhcprLib.o dhcpsLib.o \
        etherLib.o etherMultiLib.o ftpLib.o ftpdLib.o \
        hostLib.o icmpLib.o ifLib.o igmpLib.o inetLib.o ipLib.o m2IcmpLib.o \
	m2IfLib.o m2IpLib.o m2Lib.o m2SysLib.o m2TcpLib.o m2UdpLib.o \
	mbufLib.o mbufSockLib.o mountLib.o muxLib.o \
	muxTkLib.o netDrv.o netLib.o \
	netShow.o nfsDrv.o nfsHash.o nfsLib.o nfsdLib.o pingLib.o proxyArpLib.o \
	proxyLib.o remLib.o rlogLib.o routeLib.o \
        routeCommonLib.o  routeUtilLib.o \
        rpcLib.o sockLib.o \
	sntpcLib.o sntpsLib.o tcpLib.o telnetdLib.o tftpLib.o tftpdLib.o \
        udpLib.o xdr_bool_t.o xdr_nfs.o xdr_nfsserv.o zbufLib.o zbufSockLib.o \
	mCastRouteLib.o igmpShow.o icmpShow.o tcpShow.o udpShow.o \
	ipFilterLib.o routeSockLib.o netBufLib.o ifIndexLib.o \
	wvNetLib.o nfsHash.o

LIB_NAME = netwrs	

CC_INCLUDE =  -I.  -I../../h  -I..

CC_DEFINES	= -DCPU=PENTIUM \
		  -DTOOL_FAMILY=gnu \
		  -DTOOL=gnu 

CFLAGS		= -march=pentium -ansi -g -gdwarf-2 -gstrict-dwarf -O2 -nostdlib -fno-builtin -fno-defer-pop -Wall $(CC_INCLUDE) $(CC_DEFINES)

.c.o :
	$(RM) $@
	cc $(CFLAGS) -c $<
###############################################################################build targets###############################################	

libnetwrs.a : $(OBJS)
	$(RM) $@ 
	$(RM) *.dump
	$(RM) $@.nm
	ar -r $@ $(OBJS)
	nm $@ > $@.nm 


