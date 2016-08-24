/* bsp.cdf - BSP-specific component descriptor file */

/* Copyright 1984-1998 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,30Sep98,ms   written
*/

/*
DESCRIPTION

This file contains BSP-specific changes to the generic component
descriptor files.
*/

/* include the NDIS END driver whenever the network is included */

Component INCLUDE_END {
	INCLUDE_WHEN	INCLUDE_NET_INIT
}

