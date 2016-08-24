/* ntEnd.c - END  network interface driver  to ULIP for vxSim for Windows NT */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01n,06feb02,jmp  fixed ntHandleRcvInt() to free pRcvBuffer if no available
		 clusters (SPR# 74108).
		 fixed END_MIB_INIT() call to use ETHERMTU rather than
		 NT_BUFSIZ (SPR# 74109).
		 fixed packet len init in ntHandleRcvInt() (SPR# 74109).
01n,12nov01,jmp  removed build warnings.
01m,21sep01,jmp  fixed ntStart() to return simUlipInit() return value instead
                 of returning always OK.
01l,19apr01,jmp  reworked simUlipInit() and ntInt() to no longer use global
                 variables (SPR# 64900).
01k,02mar01,pai  fixed ntInt() prototype and cleaned up some compiler
                 warnings.
01j,02mar01,pai  fixed SPRs #31983 and #31064.
01i,02mar01,pai  adapted some of ericl's warning fixes from VxWorks AE work.
01h,02mar01,pai  passing in correct type of pointer in strtok_r. (SPR 62224).
01g,02mar01,pai  corrected the handling of EIOCSFLAGS ioctl (SPR# 29423).
01f,02mar01,pai  removed reference to etherLib.
01e,02mar01,pai  updated ntResolv API to new network stack (SPR #31357).
01d,29mar99,dat  SPR 26119, documentation, usage of .bS/.bE
01c,10jun98,cym  changed sprintf to bcopy.
01b,01jun98,cym  changed interrupt handler to not use free.
01a,21jan98,cym	 written based on templateEnd.c.
*/

/*
DESCRIPTION

	This driver provides a fake ethernet intface to the "ULIP" driver
written by WRS.  The driver essentially gets packets from vxWorks, and writes
them directly to file, where the ULIP driver handles them.

The macro SYS_ENET_ADDR_GET is used to get the ethernet address (MAC)
for the device.  The single argument to this routine is the NTEND_DEVICE
pointer.  By default this routine copies the ethernet address stored in
the global variable ntEnetAddr into the NTEND_DEVICE structure.

INCLUDES:
end.h endLib.h etherMultiLib.h

SEE ALSO: muxLib, endLib
.I "Writing and Enhanced Network Driver"
*/

#include "vxWorks.h"
#include "stdio.h"
#include "stdlib.h"
#include "intLib.h"
#include "netLib.h"
#include "sysLib.h"
#include "net/mbuf.h"
#include "iv.h"
#include "memLib.h"
#include "sys/ioctl.h"
#include "etherMultiLib.h"		/* multicast stuff. */
#include "end.h"			/* Common END structures. */
#include "endLib.h"
#include "lstLib.h"			/* Needed to maintain protocol list. */
#include "net/protosw.h"
#include "sys/socket.h"
#include "errno.h"
#include "net/if.h"
#include "net/route.h"
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/in_var.h"
#include "netinet/ip.h"
#include "netinet/if_ether.h"
#include "net/if_subr.h"
#include "m2Lib.h"
#include "netBufLib.h"
#include "muxLib.h"

extern int simUlipAlive;
extern int simUlipRcvLen;
extern char *simUlipRcvBuffer;
extern int simUlipInit(int ** ppSimUlipRcvCount,int ** ppSimUlipReadCount);
extern int simUlipWrite();    
extern int simUlipRcvCount;
extern int simUlipReadCount;
extern void simUlipBufFree();
extern int winOut ();

/***** LOCAL DEFINITIONS *****/

/* Configuration items */

#define NT_BUFSIZ      (ETHERMTU + SIZEOF_ETHERHEADER + 6)
#define NT_SPEED        10000000

/*
 * Default macro definitions for BSP interface.
 * These macros can be redefined in a wrapper file, to generate
 * a new module with an optimized interface.
 */

/* Macro to get the ethernet address from the BSP */

#ifndef SYS_ENET_ADDR_GET
#   define SYS_ENET_ADDR_GET(pDevice) \
	{ \
	IMPORT unsigned char ntEnetAddr[]; \
	bcopy ((char *)ntEnetAddr, (char *)(&pDevice->enetAddr), 6); \
	}
#endif
/* XXX cym the ULIP driver is always ethernet address 254. */

/* A shortcut for getting the hardware address from the MIB II stuff. */

#define END_HADDR(pEnd)	\
		((pEnd)->mib2Tbl.ifPhysAddress.phyAddress)

#define END_HADDR_LEN(pEnd) \
		((pEnd)->mib2Tbl.ifPhysAddress.addrLength)

typedef struct free_args
    {
    void* arg1;
    void* arg2;
    } FREE_ARGS;
        
/* The definition of the driver control structure */

typedef struct ntend_device
    {
    END_OBJ     end;			/* The class we inherit from. */
    int		unit;			/* unit number */
    int         ivec;                   /* interrupt vector */
    int         ilevel;                 /* interrupt level */
    char*       pShMem;                	/* real ptr to shared memory */
    long	flags;			/* Our local flags. */
    UCHAR	enetAddr[6];		/* ethernet address */
    CL_POOL_ID  pClPoolId;
    END_ERR     lastError;              /* Last error passed to muxError */
    } NTEND_DEVICE;

/* Definitions for the flags field */

#define NT_PROMISCUOUS_FLAG     0x1
#define NT_RCV_HANDLING_FLAG    0x2
#define NT_POLLING		0x4

/* externals */

IMPORT int endMultiLstCnt (END_OBJ *);

/* forward declarations */

STATUS		ntPollStart (NTEND_DEVICE * pDrvCtrl);
STATUS		ntPollStop (NTEND_DEVICE * pDrvCtrl);
void		ntAddrFilterSet (NTEND_DEVICE * pDrvCtrl);
void		ntInt (NTEND_DEVICE * pDrvCtrl, void * pRcvBuf, int rcvLen);
LOCAL void	ntHandleRcvInt (NTEND_DEVICE * pDrvCtrl, void * buf, int len);
LOCAL void	ntConfig (NTEND_DEVICE * pDrvCtrl);

/* END Specific interfaces. */
/* This is the only externally visible interface. */

END_OBJ * 	ntLoad (char * initString,void * nothing);

LOCAL STATUS	ntStart	(NTEND_DEVICE * pDrvCtrl);
LOCAL STATUS	ntStop	(NTEND_DEVICE * pDrvCtrl);
LOCAL STATUS	ntUnload	();
LOCAL int	ntIoctl	(NTEND_DEVICE * pDrvCtrl, int cmd, caddr_t data);
LOCAL STATUS	ntSend	(NTEND_DEVICE * pDrvCtrl, M_BLK_ID pBuf);
			  
LOCAL STATUS	ntMCastAdd (NTEND_DEVICE * pDrvCtrl, char * pAddress);
LOCAL STATUS	ntMCastDel (NTEND_DEVICE * pDrvCtrl, char * pAddress);
LOCAL STATUS	ntMCastGet (NTEND_DEVICE * pDrvCtrl,
			    MULTI_TABLE * pTable);
LOCAL STATUS	ntPollSend (NTEND_DEVICE * pDrvCtrl, M_BLK_ID pBuf);
LOCAL STATUS	ntPollRcv (NTEND_DEVICE * pDrvCtrl, M_BLK_ID pBuf);

LOCAL STATUS	ntParse	();
LOCAL STATUS	ntMemInit	();
LOCAL int	ntArpResolv (struct arpcom * ap, struct rtentry * rt,
			     struct mbuf * m, struct sockaddr * dst,
			     unsigned char * desten);

/* globals */

int ntRsize = 5;
int ntTsize = 5;
int ntNumMBufs = 128;

/* network buffers configuration */

M_CL_CONFIG ntMclConfig =       /* mBlk configuration table */
    {
    0, 0, NULL, 0
    };

CL_DESC ntClDescTbl [] =        /* network cluster pool configuration table */
    {
    /*
    clusterSize num             memArea         memSize
    ----------- ----            -------         -------
    */
    {NT_BUFSIZ,         0,      NULL,           0}
    };

int ntClDescTblNumEnt = (NELEMENTS(ntClDescTbl));

/* locals */

/*
 * Declare our function table.  This is static across all driver
 * instances.
 */
LOCAL NET_FUNCS ntFuncTable =
    {
    (FUNCPTR)ntStart,		/* Function to start the device. */
    (FUNCPTR)ntStop,		/* Function to stop the device. */
    (FUNCPTR)ntUnload,		/* Unloading function for the driver. */
    (FUNCPTR)ntIoctl,		/* Ioctl function for the driver. */
    (FUNCPTR)ntSend,		/* Send function for the driver. */
    (FUNCPTR)ntMCastAdd,	/* Multicast address add function for the */
				/* driver. */
    (FUNCPTR)ntMCastDel,	/* Multicast address delete function for */
				/* the driver. */
    (FUNCPTR)ntMCastGet,	/* Multicast table retrieve function for */
				/* the driver. */
    (FUNCPTR)ntPollSend,	/* Polling send function for the driver. */
    (FUNCPTR)ntPollRcv,		/* Polling receive function for the driver. */
    endEtherAddressForm,        /* Put address info into a packet.  */
    endEtherPacketDataGet,      /* Get a pointer to packet data. */
    endEtherPacketAddrGet       /* Get packet addresses. */
    };

LOCAL int *	pSimUlipRcvCount = NULL;
LOCAL int *	pSimUlipReadCount = NULL;

/*******************************************************************************
*
* ntLoad - initialize the driver and device
*
* This routine initializes the driver and the device to the operational state.
* All of the device specific parameters are passed in the initString.
*
* The string contains the target specific parameters like this:
*
* "unit:register addr:int vector:int level:shmem addr:shmem size:shmem width"
*
* RETURNS: An END object pointer or NULL on error.
*/

END_OBJ * ntLoad
    (
    char * initString,		/* String to be parse by the driver. */
    void * nothing
    )
    {
    NTEND_DEVICE *	pDrvCtrl;

    /* initialize various pointers */

    if (initString == NULL) 
	{
        return(0);
	}

    if (*initString == '\0') 
        {
        bcopy("nt", initString,3);
        return(0);
        }

    /* allocate the device structure */

    pDrvCtrl = (NTEND_DEVICE *)calloc (sizeof (NTEND_DEVICE), 1);
    if (pDrvCtrl == NULL)
	{
	goto errorExit;
	}

    /* parse the init string, filling in the device structure */

    if (ntParse (pDrvCtrl, initString) == ERROR)
	goto errorExit;

    /* Ask the BSP to provide the ethernet address. */

    SYS_ENET_ADDR_GET(pDrvCtrl);

    /* initialize the END and MIB2 parts of the structure */

    if (END_OBJ_INIT (&pDrvCtrl->end, (DEV_OBJ *)pDrvCtrl, "nt",
		      pDrvCtrl->unit, &ntFuncTable, "Windows NT ULIP END")
	== ERROR ||
	END_MIB_INIT (&pDrvCtrl->end, M2_ifType_ethernet_csmacd,
		      &pDrvCtrl->enetAddr[0], 6, ETHERMTU,
                      NT_SPEED)
	== ERROR)
	{
	goto errorExit;
	}

    /* Perform memory allocation/distribution */

    if (ntMemInit (pDrvCtrl) == ERROR)
	{
	goto errorExit;
	}

    /* reset and reconfigure the device */

    ntConfig (pDrvCtrl);

    /* set the flags to indicate readiness */

    END_OBJ_READY (&pDrvCtrl->end,
		   IFF_NOARP | IFF_UP | IFF_RUNNING |
		   IFF_POINTOPOINT );
		   /*IFF_NOTRAILERS | IFF_MULTICAST | IFF_LOAN| IFF_SCAT);*/

    return (&pDrvCtrl->end);

errorExit:
    if (pDrvCtrl != NULL)
	free ((char *)pDrvCtrl);

    return NULL;
    }

/*******************************************************************************
*
* ntParse - parse the init string
*
* Parse the input string.  Fill in values in the driver control structure.
*
* The initialization string format is:
* .CS
*  "unit:csrAdr:rapAdr:vecnum:intLvl:memAdrs:memSize:memWidth"
* .CE
*
* .IP <unit>
* Device unit number, a small integer.
* .IP <vecNum>
* Interrupt vector number (used with sysIntConnect)
* .IP <intLvl>
* Interrupt level (isn't really used)
* .LP
*
* RETURNS: OK or ERROR for invalid arguments.
*/

STATUS ntParse
    (
    NTEND_DEVICE *	pDrvCtrl,
    char *		initString
    )
    {
    char *	tok;
    char *	pHolder = NULL;
    
    /* Parse the initString */

    /* Unit number. */

    pDrvCtrl->unit = 0;
    pDrvCtrl->ivec = 0xc002;
    pDrvCtrl->ilevel = 1;

    return(OK);
    tok = strtok_r (initString, ":", &pHolder);
    if (tok == NULL)
	return ERROR;
    pDrvCtrl->unit = atoi (tok);

    /* Interrupt vector. */

    tok = strtok_r (NULL, ":", &pHolder);
    if (tok == NULL)
	return ERROR;
    pDrvCtrl->ivec = atoi (tok);

    /* Interrupt level. */

    tok = strtok_r (NULL, ":", &pHolder);
    if (tok == NULL)
	return ERROR;
    pDrvCtrl->ilevel = atoi (tok);

    return OK;
    }

/*******************************************************************************
*
* ntMemInit - initialize memory for the chip
*
* This routine is highly specific to the device.  
*
* RETURNS: OK or ERROR.
*/

STATUS ntMemInit
    (
    NTEND_DEVICE * pDrvCtrl	/* device to be initialized */
    )
    {
    /* Set up a net Pool */
    if ((pDrvCtrl->end.pNetPool = malloc (sizeof(NET_POOL))) == NULL)
        return (ERROR);

    ntMclConfig.mBlkNum = ntNumMBufs;
    ntMclConfig.clBlkNum = ntNumMBufs;

    ntMclConfig.memSize = (ntMclConfig.mBlkNum * (MSIZE + sizeof (long))) +
                          (ntMclConfig.clBlkNum * (CL_BLK_SZ + sizeof(long)));

    /* allocate memory pool */
    if ((ntMclConfig.memArea = (char *) memalign (sizeof(long),
                                                  ntMclConfig.memSize))
        == NULL)
        return (ERROR);


    ntClDescTbl[0].clNum = ntNumMBufs;

    ntClDescTbl[0].memSize = (ntClDescTbl[0].clNum * (NT_BUFSIZ + 8))
        + sizeof(int);

    ntClDescTbl[0].memArea = malloc ( ntClDescTbl[0].memSize );

    if (netPoolInit(pDrvCtrl->end.pNetPool, &ntMclConfig,
                    &ntClDescTbl[0], ntClDescTblNumEnt, NULL) == ERROR)
        {
	int lvl = intLock ();	/* lock windows system call */
	winOut ("Couldn't Initialize Net Pool\n");
	intUnlock (lvl);	/* unlock windows system call */
        return (ERROR);
        }

    /* Store the cluster pool id as others need it later. */
    pDrvCtrl->pClPoolId = clPoolIdGet(pDrvCtrl->end.pNetPool,
                                      NT_BUFSIZ, FALSE);
    return OK;
    }

/*******************************************************************************
*
* ntStart - start the device
*
* This function calls BSP functions to connect interrupts and start the
* device running in interrupt mode.
*
* RETURNS: OK or ERROR
*
*/

LOCAL STATUS ntStart
    (
    NTEND_DEVICE *pDrvCtrl
    )
    {

    /* XXX cym add stuff to interface to polled thread */
    intConnect ((VOIDFUNCPTR *)0xc002, &ntInt, (int)pDrvCtrl);

    return (simUlipInit (&pSimUlipRcvCount, &pSimUlipReadCount));
    }


/*******************************************************************************
*
* ntHandleRcvInt - task level interrupt service for input packets
*
* This routine is called at task level indirectly by the interrupt
* service routine to do any message received processing.
*
* RETURNS: N/A.
*/

LOCAL void ntHandleRcvInt
    (
    NTEND_DEVICE *	pDrvCtrl,
    void *		pRcvBuffer,
    int			len
    )
    {
    M_BLK_ID	pMblk;
    char *	pNewCluster;
    CL_BLK_ID	pClBlk;

    pNewCluster = netClusterGet (pDrvCtrl->end.pNetPool, pDrvCtrl->pClPoolId);

    if(pNewCluster == NULL)
	{
	simUlipBufFree (pRcvBuffer);
	END_ERR_ADD (&pDrvCtrl->end, MIB2_IN_UCAST, +1);
	return;
	}

    /* SPR #31064 fix use packet length rather than ETHERMTU */

    bcopy (pRcvBuffer, pNewCluster, len);
    simUlipBufFree (pRcvBuffer);

    pClBlk = netClBlkGet (pDrvCtrl->end.pNetPool, M_DONTWAIT);
    if(pClBlk == NULL)
	{
        netClFree (pDrvCtrl->end.pNetPool, pNewCluster);
	return;
	}

    pMblk = mBlkGet (pDrvCtrl->end.pNetPool, M_DONTWAIT,MT_DATA);

    if( pMblk == NULL)
	{
        netClBlkFree (pDrvCtrl->end.pNetPool, pClBlk);
        netClFree (pDrvCtrl->end.pNetPool, pNewCluster);
	return;
	}
    
    END_ERR_ADD (&pDrvCtrl->end, MIB2_IN_UCAST, +1);

    netClBlkJoin (pClBlk, pNewCluster, len, NULL, 0, 0, 0);
    netMblkClJoin (pMblk, pClBlk);

    pMblk->mBlkHdr.mFlags |= M_PKTHDR;
    pMblk->mBlkHdr.mLen = len;
    pMblk->mBlkPktHdr.len =  pMblk->mBlkHdr.mLen;

    END_RCV_RTN_CALL (&pDrvCtrl->end, pMblk);
    }

/*******************************************************************************
*
* ntSend - the driver send routine
*
* This routine takes a M_BLK_ID sends off the data in the M_BLK_ID.
* The buffer must already have the addressing information properly installed
* in it.  This is done by a higher layer.  The last arguments are a free
* routine to be called when the device is done with the buffer and a pointer
* to the argument to pass to the free routine.  
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS ntSend
    (
    NTEND_DEVICE *	pDrvCtrl,	/* device ptr */
    M_BLK_ID		pNBuff		/* data to send */
    )
    {
    int		len;
    int		oldLevel;
#if FALSE
    void *	pTemp;
    M_BLK_ID	pFree;
    M_BLK_ID	pChunk;
    int 	count;
    int		chunk;
#endif
    /* 
     * ETHERMTU is not enough to be the buffer size (SPR #31983).
     * From the test case 1534 is the maximal value, this value is 54 + 1480
     * where 54 is the header sizes and 1480 the remaining size deduced from
     * the MTU (that should be constant with ULIP). 
     */
    char 	copyBuf[1600];
  
    /*
     * This is the vector processing section.
     * A vector is an array of M_BLK_IDs pointed to by the pData
     * field of a lead M_BLK_ID.  The lead is what is passed down
     * as pNBuff.  The length field of the head M_BLK_ID is the number
     * of elements in the array.
     * Each M_BLK_ID that is actually in the array has a pData field
     * that points to the actual data as well as a length that gives the
     * amount of data that is pointed to.
     * Each M_BLK_ID in the array has its own free routine and arguments.
     */

#if FALSE /* XXX cym fix Me */
    if (pNBuff->type & END_BUF_VECTOR)
	{
        if (!(pDrvCtrl->flags & NT_POLLING))
            oldLevel = intLock ();     /* disable ints during update */

	for (chunk = 0;chunk < pNBuff->length; chunk++)
	    {

	    pChunk = pChunk->m_next;

            /*
	     * Place packets into local structures.
             * If there is an error remember to free ALL associated
             * M_BLK_ID data before returning.
	     */
            
	    /* place a transmit request */
	    len = netMblkToBufCopy (pNBuff, (char *)copyBuf, NULL);
	    len = max(len,ETHERSMALL);

	    simUlipWrite (copyBuf, len);

	    /* Advance our management index */
            }
        if (!(pDrvCtrl->flags & NT_POLLING))
            intUnlock (oldLevel);   /* now ntInt won't get confused */
        }

    /* This is the normal case where all the data is in one M_BLK_ID */

    else
#endif /* FALSE XXX cym Fix ME */
	{
        /* Set pointers in local structures to point to data. */

        if (!(pDrvCtrl->flags & NT_POLLING))
	    {
	    oldLevel = intLock ();     /* disable ints during update */
	    }

	/* place a transmit request */
	len = netMblkToBufCopy (pNBuff, (char *)copyBuf, NULL);
	len = max(len,ETHERSMALL);

	simUlipWrite (copyBuf,len);

        netMblkClChainFree (pNBuff);

	if (!(pDrvCtrl->flags & NT_POLLING))
	    {
	    intUnlock (oldLevel);   /* now ntInt won't get confused */
	    }

	/* Advance our management index */

	}

    /* Bump the statistic counter. */

    END_ERR_ADD (&pDrvCtrl->end, MIB2_OUT_UCAST, +1);

    return (OK);
    }

/*******************************************************************************
*
* ntIoctl - the driver I/O control routine
*
* Process an ioctl request.
*/

LOCAL int ntIoctl
    (
    NTEND_DEVICE *pDrvCtrl,
    int cmd,
    caddr_t data
    )
    {
    int error = 0;
    long value;

    switch (cmd)
        {
        case EIOCSADDR:
	    if (data == NULL)
		return (EINVAL);
            bcopy ((char *)data, (char *)END_HADDR(&pDrvCtrl->end),
		   END_HADDR_LEN(&pDrvCtrl->end));
            break;

        case EIOCGADDR:
	    if (data == NULL)
		return (EINVAL);
            bcopy ((char *)END_HADDR(&pDrvCtrl->end), (char *)data,
		    END_HADDR_LEN(&pDrvCtrl->end));
            break;

        case EIOCSFLAGS:
	    value = (long)data;
	    if (value < 0)
		{
		value = -value;
		value--;
		END_FLAGS_CLR (&pDrvCtrl->end, value);
		}
	    else
		{
		END_FLAGS_SET (&pDrvCtrl->end, value);
		}
	    ntConfig (pDrvCtrl);
            break;

        case EIOCGFLAGS:
	    *(int *)data = END_FLAGS_GET(&pDrvCtrl->end);
            break;

	case EIOCPOLLSTART:
	    ntPollStart (pDrvCtrl);
	    break;

	case EIOCPOLLSTOP:
	    ntPollStop (pDrvCtrl);
	    break;

        case EIOCGMIB2:
            if (data == NULL)
                return (EINVAL);
            bcopy((char *)&pDrvCtrl->end.mib2Tbl, (char *)data,
                  sizeof(pDrvCtrl->end.mib2Tbl));
            break;
        case EIOCGFBUF:
            if (data == NULL)
                return (EINVAL);
            *(int *)data = 100; /* XXX cym macro-ize me */
            break;
        default:
            error = EINVAL;
        }

    return (error);
    }

/******************************************************************************
*
* ntConfig - reconfigure the interface under us.
*
* Reconfigure the interface setting promiscuous mode, and changing the
* multicast interface list.
*
* NOMANUAL
*/
LOCAL void ntConfig
    (
    NTEND_DEVICE *pDrvCtrl
    )
    {
#if FALSE
    u_short stat;
    void* pTemp;
    int timeoutCount = 0;
    int ix;
#endif

    /* Set promiscuous mode if it's asked for. */

    if (END_FLAGS_GET(&pDrvCtrl->end) & IFF_PROMISC)
	{
	}
    else
	{
	}

    /* Set up address filter for multicasting. */
	
    if (END_MULTI_LST_CNT(&pDrvCtrl->end) > 0)
	{
	ntAddrFilterSet (pDrvCtrl);
	}

    /* TODO - shutdown device completely */

    /* TODO - reset all device counters/pointers, etc. */

    /* TODO - initialise the hardware according to flags */

    return;
    }

/******************************************************************************
*
* ntAddrFilterSet - set the address filter for multicast addresses
*
* This routine goes through all of the multicast addresses on the list
* of addresses (added with the endAddrAdd() routine) and sets the
* device's filter correctly.
*
* NOMANUAL
*/
void ntAddrFilterSet
    (
    NTEND_DEVICE *pDrvCtrl
    )
    {
    ETHER_MULTI* pCurr;

    pCurr = END_MULTI_LST_FIRST (&pDrvCtrl->end);

    while (pCurr != NULL)
	{

        /* TODO - set up the multicast list */
        
	pCurr = END_MULTI_LST_NEXT(pCurr);
	}
    }

/*******************************************************************************
*
* ntPollRcv - routine to receive a packet in polled mode.
*
* This routine is called by a user to try and get a packet from the
* device.
*/

LOCAL STATUS ntPollRcv
    (
    NTEND_DEVICE *pDrvCtrl,
    M_BLK_ID pNBuff
    )
    {
    int len;
    int lvl;

    /* TODO - If no packet is available return immediately */

    /* see if any received packets haven't been read by the ISR */
    if(simUlipReadCount==simUlipRcvCount) 
	return(EAGAIN);

    lvl = intLock ();		/* lock windows system call */
    winOut ("Can't do polled mode right now \n");
    intUnlock (lvl);		/* unlock windows system call */

    return(ERROR);
    /* Upper layer must provide a valid buffer. */

    if (pNBuff->mBlkHdr.mData == NULL || pNBuff->mBlkHdr.mLen < len)
	{
	return (EAGAIN);
	}

    /* TODO - clear any status bits that may be set. */

    /* TODO - Check packet and device for errors */

    END_ERR_ADD (&pDrvCtrl->end, MIB2_IN_UCAST, +1);

    /* TODO - Process device packet into net buffer */

    /* TODO - Done with packet, clean up and give it to the device. */

    return (OK);
    }

/*******************************************************************************
*
* ntPollSend - routine to send a packet in polled mode.
*
* This routine is called by a user to try and send a packet on the
* device.
*/

static STATUS ntPollSend
    (
    NTEND_DEVICE* pDrvCtrl,
    M_BLK_ID pMblk 
    )
    {
    int         len;
    char	copyBuf[ETHERMTU];

    len = netMblkToBufCopy (pMblk, (char *)copyBuf, NULL);
    len = max(len,ETHERSMALL);

    simUlipWrite (copyBuf,len);

    END_ERR_ADD (&pDrvCtrl->end, MIB2_OUT_UCAST, +1);

    return (OK);
    }

/*******************************************************************************
*
* ntStop - stop the device
*
* This function calls BSP functions to disconnect interrupts and stop
* the device from operating in interrupt mode.
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS ntStop
    (
    NTEND_DEVICE *pDrvCtrl
    )
    {
    STATUS result = OK;

    /* TODO - stop/disable the device. */
    /* XXX cym toggle the fake driver */
    simUlipAlive = 0;

    return (result);
    }

/******************************************************************************
*
* ntUnload - unload a driver from the system
*
* This function first brings down the device, and then frees any
* stuff that was allocated by the driver in the load function.
*/

LOCAL STATUS ntUnload
    (
    NTEND_DEVICE* pDrvCtrl
    )
    {
    simUlipAlive = 0;
    END_OBJECT_UNLOAD (&pDrvCtrl->end); /* generic end unload functions */

    /* TODO - Free any shared DMA memory */

    return (OK);
    }


/*******************************************************************************
*
* ntPollStart - start polled mode operations
*
* RETURNS: OK or ERROR.
*/

STATUS ntPollStart
    (
    NTEND_DEVICE* pDrvCtrl
    )
    {
    int         oldLevel;

    oldLevel = intLock ();          /* disable ints during update */

    pDrvCtrl->flags |= NT_POLLING;

    intUnlock (oldLevel);   /* now ntInt won't get confused */

    ntConfig (pDrvCtrl);	/* reconfigure device */

    return (OK);
    }

/*******************************************************************************
*
* ntPollStop - stop polled mode operations
*
* This function terminates polled mode operation.  The device returns to
* interrupt mode.
*
* The device interrupts are enabled, the current mode flag is switched
* to indicate interrupt mode and the device is then reconfigured for
* interrupt operation.
*
* RETURNS: OK or ERROR.
*/

STATUS ntPollStop
    (
    NTEND_DEVICE* pDrvCtrl
    )
    {
    int         oldLevel;

    oldLevel = intLock ();	/* disable ints during register updates */

    /* TODO - re-enable interrupts */

    pDrvCtrl->flags &= ~NT_POLLING;

    intUnlock (oldLevel);

    ntConfig (pDrvCtrl);

    return (OK);
    }

/*****************************************************************************
*
* ntMCastAdd - add a multicast address for the device
*
* This routine adds a multicast address to whatever the driver
* is already listening for.  It then resets the address filter.
*/

LOCAL STATUS ntMCastAdd
    (
    NTEND_DEVICE *pDrvCtrl,
    char* pAddress
    )
    {
    int error;

    if ((error = etherMultiAdd (&pDrvCtrl->end.multiList,
		pAddress)) == ENETRESET)
	ntConfig (pDrvCtrl);

    return (OK);
    }

/*****************************************************************************
*
* ntMCastDel - delete a multicast address for the device
*
* This routine removes a multicast address from whatever the driver
* is listening for.  It then resets the address filter.
*/

LOCAL STATUS ntMCastDel
    (
    NTEND_DEVICE* pDrvCtrl,
    char* pAddress
    )
    {
    int error;

    if ((error = etherMultiDel (&pDrvCtrl->end.multiList,
	     (char *)pAddress)) == ENETRESET)
	ntConfig (pDrvCtrl);

    return (OK);
    }

/*****************************************************************************
*
* ntMCastGet - get the multicast address list for the device
*
* This routine gets the multicast list of whatever the driver
* is already listening for.
*/

LOCAL STATUS ntMCastGet
    (
    NTEND_DEVICE* pDrvCtrl,
    MULTI_TABLE* pTable
    )
    {
    int error;

    error = etherMultiGet (&pDrvCtrl->end.multiList, pTable);

    return (error);
    }

/*******************************************************************************
*
* ntInt - handle controller interrupt
*
* This routine is called at interrupt level in response to an interrupt from
* the controller.
*
* RETURNS: N/A.
*/

void ntInt
    (
    NTEND_DEVICE  *	pDrvCtrl,
    void *	 	pRcvBuffer,
    int			rcvLen
    )
    {

    /* Have netTask handle any input packets */
    netJobAdd ((FUNCPTR)ntHandleRcvInt, (int)pDrvCtrl,
	       (int)pRcvBuffer, rcvLen, 0, 0);
    }

int ntResolv
   (
   FUNCPTR           ipArpCallBackRtn,
   struct mbuf *     pMbuf,
   struct sockaddr * dstIpAddr,
   struct ifnet *    ifp,
   struct rtentry *  rt,
   char *            dstBuff
   )
   {
   return (ntArpResolv ((struct arpcom *)ifp, rt, pMbuf, dstIpAddr, dstBuff));
   }

static int ntArpResolv
    (
    struct arpcom *	ap,
    struct rtentry *	rt,
    struct mbuf *	m,
    struct sockaddr *	dst,
    unsigned char *	desten
    )
    {
    * (desten) = 0;
    * (desten+1) = 0;
    * (desten+2) = 0;
    * (desten+3) = 0;
    * (desten+4) = 0;
    * (desten+5) = 254;
    return(1);
    }
