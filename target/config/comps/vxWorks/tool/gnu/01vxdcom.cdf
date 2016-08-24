/* 01vxdcom.cdf */

/* Copyright 1999-2001, Wind River Systems, Inc. */

/*
modification history
--------------------
01j,13mar02,nel  SPR#74176. Correct OPC component syntax error.
01i,03jan02,nel  Correct proxy/stub protos.
01h,07dec01,nel  Add parameter to dcomShowInit call.
01g,06dec01,nel  Add extra parameter to dcomShow.
01f,19oct01,nel  Add DCOM_SHOW.
01e,10oct01,nel  Correct Synopsis.
01d,10oct01,nel  SPR#70841. Add SCM stack size parameter.
01c,17sep01,nel  Make component layout the same as AE.
01b,20jul01,dbs  separate vxcom and vxdcom
01a,18jul01,dbs  added modhist, fix proxy modules
*/

Folder FOLDER_DCOM {
	NAME		VxDCOM (Distributed Component Object Model)
	SYNOPSIS	DCOM (Distributed Component Object Model) C++ Support
	CHILDREN	INCLUDE_DCOM \
			INCLUDE_DCOM_PROXY \
			INCLUDE_DCOM_OPC \
			INCLUDE_DCOM_SHOW
	_CHILDREN	FOLDER_ROOT
}

Component INCLUDE_DCOM {
	NAME		DCOM Core
	SYNOPSIS	VxWorks DCOM (Distributed COM) Support
	INIT_RTN	usrVxdcomInit ();
	MODULES		EventHandler.o \
			HandleSet.o \
			INETSockAddr.o \
			Reactor.o \
			ReactorHandle.o \
			SockAcceptor.o \
			SockAddr.o \
			SockConnector.o \
			SockEP.o \
			SockIO.o \
			SockStream.o \
			ThreadPool.o \
			TimeValue.o \
			RpcDispatcher.o \
			RpcEventHandler.o \
			RpcIfClient.o \
			RpcIfServer.o \
			RpcPdu.o \
			RpcPduFactory.o \
			RpcProxyMsg.o \
			RpcStringBinding.o \
			DceDispatchTable.o \
			InterfaceProxy.o \
			NdrStreams.o \
			NdrTypes.o \
			ObjectExporter.o \
			ObjectTable.o \
			PSFactory.o \
			RemoteOxid.o \
			RemoteRegistry.o \
			RemoteSCM.o \
			SCM.o \
			StdMarshaler.o \
			StdProxy.o \
			StdStub.o \
			Stublet.o \
			dcomLib.o \
			ntlmssp.o \
			orpcLib.o \
			vxdcomExtent.o \
			vxdcomGlobals.o \
			OxidResolver_i.o \
			RemUnknown_i.o \
			RemoteActivation_i.o \
			ConnectionPoint_i.o \
			orpc_i.o
	CONFIGLETTES	usrVxdcom.c
	_INIT_ORDER	usrRoot
	INIT_AFTER	INCLUDE_COM
	CFG_PARAMS	VXDCOM_BSTR_POLICY \
			VXDCOM_AUTHN_LEVEL \
			VXDCOM_THREAD_PRIORITY \
			VXDCOM_STATIC_THREADS \
			VXDCOM_DYNAMIC_THREADS \
			VXDCOM_STACK_SIZE \
			VXDCOM_SCM_STACK_SIZE \
			VXDCOM_CLIENT_PRIORITY_PROPAGATION \
			VXDCOM_OBJECT_EXPORTER_PORT_NUMBER
	REQUIRES	INCLUDE_TCP \
			INCLUDE_COM
}

