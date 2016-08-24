/* usrNetTelnetdCfg.c - Initialization routine for the Telnet server */

/* Copyright 1992 - 2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,30apr02,elr  changed order of initialization
01a,14feb01,spm  inherited from version 01a of tor2_0_x branch
*/

/*
DESCRIPTION
This configlette contains the initialization routine for the 
INCLUDE_TELNET component.

NOMANUAL
*/

void usrTelnetdStart (void)
    {

     /* Configure telnet server to use a shell as a command interpreter. */

    if (telnetdParserSet (TELNETD_PARSER_HOOK) == ERROR)
        printf ("Error %x: unable to add command interpreter to telnet server.\n", errno);
    else 
    if (telnetdInit (TELNETD_MAX_CLIENTS, TELNETD_TASKFLAG) == ERROR)
        printf ("Error %x: unable to initialize telnet server.\n", errno);
    else if (telnetdStart (TELNETD_PORT) == ERROR)
        printf ("Error %x: unable to start telnet server.\n", errno);
    }
