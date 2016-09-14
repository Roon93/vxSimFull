/*
 * $Log:   P:/user/amir/lite/vcs/amdmtd.c_v  $
 * 
 *    Rev 1.21   03 Nov 1997 16:07:06   danig
 * Support RFA
 *
 *    Rev 1.20   02 Nov 1997 11:06:38   ANDRY
 * bug fix in AMDErase() for RFA on PowerPC
 *
 *    Rev 1.19   20 Oct 1997 14:08:56   danig
 * Resume erase only when needed
 *
 *    Rev 1.18   19 Oct 1997 16:39:50   danig
 * Deal with the last word in interleaving 4
 *
 *    Rev 1.17   29 Sep 1997 18:21:08   danig
 * Try different interleavings in amdMTDIdentify()
 *
 *    Rev 1.16   24 Sep 1997 17:45:52   danig
 * Default interleaving value is 4
 *
 *    Rev 1.15   10 Sep 1997 16:22:00   danig
 * Got rid of generic names
 *
 *    Rev 1.14   08 Sep 1997 18:56:50   danig
 * Support interleaving 4
 *
 *    Rev 1.13   04 Sep 1997 17:39:34   danig
 * Debug messages
 *
 *    Rev 1.12   31 Aug 1997 14:53:48   danig
 * Registration routine return status
 *
 *    Rev 1.11   10 Aug 1997 17:56:02   danig
 * Comments
 *
 *    Rev 1.10   24 Jul 1997 17:51:54   amirban
 * FAR to FAR0
 *
 *    Rev 1.9   20 Jul 1997 17:16:54   amirban
 * No watchDogTimer
 *
 *    Rev 1.8   07 Jul 1997 15:20:54   amirban
 * Ver 2.0
 *
 *    Rev 1.5   06 Feb 1997 18:18:34   danig
 * Different unlock addresses for series C
 *
 *    Rev 1.4   17 Nov 1996 15:45:16   danig
 * added LV017 support.
 *
 *    Rev 1.3   14 Oct 1996 17:57:00   danig
 * new IDs and eraseFirstBlockLV008.
 *
 *    Rev 1.2   09 Sep 1996 11:38:26   amirban
 * Correction for Fujitsu 8-mbit
 *
 *    Rev 1.1   29 Aug 1996 14:14:46   amirban
 * Warnings
 *
 *    Rev 1.0   15 Aug 1996 15:16:38   amirban
 * Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1996			*/
/*									*/
/************************************************************************/

/*----------------------------------------------------------------------*/
/*                                                                      */
/* This MTD supports the following Flash technologies:                  */
/*                                                                      */
/* - AMD Am29F080 8-mbit devices					*/
/* - AMD Am29LV080 8-mbit devices			       		*/
/* - AMD Am29F016 16-mbit devices					*/
/* - Fujitsu MBM29F080 8-mbit devices					*/
/*									*/
/* And (among others) the following Flash media and cards:		*/
/*                                                                      */
/* - AMD Series-D PCMCIA cards                                        	*/
/* - AMD AmMC0XXA Miniature cards					*/
/* - AMD AmMCL0XXA Miniature cards					*/
/*									*/
/*----------------------------------------------------------------------*/


#include "flflash.h"
#include "backgrnd.h"


#define NO_UNLOCK_ADDR 0xffffffffL

typedef struct {
  unsigned long  unlockAddr1,
		 unlockAddr2;
  unsigned long  baseMask;
} Vars;

static Vars mtdVars[DRIVES];

#define thisVars   ((Vars *) vol.mtdVars)


#define SETUP_ERASE	0x80
#define SETUP_WRITE	0xa0
#define READ_ID 	0x90
#define SUSPEND_ERASE	0xb0
#define SECTOR_ERASE	0x30
#define RESUME_ERASE	0x30
#define READ_ARRAY	0xf0

#define UNLOCK_1	0xaa
#define	UNLOCK_2	0x55

#define UNLOCK_ADDR1	0x5555u
#define	UNLOCK_ADDR2	0x2aaau

#define	D2		4	/* Toggles when erase suspended */
#define D5		0x20	/* Set when programming timeout */
#define	D6		0x40	/* Toggles when programming */

