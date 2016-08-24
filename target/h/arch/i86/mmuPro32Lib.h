/* mmuPro32Lib.h - mmuPro32Lib header for i86. */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,12jun02,hdn  added arch specific VM library APIs' prototype
01d,23may02,hdn  replaced PAGE_SIZE with PD_SIZE and PT_SIZE
01c,22aug01,hdn  moved GDT and GDT_ENTRIES to regsI86.h
01b,13may98,hdn  added _ASMLANGUAGE macro. 
01a,11apr97,hdn  written.
*/

#ifndef __INCmmuPro32Libh
#define __INCmmuPro32Libh

#ifdef __cplusplus
extern "C" {
#endif


#define PD_SIZE			0x1000
#define PT_SIZE			0x1000
#define PAGE_BLOCK_SIZE		0x400000
#define PAGE_SIZE_4KB		0x1000
#define PAGE_SIZE_4MB		0x400000

#define DIR_BITS		0xffc00000
#define TBL_BITS		0x003ff000
#define OFFSET_BITS_4KB		0x00000fff
#define OFFSET_BITS_4MB		0x003fffff
#define DIR_INDEX		22
#define TBL_INDEX		12
#define PTE_TO_ADDR_4KB		0xfffff000
#define PTE_TO_ADDR_4MB		0xffc00000
#define ADDR_TO_PAGE		12
#define ADDR_TO_PAGEBASE	0xffc00000

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
#define MMU_STATE_MASK_CACHEABLE	0x010
#define MMU_STATE_MASK_WBACK		0x008
#define MMU_STATE_MASK_GLOBAL		0x100

#define MMU_STATE_VALID			0x001
#define MMU_STATE_VALID_NOT		0x000
#define MMU_STATE_WRITABLE		0x002
#define MMU_STATE_WRITABLE_NOT		0x000
#define MMU_STATE_CACHEABLE		0x000
#define MMU_STATE_CACHEABLE_NOT		0x010
#define MMU_STATE_WBACK			0x000
#define MMU_STATE_WBACK_NOT		0x008
#define MMU_STATE_GLOBAL		0x100
#define MMU_STATE_GLOBAL_NOT		0x000

#define MMU_STATE_CACHEABLE_WT		0x008


/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

extern STATUS	mmuPro32LibInit (int pageSize);
extern STATUS	mmuPro32Enable (BOOL enable);
extern void	mmuPro32On ();
extern void	mmuPro32Off ();
extern void	mmuPro32TLBFlush ();
extern void	mmuPro32PdbrSet (MMU_TRANS_TBL *transTbl);
extern MMU_TRANS_TBL *mmuPro32PdbrGet ();

extern void	vmBaseArch32LibInit (void);
extern STATUS	vmBaseArch32Map (void * virtAddr, void * physAddr,
		    UINT32 stateMask, UINT32 state, UINT32 len);
extern STATUS	vmBaseArch32Translate (void * virtAddr, void ** physAddr);
extern void	vmArch32LibInit (void);
extern STATUS	vmArch32Map (VM_CONTEXT_ID context, void * virtAddr,
    		    void * physAddr, UINT32 stateMask, UINT32 state, UINT32 len);
extern STATUS	vmArch32Translate (VM_CONTEXT_ID context, void * virtAddr,
		    void ** physAddr);

#else   /* __STDC__ */

extern STATUS	mmuPro32LibInit ();
extern STATUS	mmuPro32Enable ();
extern void	mmuPro32On ();
extern void	mmuPro32Off ();
extern void	mmuPro32TLBFlush ();
extern void	mmuPro32PdbrSet ();
extern MMU_TRANS_TBL *mmuPro32PdbrGet ();

extern void	vmBaseArch32LibInit ();
extern STATUS	vmBaseArch32PageMap ();
extern STATUS	vmBaseArch32Translate ();
extern void	vmArch32LibInit ();
extern STATUS	vmArch32Map ();
extern STATUS	vmArch32Translate ();

#endif  /* __STDC__ */

#endif	/* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __INCmmuPro32Libh */
