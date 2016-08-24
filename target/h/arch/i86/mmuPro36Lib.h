/* mmuPro36Lib.h - mmuPro36Lib header for i86. */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,12jun02,hdn  added arch specific VM library APIs' prototype
01d,06jun02,hdn  replaced PAGE_SIZE with PD_SIZE and PT_SIZE
01c,22aug01,hdn  moved GDT and GDT_ENTRIES to regsI86.h
01b,13may98,hdn  added _ASMLANGUAGE macro. 
01a,11apr97,hdn  written.
*/

#ifndef __INCmmuPro36Libh
#define __INCmmuPro36Libh

#ifdef __cplusplus
extern "C" {
#endif

#include "arch/i86/pentiumLib.h"


#define PD_SIZE			0x1000
#define PT_SIZE			0x1000
#define PAGE_BLOCK_SIZE		0x200000
#define PAGE_SIZE_4KB		0x1000
#define PAGE_SIZE_2MB		0x200000

#define DIR_PTR_BITS		0xc0000000
#define DIR_BITS		0x3fe00000
#define TBL_BITS		0x001ff000
#define OFFSET_BITS_4KB		0x00000fff
#define OFFSET_BITS_2MB		0x001fffff
#define DIR_PTR_INDEX		30
#define DIR_INDEX		21
#define TBL_INDEX		12
#define PTE_TO_ADDR_4KB		0xfffff000
#define PTE_TO_ADDR_2MB		0xffe00000
#define PDP_TO_ADDR		0xfffff000
#define ADDR_TO_PAGE		12
#define ADDR_TO_PAGEBASE	0xffe00000

#ifndef	_ASMLANGUAGE

typedef struct
    {
    unsigned present:1;
    unsigned rw:1;
    unsigned us:1;
    unsigned pwt:1;
    unsigned pcd:1;
    unsigned access:1;
    unsigned dirty:1;
    unsigned pagesize:1;
    unsigned global:1;
    unsigned avail:3;
    unsigned page:20;
    unsigned page36:4;
    unsigned reserved:28;
    } PTE_FIELD;

typedef union pte
    {
    PTE_FIELD field;
    UINT bits[2];
    } PTE;

typedef struct mmuTransTblStruct
    {
    PTE * pDirectoryTable;	/* pointer to dir ptr tbl */
    } MMU_TRANS_TBL;


#define MMU_STATE_MASK_VALID		0x001
#define MMU_STATE_MASK_WRITABLE		0x002
#define MMU_STATE_MASK_CACHEABLE	0x018
#define MMU_STATE_MASK_WBACK		0x008
#define MMU_STATE_MASK_GLOBAL		0x100

#define MMU_STATE_VALID			0x001
#define MMU_STATE_VALID_NOT		0x000
#define MMU_STATE_WRITABLE		0x002
#define MMU_STATE_WRITABLE_NOT		0x000
#define MMU_STATE_CACHEABLE		0x008
#define MMU_STATE_CACHEABLE_NOT		0x018
#define MMU_STATE_WBACK			0x000
#define MMU_STATE_WBACK_NOT		0x008
#define MMU_STATE_GLOBAL		0x100
#define MMU_STATE_GLOBAL_NOT		0x000

#define MMU_STATE_CACHEABLE_WT		0x008


/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

extern STATUS	mmuPro36LibInit (int pageSize);
extern STATUS	mmuPro36Enable (BOOL enable);
extern void	mmuPro36On ();
extern void	mmuPro36Off ();
extern void	mmuPro36TLBFlush ();
extern void	mmuPro36PdbrSet (MMU_TRANS_TBL *transTbl);
extern MMU_TRANS_TBL *mmuPro36PdbrGet ();
extern STATUS	mmuPro36PageMap (MMU_TRANS_TBL * transTbl,
		    void * virtAddr, LL_INT physPage);
extern STATUS	mmuPro36Translate (MMU_TRANS_TBL * transTbl,
		    void * virtAddr, LL_INT * physAddr);

extern void	vmBaseArch36LibInit (void);
extern STATUS	vmBaseArch36Map (void * virtAddr, LL_INT physAddr,
		    UINT32 stateMask, UINT32 state, UINT32 len);
extern STATUS	vmBaseArch36Translate (void * virtAddr, LL_INT * physAddr);
extern void	vmArch36LibInit (void);
extern STATUS	vmArch36Map (VM_CONTEXT_ID context, void * virtAddr,
    		    LL_INT physAddr, UINT32 stateMask, UINT32 state, UINT32 len);
extern STATUS	vmArch36Translate (VM_CONTEXT_ID context, void * virtAddr,
		    LL_INT * physAddr);

#else   /* __STDC__ */

extern STATUS	mmuPro36LibInit ();
extern STATUS	mmuPro36Enable ();
extern void	mmuPro36On ();
extern void	mmuPro36Off ();
extern void	mmuPro36TLBFlush ();
extern void	mmuPro36PdbrSet ();
extern MMU_TRANS_TBL *mmuPro36PdbrGet ();
extern STATUS	mmuPro36PageMap ();
extern STATUS	mmuPro36Translate ();

extern void	vmBaseArch36LibInit ();
extern STATUS	vmBaseArch36PageMap ();
extern STATUS	vmBaseArch36Translate ();
extern void	vmArch36LibInit ();
extern STATUS	vmArch36Map ();
extern STATUS	vmArch36Translate ();

#endif  /* __STDC__ */

#endif	/* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __INCmmuPro36Libh */