/* JEDEC ids for this MTD */
#define Am29F040_FLASH          0x01a4
#define	Am29F080_FLASH		0x01d5
#define	Am29LV080_FLASH		0x0138
#define Am29LV008_FLASH         0x0137
#define Am29F016_FLASH          0x01ad
#define	Am29F016C_FLASH  	0x013d
#define Am29LV017_FLASH         0x01c8

#define Fuj29F040_FLASH         0x04a4
#define	Fuj29F080_FLASH		0x04d5
#define	Fuj29LV080_FLASH	0x0438
#define Fuj29LV008_FLASH        0x0437
#define Fuj29F016_FLASH         0x04ad
#define	Fuj29F016C_FLASH  	0x043d
#define Fuj29LV017_FLASH        0x04c8


/*----------------------------------------------------------------------*/
/*                         m a p B a s e				*/
/*									*/
/* Map the window to a page base (page is 4KB or 32KB depends on the	*/
/* media type) and return a pointer to the base. Also return the offset	*/
/* of the given address from the base.					*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      address		: Card address to map				*/
/*	offset		: receives the offset from the base		*/
/*	length		: length to map					*/
/*                                                                      */
/* Returns:								*/
/*	FlashPTR	: pointer to the page base.			*/
/*									*/
/*----------------------------------------------------------------------*/

static FlashPTR mapBase(FLFlash        vol,
                        CardAddress    address,
                        unsigned long *offset,
                        int            length)
{
  CardAddress base = address & thisVars->baseMask;

  *offset = (unsigned long)(address - base);
  return (FlashPTR)vol.map(&vol, base, length + *offset);
}

