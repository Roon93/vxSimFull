/* amd29LvMtd.c - TrueFFS MTD for AMD AM29LV devices */
 
/* Copyright 2000 Wind River, Inc. */

/*
modification history
--------------------
01h,27oct01,mil  Removed inline eieio and replaced with CACHE_PIPE_FLUSH().
01g,11oct01,mil  Merged from post T2.1 release into T2.2.
01f,22aug01,mil  Decoupled this MTD from BSP by using tffsFlashBaseAdrs from
                 sysTffs.c.
01e,21aug01,mil  Created in mcpn765 for TFFS until merged back to
                 target/src/drv/tffs/amdmtd.c.
01d,06aug00,srr  Modified amd29lv323CT support for 16 MB.
01c,31jul00,cak  Renamed from mv2400mtd to amd29lvmtd.
                 Added support for the amd29lv323. 
01b,09may00,add  Fixed erase bug (last 4 sectors are odd sizes)
01a,21apr00,add  Created.
*/

/*
DESCRIPTION
This module implements an TrueFFS MTD for the AMD AM29LV160D and
AM29LV323 flash devices.
 
*/

/* includes */

#include <vxWorks.h>
#include <taskLib.h>
#include <logLib.h>
#include <stdio.h>
#include <cacheLib.h>

#include "tffs/flflash.h"
#include "tffs/backgrnd.h"

IMPORT int sysClkRateGet();

/* defines */

#define AMD29LV_MTD_SECTOR_SIZE         (0x40000)
#define AMD29LV_160_CHIP_SIZE           (0x800000)	/*  8MB */
#define AMD29LV_160_LAST_SECTOR_NUM     (AMD29LV_160_CHIP_SIZE / AMD29LV_MTD_SECTOR_SIZE - 1)

#define AMD29LV_323_CHIP_SIZE           (0x1000000)	/* 16MB */
#define AMD29LV_323_LAST_SECTOR_SIZE    (0x8000)
#define AMD29LV_323_LAST_SECTOR_NUM     (AMD29LV_323_CHIP_SIZE / AMD29LV_MTD_SECTOR_SIZE - 1)

#define AMD29LV_MTD_CHIP_CNT            (1)
#define AMD29LV_MTD_INTERLEAVE          (1)

#define DEBUG_READ     0x00000001
#define DEBUG_WRITE    0x00000002
#define DEBUG_PROGRAM  0x00000004
#define DEBUG_ERASE    0x00000008
#define DEBUG_ID       0x00000010
#define DEBUG_MAP      0x00000020
#define DEBUG_PROG32   0x00000040
#define DEBUG_ALWAYS   0xffffffff

#define DEBUG
#ifdef  DEBUG
    LOCAL UINT32 debug;
    #define DEBUG_PRINT(mask, string) \
                if ((debug & mask) || (mask == DEBUG_ALWAYS)) \
                printf string
#else
    #define DEBUG_PRINT(mask, string)
#endif

/* local routines */

LOCAL FLStatus amd29lvSectorRangeErase(FLFlash* pVol, int, int);

LOCAL FLStatus amd29lvProgram(FLFlash*, CardAddress, const void FAR1*, int,
                             FLBoolean);

LOCAL void FAR0* amd29lvMap(FLFlash*, CardAddress, int);

LOCAL void flashReset(FLFlash*, BOOL);
LOCAL void flashIdGet(FLFlash*, UINT16*, UINT16*);
LOCAL void flashUnlock(FLFlash*, BOOL);
LOCAL STATUS flashProgram32Bits(FLFlash*, volatile UINT32*, UINT32, BOOL);
LOCAL void flashRegWrite32Bits(FLFlash*, UINT32, UINT32, BOOL);
LOCAL UINT16 flashRegRead16Bits(FLFlash*, UINT32, BOOL);
LOCAL STATUS flashHalfSectorErase(FLFlash*, int, BOOL);


/******************************************************************************
*
* amd29lvMTDIdentify - MTD identify routine (see TrueFFS Programmer's Guide)
*
* RETURNS: FLStatus
*
*/

