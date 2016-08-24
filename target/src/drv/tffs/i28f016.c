/*
 * $Log:   P:/user/amir/lite/vcs/i28f016.c_v  $
 * 
 *    Rev 1.10   06 Oct 1997  9:45:48   danig
 * VPP functions under #ifdef
 * 
 *    Rev 1.9   10 Sep 1997 16:48:24   danig
 * Debug messages & got rid of generic names
 * 
 *    Rev 1.8   31 Aug 1997 15:09:20   danig
 * Registration routine return status
 * 
 *    Rev 1.7   24 Jul 1997 17:52:58   amirban
 * FAR to FAR0
 * 
 *    Rev 1.6   20 Jul 1997 17:17:06   amirban
 * No watchDogTimer
 * 
 *    Rev 1.5   07 Jul 1997 15:22:08   amirban
 * Ver 2.0
 * 
 *    Rev 1.4   04 Mar 1997 16:44:22   amirban
 * Page buffer bug fix
 * 
 *    Rev 1.3   18 Aug 1996 13:48:24   amirban
 * Comments
 * 
 *    Rev 1.2   12 Aug 1996 15:49:04   amirban
 * Added suspend/resume
 * 
 *    Rev 1.1   31 Jul 1996 14:30:50   amirban
 * Background stuff
 * 
 *    Rev 1.0   18 Jun 1996 16:34:30   amirban
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
/* - Intel 28F016SA/28016SV/Cobra 16-mbit devices			*/
/*									*/
/* And (among else), the following Flash media and cards:		*/
/*                                                                      */
/* - Intel Series-2+ PCMCIA cards                                       */
/*									*/
/*----------------------------------------------------------------------*/



#include "flflash.h"
#include "backgrnd.h"


/* JEDEC ids for this MTD */
#define I28F016_FLASH	0x89a0

#define SETUP_ERASE	0x2020
#define SETUP_WRITE	0x4040
#define CLEAR_STATUS	0x5050
#define READ_STATUS	0x7070
#define READ_ID 	0x9090
#define SUSPEND_ERASE	0xb0b0
#define CONFIRM_ERASE	0xd0d0
#define	RESUME_ERASE	0xd0d0
#define READ_ARRAY	0xffff

#define LOAD_PAGE_BUFFER 0xe0e0
#define WRITE_PAGE_BUFFER 0x0c0c
#define READ_EXTENDED_REGS 0x7171

#define	WSM_VPP_ERROR	0x08
#define WSM_ERROR	0x38
#define WSM_SUSPENDED	0x40
#define WSM_READY	0x80

#define GSR_ERROR	0x20

#define both(word)	(vol.interleaving == 1 ? (word) : (word) & ((word) >> 8))
#define any(word)	((word) | ((word) >> 8))

/*----------------------------------------------------------------------*/
/*                    i 2 8 f 0 1 6 W o r d S i z e			*/
/*									*/
/* Identify the card size for an Intel 28F016 word-mode Flash array.	*/
/* Sets the value of vol.noOfChips.					*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 = OK, otherwise failed (invalid Flash array)*/
/*----------------------------------------------------------------------*/