/*----------------------------------------------------------------------*/
/*                         a m d C o m m a n d				*/
/*									*/
/* Writes an AMD command with the required unlock sequence		*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      address		: Card address at which to write command	*/
/*	command		: command to write				*/
/*	flashPtr	: pointer to the window				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void amdCommand(FLFlash vol,
		       CardAddress address,
		       unsigned char command,
		       FlashPTR flashPtr)
{
  if (thisVars->unlockAddr1 != NO_UNLOCK_ADDR) {
    *((FlashPTR) flAddLongToFarPointer((void FAR0 *)flashPtr,
             ((int) address & (vol.interleaving - 1)) + thisVars->unlockAddr1))
                                                                   = UNLOCK_1;
    *((FlashPTR) flAddLongToFarPointer((void FAR0 *)flashPtr,
             ((int) address & (vol.interleaving - 1)) + thisVars->unlockAddr2))
                                                                   = UNLOCK_2;
    *((FlashPTR) flAddLongToFarPointer((void FAR0 *)flashPtr,
             ((int) address & (vol.interleaving - 1)) + thisVars->unlockAddr1))
                                                                   = command;
  }
  else {
    CardAddress baseAddress = address & (-0x10000l | (vol.interleaving - 1));

    *(FlashPTR) flMap(vol.socket,baseAddress + vol.interleaving * UNLOCK_ADDR1) = UNLOCK_1;
    *(FlashPTR) flMap(vol.socket,baseAddress + vol.interleaving * UNLOCK_ADDR2) = UNLOCK_2;
    *(FlashPTR) flMap(vol.socket,baseAddress + vol.interleaving * UNLOCK_ADDR1) = command;
    flMap(vol.socket, address);
  }
}


/*----------------------------------------------------------------------*/
/*                      a m d M T D W r i t e				*/
/*									*/
/* Write a block of bytes to Flash					*/
/*									*/
/* This routine will be registered as the MTD vol.write routine	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      address		: Card address to write to			*/
/*      buffer		: Address of data to write			*/
/*	length		: Number of bytes to write			*/
/*	overwrite	: TRUE if overwriting old Flash contents	*/
/*			  FALSE if old contents are known to be erased	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus amdMTDWrite(FLFlash vol,
			  CardAddress address,
			  const void FAR1 *buffer,
			  int length,
			  FLBoolean overwrite)
{
  /* Set timeout to 5 seconds from now */
  unsigned long writeTimeout = flMsecCounter + 5000;
  int cLength;
  FlashPTR flashPtr, unlockAddr1, unlockAddr2;
  unsigned long offset;

  if (flWriteProtected(vol.socket))
    return flWriteProtect;

  flashPtr = mapBase(&vol, address, &offset, length);
  unlockAddr1 = (FlashPTR) flAddLongToFarPointer((void FAR0 *)flashPtr,
                                                       thisVars->unlockAddr1);
  unlockAddr2 = (FlashPTR) flAddLongToFarPointer((void FAR0 *)flashPtr,
                                                       thisVars->unlockAddr2);
  flashPtr    = (FlashPTR) flAddLongToFarPointer((void FAR0 *)flashPtr,
                                                       offset);

  cLength = length;
  if (vol.interleaving == 1) {
lastByte:
#ifdef __cplusplus
    #define bFlashPtr  flashPtr
    #define bBuffer ((const unsigned char FAR1 * &) buffer)
#else
    #define bFlashPtr  flashPtr
    #define bBuffer ((const unsigned char FAR1 *) buffer)
#endif
    while (cLength >= 1) {
      *unlockAddr1 = UNLOCK_1;
      *unlockAddr2 = UNLOCK_2;
      *unlockAddr1 = SETUP_WRITE;
      *bFlashPtr = *bBuffer;
      cLength--;
      buffer = (unsigned char *)buffer + 1;  /* bBuffer++; */
      bFlashPtr++;
      while (bFlashPtr[-1] != bBuffer[-1] && flMsecCounter < writeTimeout) {
	if ((bFlashPtr[-1] & D5) && bFlashPtr[-1] != bBuffer[-1]) {
	  bFlashPtr[-1] = READ_ARRAY;
	#ifdef DEBUG_PRINT
	  DEBUG_PRINT("Debug: write failed in AMD MTD.\n");
	#endif
	  return flWriteFault;
	}
      }
    }
  }
  else if (vol.interleaving == 2)  {
lastWord:
#ifdef __cplusplus
    #define wFlashPtr ((FlashWPTR &) flashPtr)
    #define wBuffer ((const unsigned short FAR1 * &) buffer)
    #define wUnlockAddr1 ((FlashWPTR &) unlockAddr1)
    #define wUnlockAddr2 ((FlashWPTR &) unlockAddr2)
#else
    #define wFlashPtr ((FlashWPTR) flashPtr)
    #define wBuffer ((const unsigned short FAR1 *) buffer)
    #define wUnlockAddr1 ((FlashWPTR) unlockAddr1)
    #define wUnlockAddr2 ((FlashWPTR) unlockAddr2)
#endif
    while (cLength >= 2) {
      *wUnlockAddr1 = UNLOCK_1 * 0x101;
      *wUnlockAddr2 = UNLOCK_2 * 0x101;
      *wUnlockAddr1 = SETUP_WRITE * 0x101;
      *wFlashPtr = *wBuffer;
      cLength -= 2;
      buffer = (unsigned char *)buffer + sizeof(short);  /* wBuffer++; */
      flashPtr = (unsigned char *)flashPtr + sizeof(short); /* wFlashPtr++; */
      while ((wFlashPtr[-1] != wBuffer[-1]) && (flMsecCounter < writeTimeout)) {
        if (((wFlashPtr[-1] &  D5         ) && ((wFlashPtr[-1] ^ wBuffer[-1]) &   0xff))
                          ||
            ((wFlashPtr[-1] & (D5 * 0x100)) && ((wFlashPtr[-1] ^ wBuffer[-1]) & 0xff00))) {
	  wFlashPtr[-1] = READ_ARRAY * 0x101;
	#ifdef DEBUG_PRINT
	  DEBUG_PRINT("Debug: write failed in AMD MTD.\n");
	#endif
	  return flWriteFault;
	}
      }
    }
    if (cLength > 0)
      goto lastByte;
  }
  else /* if (vol.interleaving >= 4) */ {
#ifdef __cplusplus
    #define dFlashPtr ((FlashDPTR &) flashPtr)
    #define dBuffer ((const unsigned long FAR1 * &) buffer)
    #define dUnlockAddr1 ((FlashDPTR &) unlockAddr1)
    #define dUnlockAddr2 ((FlashDPTR &) unlockAddr2)
#else
    #define dFlashPtr ((FlashDPTR) flashPtr)
    #define dBuffer ((const unsigned long FAR1 *) buffer)
    #define dUnlockAddr1 ((FlashDPTR) unlockAddr1)
    #define dUnlockAddr2 ((FlashDPTR) unlockAddr2)
#endif
    while (cLength >= 4) {
      *dUnlockAddr1 = UNLOCK_1 * 0x1010101lu;
      *dUnlockAddr2 = UNLOCK_2 * 0x1010101lu;
      *dUnlockAddr1 = SETUP_WRITE * 0x1010101lu;
      *dFlashPtr = *dBuffer;
      cLength -= 4;
      buffer = (unsigned char *)buffer + sizeof(long); /* dBuffer++; */
      flashPtr = (unsigned char *)flashPtr + sizeof(long); /* dFlashPtr++; */
      while ((dFlashPtr[-1] != dBuffer[-1]) && (flMsecCounter < writeTimeout)) {
        if (((dFlashPtr[-1] &  D5               ) && ((dFlashPtr[-1]  ^ dBuffer[-1]) &         0xff))
                               ||
            ((dFlashPtr[-1] & (D5 *       0x100)) && ((dFlashPtr[-1]  ^ dBuffer[-1]) &       0xff00))
                               ||
            ((dFlashPtr[-1] & (D5 *   0x10000lu)) && ((dFlashPtr[-1]  ^ dBuffer[-1]) &   0xff0000lu))
                               ||
            ((dFlashPtr[-1] & (D5 * 0x1000000lu)) && ((dFlashPtr[-1]  ^ dBuffer[-1]) & 0xff000000lu))) {
	  dFlashPtr[-1] = READ_ARRAY * 0x1010101lu;
	#ifdef DEBUG_PRINT
	  DEBUG_PRINT("Debug: write failed in AMD MTD.\n");
	#endif
	  return flWriteFault;
	}
      }
    }
    if (cLength > 0)
      goto lastWord;
  }

  flashPtr -= length;
  buffer = (unsigned char *)buffer - length; /* bBuffer -= length; */

  if (tffscmp((void FAR0 *) flashPtr,buffer,length)) {
    /* verify the data */
	#ifdef DEBUG_PRINT
	  DEBUG_PRINT("Debug: write failed in AMD MTD on verification.\n");
	#endif
    return flWriteFault;
  }

  return flOK;
}