FLStatus amd29lvMTDIdentify
    (
    FLFlash* pVol
    )
    {
    UINT16 manCode;
    UINT16 devCode;

    flashIdGet(pVol, &manCode, &devCode);

    if (manCode != 0x0001)
        {
        DEBUG_PRINT(DEBUG_ALWAYS,
                    ("amd29lvMTDIdentify Manufacturer unknown: 0x%02x\n",
                    manCode));
        return(flUnknownMedia);
        }

    if (devCode == 0x22C4)		/* amd29LV160BT */
	{
	pVol->type = 0x01C4;
	pVol->erasableBlockSize = AMD29LV_MTD_SECTOR_SIZE;
	pVol->chipSize = AMD29LV_160_CHIP_SIZE;
	pVol->noOfChips = AMD29LV_MTD_CHIP_CNT;
	pVol->interleaving = AMD29LV_MTD_INTERLEAVE;
	pVol->write = amd29lvProgram;
	pVol->erase = amd29lvSectorRangeErase;
	pVol->map = amd29lvMap;
	}
    else if (devCode == 0x2250)		/* amd29LV323CT */
	{
	pVol->type = 0x0150;
	pVol->erasableBlockSize = AMD29LV_MTD_SECTOR_SIZE;
	pVol->chipSize = AMD29LV_323_CHIP_SIZE;
	pVol->noOfChips = AMD29LV_MTD_CHIP_CNT;
	pVol->interleaving = AMD29LV_MTD_INTERLEAVE;
	pVol->write = amd29lvProgram;
	pVol->erase = amd29lvSectorRangeErase;
	pVol->map = amd29lvMap;
	}
    else
        {
        DEBUG_PRINT(DEBUG_ALWAYS,
                    ("amd29lvMTDIdentify Device unknown: 0x%02x\n",
                    devCode));
        return(flUnknownMedia);
        }    

    DEBUG_PRINT(DEBUG_ID, ("amd29lvMTDIdentify succeeds!\n"));
    return(flOK);
    }

/******************************************************************************
*
* amd29lvProgram - MTD write routine (see TrueFFS Programmer's Guide)
*
* RETURNS: FLStatus
*
*/

LOCAL FLStatus amd29lvProgram
    (
    FLFlash*          pVol,
    CardAddress       address,
    const void FAR1*  buffer,
    int               length,
    FLBoolean         overwrite
    )

    {
    volatile UINT32* pFlash;
    UINT32* pBuffer;
    STATUS rc = OK;
    BOOL upper;
    int i;
    BOOL doFree = FALSE;

    DEBUG_PRINT(DEBUG_PROGRAM,
                ("Program: 0x%08x, 0x%08x, %d\n", (unsigned int) address,
                 length, overwrite));

    if (flWriteProtected(vol.socket))
        {
        return(flWriteProtect);
        }

    /* Check alignment */

    if (((address & 0x03) != 0) || (((UINT32) buffer) &  0x03))
        {
        DEBUG_PRINT(DEBUG_ALWAYS, ("amd29lvProgram: Alignment error\n"));
        return(flBadParameter);
        }

    if (overwrite && length == 2)
        {
        int sector;
        int offset;

        pFlash = (volatile UINT32*) pVol->map(pVol, address, length);

        pBuffer = (UINT32*) malloc(AMD29LV_MTD_SECTOR_SIZE);

        if (pBuffer == 0)
            {
            DEBUG_PRINT(DEBUG_ALWAYS, ("amd29lvProgram: No memory\n"));
            return(flBadParameter);
            }

        /* Determine sector and offset */

        sector = address / AMD29LV_MTD_SECTOR_SIZE;
        offset = address % AMD29LV_MTD_SECTOR_SIZE;

        DEBUG_PRINT(DEBUG_PROGRAM,("Overwrite sector: 0x%08x, offset: 0x%08x\n",
                    sector,offset));

        /* Get a pointer to the flash sector */

        pFlash = (volatile UINT32*) pVol->map(pVol,
                                              sector * AMD29LV_MTD_SECTOR_SIZE,
                                              AMD29LV_MTD_SECTOR_SIZE);


        /* Copy the sector from flash to memory */

        memcpy(pBuffer, (void*) pFlash, AMD29LV_MTD_SECTOR_SIZE);

        /* Overwrite the sector in memory */

       memcpy(((UINT8*) pBuffer) + offset, buffer, length);

        /* Erase sector */

        rc = amd29lvSectorRangeErase(pVol, sector, 1);
        if (rc != flOK)
            {
            free(pBuffer);
            return(rc);
            }

        length = AMD29LV_MTD_SECTOR_SIZE;
        doFree = TRUE;
        }

    else
        {
        if ((length & 0x03) != 0)
            {
            DEBUG_PRINT(DEBUG_ALWAYS, ("amd29lvProgram: length: %d\n", length));

            return(flBadParameter);
            }

        pBuffer = (UINT32*) buffer;
        pFlash = (volatile UINT32*) pVol->map(pVol, address, length);
        }

    /* Program 'length' bytes (4 bytes each iterations) */

    upper = ((((UINT32) pFlash) & 0x04) != 0);
    for (i = 0; i < (length / 4); i++, pFlash++, upper = !upper)
        {

        /* Don't bother programming if buffer data == format value */

        if (pBuffer[i] == 0xffffffff)
            continue;

        /* Program 32 bits */

        rc = flashProgram32Bits(pVol, pFlash, pBuffer[i], upper);
        if (rc != OK)
            break;
        }

    if (doFree)
        {
        free(pBuffer);
        }

    return((rc == OK) ? flOK : flTimedOut);

    }

