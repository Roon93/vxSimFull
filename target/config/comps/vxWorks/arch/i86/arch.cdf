/* 00arch.cdf - architecture component description file */

/* Copyright 1998-2001 Wind River Systems, Inc.  */


/*
modification history
--------------------
01b,27sep01,pad  Switched to ELF loader.
01a,24sep01,pai  Added architecture-specific LPT component configuration
                 (SPR 30067).
*/

/*
DESCRIPTION
The Tornado Project Facility sources all component description files (CDF)
in a particular order.  The CDF files in these directories are processed in
the order in which they are presented: comps/vxWorks,
comps/vxWorks/arch/<arch>, config/<bsp>, and the project directory.  If
two CDF files describe the same component, the file read last overrides
all earlier files.  As a result, CDF files processed later by the Project
Facility are said to have a higher precedence than those processed
earlier.  Given such functionality, this file contains
architecture-specific VxWorks components and component definitions which
override "generic" components defined in comps/vxWorks.

If a new component is released, a new .cdf file should be deposited
in the appropriate directory, rather than modifying existing files.

For more information, see the .I "Tornado BSP Developer's Kit for VxWorks
User's Guide, Tornado 2.0: Components".
*/

Component INCLUDE_LOADER {
        SYNOPSIS	ELF loader
        MODULES         loadLib.o loadElfLib.o
        INIT_RTN        loadElfInit();
        HDR_FILES       loadElfLib.h
}

Component INCLUDE_LPT {
	NAME		parallel port
	SYNOPSIS	Parallel port components
	MODULES		lptDrv.o
	INIT_RTN	lptDrv (LPT_CHANNELS, &lptResources[0]);
	CFG_PARAMS	LPT_CHANNELS \
	LPT0_BASE_ADRS LPT0_INT_LVL \
	LPT1_BASE_ADRS LPT1_INT_LVL \
	LPT2_BASE_ADRS LPT2_INT_LVL
	HDR_FILES	drv/parallel/lptDrv.h
}

Parameter LPT_CHANNELS {
	NAME		number of channels	
	SYNOPSIS	Number of parallel port channels (3 maximum)
	TYPE		uint
	DEFAULT		(1)
}

Parameter LPT0_BASE_ADRS {
	NAME		I/O address
	SYNOPSIS	Parallel port 1 I/O base address
	TYPE		uint
	DEFAULT		(0x3bc)
}

Parameter LPT0_INT_LVL {
	NAME		interupt level
	SYNOPSIS	Parallel port 1 interrupt level
	TYPE		uint
	DEFAULT		(0x07)
}

Parameter LPT1_BASE_ADRS {
	NAME		I/O address
	SYNOPSIS	Parallel port 2 I/O base address
	TYPE		uint
	DEFAULT		(0x378)
}

Parameter LPT1_INT_LVL {
	NAME		interrupt level
	SYNOPSIS	Parallel port 2 interrupt level
	TYPE		uint
	DEFAULT		(0x05)
}

Parameter LPT2_BASE_ADRS {
	NAME		I/O address
	SYNOPSIS	Parallel port 3 I/O base address
	TYPE		uint
	DEFAULT		(0x278)
}

Parameter LPT2_INT_LVL {
	NAME		interrupt level
	SYNOPSIS	Parallel port 3 interrupt level
	TYPE		uint
	DEFAULT		(0x09)
}

Parameter INT_LOCK_LEVEL {
	NAME		INT lock level
	SYNOPSIS	Interrupt lock level
	DEFAULT		0x0
}

Parameter ROOT_STACK_SIZE {
	NAME		Root stack size
	SYNOPSIS	Root task stack size (bytes)
	DEFAULT		10000
}

Parameter SHELL_STACK_SIZE {
	NAME		Shell stack size
	SYNOPSIS	Shell stack size (bytes)
	DEFAULT		10000
}

Parameter WDB_STACK_SIZE {
	NAME		WDB Stack size
	SYNOPSIS	WDB Stack size (bytes)
	DEFAULT		0x1000
}

Parameter ISR_STACK_SIZE {
	NAME		ISR stack size
	SYNOPSIS	ISR Stack size (bytes)
	DEFAULT		1000
}

Parameter VEC_BASE_ADRS {
	NAME		Vector base address
	SYNOPSIS	Vector base address
	DEFAULT		((char *)LOCAL_MEM_LOCAL_ADRS)
}

Parameter ROM_WARM_ADRS {
	NAME		ROM warm boot address
	SYNOPSIS	ROM warm boot address
#if	defined(I80486)
	DEFAULT		(ROM_TEXT_ADRS + 0x1a0)
#else
	DEFAULT		("unused")	
#endif
}

