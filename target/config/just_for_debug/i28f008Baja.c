/*
 * $Log:   V:/i28f008.c_v  $
 *
 *    Rev 1.16   08 Jan 1998 16:37:00   Hdei
 * written based on i28f008.c
 *
 *    Rev 1.16   06 Oct 1997 18:37:30   ANDRY
 * no COBUX
 *
 *    Rev 1.15   05 Oct 1997 15:32:40   ANDRY
 * COBUX (16-bit Motorola M68360 board)
 *
 *    Rev 1.14   05 Oct 1997 14:35:36   ANDRY
 * flNeedVpp() and flDontNeedVpp() are under #ifdef SOCKET_12_VOLTS
 *
 *    Rev 1.13   10 Sep 1997 16:18:10   danig
 * Got rid of generic names
 *
 *    Rev 1.12   04 Sep 1997 18:47:20   danig
 * Debug messages
 *
 *    Rev 1.11   31 Aug 1997 15:06:40   danig
 * Registration routine return status
 *
 *    Rev 1.10   24 Jul 1997 17:52:30   amirban
 * FAR to FAR0
 *
 *    Rev 1.9   21 Jul 1997 14:44:06   danig
 * No parallelLimit
 *
 *    Rev 1.8   20 Jul 1997 17:17:00   amirban
 * No watchDogTimer
 *
 *    Rev 1.7   07 Jul 1997 15:22:06   amirban
 * Ver 2.0
 *
 *    Rev 1.6   15 Apr 1997 19:16:40   danig
 * Pointer conversions.
 *
 *    Rev 1.5   29 Aug 1996 14:17:48   amirban
 * Warnings
 *
 *    Rev 1.4   18 Aug 1996 13:48:44   amirban
 * Comments
 *
 *    Rev 1.3   31 Jul 1996 14:31:10   amirban
 * Background stuff
 *
 *    Rev 1.2   04 Jul 1996 18:20:06   amirban
 * New flag field
 *
 *    Rev 1.1   03 Jun 1996 16:28:58   amirban
 * Cobra additions
 *
 *    Rev 1.0   20 Mar 1996 13:33:06   amirban
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
/* - Intel 28F008/Cobra 8-mbit devices					*/
/* - Intel 28F016SA/28016SV/Cobra 16-mbit devices (byte-mode operation)	*/
/*									*/
/* And (among else), the following Flash media and cards:		*/
/*                                                                      */
/* - Intel Series-2 PCMCIA cards                                        */
/* - Intel Series-2+ PCMCIA cards                                       */
/* - M-Systems ISA/Tiny/PC-104 Flash Disks                              */
/* - M-Systems NOR PCMCIA cards                                         */
/* - Intel Value-100 cards                                              */
/*									*/
/*----------------------------------------------------------------------*/


#include "flflash.h"
#include "backgrnd.h"

#define flash (*pFlash)

#define SETUP_ERASE	0x20
#define SETUP_WRITE	0x40
#define CLEAR_STATUS	0x50
#define READ_STATUS	0x70
#define READ_ID 	0x90
#define SUSPEND_ERASE	0xb0
#define CONFIRM_ERASE	0xd0
#define RESUME_ERASE	0xd0
#define READ_ARRAY	0xff

#define WSM_ERROR	0x38
#define	WSM_VPP_ERROR	0x08
#define WSM_SUSPENDED	0x40
#define WSM_READY	0x80

/* JEDEC ids for this MTD */
#define	I28F008_FLASH  	0x89a2
#define I28F016_FLASH	0x89a0
#define	COBRA004_FLASH	0x89a7
#define	COBRA008_FLASH	0x89a6
#define	COBRA016_FLASH	0x89aa

#define	MOBILE_MAX_INLV_4 0x8989
#define	LDP_1MB_IN_16BIT_MODE 0x89ff

/* Definition of MTD specific vol.flags bits: */

#define	NO_12VOLTS		0x100	/* Card does not need 12 Volts Vpp */

/* MIPS hkbaja47 specific functions */

extern int sysTffsByteWrite ();	/* flash mem byte write */


/*----------------------------------------------------------------------*/
/*                      i 2 8 f 0 0 8 W r i t e				*/
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