/*----------------------------------------------------------------------*/
/*               e r a s e F i r s t B l o c k L V 0 0 8		*/
/*									*/
/* Erase the first block in LV008 chip. This block is devided into four */
/* subblocks 16, 8, 8, and 32 kbytes in size.  				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      firstErasableBlock : Number of block to erase			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus eraseFirstBlockLV008(FLFlash vol, int firstErasableBlock)
{
  int iSubBlock;
  long subBlockSize = 0;

  for (iSubBlock = 0; iSubBlock < 4; iSubBlock++) {
    int i;
    FlashPTR flashPtr;
    FLBoolean finished;

    switch (iSubBlock) {
      case 1:
	subBlockSize = 0x4000;
	break;
      case 2:
      case 3:
	subBlockSize = 0x2000;
	break;
    }

    flashPtr = (FlashPTR)
	  vol.map(&vol,
		    firstErasableBlock + subBlockSize * vol.interleaving,
		    vol.interleaving);

    for (i = 0; i < vol.interleaving; i++) {
      amdCommand(&vol, i,SETUP_ERASE, flashPtr);
      *((FlashPTR) flAddLongToFarPointer((void FAR0 *)flashPtr,
                                          i + thisVars->unlockAddr1)) = UNLOCK_1;
      *((FlashPTR) flAddLongToFarPointer((void FAR0 *)flashPtr,
                                          i + thisVars->unlockAddr2)) = UNLOCK_2;
      flashPtr[i] = SECTOR_ERASE;
    }

    do {
#ifdef BACKGROUND
      while (flForeground(1) == BG_SUSPEND) {		/* suspend */
	for (i = 0; i < vol.interleaving; i++) {
	  flashPtr[i] = SUSPEND_ERASE;
	  /* Wait for D6 to stop toggling */
	  while ((flashPtr[i] ^ flashPtr[i]) & D6)
	    ;
	}
      }