Component INCLUDE_DCOM_PROXY {
	NAME		DCOM Proxy/Stubs
	SYNOPSIS	VxWorks DCOM (Distributed COM) Proxy Stubs
	PROTOTYPE	void include_ClassFactory (); \
			void include_ConnectionPoint (); \
			void include_OxidResolver (); \
			void include_RemUnknown (); \
			void include_RemoteActivation (); \
			void include_comAutomation (); \
			void include_comCoreTypes (); \
			void include_orpc (); \
			void include_vxStream (); \
			void include_vxidl ();
	INIT_RTN	include_ClassFactory (); \
			include_ConnectionPoint (); \
			include_OxidResolver (); \
			include_RemUnknown (); \
			include_RemoteActivation (); \
			include_comAutomation (); \
			include_comCoreTypes (); \
			include_orpc (); \
			include_vxStream (); \
			include_vxidl ();
	MODULES		ClassFactory_ps.o \
			ConnectionPoint_ps.o \
			OxidResolver_ps.o \
			RemUnknown_ps.o \
			RemoteActivation_ps.o \
			comAutomation_ps.o \
			comCoreTypes_ps.o \
			orpc_ps.o \
			vxStream_ps.o \
			vxidl_ps.o
	_INIT_ORDER	usrRoot
	INIT_AFTER	INCLUDE_DCOM
	REQUIRES	INCLUDE_DCOM
}

Component INCLUDE_DCOM_OPC {
	NAME		OPC Proxy/Stubs
	SYNOPSIS	VxWorks DCOM (Distributed COM) OPC Proxy Stubs
	PROTOTYPE	void include_opc_ae (); \
			void include_opccomn (); \
			void include_opcda ();
	INIT_RTN	include_opc_ae (); \
			include_opccomn (); \
			include_opcda ();
	MODULES		opc_ae_ps.o \
			opccomn_ps.o \
			opcda_ps.o
	_INIT_ORDER	usrRoot
	INIT_AFTER	INCLUDE_DCOM
	REQUIRES	INCLUDE_DCOM \
			INCLUDE_DCOM_PROXY
}

Component INCLUDE_DCOM_SHOW {
    	NAME		VxWorks DCOM Protocol Debuging Show Routines
	SYNOPSIS	VxWorks DCOM (Distibuted COM) Show Routines
	MODULES		dcomShow.o
	_INIT_ORDER	usrRoot
	INIT_AFTER	INCLUDE_DCOM
	REQUIRES	INCLUDE_COM
        PROTOTYPE	void dcomShowInit (int);
	INIT_RTN	dcomShowInit (VXDCOM_DCOM_SHOW_PORT);
	MODULES		dcomShow.o
	CFG_PARAMS	VXDCOM_DCOM_SHOW_PORT
}

Parameter VXDCOM_DCOM_SHOW_PORT {
        NAME            Debug Output Port
        SYNOPSIS        Port to connect to to retrieve debug output from
        TYPE            int
        DEFAULT         0
}

Parameter VXDCOM_BSTR_POLICY {
        NAME            BSTR marshaling policy
        SYNOPSIS        Treat BSTRs as byte-arrays when marshaling
        TYPE            bool
        DEFAULT         FALSE
}

Parameter VXDCOM_AUTHN_LEVEL {
        NAME            NTLM Authentication Level
        SYNOPSIS        Level of authentication required
        TYPE            int
        DEFAULT         0
}

Parameter VXDCOM_THREAD_PRIORITY {
        NAME            Server thread priority
        SYNOPSIS        Specifies default priority of threads in server threadpool.
        TYPE            int
        DEFAULT         150
}

Parameter VXDCOM_STATIC_THREADS {
        NAME            Number of static threads
        SYNOPSIS        Number of threads to preallocate in the server threadpool.
        TYPE            uint
        DEFAULT         5
}

Parameter VXDCOM_DYNAMIC_THREADS {
        NAME            Number of dynamic threads
        SYNOPSIS        Number of additional threads that can be allocated at peak times.
        TYPE            uint
        DEFAULT         30
}

Parameter VXDCOM_STACK_SIZE {
        NAME            Stack size of server threads
        SYNOPSIS        Stack size of threads in the server threadpool.
        TYPE            uint
        DEFAULT         16384
}

Parameter VXDCOM_SCM_STACK_SIZE {
        NAME            Stack size of the SCM task.
        SYNOPSIS        Stack size of Service Control Manager (SCM) task.
        TYPE            uint
        DEFAULT         32000
}

Parameter VXDCOM_CLIENT_PRIORITY_PROPAGATION {
        NAME            Client priority propagation
        SYNOPSIS        Indicates whether or not client priority should be propagated.
        TYPE            bool
        DEFAULT         TRUE
}

Parameter VXDCOM_OBJECT_EXPORTER_PORT_NUMBER {
        NAME            Port number for the Object Exporter
        SYNOPSIS        If zero the port number will be assigned dynamically.
        TYPE            int
        DEFAULT         65000
}