/******************************************************************************
*
* amd29lvSectorRangeErase - MTD erase routine (see TrueFFS Programmer's Guide)
*
* RETURNS: FLStatus
*
*/

LOCAL FLStatus amd29lvSectorRangeErase
    (
    FLFlash* pVol,
    int sectorNum,
    int sectorCount
    )
    {
    int i;
    STATUS rc;

    /* Check for valid range */

    if (pVol->type == 0x1C4)			/* amd29LV160BT */
	{
	if (sectorNum + sectorCount >  AMD29LV_160_LAST_SECTOR_NUM + 1)
	    {
	    DEBUG_PRINT(DEBUG_ALWAYS, ("Invalid sector range: %d - %d\n",
	                sectorNum, sectorCount));
	    }

	/* Last sector is really 4 seperately erasable sectors */

	if (sectorNum + sectorCount == AMD29LV_160_LAST_SECTOR_NUM + 1)
	    {
	    sectorCount += 3;
	    }
	}
    else					 /* amd29LV323CT */
	{
	if (sectorNum + sectorCount >  AMD29LV_323_LAST_SECTOR_NUM + 1)
	    {
	    DEBUG_PRINT(DEBUG_ALWAYS, ("Invalid sector range: %d - %d\n",
	                sectorNum, sectorCount));
	    }

	/* Last sector is really 8 seperately erasable sectors */

	if (sectorNum + sectorCount == AMD29LV_323_LAST_SECTOR_NUM + 1)
	    {
	    sectorCount += 7;
	    }
	}

    for (i = 0; i < sectorCount; i++)
        {

        /* Erase lower half */

        rc = flashHalfSectorErase(pVol, sectorNum + i, FALSE);
        if (rc != OK)
            return(flTimedOut);

        /* Erase upper half */

        rc = flashHalfSectorErase(pVol, sectorNum + i, TRUE);
        if (rc != OK)
            return(flTimedOut);
        }
    return(flOK);
    }


/******************************************************************************
*
* amd29lvMap - MTD map routine (see TrueFFS Programmer's Guide)
*
* RETURNS: FLStatus
*
*/
LOCAL void FAR0* amd29lvMap
    (
    FLFlash* pVol,
    CardAddress address,
    int length
    )
    {
    UINT32 flashBaseAddr = (pVol->socket->window.baseAddress << 12);
    void FAR0* pFlash = (void FAR0*) (flashBaseAddr + address);
    DEBUG_PRINT(DEBUG_MAP, ("Mapping 0x%08x bytes at 0x%08x to %p\n", length,
                (unsigned int) address, pFlash));
    return(pFlash);
    }


/******************************************************************************
*
* flashProgram32Bits - Program 32 bits at 4 byte aligned address.
*
* RETURNS: OK or ERROR
*
*/

LOCAL STATUS flashProgram32Bits
    (
    FLFlash* pVol,
    volatile UINT32* pData,
    UINT32 data,
    BOOL upper
    )
    {
    BOOL programmed = FALSE;
    int timeout;

    flashUnlock(pVol, upper);
    flashRegWrite32Bits(pVol, 0x555, 0x00a000a0, upper);

    DEBUG_PRINT(DEBUG_PROG32, ("Programming 0x%08x to %p, upper = %d\n",
                data, pData, upper));

    *pData = data;
    CACHE_PIPE_FLUSH();

    for (timeout = flMsecCounter + 3000; flMsecCounter < timeout;)
        {
        if (*pData == data)
            {
            programmed = TRUE;
            break;
            }
        taskDelay(0);
        }

    if (!programmed)
        {
        DEBUG_PRINT(DEBUG_ALWAYS, ("Timeout\n"));
        return(ERROR);
        }

    return(OK);

    }

/******************************************************************************
*
* flashHalfSectorErase - Erase lower or upper half of sector.
*
* RETURNS: OK or ERROR
*
*/

