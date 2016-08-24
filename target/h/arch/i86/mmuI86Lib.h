/* mmuI86Lib.h - mmuI86Lib header for i86. */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,12jun02,hdn  added arch specific VM library APIs' prototype
01e,22aug01,hdn  moved GDT and GDT_ENTRIES to regsI86.h
01d,13may98,hdn  added PAGE_SIZE_XXX and _ASMLANGUAGE macros. 
01c,07jan95,hdn  added GDT and GDT_ENTRIES.
01b,01nov94,hdn  added MMU_STATE_CACHEABLE_WT for Pentium.
01a,26jul93,hdn  written based on mc68k's version.
*/

#ifndef __INCmmuI86Libh
#define __INCmmuI86Libh

#ifdef __cplusplus
extern "C" {
#endif


#define PAGE_SIZE	0x1000
#define PAGE_BLOCK_SIZE	0x400000
#define PAGE_SIZE_4KB	0x1000
#define PAGE_SIZE_2MB	0x200000
#define PAGE_SIZE_4MB	0x400000

#define DIRECTORY_BITS	0xffc00000
#define TABLE_BITS	0x003ff000
#define OFFSET_BITS	0x00000fff
#define DIRECTORY_INDEX	22
#define TABLE_INDEX	12
#define PTE_TO_ADDR	0xfffff000
#define ADDR_TO_PAGE	12

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
    unsigned zero:2;
    unsigned avail:3;
    unsigned page:20;
    } PTE_FIELD;

typedef union pte
    {
    PTE_FIELD field;
    unsigned int bits;
    } PTE;

typedef struct mmuTransTblStruct
    {
    PTE *pDirectoryTable;
    } MMU_TRANS_TBL;


#define MMU_STATE_MASK_VALID		0x001
#define MMU_STATE_MASK_WRITABLE		0x002
#define MMU_STATE_MASK_CACHEABLE	0x018

#define MMU_STATE_VALID			0x001
#define MMU_STATE_VALID_NOT		0x000
#define MMU_STATE_WRITABLE		0x002
#define MMU_STATE_WRITABLE_NOT		0x000
#define MMU_STATE_CACHEABLE		0x008
#define MMU_STATE_CACHEABLE_NOT		0x018

#define MMU_STATE_CACHEABLE_WT		0x008


/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

extern STATUS	mmuI86LibInit (int pageSize);
extern STATUS	mmuI86Enable (BOOL enable);
extern void	mmuI86On ();
extern void	mmuI86Off ();
extern void	mmuI86TLBFlush ();
extern void	mmuI86PdbrSet (MMU_TRANS_TBL * transTbl);
extern MMU_TRANS_TBL *	mmuI86PdbrGet ();

extern void	vmBaseArch32LibInit (void);
extern void	vmBaseArch36LibInit (void);
extern STATUS	vmBaseArch32Map (void * virtAddr, void * physAddr,
		    UINT32 stateMask, UINT32 state, UINT32 len);
extern STATUS	vmBaseArch32Translate (void * virtAddr, void ** physAddr);
extern void	vmArch32LibInit (void);
extern void	vmArch36LibInit (void);
extern STATUS	vmArch32Map ();
extern STATUS	vmArch32Translate ();

#else   /* __STDC__ */

extern STATUS	mmuI86LibInit ();
extern STATUS	mmuI86Enable ();
extern void	mmuI86On ();
extern void	mmuI86Off ();
extern void	mmuI86TLBFlush ();
extern void	mmuI86PdbrSet ();
extern MMU_TRANS_TBL *	mmuI86PdbrGet ();

extern void	vmBaseArch32LibInit ();
extern void	vmBaseArch36LibInit ();
extern STATUS	vmBaseArch32Map ();
extern STATUS	vmBaseArch32Translate ();
extern void	vmArch32LibInit ();
extern void	vmArch36LibInit ();
extern STATUS	vmArch32Map ();
extern STATUS	vmArch32Translate ();

#endif  /* __STDC__ */

#endif	/* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __INCmmuI86Libh */