#endif
      finished = TRUE;
      for (i = 0; i < vol.interleaving; i++) {
	flashPtr[i] = RESUME_ERASE;
	if (flashPtr[i] != 0xff) {
	  if ((flashPtr[i] & D5) && flashPtr[i] != 0xff) {
	    flashPtr[i] = READ_ARRAY;
	    return flWriteFault;
	  }
	  finished = FALSE;
	}
      }
    } while (!finished);
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                     a m d M T D E r a s e				*/
/*									*/
/* Erase one or more contiguous Flash erasable blocks			*/
/*									*/
/* This routine will be registered as the MTD vol.erase routine	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      firstErasableBlock : Number of first block to erase		*/
/*	numOfErasableBlocks: Number of blocks to erase			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/
static FLStatus amdMTDErase(FLFlash vol,
			  int firstErasableBlock,
			  int numOfErasableBlocks)
{
  int iBlock;

  if (flWriteProtected(vol.socket))
    return flWriteProtect;

  for (iBlock = 0; iBlock < numOfErasableBlocks; iBlock++) {
    int i;
    FLBoolean finished;
    FlashPTR flashPtr;

    /* The first block in an LV008 chip requires special care.*/
    if ((vol.type == Am29LV008_FLASH) || (vol.type == Fuj29LV008_FLASH))
      if ((firstErasableBlock + iBlock) % (vol.chipSize / 0x10000l) == 0) {
	checkStatus(eraseFirstBlockLV008(&vol, firstErasableBlock + iBlock));
	continue;
      }

    /* No need to call mapBase because we know we are on a unit boundary */
    flashPtr = (FlashPTR)
	  vol.map(&vol,
		    (firstErasableBlock + iBlock) * vol.erasableBlockSize,
		    vol.interleaving);

    for (i = 0; i < vol.interleaving; i++) {
      amdCommand(&vol, i,SETUP_ERASE, flashPtr);
      *((FlashPTR) flAddLongToFarPointer((void FAR0 *)flashPtr,
                                          i + thisVars->unlockAddr1)) = UNLOCK_1;
      *((FlashPTR) flAddLongToFarPointer((void FAR0 *)flashPtr,
                                          i + thisVars->unlockAddr2)) = UNLOCK_2;
      flashPtr[i] = SECTOR_ERASE;
    }

    do {
#ifdef BACKGROUND
      FLBoolean eraseSuspended = FALSE;

      while (flForeground(1) == BG_SUSPEND) {		/* suspend */
        eraseSuspended = TRUE;
	for (i = 0; i < vol.interleaving; i++) {
	  flashPtr[i] = SUSPEND_ERASE;
	  /* Wait for D6 to stop toggling */
	  while ((flashPtr[i] ^ flashPtr[i]) & D6)
	    ;
	}
      }

      if (eraseSuspended) {  				/* resume */
        eraseSuspended = FALSE;
        for(i = 0; i < vol.interleaving; i++)
          flashPtr[i] = RESUME_ERASE;
      }
#endif
      finished = TRUE;
      for (i = 0; i < vol.interleaving; i++) {
	if (flashPtr[i] != 0xff) {
	  if ((flashPtr[i] & D5) && flashPtr[i] != 0xff) {
	    flashPtr[i] = READ_ARRAY;
	  #ifdef DEBUG_PRINT
	    DEBUG_PRINT("Debug: erase failed in AMD MTD.\n");
	  #endif
	    return flWriteFault;
	  }
	  finished = FALSE;
	}
      }
    } while (!finished);
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                    a m d M T D I d e n t i f y			*/
/*									*/
/* Identifies AMD and Fujitsu flash media and registers as an MTD for   */
/* such.								*/
/*									*/
/* On successful identification, the Flash structure is filled out and	*/
/* the write and erase routines registered.				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on positive identificaion, failed otherwise	*/
/*----------------------------------------------------------------------*/