LOCAL STATUS flashHalfSectorErase
    (
    FLFlash* pVol,
    int sectorNum,
    BOOL upper
    )
    {

    BOOL erased = FALSE;
    int timeout = sysClkRateGet() * 5;
    UINT32 offset;
    UINT32 size;
    volatile UINT32* pFlash;
    UINT32 sectorAddr;


    if (pVol->type == 0x1C4)			/* amd29LV160BT */
	{

	/* Sectors 31 - 34 are funky sizes */

	switch (sectorNum)
	    {
	    case 31:
	        offset = 0x7c0000;
	        size   = 0x20000;
	        sectorAddr = 0xf8000;
	        break;
	    case 32:
	        offset = 0x7e0000;
	        size   = 0x8000;
	        sectorAddr = 0xfc000;
	        break;
	    case 33:
	        offset = 0x7e8000;
	        size   = 0x8000;
	        sectorAddr = 0xfd000;
	        break;
	    case 34:
	        offset = 0x7f0000;
	        size   = 0x10000;
	        sectorAddr = 0xfe000;
	        break;
	    default:
	        offset = sectorNum * AMD29LV_MTD_SECTOR_SIZE;
	        size = AMD29LV_MTD_SECTOR_SIZE;
	        sectorAddr = (sectorNum << 15);
	    }
	}
    else					 /* amd29LV323CT */
	{

	/* Sectors 63 - 70 are 1/8th size of other sectors */

	if (sectorNum < AMD29LV_323_LAST_SECTOR_NUM)
	    {
	    offset     = sectorNum * AMD29LV_MTD_SECTOR_SIZE;
	    size       = AMD29LV_MTD_SECTOR_SIZE;
	    sectorAddr = (sectorNum << 15);
	    }
	else
	    {
	    offset  = AMD29LV_323_LAST_SECTOR_NUM * AMD29LV_MTD_SECTOR_SIZE;
	    offset += (sectorNum - AMD29LV_323_LAST_SECTOR_NUM) *
	              AMD29LV_323_LAST_SECTOR_SIZE;
	    size    = AMD29LV_323_LAST_SECTOR_SIZE;

	    sectorAddr  = (AMD29LV_323_LAST_SECTOR_NUM << 15);
	    sectorAddr += ((sectorNum - AMD29LV_323_LAST_SECTOR_NUM) << 12);
	    }
	}

    DEBUG_PRINT(DEBUG_ERASE, ("Erasing sector %d, 0x%02x, upper = %d\n",
                sectorNum, sectorAddr, upper));

    /* Erase the sector half */

    flashUnlock(pVol, upper);
    flashRegWrite32Bits(pVol, 0x555, 0x00800080, upper);
    flashUnlock(&vol, upper);
    flashRegWrite32Bits(pVol, sectorAddr, 0x00300030, upper);

    pFlash = (volatile UINT32*) pVol->map(pVol, offset, size);
    if (upper)
        pFlash++;

    /* Sector's  upper/lower half erased?  If not, return ERROR */


    for (timeout = flMsecCounter + 5000; flMsecCounter < timeout;)
        {
        if (*pFlash == 0xffffffff)
            {
            erased = TRUE;
            break;
            }
        }

    if (!erased)
        {
        DEBUG_PRINT(DEBUG_ALWAYS, ("Sector erase timeout, %p\n", pFlash));
        return(ERROR);
        }

    return(OK);
    }

/******************************************************************************
*
* flashRegWrite32Bits - Write 32 bits to 4 byte aligned address.
*
* RETURNS: N/A
*
*/
LOCAL void flashRegWrite32Bits
    (
    FLFlash* pVol,
    UINT32 addr,
    UINT32 data,
    BOOL   upper
    )
    {

    UINT32 flashBaseAddr = (pVol->socket->window.baseAddress << 12);

    /* Adjust addr for amd29LV323 */

    addr = flashBaseAddr + 8 * addr;
    if (upper)
        addr += 4;

    DEBUG_PRINT(DEBUG_WRITE, ("Writing 0x%08x to 0x%08x\n", data, addr));

    /* Write */

    *((volatile UINT32*) addr) = data;

    CACHE_PIPE_FLUSH();
    }

