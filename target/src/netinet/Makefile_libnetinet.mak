OBJS=	if.o if_ether.o if_subr.o igmp.o in.o in_cksum.o in_pcb.o \
	in_proto.o ip_icmp.o ip_input.o ip_mroute.o ip_output.o radix.o \
	raw_cb.o raw_ip.o raw_usrreq.o route.o rtsock.o sl_compress.o \
	sys_socket.o tcp_debug.o tcp_input.o tcp_output.o tcp_subr.o \
	tcp_timer.o tcp_usrreq.o udp_usrreq.o uipc_dom.o uipc_mbuf.o \
	uipc_sock.o uipc_sock2.o unixLib.o \

LIB_NAME = netinet	

CC_INCLUDE =  -I.  -I../../h  -I..

CC_DEFINES	= -DCPU=PENTIUM \
		  -DTOOL_FAMILY=gnu \
		  -DTOOL=gnu 

CFLAGS		= -march=pentium -ansi -g -gdwarf-2 -gstrict-dwarf -O2 -nostdlib -fno-builtin -fno-defer-pop -Wall $(CC_INCLUDE) $(CC_DEFINES)

.c.o :
	$(RM) $@
	cc $(CFLAGS) -c $<
###############################################################################build targets###############################################	

libnetinet.a : $(OBJS)
	$(RM) $@ 
	$(RM) *.dump
	$(RM) $@.nm
	ar -r $@ $(OBJS)
	nm $@ > $@.nm 