static FLStatus i28f008Write(FLFlash vol,
			   CardAddress address,
			   const void FAR1 *buffer,
			   int length,
			   FLBoolean overwrite)
{
  /* Set timeout ot 5 seconds from now */
  unsigned long writeTimeout = flMsecCounter + 5000;

  FLStatus status;
  int cLength;
  FlashPTR flashPtr;
  FlashPTR alignPtr;		/* Long word aligned pointer */

  if (flWriteProtected(vol.socket))
    return flWriteProtect;

#ifdef SOCKET_12_VOLTS
  if (!(vol.flags & NO_12VOLTS))
    checkStatus(flNeedVpp(vol.socket));
#endif

  flashPtr = (FlashPTR) vol.map(&vol, address,length);
  cLength = length;

#ifdef __cplusplus
    #define bFlashPtr  flashPtr
    #define bBuffer ((const unsigned char FAR1 * &) buffer)
#else
    #define bFlashPtr  flashPtr
    #define bBuffer ((const unsigned char FAR1 *) buffer)
#endif

    while (cLength >= 1) {
      sysTffsByteWrite (bFlashPtr, *bBuffer);	/* byte write */
      alignPtr = (FlashPTR)((int)bFlashPtr & ~3);
      cLength--;
      buffer = (unsigned char *)buffer + 1; /* bBuffer++; */
      bFlashPtr++;
      while (!(*alignPtr & WSM_READY) && flMsecCounter < writeTimeout)
	;

#if	FALSE
      *bFlashPtr = SETUP_WRITE;
      *bFlashPtr = *bBuffer;
      cLength--;
      bBuffer++;
      bFlashPtr++;
      while (!(bFlashPtr[-1] & WSM_READY) && flMsecCounter < writeTimeout)
	;
#endif	/* FALSE */
    }

  flashPtr -= length;
  buffer = (unsigned char *)buffer - length; /* bBuffer -= length; */

  status = flOK;
  alignPtr = (FlashPTR)((int)flashPtr & ~3);
    if (*alignPtr & WSM_ERROR) {
    #ifdef DEBUG_PRINT
      DEBUG_PRINT("Debug: write failed for 8-bit Intel media.\n");
    #endif
      status = (*alignPtr & WSM_VPP_ERROR) ? flVppFailure : flWriteFault;
      *alignPtr = CLEAR_STATUS;
    }
    *alignPtr = READ_ARRAY;

#if	FALSE
  status = flOK;
  for (i = 0; i < vol.interleaving && i < length; i++) {
    if (flashPtr[i] & WSM_ERROR) {
    #ifdef DEBUG_PRINT
      DEBUG_PRINT("Debug: write failed for 8-bit Intel media.\n");
    #endif
      status = (flashPtr[i] & WSM_VPP_ERROR) ? flVppFailure : flWriteFault;
      flashPtr[i] = CLEAR_STATUS;
    }
    flashPtr[i] = READ_ARRAY;
  }
#endif	/* FALSE */

#ifdef SOCKET_12_VOLTS
  if (!(vol.flags & NO_12VOLTS))
    flDontNeedVpp(vol.socket);
#endif

  /* we need this to switch to the read window */
  flashPtr = (FlashPTR) vol.map(&vol, address,length);	/* ADDED */

  /* verify the data */
  if (status == flOK && tffscmp((void FAR0 *) flashPtr,buffer,length)) {
  #ifdef DEBUG_PRINT
    DEBUG_PRINT("Debug: write failed for 8-bit Intel media in verification.\n");
  #endif
    status = flWriteFault;
  }

  return status;
}


/*----------------------------------------------------------------------*/
/*                      i 2 8 f 0 0 8 E r a s e				*/
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