/******************************************************************************
*
* flashRegRead16Bits - Read 16 bits from 2 byte aligned address.
*
* RETURNS: data at specified address
*
*/
LOCAL UINT16 flashRegRead16Bits
    (
    FLFlash* pVol,
    UINT32 addr,
    BOOL   upper
    )
    {
    UINT16 data;
    UINT32 flashBaseAddr = (pVol->socket->window.baseAddress << 12);

    addr = flashBaseAddr + 8 * addr;
    if (upper)
        addr += 4;
    data = *((volatile UINT16*) addr);
    DEBUG_PRINT(DEBUG_READ, ("Read 0x%08x from 0x%08x\n", data, addr));
    CACHE_PIPE_FLUSH();
    return(data);
    }


/******************************************************************************
*
* flashIdGet - Get flash man. and device codes.
*
* RETURNS: N/A
*
*/

LOCAL void flashIdGet
    (
    FLFlash* pVol,
    UINT16* manCode,
    UINT16* devCode
    )
    {
    flashUnlock(pVol, FALSE);
    flashRegWrite32Bits(pVol, 0x555, 0x00900090, FALSE);
    *manCode = flashRegRead16Bits(pVol, 0x00, FALSE);
    *devCode = flashRegRead16Bits(pVol, 0x01, FALSE);
    flashReset(pVol, FALSE);
    }

/******************************************************************************
*
* flashUnlock - Write unlock sequence to upper or lower flash section.
*
* RETURNS: N/A
*
*/

LOCAL void flashUnlock
    (
    FLFlash* pVol,
    BOOL upper
    )
    {
    flashRegWrite32Bits(pVol, 0x555, 0x00aa00aa, upper);
    flashRegWrite32Bits(pVol, 0x2aa, 0x00550055, upper);
    }

/******************************************************************************
*
* flashReset - Write reset sequence to upper or lower flash section.
*
* RETURNS: N/A
*
*/

LOCAL void flashReset
    (
    FLFlash* pVol,
    BOOL upper
    )
    {
    flashRegWrite32Bits(pVol, 0, 0x00f000f0, upper);
    }

#define FLASH_TEST
#undef FLASH_TEST
#ifdef FLASH_TEST

#include "tffs/flsocket.h"

LOCAL FLFlash testVol;
LOCAL FLSocket testSocket;

/* Make sure this functions are NOT static (LOCAL) in sysTffs.c */

IMPORT void simmSetWindow (FLSocket vol);
IMPORT void simmSetMappingContext (FLSocket vol, unsigned page);
IMPORT FLBoolean simmWriteProtected (FLSocket vol);

/* Initialize */

LOCAL void FAR0 *flashMap(FLFlash vol, CardAddress address, int length)
{
  return flMap(vol.socket,address);
}

void fsinit()
    {
    FLStatus rc;

    testSocket.window.baseAddress = tffsFlashBaseAdrs >> 12;
    testSocket.window.currentPage = UNDEFINED_MAPPING;
    testSocket.setWindow = simmSetWindow;
    testSocket.setMappingContext = simmSetMappingContext;
    testSocket.setMappingContext = simmSetMappingContext;
    testSocket.writeProtected = simmWriteProtected;

    testVol.socket = &testSocket;
    testVol.map = flashMap;
    rc = amd29lvMTDIdentify(&testVol);
    if (rc != flOK)
        {
        printf("identify fails\n");
        }
    }

/* Show */

void fsshow()
    {
    printf("Type 0x%04x\n", testVol.type);
    printf("Eraseable block size 0x%08lx\n", testVol.erasableBlockSize);
    }

/* Erase */

STATUS fse
    (
    UINT8 sectorNum,
    UINT8 numSectors
    )
    {
    FLStatus rc;
    if (sectorNum <= 3)
        {
        printf("Sector contains bootrom!\n");
        return(ERROR);
        }
    rc = testVol.erase(&testVol, sectorNum, numSectors);
    return(rc);
    }

/* Program */

STATUS fprog
    (
    UINT32 offset,
    UINT32 bufSize,
    UINT8  pattern
    )
    {
    FLStatus rc;
    UINT32* pBuf;

    UINT32 sectorNum = offset / AMD29LV_MTD_SECTOR_SIZE;
    if (sectorNum <= 3)
        {
        printf("Sector contains bootrom!\n");
        return(ERROR);
        }

    if (bufSize == 0)
        bufSize = 4;

    if ((bufSize & 0x03) != 0)
        bufSize += 4 - (bufSize & 0x03);

    pBuf = memalign(4, bufSize);
    if (pBuf == 0)
        return(ERROR);

    memset(pBuf, pattern, bufSize);

    rc = testVol.write(&testVol, offset, pBuf, bufSize, FALSE);

    free(pBuf);

    return(rc == flOK ? OK : ERROR);
    }

#endif

