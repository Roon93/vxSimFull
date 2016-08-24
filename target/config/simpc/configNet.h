/* configNet.h - network configuration header */

/* Copyright 1984-1997 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,19jan98,gnn  made addresses slave to new, patched BSP, addresses.
01e,20nov97,gnn  fixed spr#9555, configurable numRds and numTds
01d,03oct97,gnn  added function prototype for load function
01c,25sep97,gnn  SENS beta feedback fixes
01b,02jun97,map  updated ivec and ilevel.
01a,25apr97,map  written.

*/
 
#ifndef INCnetConfigh
#define INCnetConfigh

#include "vxWorks.h"
#include "end.h"

#define WIN_LOAD_FUNC	ntLoad	
#define WIN_BUFF_LOAN   1

/* <unit>:<vector>:<level> */
#define	WIN_LOAD_STRING	"0:0xc002:1:0:0"

IMPORT END_OBJ* WIN_LOAD_FUNC (char*,void *);

END_TBL_ENTRY endDevTbl [] =
{
    { 0, WIN_LOAD_FUNC, WIN_LOAD_STRING, WIN_BUFF_LOAN, NULL, FALSE},
    { 0, END_TBL_END, NULL, 0, NULL, FALSE},
};

#endif /* INCnetConfigh */