static FLStatus i28f016WordSize(FLFlash vol)
{
  FlashWPTR flashPtr = (FlashWPTR) flMap(vol.socket,0);
  flashPtr[0] = CLEAR_STATUS;
  flashPtr[0] = READ_ID;
  /* We leave the first chip in Read ID mode, so that we can		*/
  /* discover an address wraparound.					*/

  for (vol.noOfChips = 1;	/* Scan the chips */
       vol.noOfChips < 2000;  /* Big enough ? */
       vol.noOfChips++) {
    flashPtr = (FlashWPTR) flMap(vol.socket,vol.noOfChips * vol.chipSize);

    if (flashPtr[0] == 0x0089 && flashPtr[1] == 0x66a0)
      break;	  /* We've wrapped around to the first chip ! */

    flashPtr[0] = READ_ID;
    if (!(flashPtr[0] == 0x0089 && flashPtr[1] == 0x66a0))
      break;
    flashPtr[0] = CLEAR_STATUS;
    flashPtr[0] = READ_ARRAY;
  }

  flashPtr = (FlashWPTR) flMap(vol.socket,0);
  flashPtr[0] = READ_ARRAY;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                      i 2 8 f 0 1 6 W r i t e				*/
/*									*/
/* Write a block of bytes to Flash					*/
/*									*/
/* This routine will be registered as the MTD flash.write routine	*/
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

static FLStatus i28f016Write(FLFlash vol,
			   CardAddress address,
			   const void FAR1 *buffer,
			   int length,
			   FLBoolean overwrite)
{
  /* Set timeout ot 5 seconds from now */
  unsigned long writeTimeout = flMsecCounter + 5000;

  FLStatus status = flOK;
  FlashWPTR flashPtr;
  int maxLength, i, from;

  if (flWriteProtected(vol.socket))
    return flWriteProtect;

  if ((length & 1) || (address & 1))	/* Only write words on word-boundary */
    return flBadParameter;

#ifdef SOCKET_12_VOLTS
  checkStatus(flNeedVpp(vol.socket));
#endif

  maxLength = 256 * vol.interleaving;
  for (from = 0; from < length && status == flOK; from += maxLength) {
    FlashWPTR currPtr;
    unsigned lengthWord;
    int tailBytes;
    int thisLength = length - from;

    if (thisLength > maxLength)
      thisLength = maxLength;
    lengthWord = (thisLength + vol.interleaving - 1) /
		 (vol.interleaving == 1 ? 2 : vol.interleaving) - 1;
    if (vol.interleaving != 1)
      lengthWord |= (lengthWord << 8);
    flashPtr = (FlashWPTR) flMap(vol.socket,address + from);

    tailBytes = ((thisLength - 1) & (vol.interleaving - 1)) + 1;
    for (i = 0, currPtr = flashPtr;
	 i < vol.interleaving && i < thisLength;
	 i += 2, currPtr++) {
      *currPtr = LOAD_PAGE_BUFFER;
      *currPtr = i < tailBytes ? lengthWord : lengthWord - 1;
      *currPtr = 0;
    }

    tffscpyWords((unsigned long FAR0 *) flashPtr,
	    (const char FAR1 *) buffer + from,
	    thisLength);

    for (i = 0, currPtr = flashPtr;
	 i < vol.interleaving && i < thisLength;
	 i += 2, currPtr++) {
      *currPtr = WRITE_PAGE_BUFFER;
      if (!((address + from + i) & vol.interleaving)) {
	/* Even address */
	*currPtr = lengthWord;
	*currPtr = 0;
      }
      else {
	/* Odd address */
	*currPtr = 0;
	*currPtr = lengthWord;
      }

    }

    /* map to the GSR & BSR */
    flashPtr = (FlashWPTR) flMap(vol.socket,
			       ((address + from) & -vol.erasableBlockSize) +
			       4 * vol.interleaving);

    for (i = 0, currPtr = flashPtr;
	 i < vol.interleaving && i < thisLength;
	 i += 2, currPtr++) {
      *currPtr = READ_EXTENDED_REGS;
      while (!(both(*currPtr) & WSM_READY) && flMsecCounter < writeTimeout)
	;
      if ((any(*currPtr) & GSR_ERROR) || !(both(*currPtr) & WSM_READY)) {
      #ifdef DEBUG_PRINT
	DEBUG_PRINT("Debug: write failed for 16-bit Intel media.\n");
      #endif
	status = flWriteFault;
	*currPtr = CLEAR_STATUS;
      }
      *currPtr = READ_ARRAY;
    }
  }

#ifdef SOCKET_12_VOLTS
  flDontNeedVpp(vol.socket);
#endif

  flashPtr = (FlashWPTR) flMap(vol.socket, address);
  /* verify the data */
  if (status == flOK && tffscmpWords((void FAR0 *) flashPtr, (void FAR1 *) buffer,length)) {
  #ifdef DEBUG_PRINT
    DEBUG_PRINT("Debug: write failed for 16-bit Intel media in verification.\n");
  #endif
    status = flWriteFault;
  }

  return status;
}


/*----------------------------------------------------------------------*/
/*                      i 2 8 f 0 1 6 E r a s e				*/
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

static FLStatus i28f016Erase(FLFlash vol,
			   int firstErasableBlock,
			   int numOfErasableBlocks)
{
  FLStatus status = flOK;	/* unless proven otherwise */
  int iBlock;

  if (flWriteProtected(vol.socket))
    return flWriteProtect;

#ifdef SOCKET_12_VOLTS
  checkStatus(flNeedVpp(vol.socket));
#endif

  for (iBlock = 0; iBlock < numOfErasableBlocks && status == flOK; iBlock++) {
    FlashWPTR currPtr;
    int i;
    FLBoolean finished;

    FlashWPTR flashPtr = (FlashWPTR)
	   flMap(vol.socket,(firstErasableBlock + iBlock) * vol.erasableBlockSize);

    for (i = 0, currPtr = flashPtr;
	 i < vol.interleaving;
	 i += 2, currPtr++) {
      *currPtr = SETUP_ERASE;
      *currPtr = CONFIRM_ERASE;
    }

    do {
#ifdef BACKGROUND
      while (flForeground(1) == BG_SUSPEND) {		/* suspend */
	for (i = 0, currPtr = flashPtr;
	     i < vol.interleaving;
	     i += 2, currPtr++) {
	  *currPtr = READ_STATUS;
	  if (!(both(*currPtr) & WSM_READY)) {
	    *currPtr = SUSPEND_ERASE;
	    *currPtr = READ_STATUS;
	    while (!(both(*currPtr) & WSM_READY))
	      ;
	  }
	  *currPtr = READ_ARRAY;
	}
      }
#endif
      finished = TRUE;
      for (i = 0, currPtr = flashPtr;
	   i < vol.interleaving;
	   i += 2, currPtr++) {
	*currPtr = READ_STATUS;

	if (any(*currPtr) & WSM_SUSPENDED) {
	  *currPtr = RESUME_ERASE;
	  finished = FALSE;
	}
	else if (!(both(*currPtr) & WSM_READY))
	  finished = FALSE;
	else {
	  if (any(*currPtr) & WSM_ERROR) {
	  #ifdef DEBUG_PRINT
	    DEBUG_PRINT("Debug: erase failed for 16-bit Intel media.\n");
	  #endif
	    status = (any(*currPtr) & WSM_VPP_ERROR) ? flVppFailure : flWriteFault;
	    *currPtr = CLEAR_STATUS;
	  }
	  *currPtr = READ_ARRAY;
	}
      }
    } while (!finished);
  }

#ifdef SOCKET_12_VOLTS
  flDontNeedVpp(vol.socket);
#endif

  return status;
}


/*----------------------------------------------------------------------*/
/*                     i 2 8 f 0 1 6 I d e n t i f y			*/
/*									*/
/* Identifies media based on Intel 28F016 and registers as an MTD for	*/
/* such.								*/
/*									*/
/* This routine will be placed on the MTD list in custom.h. It must be	*/
/* an extern routine.							*/
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

FLStatus i28f016Identify(FLFlash vol)
{
  FlashWPTR flashPtr;

#ifdef DEBUG_PRINT
  DEBUG_PRINT("Debug: entering 16-bit Intel media identification routine.\n");
#endif

  flSetWindowBusWidth(vol.socket,16);/* use 16-bits */
  flSetWindowSpeed(vol.socket,120);  /* 120 nsec. */
  flSetWindowSize(vol.socket,2);	/* 8 KBytes */

  flashPtr = (FlashWPTR) flMap(vol.socket,0);

  vol.noOfChips = 0;
  flashPtr[0] = READ_ID;
  if (flashPtr[0] == 0x0089 && flashPtr[1] == 0x66a0) {
    /* Word mode */
    vol.type = I28F016_FLASH;
    vol.interleaving = 1;
    flashPtr[0] = READ_ARRAY;
  }
  else {
    /* Use standard identification routine to detect byte-mode */
    flIntelIdentify(&vol, NULL,0);
    if (vol.interleaving == 1)
      vol.type = NOT_FLASH;	/* We cannot handle byte-mode interleaving-1 */
  }

  if (vol.type == I28F016_FLASH) {
    vol.chipSize = 0x200000L;
    vol.erasableBlockSize = 0x10000L * vol.interleaving;
    checkStatus(vol.interleaving == 1 ?
		i28f016WordSize(&vol) :
		flIntelSize(&vol, NULL,0));

    /* Register our flash handlers */
    vol.write = i28f016Write;
    vol.erase = i28f016Erase;

  #ifdef DEBUG_PRINT
    DEBUG_PRINT("Debug: identified 16-bit Intel media.\n");
  #endif
    return flOK;
  }
  else {
  #ifdef DEBUG_PRINT
    DEBUG_PRINT("Debug: failed to identify 16-bit Intel media.\n");
  #endif
    return flUnknownMedia; 	/* not ours */
  }
}


#if	FALSE
/*----------------------------------------------------------------------*/
/*                   f l R e g i s t e r I 2 8 F 0 1 6			*/
/*									*/
/* Registers this MTD for use						*/
/*									*/
/* Parameters:                                                          */
/*	None								*/
/*                                                                      */
/* Returns:								*/
/*	FLStatus	: 0 on success, otherwise failure		*/
/*----------------------------------------------------------------------*/

FLStatus flRegisterI28F016(void)
{
  if (noOfMTDs >= MTDS)
    return flTooManyComponents;

  mtdTable[noOfMTDs++] = i28f016Identify;

  return flOK;
}
#endif	/* FALSE */
