/* 01vxdcom.cdf */

/* Copyright 1999-2001, Wind River Systems, Inc. */

/*
modification history
--------------------
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
	PROTOTYPE	int usrVxdcomInit (void); \
			int include_vxdcom_EventHandler (void); \
			int include_vxdcom_HandleSet (void); \
			int include_vxdcom_INETSockAddr (void); \
			int include_vxdcom_Reactor (void); \
			int include_vxdcom_ReactorHandle (void); \
			int include_vxdcom_SockAcceptor (void); \
			int include_vxdcom_SockAddr (void); \
			int include_vxdcom_SockConnector (void); \
			int include_vxdcom_SockEP (void); \
			int include_vxdcom_SockIO (void); \
			int include_vxdcom_SockStream (void); \
			int include_vxdcom_ThreadPool (void); \
			int include_vxdcom_TimeValue (void); \
			int include_vxdcom_RpcDispatcher (void); \
			int include_vxdcom_RpcEventHandler (void); \
			int include_vxdcom_RpcIfClient (void); \
			int include_vxdcom_RpcIfServer (void); \
			int include_vxdcom_RpcPdu (void); \
			int include_vxdcom_RpcPduFactory (void); \
			int include_vxdcom_RpcProxyMsg (void); \
			int include_vxdcom_RpcStringBinding (void); \
			int include_vxdcom_DceDispatchTable (void); \
			int include_vxdcom_InterfaceProxy (void); \
			int include_vxdcom_NdrStreams (void); \
			int include_vxdcom_NdrTypes (void); \
			int include_vxdcom_ObjectExporter (void); \
			int include_vxdcom_ObjectTable (void); \
			int include_vxdcom_PSFactory (void); \
			int include_vxdcom_RemoteOxid (void); \
			int include_vxdcom_RemoteRegistry (void); \
			int include_vxdcom_RemoteSCM (void); \
			int include_vxdcom_SCM (void); \
			int include_vxdcom_StdMarshaler (void); \
			int include_vxdcom_StdProxy (void); \
			int include_vxdcom_StdStub (void); \
			int include_vxdcom_Stublet (void); \
			int include_vxdcom_dcomLib (void); \
			int include_vxdcom_ntlmssp (void); \
			int include_vxdcom_orpcLib (void); \
			int include_vxdcom_vxdcomExtent (void); \
			int include_vxdcom_vxdcomGlobals (void); \
			int include_vxdcom_OxidResolver_i (void); \
			int include_vxdcom_RemUnknown_i (void); \
			int include_vxdcom_RemoteActivation_i (void); \
			int include_orpc (void);
	INIT_RTN	usrVxdcomInit (); \
			include_vxdcom_EventHandler (); \
			include_vxdcom_HandleSet (); \
			include_vxdcom_INETSockAddr (); \
			include_vxdcom_Reactor (); \
			include_vxdcom_ReactorHandle (); \
			include_vxdcom_SockAcceptor (); \
			include_vxdcom_SockAddr (); \
			include_vxdcom_SockConnector (); \
			include_vxdcom_SockEP (); \
			include_vxdcom_SockIO (); \
			include_vxdcom_SockStream (); \
			include_vxdcom_ThreadPool (); \
			include_vxdcom_TimeValue (); \
			include_vxdcom_RpcDispatcher (); \
			include_vxdcom_RpcEventHandler (); \
			include_vxdcom_RpcIfClient (); \
			include_vxdcom_RpcIfServer (); \
			include_vxdcom_RpcPdu (); \
			include_vxdcom_RpcPduFactory (); \
			include_vxdcom_RpcProxyMsg (); \
			include_vxdcom_RpcStringBinding (); \
			include_vxdcom_DceDispatchTable (); \
			include_vxdcom_InterfaceProxy (); \
			include_vxdcom_NdrStreams (); \
			include_vxdcom_NdrTypes (); \
			include_vxdcom_ObjectExporter (); \
			include_vxdcom_ObjectTable (); \
			include_vxdcom_PSFactory (); \
			include_vxdcom_RemoteOxid (); \
			include_vxdcom_RemoteRegistry (); \
			include_vxdcom_RemoteSCM (); \
			include_vxdcom_SCM (); \
			include_vxdcom_StdMarshaler (); \
			include_vxdcom_StdProxy (); \
			include_vxdcom_StdStub (); \
			include_vxdcom_Stublet (); \
			include_vxdcom_dcomLib (); \
			include_vxdcom_ntlmssp (); \
			include_orpc (); \
			include_vxdcom_orpcLib (); \
			include_vxdcom_vxdcomExtent (); \
			include_vxdcom_vxdcomGlobals ();
	MODULES		OxidResolver_i.o \
			RemoteActivation_i.o \
			RemUnknown_i.o
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
	PROTOTYPE	int include_ClassFactory (void); \
			int include_ConnectionPoint (void); \
			int include_OxidResolver (void); \
			int include_RemUnknown (void); \
			int include_RemoteActivation (void); \
			int include_comAutomation (void); \
			int include_comCoreTypes (void); \
			int include_orpc (void); \
			int include_vxStream (void); \
			int include_vxidl (void);
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
	_INIT_ORDER	usrRoot
	INIT_AFTER	INCLUDE_DCOM
	REQUIRES	INCLUDE_DCOM
}

Component INCLUDE_DCOM_OPC {
	NAME		OPC Proxy/Stubs
	SYNOPSIS	VxWorks DCOM (Distributed COM) OPC Proxy Stubs
	PROTOTYPE	int include_opc_ae (void); \
			int include_opccomn (void); \
			int include_opcda (void);
	INIT_RTN	include_opc_ae (); \
			include_opccomn (); \
			include_opcda ();
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
