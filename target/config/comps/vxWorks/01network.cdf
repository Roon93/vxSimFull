/*
Copyright 1984 - 2002 Wind River Systems, Inc.

modification history
--------------------
01g,20mar02,vvv  removed reference to INCLUDE_BOOTP (SPR #74183)
01f,02oct01,vvv  added parameter to usrDhcpsStart in INCLUDE_DHCPS
01e,13dec00,spm  removed duplicate INCLUDE_DHCPR; added correct DHCP server
01d,12dec00,spm  removed excess modules to fix DHCP scalability
01c,29nov00,niq  Remove SELECT_NETADDR_INIT which is not necessary at all
01b,29nov00,niq  Changing entries so that new components add themselves as
                 children.
01a,26jun00,niq  written

DESCRIPTION

This file contains updated descriptions for any network components altered
or added in the Tornado maintenance release.
*/

Component INCLUDE_BPF 
	{
        NAME            Berkeley Packet Filter Driver
        SYNOPSIS        Provides direct access to link-level frames
        HDR_FILES       net/bpf.h
        MODULES         bpfDrv.o \
                        bpfProto.o \
                        bpf_filter.o
        _CHILDREN       FOLDER_NET_APP
	}
 
Component INCLUDE_DHCPC 
	{
        NAME 		DHCPv4 runtime client
        SYNOPSIS 	Dynamic host configuration protocol client
        MODULES         dhcpcLib.o
	CONFIGLETTES	net/usrNetDhcpcCfg.c net/usrNetBoot.c
        CFG_PARAMS      DHCPC_CPORT \
                        DHCPC_DEFAULT_LEASE \
                        DHCPC_MAX_LEASES \
                        DHCPC_MAX_MSGSIZE \
                        DHCPC_MIN_LEASE \
                        DHCPC_OFFER_TIMEOUT \
                        DHCPC_SPORT
	INIT_RTN	usrDhcpcStart ();
        REQUIRES        INCLUDE_BPF \
                        INCLUDE_NET_SETUP \
                        INCLUDE_ROUTE_SOCK
	HDR_FILES	dhcp/dhcpcInit.h sysLib.h
	}

Parameter DHCPC_MAX_MSGSIZE 
	{
        NAME            DHCP Client Maximum Message Size
        SYNOPSIS        Default allows minimum DHCP message in Ethernet frame
        TYPE            uint
        DEFAULT         590
	}

Component INCLUDE_DHCPS
	{
	NAME            DHCP server
	SYNOPSIS        Dynamic host configuration protocol server
	CONFIGLETTES    net/usrNetDhcpsCfg.c
	HDR_FILES       ioLib.h
	INIT_RTN        usrDhcpsStart (&dhcpsDfltCfgParams);
	CFG_PARAMS      DHCPS_CPORT \
			DHCPS_SPORT \
			DHCPS_MAX_MSGSIZE \
			DHCPS_ADDRESS_HOOK \
			DHCPS_DEFAULT_LEASE \
			DHCPS_LEASE_HOOK \
			DHCP_MAX_HOPS
	EXCLUDES        INCLUDE_DHCPR
	MODULES 	dhcpsLib.o
	REQUIRES        INCLUDE_NET_SETUP
	}

Parameter DHCPS_ADDRESS_HOOK
	{
	NAME            DHCP Server Address Storage Routine
	SYNOPSIS        Function pointer for preserving runtime pool entries
	TYPE            FUNCPTR
	DEFAULT         NULL
	}

Parameter DHCPS_DEFAULT_LEASE
	{
	NAME            DHCP Server Standard Lease Length
	SYNOPSIS        Default lease duration in seconds
	TYPE            uint
	DEFAULT         3600
	}

Parameter DHCPS_LEASE_HOOK
	{
	NAME            DHCP Server Lease Storage Routine
	SYNOPSIS        Function pointer for recording active leases
	TYPE            FUNCPTR
	DEFAULT         NULL
	}

Parameter DHCPS_MAX_MSGSIZE
	{
	NAME		DHCP Server/Relay Agent Maximum Message Size
	SYNOPSIS	Default allows minimum DHCP message in Ethernet frame
	DEFAULT		590
	TYPE		uint
	}

Component INCLUDE_MUX
	{
	NAME		network mux initialization
	SYNOPSIS	network driver to protocol multiplexer
	CFG_PARAMS      MUX_MAX_BINDS
	CONFIGLETTES	net/usrNetMuxCfg.c
	MODULES		muxLib.o
	INIT_RTN	usrMuxLibInit ();
        REQUIRES 	INCLUDE_NET_SETUP
	HDR_FILES       muxLib.h muxTkLib.h
	}

Parameter MUX_MAX_BINDS
	{
	NAME		MUX max bind value
	SYNOPSIS	Default number of bindings that MUX allows
	TYPE		uint
	DEFAULT		16
	}

Component INCLUDE_BSD_SOCKET
	{
	NAME		BSD SOCKET
	SYNOPSIS	BSD Socket Support
	CONFIGLETTES	net/usrBsdSocket.c
	CFG_PARAMS	NUM_FILES USR_MAX_LINK_HDR
	INIT_RTN	usrBsdSockLibInit();
	MODULES		bsdSockLib.o sockLib.o
        REQUIRES 	INCLUDE_NET_SETUP
	HDR_FILES	sys/socket.h bsdSockLib.h net/mbuf.h
	}

Parameter USR_MAX_LINK_HDR
	{
	NAME		Max link level header size
	SYNOPSIS	User defined MAX link-level header size
	TYPE		uint
	DEFAULT		16
	}