static FLStatus i28f008Erase(FLFlash vol,
			   int firstErasableBlock,
			   int numOfErasableBlocks)
{
  int iBlock;

  FLStatus status = flOK;	/* unless proven otherwise */

  if (flWriteProtected(vol.socket))
    return flWriteProtect;

#ifdef SOCKET_12_VOLTS
  if (!(vol.flags & NO_12VOLTS))
    checkStatus(flNeedVpp(vol.socket));
#endif

  for (iBlock = 0; iBlock < numOfErasableBlocks && status == flOK; iBlock++) {
    int j;
    FLBoolean finished;

    FlashPTR flashPtr = (FlashPTR)
	  vol.map(&vol,
		    (firstErasableBlock + iBlock) * vol.erasableBlockSize,
		    vol.interleaving);

    for (j = 0; j < vol.interleaving; j++) {
      flashPtr[j] = SETUP_ERASE;	flDelayLoop (2);
      flashPtr[j] = CONFIRM_ERASE;	flDelayLoop (2);
    }

    do {
#ifdef BACKGROUND
      while (flForeground(1) == BG_SUSPEND) {		/* suspend */
	for (j = 0; j < vol.interleaving; j++) {
	  flashPtr[j] = READ_STATUS;
	  if (!(flashPtr[j] & WSM_READY)) {
	    flashPtr[j] = SUSPEND_ERASE;
	    flashPtr[j] = READ_STATUS;
	    while (!(flashPtr[j] & WSM_READY))
	      ;
	  }
	  flashPtr[j] = READ_ARRAY;
	}
      }
#endif
      finished = TRUE;
      for (j = 0; j < vol.interleaving; j++) {
	flashPtr[j] = READ_STATUS;
	if (flashPtr[j] & WSM_SUSPENDED) {
	  flashPtr[j] = RESUME_ERASE;
	  finished = FALSE;
	}
	else if (!(flashPtr[j] & WSM_READY))
	  finished = FALSE;
	else {
	  if (flashPtr[j] & WSM_ERROR) {
	  #ifdef DEBUG_PRINT
	    DEBUG_PRINT("Debug: erase failed for 8-bit Intel media.\n");
	  #endif
	    status = (flashPtr[j] & WSM_VPP_ERROR) ? flVppFailure : flWriteFault;
	  #ifdef DEBUG_PRINT
	    DEBUG_PRINT("Debug: flash status = 0x%x\n", flashPtr[j]);
	  #endif
	    flashPtr[j] = CLEAR_STATUS;
	  #ifdef DEBUG_PRINT
	    DEBUG_PRINT("Debug: erase failed status = 0x%x\n", status);
	  #endif
	  }
	  flashPtr[j] = READ_ARRAY;
	}
      }
    } while (!finished);
  } /* block loop */

#ifdef SOCKET_12_VOLTS
  if (!(vol.flags & NO_12VOLTS))
    flDontNeedVpp(vol.socket);
#endif

  return status;
}


/*----------------------------------------------------------------------*/
/*                     i 2 8 f 0 0 8 I d e n t i f y			*/
/*									*/
/* Identifies media based on Intel 28F008 and Intel 28F016 and		*/
/* registers as an MTD for such                                         */
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

FLStatus i28f008BajaIdentify(FLFlash vol)
{
  int iChip;

  CardAddress idOffset = 0;

#ifdef DEBUG_PRINT
  DEBUG_PRINT("Debug: entering 8-bit Intel media identification routine.\n");
#endif

  flSetWindowBusWidth(vol.socket, 8);/* use 16-bits */
  flSetWindowSpeed(vol.socket, 120);  /* 120 nsec. */
  flSetWindowSize(vol.socket, 2);	/* 8 KBytes */

  vol.interleaving = 1;
  vol.type = I28F008_FLASH;

  vol.chipSize = 0x100000l;
  vol.chipSize = flFitInSocketWindow (vol.chipSize, vol.interleaving, vol.socket->window.size);

  vol.erasableBlockSize = 0x10000l * vol.interleaving;

  checkStatus(flIntelSize(&vol, NULL,idOffset));

  if (vol.type == MOBILE_MAX_INLV_4)
    vol.type = I28F008_FLASH;

  for (iChip = 0; iChip < vol.noOfChips; iChip += vol.interleaving) {
    int i;
    FlashPTR flashPtr;

    flNeedVpp(vol.socket);        /* ADDED, first thing to do */
    flashPtr = (FlashPTR)
            vol.map(&vol,iChip * vol.chipSize,vol.interleaving);

    for (i = 0; i < vol.interleaving; i++)
      flashPtr[i] = CLEAR_STATUS;
    
    flDontNeedVpp(vol.socket);   /* ADDED, last thing to do */
  }

  /* Register our flash handlers */
  vol.write = i28f008Write;
  vol.erase = i28f008Erase;

#ifdef DEBUG_PRINT
  DEBUG_PRINT("Debug: identified 8-bit Intel media.\n");
#endif

  return flOK;
}


#if	FALSE
/*----------------------------------------------------------------------*/
/*                   f l R e g i s t e r I 2 8 F 0 0 8			*/
/*									*/
/* Registers this MTD for use						*/
/*									*/
/* Parameters:                                                          */
/*	None								*/
/*                                                                      */
/* Returns:								*/
/*	FLStatus	: 0 on success, otherwise failure		*/
/*----------------------------------------------------------------------*/

FLStatus flRegisterI28F008(void)
{
  if (noOfMTDs >= MTDS)
    return flTooManyComponents;

  mtdTable[noOfMTDs++] = i28f008Identify;

  return flOK;
}
#endif	/* FALSE */
