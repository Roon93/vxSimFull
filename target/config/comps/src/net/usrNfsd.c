/* usrNfsd.c - Support for NFS server */

/* Copyright 2001-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,20dec01,vvv  allow client and server to be built together 
01a,06nov01,vvv  written
*/

/*
DESCRIPTION
This file is used to include support for the NFS server. The contents of this
file are included in the project configuration file when the component
INCLUDE_NFS_SERVER is defined. This file initializes the variable nfsMaxPath
which uses the configuration parameter NFS_MAXPATH to set the maximum file
path length handled by the server. It also initializes the server by calling
the init routine.

NOMANUAL
*/

#ifndef INCLUDE_NFS
const int nfsMaxPath = NFS_MAXPATH + 1;
#endif

/******************************************************************************
*
* usrNfsdInit - initialize NFS server
*
* This routine initializes the NFS server.
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

STATUS usrNfsdInit (void)
   {
   if (nfsMaxPath < 1)
       {
       printf ("Error initializing NFS server, invalid NFS_MAXPATH\n");
       return (ERROR);
       }

   if (nfsdInit (0, 0, 0, 0, 0, 0) == ERROR)
       {
       printf ("Error initializing NFS server\n");
       return (ERROR);
       }

   return (OK);
   }
