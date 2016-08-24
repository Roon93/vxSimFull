/* usrBsdSocket.c - Initialization configlette for BSD sockets */

/* Copyright 1992 - 1998 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,14nov00,rae  removed unused argument from sockLibAdd()
01c,30jul99,pul  configure max_linkhdr
01b,30jul99,pul  modify sockLibAdd to add extra parameter for future
                 scalability
01a,18aug98,ann  created this configlette from usrNetwork.c
*/

/*
DESCRIPTION
This configlette is included when the INCLUDE_BSD_SOCKET component is
chosen. It initializes the BSD socket library and adds the required
socket library back-ends.

NOMANUAL
*/


STATUS usrBsdSockLibInit(void)
    {
    
    max_linkhdr=USR_MAX_LINK_HDR;
    if (sockLibInit (NUM_FILES) == ERROR)
        {
        printf ("usrBsdSockLibInit failed\n");
	return (ERROR);
        }
    if (sockLibAdd ((FUNCPTR) bsdSockLibInit, AF_INET_BSD, AF_INET) == ERROR)
        {
        printf ("usrBsdSockLibInit failed in add\n");
        return (ERROR);
        }
    if (sockLibAdd ((FUNCPTR) bsdSockLibInit, AF_INET, AF_INET) == ERROR)
        {
        printf ("usrBsdSockLibInit failed in adding other second INET.\n");
        return (ERROR);
        }
    return (OK);
    }