FLStatus amdMTDIdentify(FLFlash vol)
{
  int inlv;

  #ifdef DEBUG_PRINT
    DEBUG_PRINT("Debug: entering AMD MTD identification routine.\n");
  #endif
  flSetWindowBusWidth(vol.socket,16);/* use 16-bits */
  flSetWindowSpeed(vol.socket,120);  /* 120 nsec. */
  flSetWindowSize(vol.socket,2);	/* 8 KBytes */

  vol.mtdVars = &mtdVars[flSocketNoOf(vol.socket)];
  thisVars->unlockAddr1 = NO_UNLOCK_ADDR;

  /* try different interleavings */
  for (inlv = 4; inlv > 0; inlv >>= 1) {
    if (inlv == 1)
      flSetWindowBusWidth(vol.socket,8); /* use 8-bits */
    vol.interleaving = inlv;
    flIntelIdentify(&vol, amdCommand,0);
    if (vol.type == Am29F016_FLASH ||
	vol.type == Fuj29F016_FLASH ||
	vol.type == Am29F016C_FLASH ||
	vol.type == Fuj29F016C_FLASH ||
	vol.type == Am29F080_FLASH ||
	vol.type == Fuj29F080_FLASH ||
	vol.type == Am29LV080_FLASH ||
	vol.type == Fuj29LV080_FLASH ||
	vol.type == Am29LV008_FLASH ||
	vol.type == Fuj29LV008_FLASH ||
	vol.type == Am29F040_FLASH ||
	vol.type == Fuj29F040_FLASH ||
	vol.type == Am29LV017_FLASH ||
	vol.type == Fuj29LV017_FLASH)
      break;
  }

  if (vol.type == Am29F016_FLASH ||
      vol.type == Fuj29F016_FLASH ||
      vol.type == Am29F016C_FLASH ||
      vol.type == Fuj29F016C_FLASH ||
      vol.type == Am29LV017_FLASH ||
      vol.type == Fuj29LV017_FLASH)
    vol.chipSize = 0x200000l;
  else if (vol.type == Fuj29F080_FLASH ||
	   vol.type == Am29F080_FLASH ||
	   vol.type == Fuj29LV080_FLASH ||
	   vol.type == Am29LV080_FLASH ||
	   vol.type == Fuj29LV008_FLASH ||
	   vol.type == Am29LV008_FLASH)
    vol.chipSize = 0x100000l;
  else if (vol.type == Fuj29F040_FLASH ||
	   vol.type == Am29F040_FLASH)
    vol.chipSize = 0x80000l;
  else {
  #ifdef DEBUG_PRINT
    DEBUG_PRINT("Debug: did not identify AMD or Fujitsu flash media.\n");
  #endif
    return flUnknownMedia;
  }

  if ((vol.type == Am29F016C_FLASH) || (vol.type == Fuj29F016C_FLASH)) {
    thisVars->unlockAddr1 = thisVars->unlockAddr2 = 0L;
    thisVars->baseMask = 0xfffff800L * vol.interleaving;
  }
  else if ((vol.type == Am29F040_FLASH) || (vol.type == Fuj29F040_FLASH)){
    flSetWindowSize(vol.socket,8 * vol.interleaving);
    thisVars->unlockAddr1 = 0x5555u * vol.interleaving;
    thisVars->unlockAddr2 = 0x2aaau * vol.interleaving;
    thisVars->baseMask = 0xffff8000L * vol.interleaving;
  }
  else {
    thisVars->unlockAddr1 = 0x555 * vol.interleaving;
    thisVars->unlockAddr2 = 0x2aa * vol.interleaving;
    thisVars->baseMask = 0xfffff800L * vol.interleaving;
  }

  checkStatus(flIntelSize(&vol,amdCommand,0));

  vol.erasableBlockSize = 0x10000l * vol.interleaving;
  vol.flags |= SUSPEND_FOR_WRITE;

  /* Register our flash handlers */
  vol.write = amdMTDWrite;
  vol.erase = amdMTDErase;

#ifdef DEBUG_PRINT
  DEBUG_PRINT("Debug: Identified AMD or Fujitsu flash media.\n");
#endif
  return flOK;
}


#if	FALSE
/*----------------------------------------------------------------------*/
/*                   f l R e g i s t e r A M D M T D			*/
/*									*/
/* Registers this MTD for use						*/
/*									*/
/* Parameters:                                                          */
/*	None								*/
/*                                                                      */
/* Returns:								*/
/*	FLStatus	: 0 on success, otherwise failure		*/
/*----------------------------------------------------------------------*/

FLStatus flRegisterAMDMTD(void)
{
  if (noOfMTDs >= MTDS)
    return flTooManyComponents;

  mtdTable[noOfMTDs++] = amdMTDIdentify;

  return flOK;
}
#endif	/* FALSE */
