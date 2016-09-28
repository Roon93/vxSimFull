/* regsI86.h - I80x86 registers */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01k,26aug02,hdn  added Thermal Monitor and Geyserville III support
01j,07mar02,hdn  added Pentium4 HTT support (spr 73359)
01i,14aug01,hdn  added Pentium2/3/4 support
		 removed CODE_SELECTOR (use sysCsSuper instead)
		 moved WIND_TCB_XXX offsets from taskLibP.h
		 added INT_LOCK/UNLOCK macro
01h,16apr98,hdn  added support for Pentium, PentiumPro.
01g,30dec97,dbt  added common names for registers
01f,21sep95,hdn  added X86CPU_NS486.
01e,01nov94,hdn  added X86CPU_386, X86CPU_486, X86CPU_PENTIUM.
01d,27oct94,hdn  added CR0_CD_NOT and CR0_NW_NOT.
01c,08jun93,hdn  added support for c++. updated to 5.1.
01b,26mar93,hdn  added some defines for 486.
01a,28feb92,hdn  written based on TRON, 68k version.
*/

#ifndef	__INCregsI86h
#define	__INCregsI86h

#ifdef __cplusplus
extern "C" {
#endif

#define IO_BMAP_SIZE	32	/* IO bitmap for port 0x000 - 0x3ff */

#ifndef	_ASMLANGUAGE

#define GREG_NUM	8	/* has 8 32-bit general registers */

typedef struct			/* REG_SET - 80x86 register set */
    {
    ULONG edi;			/* general register */
    ULONG esi;			/* general register */
    ULONG ebp;			/* frame pointer register */
    ULONG esp;			/* stack pointer register */
    ULONG ebx;			/* general register */
    ULONG edx;			/* general register */
    ULONG ecx;			/* general register */
    ULONG eax;			/* general register */
    ULONG eflags;		/* status register (must be second to last) */
    INSTR *pc;			/* program counter (must be last) */
    } REG_SET;

typedef struct cpuid		/* CPUID - 80x86 cpuid version/feature */
    {
    int highestValue;		/* EAX=0: highest integer value */
    int vendorId[3];		/* EAX=0: vendor identification string */
    int signature;		/* EAX=1: processor signature */
    int featuresEbx;		/* EAX=1: feature flags EBX */
    int featuresEcx;		/* EAX=1: feature flags ECX */
    int featuresEdx;		/* EAX=1: feature flags EDX */
    int cacheEax;		/* EAX=2: config parameters EAX */
    int cacheEbx;		/* EAX=2: config parameters EBX */
    int cacheEcx;		/* EAX=2: config parameters ECX */
    int cacheEdx;		/* EAX=2: config parameters EDX */
    int serialNo64[2];		/* EAX=3: lower 64 of 96 bit serial no */
    int brandString[12];	/* EAX=0x8000000[234]: brand strings */
    } CPUID;

typedef struct mtrr_fix		/* MTRR - fixed range register */
    {
    char type[8];
    } MTRR_FIX;

typedef struct mtrr_var		/* MTRR - variable range register */
    {
    long long int base;
    long long int mask;
    } MTRR_VAR;

typedef struct mtrr		/* MTRR */
    {
    int cap[2];			/* MTRR cap register */
    int deftype[2];		/* MTRR defType register */
    MTRR_FIX fix[11];		/* MTRR fixed range registers */
    MTRR_VAR var[8];		/* MTRR variable range registers */
    } MTRR;
typedef MTRR *		MTRR_ID;

typedef struct gdt		/* GDT */
{
    unsigned short	limit00;
    unsigned short	base00;
    unsigned char	base01;
    unsigned char	type;
    unsigned char	limit01;
    unsigned char	base02;
} GDT;

typedef struct tss		/* TSS - 80x86 Task State Segment */
    {
    UINT16 link;		/* link to previous task */
    UINT16 link_pad;
    UINT32 esp0;		/* privilege level 0 SP */
    UINT16 ss0;			/*   ''              SS */
    UINT16 ss0_pad;
    UINT32 esp1;		/* privilege level 1 SP */
    UINT16 ss1;			/*   ''              SS */
    UINT16 ss1_pad;
    UINT32 esp2;		/* privilege level 2 SP */
    UINT16 ss2;			/*   ''              SS */
    UINT16 ss2_pad;
    UINT32 cr3;			/* control register CR3 */
    INSTR * eip;		/* program counter  EIP */
    UINT32 eflags;		/* status register  EFLAGS */
    UINT32 eax;			/* general register EAX */
    UINT32 ecx;			/* general register ECX */
    UINT32 edx;			/* general register EDX */
    UINT32 ebx;			/* general register EBX */
    UINT32 esp;			/* stack pointer register ESP */
    UINT32 ebp;			/* frame pointer register EBP */
    UINT32 esi;			/* general register ESI */
    UINT32 edi;			/* general register EDI */
    UINT16 es;			/* segment selector ES */
    UINT16 es_pad;
    UINT16 cs;			/* segment selector CS */
    UINT16 cs_pad;
    UINT16 ss;			/* segment selector SS */
    UINT16 ss_pad;
    UINT16 ds;			/* segment selector DS */
    UINT16 ds_pad;
    UINT16 fs;			/* segment selector FS */
    UINT16 fs_pad;
    UINT16 gs;			/* segment selector GS */
    UINT16 gs_pad;
    UINT16 ldt;			/* segment selector LDT */
    UINT16 ldt_pad;
    UINT16 tflag;		/* debug trap flag T */
    UINT16 iomapb;		/* IO map base address */
    UINT32 iobmap[IO_BMAP_SIZE + 1];
    UINT32 reserved0;		/* TSS selector */
    UINT32 reserved1;
    UINT32 reserved2;
    UINT32 reserved3;
    UINT32 reserved4;
    UINT32 reserved5;
    UINT32 reserved6;
    UINT32 reserved7;
    } TSS;

typedef struct segdesc		/* segment descriptor */
    {
    UINT16	limitLW;	/* limit 15:00 */
    UINT16	baseLW;		/* base address 15:00 */
    UCHAR	baseMB;		/* base address 23:16 */
    UCHAR	type;		/* P, DPL, S, Type */
    UCHAR	limitUB;	/* G, DB, 0, AVL, limit 23:16 */
    UCHAR	baseUB;		/* base address 31:24 */
    } SEGDESC;

typedef struct callGate		/* call gate */
    {
    UINT16	offsetLo;
    UINT16	selector;
    UCHAR 	params;
    UCHAR 	type;
    UINT16	offsetHi;
    } CALL_GATE;


/* some common names for registers */

#define fpReg		ebp	/* frame pointer */
#define	spReg		esp	/* stack pointer */
#define reg_pc		pc	/* program counter */
#define reg_sp		spReg	/* stack pointer */
#define reg_fp		fpReg	/* frame pointer */

#define  G_REG_BASE	0x00	/* data reg's base offset to REG_SET */
#define  G_REG_OFFSET(n)	(G_REG_BASE + (n)*sizeof(ULONG))
#define  SR_OFFSET		G_REG_OFFSET(GREG_NUM)
#define  PC_OFFSET		(SR_OFFSET + sizeof(ULONG))

#endif	/* _ASMLANGUAGE */

/* CPU FAMILY & FPU type */

#define X86CPU_386	0	/* CPU FAMILY: 80386		*/
#define X86CPU_486	1	/* CPU FAMILY: 80486		*/
#define X86CPU_PENTIUM	2	/* CPU FAMILY: Pentium/P5	*/
#define X86CPU_NS486	3	/* CPU FAMILY: NS486		*/
#define X86CPU_PENTIUMPRO 4	/* CPU FAMILY: Pentiumpro/P6	*/
#define X86CPU_PENTIUM4 5       /* CPU FAMILY: Pentium4/P7      */
#define X86FPU_387	1	/* FPU: 80387			*/
#define X86FPU_487	2	/* FPU: 80487			*/

/* offset to registers in REG_SET */

#define REG_EDI			0x00
#define REG_ESI			0x04
#define REG_EBP			0x08
#define REG_ESP			0x0c
#define REG_EBX			0x10
#define REG_EDX			0x14
#define REG_ECX			0x18
#define REG_EAX			0x1c
#define REG_EFLAGS		0x20
#define REG_PC			0x24

/* offset to registers in REG_SET in TCB */

#define WIND_TCB_EDI            WIND_TCB_REGS + REG_EDI
#define WIND_TCB_ESI            WIND_TCB_REGS + REG_ESI
#define WIND_TCB_EBP            WIND_TCB_REGS + REG_EBP
#define WIND_TCB_ESP            WIND_TCB_REGS + REG_ESP
#define WIND_TCB_EBX            WIND_TCB_REGS + REG_EBX
#define WIND_TCB_EDX            WIND_TCB_REGS + REG_EDX
#define WIND_TCB_ECX            WIND_TCB_REGS + REG_ECX
#define WIND_TCB_EAX            WIND_TCB_REGS + REG_EAX
#define WIND_TCB_EFLAGS         WIND_TCB_REGS + REG_EFLAGS
#define WIND_TCB_PC             WIND_TCB_REGS + REG_PC

/* bits on EFLAGS */

#define	EFLAGS_BRANDNEW	0x00000200	/* brand new EFLAGS */
#define	EFLAGS_N_MASK	0xffffbfff	/* N(nested task flag) bit mask */
#define	EFLAGS_TF_MASK	0xfffffeff	/* TF(trap flag) bit mask */
#define	EFLAGS_TF	0x00000100	/* TF(trap flag) bit */
#define	EFLAGS_IF	0x00000200	/* IF(interrupt enable flag) bit */
#define	EFLAGS_IOPL	0x00003000	/* IOPL(IO privilege level) bits */
#define	EFLAGS_NT	0x00004000	/* NT(nested task flag) bit */
#define	EFLAGS_RF	0x00010000	/* RF(resume flag) bit */
#define	EFLAGS_VM	0x00020000	/* VM(virtual 8086 mode) bit */
#define	EFLAGS_AC	0x00040000	/* AC(alignment check) bit */
#define	EFLAGS_VIF	0x00080000	/* VIF(virtual int flag) bit */
#define	EFLAGS_VIP	0x00100000	/* VIP(virtual int pending) bit */
#define	EFLAGS_ID	0x00200000	/* ID(identification flag) bit */

/* control and test registers */

#define CR0		1
#define CR1		2
#define CR2		3
#define CR3		4
#define TR3		5
#define TR4		6
#define TR5		7
#define TR6		8
#define TR7		9

/* bits on CR0 */

#define CR0_PE		0x00000001	/* protection enable */
#define CR0_MP		0x00000002	/* math present */
#define CR0_EM		0x00000004	/* emulation */
#define CR0_TS		0x00000008	/* task switch */
#define CR0_NE		0x00000020	/* numeric error */
#define CR0_WP		0x00010000	/* write protect */
#define CR0_AM		0x00040000	/* alignment mask */
#define CR0_NW		0x20000000	/* not write through */
#define CR0_CD		0x40000000	/* cache disable */
#define CR0_PG		0x80000000	/* paging */
#define CR0_NW_NOT	0xdfffffff	/* write through */
#define CR0_CD_NOT	0xbfffffff	/* cache disable */

/* bits on CR4 */

#define CR4_VME		0x00000001	/* virtual-8086 mode extensions */
#define CR4_PVI		0x00000002	/* protected-mode virtual interrupts */
#define CR4_TSD		0x00000004	/* timestamp disable */
#define CR4_DE		0x00000008	/* debugging extensions */
#define CR4_PSE		0x00000010	/* page size extensions */
#define CR4_PAE		0x00000020	/* physical address extension */
#define CR4_MCE		0x00000040	/* machine check enable */
#define CR4_PGE		0x00000080	/* page global enable */
#define CR4_PCE		0x00000100	/* performance-monitoring enable */
#define CR4_OSFXSR	0x00000200	/* use fxsave/fxrstor instructions */
#define CR4_OSXMMEXCEPT	0x00000400	/* streaming SIMD exception */

/* CPUID: signature bit definitions */

#define CPUID_STEPID	0x0000000f	/* processor stepping id mask	*/
#define CPUID_MODEL	0x000000f0	/* processor model mask		*/
#define CPUID_FAMILY	0x00000f00	/* processor family mask	*/
#define CPUID_TYPE	0x00003000	/* processor type mask		*/
#define CPUID_EXT_MODEL	0x000f0000	/* processor extended model mask */
#define CPUID_EXT_FAMILY 0x0ff00000	/* processor extended family mask */
#define CPUID_486	0x00000400	/* family: 486			*/
#define CPUID_PENTIUM	0x00000500	/* family: Pentium		*/
#define CPUID_PENTIUMPRO 0x00000600	/* family: Pentium PRO		*/
#define CPUID_EXTENDED	0x00000f00	/* family: Extended		*/
#define CPUID_PENTIUM4	0x00000000	/* extended family: PENTIUM4	*/
#define CPUID_ORIG      0x00000000	/* type: original OEM		*/
#define CPUID_OVERD     0x00001000	/* type: overdrive		*/
#define CPUID_DUAL      0x00002000	/* type: dual			*/
#define CPUID_CHUNKS	0x0000ff00	/* bytes flushed by CLFLUSH mask */

/* CPUID: feature bit definitions */

#define CPUID_FPU	0x00000001	/* FPU on chip			*/
#define CPUID_VME	0x00000002	/* virtual 8086 mode enhancement */
#define CPUID_DE	0x00000004	/* debugging extensions		*/
#define CPUID_PSE	0x00000008	/* page size extension		*/
#define CPUID_TSC	0x00000010	/* time stamp counter		*/
#define CPUID_MSR	0x00000020	/* RDMSR and WRMSR support	*/
#define CPUID_PAE	0x00000040	/* physical address extensions	*/
#define CPUID_MCE	0x00000080	/* machine check exception	*/
#define CPUID_CXS	0x00000100	/* CMPXCHG8 inst		*/
#define CPUID_APIC	0x00000200	/* APIC on chip			*/
#define CPUID_SEP	0x00000800	/* SEP, Fast System Call	*/
#define CPUID_MTRR	0x00001000	/* MTRR				*/
#define CPUID_PGE	0x00002000	/* PTE global bit		*/
#define CPUID_MCA	0x00004000	/* machine check arch.		*/
#define CPUID_CMOV	0x00008000	/* cond. move/cmp. inst		*/
#define CPUID_PAT	0x00010000	/* page attribute table		*/
#define CPUID_PSE36	0x00020000	/* 36 bit page size extension	*/
#define CPUID_PSNUM	0x00040000	/* processor serial number	*/
#define CPUID_CLFLUSH	0x00080000	/* CLFLUSH inst supported	*/
#define CPUID_DTS	0x00200000	/* Debug Store			*/
#define CPUID_ACPI	0x00400000	/* TM and SCC supported		*/
#define CPUID_MMX	0x00800000	/* MMX technology supported	*/
#define CPUID_FXSR	0x01000000	/* fast FP save and restore	*/
#define CPUID_SSE	0x02000000	/* SSE supported		*/
#define CPUID_SSE2	0x04000000	/* SSE2 supported		*/
#define CPUID_SS	0x08000000	/* Self Snoop supported		*/
#define CPUID_HTT	0x10000000	/* Hyper Threading Technology   */
#define CPUID_TM	0x20000000	/* Thermal Monitor supported	*/

/* CPUID: extended feature bit definitions */

#define CPUID_GV3	0x00000080	/* Geyserville 3 supported	*/
#define CPUID_TM2	0x00000100	/* Thermal Monitor 2 supported	*/

/* CPUID: offset in CPUID structure */

#define CPUID_HIGHVALUE		0	/* offset to highestValue	*/
#define CPUID_VENDORID		4	/* offset to vendorId		*/
#define CPUID_SIGNATURE		16	/* offset to signature		*/
#define CPUID_FEATURES_EBX	20	/* offset to featuresEbx	*/
#define CPUID_FEATURES_ECX	24	/* offset to featuresEcx	*/
#define CPUID_FEATURES_EDX	28	/* offset to featuresEdx	*/
#define CPUID_CACHE_EAX		32	/* offset to cacheEax		*/
#define CPUID_CACHE_EBX		36	/* offset to cacheEbx		*/
#define CPUID_CACHE_ECX		40	/* offset to cacheEcx		*/
#define CPUID_CACHE_EDX		44	/* offset to cacheEdx		*/
#define CPUID_SERIALNO		48	/* offset to serialNo64		*/
#define CPUID_BRAND_STR		56	/* offset to brandString[0]	*/

/* MSR, Model Specific Registers */

/* MSR, P5 only */

#define MSR_P5_MC_ADDR		0x0000
#define MSR_P5_MC_TYPE		0x0001
#define MSR_TSC			0x0010
#define MSR_CESR                0x0011
#define MSR_CTR0                0x0012
#define MSR_CTR1                0x0013

/* MSR, P5 and P6 */

#define MSR_APICBASE		0x001b
#define MSR_EBL_CR_POWERON	0x002a
#define MSR_TEST_CTL		0x0033
#define MSR_BIOS_UPDT_TRIG	0x0079
#define MSR_BBL_CR_D0		0x0088	/* P6 only */
#define MSR_BBL_CR_D1		0x0089	/* P6 only */
#define MSR_BBL_CR_D2		0x008a	/* P6 only */
#define MSR_BIOS_SIGN		0x008b
#define MSR_PERFCTR0		0x00c1
#define MSR_PERFCTR1		0x00c2
#define MSR_MTRR_CAP		0x00fe
#define MSR_BBL_CR_ADDR		0x0116	/* P6 only */
#define MSR_BBL_CR_DECC		0x0118	/* P6 only */
#define MSR_BBL_CR_CTL		0x0119	/* P6 only */
#define MSR_BBL_CR_TRIG		0x011a	/* P6 only */
#define MSR_BBL_CR_BUSY		0x011b	/* P6 only */
#define MSR_BBL_CR_CTL3		0x011e	/* P6 only */
#define MSR_SYSENTER_CS		0x0174	/* P6 + SEP only */
#define MSR_SYSENTER_ESP	0x0175	/* P6 + SEP only */
#define MSR_SYSENTER_EIP	0x0176	/* P6 + SEP only */
#define MSR_MCG_CAP		0x0179
#define MSR_MCG_STATUS		0x017a
#define MSR_MCG_CTL		0x017b
#define MSR_EVNTSEL0		0x0186
#define MSR_EVNTSEL1		0x0187
#define MSR_DEBUGCTLMSR		0x01d9
#define MSR_LASTBRANCH_FROMIP	0x01db
#define MSR_LASTBRANCH_TOIP	0x01dc
#define MSR_LASTINT_FROMIP	0x01dd
#define MSR_LASTINT_TOIP	0x01de
#define MSR_ROB_CR_BKUPTMPDR6	0x01e0
#define MSR_MTRR_PHYS_BASE0	0x0200
#define MSR_MTRR_PHYS_MASK0	0x0201
#define MSR_MTRR_PHYS_BASE1	0x0202
#define MSR_MTRR_PHYS_MASK1	0x0203
#define MSR_MTRR_PHYS_BASE2	0x0204
#define MSR_MTRR_PHYS_MASK2	0x0205
#define MSR_MTRR_PHYS_BASE3	0x0206
#define MSR_MTRR_PHYS_MASK3	0x0207
#define MSR_MTRR_PHYS_BASE4	0x0208
#define MSR_MTRR_PHYS_MASK4	0x0209
#define MSR_MTRR_PHYS_BASE5	0x020a
#define MSR_MTRR_PHYS_MASK5	0x020b
#define MSR_MTRR_PHYS_BASE6	0x020c
#define MSR_MTRR_PHYS_MASK6	0x020d
#define MSR_MTRR_PHYS_BASE7	0x020e
#define MSR_MTRR_PHYS_MASK7	0x020f
#define MSR_MTRR_FIX_00000	0x0250
#define MSR_MTRR_FIX_80000	0x0258
#define MSR_MTRR_FIX_A0000	0x0259
#define MSR_MTRR_FIX_C0000	0x0268
#define MSR_MTRR_FIX_C8000	0x0269
#define MSR_MTRR_FIX_D0000	0x026a
#define MSR_MTRR_FIX_D8000	0x026b
#define MSR_MTRR_FIX_E0000	0x026c
#define MSR_MTRR_FIX_E8000	0x026d
#define MSR_MTRR_FIX_F0000	0x026e
#define MSR_MTRR_FIX_F8000	0x026f
#define MSR_MTRR_DEFTYPE	0x02ff
#define MSR_MC0_CTL		0x0400
#define MSR_MC0_STATUS		0x0401
#define MSR_MC0_ADDR		0x0402
#define MSR_MC0_MISC		0x0403
#define MSR_MC1_CTL		0x0404
#define MSR_MC1_STATUS		0x0405
#define MSR_MC1_ADDR		0x0406
#define MSR_MC1_MISC		0x0407
#define MSR_MC2_CTL		0x0408
#define MSR_MC2_STATUS		0x0409
#define MSR_MC2_ADDR		0x040a
#define MSR_MC2_MISC		0x040b
#define MSR_MC4_CTL		0x040c
#define MSR_MC4_STATUS		0x040d
#define MSR_MC4_ADDR		0x040e
#define MSR_MC4_MISC		0x040f
#define MSR_MC3_CTL		0x0410
#define MSR_MC3_STATUS		0x0411
#define MSR_MC3_ADDR		0x0412
#define MSR_MC3_MISC		0x0413

/* MSR, Architectural MSRs (common MSRs in IA32) */

#define	IA32_P5_MC_ADDR		MSR_P5_MC_ADDR		/* P5 */
#define	IA32_P5_MC_TYPE		MSR_P5_MC_TYPE		/* P5 */
#define	IA32_TIME_STAMP_COUNTER	MSR_TSC			/* P5 */
#define	IA32_PLATFORM_ID	0x0017			/* P6 */
#define	IA32_APIC_BASE		MSR_APICBASE		/* P6 */
#define	IA32_BIOS_UPDT_TRIG	MSR_BIOS_UPDT_TRIG	/* P6 */
#define	IA32_BIOS_SIGN_ID	MSR_BIOS_SIGN		/* P6 */
#define	IA32_MTRRCAP		MSR_MTRR_CAP		/* P6 */
#define	IA32_MISC_CTL		MSR_BBL_CR_CTL		/* P6 */
#define	IA32_SYSENTER_CS	MSR_SYSENTER_CS		/* P6 */
#define	IA32_SYSENTER_ESP	MSR_SYSENTER_ESP	/* P6 */
#define	IA32_SYSENTER_EIP	MSR_SYSENTER_EIP	/* P6 */
#define	IA32_MCG_CAP		MSR_MCG_CAP		/* P6 */
#define	IA32_MCG_STATUS		MSR_MCG_STATUS		/* P6 */
#define	IA32_MCG_CTL		MSR_MCG_CTL		/* P6 */
#define	IA32_MCG_EAX		0x0180			/* Pentium4 */
#define	IA32_MCG_EBX		0x0181			/* Pentium4 */
#define	IA32_MCG_ECX		0x0182			/* Pentium4 */
#define	IA32_MCG_EDX		0x0183			/* Pentium4 */
#define	IA32_MCG_ESI		0x0184			/* Pentium4 */
#define	IA32_MCG_EDI		0x0185			/* Pentium4 */
#define	IA32_MCG_EBP		0x0186			/* Pentium4 */
#define	IA32_MCG_ESP		0x0187			/* Pentium4 */
#define	IA32_MCG_EFLAGS		0x0188			/* Pentium4 */
#define	IA32_MCG_EIP		0x0189			/* Pentium4 */
#define	IA32_MCG_MISC		0x018a			/* Pentium4 */
#define	IA32_THERM_CONTROL	0x019a			/* Pentium4 */
#define	IA32_THERM_INTERRUPT	0x019b			/* Pentium4 */
#define	IA32_THERM_STATUS	0x019c			/* Pentium4 */
#define	IA32_MISC_ENABLE	0x01a0			/* Pentium4 */
#define	IA32_DEBUGCTL		MSR_DEBUGCTLMSR		/* P6 */
#define	IA32_MTRR_PHYSBASE0	MSR_MTRR_PHYS_BASE0	/* P6 */
#define	IA32_MTRR_PHYSMASK0	MSR_MTRR_PHYS_MASK0	/* P6 */
#define	IA32_MTRR_PHYSBASE1	MSR_MTRR_PHYS_BASE1	/* P6 */
#define	IA32_MTRR_PHYSMASK1	MSR_MTRR_PHYS_MASK1	/* P6 */
#define	IA32_MTRR_PHYSBASE2	MSR_MTRR_PHYS_BASE2	/* P6 */
#define	IA32_MTRR_PHYSMASK2	MSR_MTRR_PHYS_MASK2	/* P6 */
#define	IA32_MTRR_PHYSBASE3	MSR_MTRR_PHYS_BASE3	/* P6 */
#define	IA32_MTRR_PHYSMASK3	MSR_MTRR_PHYS_MASK3	/* P6 */
#define	IA32_MTRR_PHYSBASE4	MSR_MTRR_PHYS_BASE4	/* P6 */
#define	IA32_MTRR_PHYSMASK4	MSR_MTRR_PHYS_MASK4	/* P6 */
#define	IA32_MTRR_PHYSBASE5	MSR_MTRR_PHYS_BASE5	/* P6 */
#define	IA32_MTRR_PHYSMASK5	MSR_MTRR_PHYS_MASK5	/* P6 */
#define	IA32_MTRR_PHYSBASE6	MSR_MTRR_PHYS_BASE6	/* P6 */
#define	IA32_MTRR_PHYSMASK6	MSR_MTRR_PHYS_MASK6	/* P6 */
#define	IA32_MTRR_PHYSBASE7	MSR_MTRR_PHYS_BASE7	/* P6 */
#define	IA32_MTRR_PHYSMASK7	MSR_MTRR_PHYS_MASK7	/* P6 */
#define	IA32_MTRR_FIX64K_00000	MSR_MTRR_FIX_00000	/* P6 */
#define	IA32_MTRR_FIX16K_80000	MSR_MTRR_FIX_80000	/* P6 */
#define	IA32_MTRR_FIX16K_A0000	MSR_MTRR_FIX_A0000	/* P6 */
#define	IA32_MTRR_FIX4K_C0000	MSR_MTRR_FIX_C0000	/* P6 */
#define	IA32_MTRR_FIX4K_C8000	MSR_MTRR_FIX_C8000	/* P6 */
#define	IA32_MTRR_FIX4K_D0000	MSR_MTRR_FIX_D0000	/* P6 */
#define	IA32_MTRR_FIX4K_D8000	MSR_MTRR_FIX_D8000	/* P6 */
#define	IA32_MTRR_FIX4K_E0000	MSR_MTRR_FIX_E0000	/* P6 */
#define	IA32_MTRR_FIX4K_E8000	MSR_MTRR_FIX_E8000	/* P6 */
#define	IA32_MTRR_FIX4K_F0000	MSR_MTRR_FIX_F0000	/* P6 */
#define	IA32_MTRR_FIX4K_F8000	MSR_MTRR_FIX_F8000	/* P6 */
#define	IA32_CR_PAT		0x0277			/* P6 */
#define	IA32_MTRR_DEF_TYPE	MSR_MTRR_DEFTYPE	/* P6 */
#define	IA32_PEBS_ENABLE	0x03f1			/* Pentium4 */
#define	IA32_MC0_CTL		MSR_MC0_CTL		/* P6 */
#define	IA32_MC0_STATUS		MSR_MC0_STATUS		/* P6 */
#define	IA32_MC0_ADDR		MSR_MC0_ADDR		/* P6 */
#define	IA32_MC0_MISC		MSR_MC0_MISC		/* P6 */
#define	IA32_MC1_CTL		MSR_MC1_CTL		/* P6 */
#define	IA32_MC1_STATUS		MSR_MC1_STATUS		/* P6 */
#define	IA32_MC1_ADDR		MSR_MC1_ADDR		/* P6 */
#define	IA32_MC1_MISC		MSR_MC1_MISC		/* P6 */
#define	IA32_MC2_CTL		MSR_MC2_CTL		/* P6 */
#define	IA32_MC2_STATUS		MSR_MC2_STATUS		/* P6 */
#define	IA32_MC2_ADDR		MSR_MC2_ADDR		/* P6 */
#define	IA32_MC2_MISC		MSR_MC2_MISC		/* P6 */
#define	IA32_MC3_CTL		0x040c			/* P6, addr changed */
#define	IA32_MC3_STATUS		0x040d			/* P6, addr changed */
#define	IA32_MC3_ADDR		0x040e			/* P6, addr changed */
#define	IA32_MC3_MISC		0x040f			/* P6, addr changed */
#define	IA32_DS_AREA		0x0600			/* Pentium4 */

/* MSR, IA32_DEBUGCTL, in Pentium4, bits */

#define	DBG_P7_LBR		0x00000001
#define	DBG_P7_BTF		0x00000002
#define	DBG_P7_TR		0x00000004
#define	DBG_P7_BTS		0x00000008
#define	DBG_P7_BTINT		0x00000010

/* MSR, IA32_DEBUGCTL, in P6, bits */

#define	DBG_P6_LBR		0x00000001
#define	DBG_P6_BTF		0x00000002
#define	DBG_P6_PB0		0x00000004
#define	DBG_P6_PB1		0x00000008
#define	DBG_P6_PB2		0x00000010
#define	DBG_P6_PB3		0x00000020
#define	DBG_P6_TR		0x00000040

/* MSR, MSR_LASTBRANCH_TOS, in Pentium4, bits */

#define	TOS_MASK		0x00000003

/* MSR, IA32_MISC_ENABLE bits */

#define	MSC_FAST_STRING_ENABLE	0x00000001
#define	MSC_FOPCODE_ENABLE	0x00000004
#define	MSC_THERMAL_MON_ENABLE	0x00000008
#define	MSC_SPLIT_LOCK_DISABLE	0x00000010
#define	MSC_PMON_AVAILABLE	0x00000080
#define	MSC_BTS_UNAVAILABLE	0x00000800
#define	MSC_PEBS_UNAVAILABLE	0x00001000
#define	MSC_GV1_EN		0x00008000
#define	MSC_GV3_EN		0x00010000
#define	MSC_GV_SEL_LOCK		0x00100000

/* MSR, IA32_PEBS_ENABLE bits */

#define	PEBS_METRICS		0x00001fff
#define	PEBS_UOP_TAG		0x01000000
#define	PEBS_ENABLE		0x02000000

/* MSR, IA32_PLATFORM_ID bits (upper 32) */

#define	PFM_PLATFORM_ID		0x001c0000
#define	PFM_MOBILE_GV		0x00040000

/* MSR, IA32_PLATFORM_ID bits (lower 32) */

#define	PFM_MAX_VID		0x0000003f
#define	PFM_MAX_FREQ		0x00000f80
#define	PFM_RATIO_LOCKED	0x00008000
#define	PFM_GV3_TM_DISABLED	0x00010000
#define	PFM_GV3_DISABLED	0x00020000
#define	PFM_GV1_DISABLED	0x00040000
#define	PFM_TM_DISABLED		0x00080000
#define	PFM_L2_CACHE_SIZE	0x06000000
#define	PFM_SAMPLE		0x08000000

/* MSR, IA32_THERM_CONTROL bits */

#define	THERM_DUTY_CYCLE	0x0000000e
#define	THERM_TCC_EN		0x00000010

/* MSR, IA32_THERM_STATUS bits */

#define	THERM_HOT_NOW		0x00000001
#define	THERM_HOT_LOG		0x00000002

/* MSR, IA32_THERM_INTERRUPT bits */

#define	THERM_HOT_INT_EN	0x00000001
#define	THERM_COLD_INT_EN	0x00000002


/* PMC, Performance Monitoring Event Select MSR bits */

/* P5 specific */

#define P5PMC_PC                0x00000200
#define P5PMC_CC_DISABLE        0x00000000
#define P5PMC_CC_EVT_CPL012     0x00000040
#define P5PMC_CC_EVT_CPL3       0x00000080
#define P5PMC_CC_EVT            0x000000C0
#define P5PMC_CC_CLK_CPL012     0x00000140
#define P5PMC_CC_CLK_CPL3       0x00000180
#define P5PMC_CC_CLK            0x000001C0

/* P6 specific */

#define PMC_USR               0x00010000
#define PMC_OS                0x00020000
#define PMC_E                 0x00040000
#define PMC_PC                0x00080000
#define PMC_INT               0x00100000
#define PMC_EN                0x00400000
#define PMC_INV               0x00800000

/* PMC, Performance Monitoring Events */

/* P5 PMC event list */

#define P5PMC_DATA_RD			0x00
#define P5PMC_DATA_WR			0x01
#define P5PMC_DATA_TBL_MISS		0x02
#define P5PMC_DATA_RD_MISS		0x03
#define P5PMC_DATA_WR_MISS		0x04
#define P5PMC_WR_HIT_M_E_STATE_LINE	0x05
#define P5PMC_DCACHE_WR_BACK		0x06
#define P5PMC_EXT_SNOOPS		0x07
#define P5PMC_EXT_DCACHE_SNOOPS_HIT	0x08
#define P5PMC_MEM_ACCESS_BOTH_PIPES	0x09
#define P5PMC_BANK_CONFLICT		0x0A
#define P5PMC_MISC_DMEM_IO_REF		0x0B
#define P5PMC_CODE_RD			0x0C
#define P5PMC_CODE_TBL_MISS		0x0D
#define P5PMC_CODE_CACHE_MISS		0x0E
#define P5PMC_SEGMENT_REG_LOAD		0x0F
#define P5PMC_BRANCH			0x12
#define P5PMC_BTB_HIT			0x13
#define P5PMC_TAKEN_BRANCH_BTB_HIT	0x14
#define P5PMC_PIPELINE_FLUSH		0x15
#define P5PMC_INST_EXECUTED		0x16
#define P5PMC_INST_EXECUTED_VPIPE	0x17
#define P5PMC_BUS_CYC_DURATION		0x18
#define P5PMC_WR_BUF_FULL_STALL_DURATION 0x19
#define P5PMC_WAIT_MEM_RD_STALL_DURATION 0x1A
#define P5PMC_STALL_ON_WR_M_E_STATE_LINE 0x1B
#define P5PMC_LOCKED_BUS_CYC		0x1C
#define P5PMC_IO_RD_WR_CYC		0x1D
#define P5PMC_NONCACHE_MEM_RD		0x1E
#define P5PMC_PIPELINE_AGI_STALL	0x1F
#define P5PMC_FLOPS			0x22
#define P5PMC_BK_MATCH_DR0		0x23
#define P5PMC_BK_MATCH_DR1		0x24
#define P5PMC_BK_MATCH_DR2		0x25
#define P5PMC_BK_MATCH_DR3		0x26
#define P5PMC_HW_INT			0x27
#define P5PMC_DATA_RD_WR		0x28
#define P5PMC_DATA_RD_WR_MISS		0x29
#define P5PMC_BUS_OWNER_LATENCY		0x2A
#define P5PMC_BUS_OWNER_TRANSFER	0x2A
#define P5PMC_MMX_INST_UPIPE		0x2B
#define P5PMC_MMX_INST_VPIPE		0x2B
#define P5PMC_CACHE_M_LINE_SHARE	0x2C
#define P5PMC_CACHE_LINE_SHARE		0x2C
#define P5PMC_EMMS_INTS_EXECUTED	0x2D
#define P5PMC_TRANS_MMX_FP_INST		0x2D
#define P5PMC_BUS_UTIL_PROCESSOR_ACT	0x2D
#define P5PMC_WR_NOCACHEABLE_MEM	0x2E
#define P5PMC_SATURATING_MMX_INST	0x2F
#define P5PMC_SATURATION_PERFORMED	0x2F
#define P5PMC_NUM_CYC_NOT_HALT_STATE	0x30
#define P5PMC_DCACHE_TLB_MISS_STALL_DUR	0x30
#define P5PMC_MMX_INST_DATA_RD		0x31
#define P5PMC_MMX_INST_DATA_RD_MISS	0x31
#define P5PMC_FP_STALL_DUR		0x32
#define P5PMC_TAKEN_BRANCH		0x32
#define P5PMC_D1_STARV_FIFO_EMPTY	0x33
#define P5PMC_D1_STARV_ONE_INST_FIFO	0x33
#define P5PMC_MMX_INST_DATA_WR		0x34
#define P5PMC_MMX_INST_DATA_WR_MISS	0x34
#define P5PMC_PL_FLUSH_WRONG_BR_PREDIC	0x35
#define P5PMC_PL_FLUSH_WRONG_BR_PREDIC_WB 0x35
#define P5PMC_MISALIGN_DMEM_REF_MMX 	0x36
#define P5PMC_PL_STALL_MMX_DMEM_RD 	0x36
#define P5PMC_MISPREDIC_UNPREDIC_RET 	0x37
#define P5PMC_PREDICED_RETURN 		0x37
#define P5PMC_MMX_MUL_UNIT_INTERLOCK 	0x38
#define P5PMC_MOVD_MOVQ_STALL_PREV_MMX 	0x38
#define P5PMC_RETURN 			0x39
#define P5PMC_BTB_FALSE_ENTRY		0x3A
#define P5PMC_BTB_MISS_PREDIC_NOT_TAKEN_BR 0x3A
#define P5PMC_FULL_WR_BUF_STALL_MMX 	0x3B
#define P5PMC_STALL_MMX_WR_E_M_STATE_LINE 0x3B

/* P6 PMC event list */

#define	PMC_DATA_MEM_REFS		0x43
#define	PMC_DCU_LINES_IN		0x45
#define	PMC_DCU_M_LINES_IN		0x46
#define	PMC_DCU_M_LINES_OUT		0x47
#define	PMC_DCU_MISS_OUTSTANDING	0x48
#define	PMC_IFU_IFETCH			0x80
#define	PMC_IFU_IFETCH_MISS		0x81
#define	PMC_ITLB_MISS			0x85
#define	PMC_IFU_MEM_STALL		0x86
#define	PMC_IDL_STALL			0x87
#define	PMC_L2_IFETCH			0x28
#define	PMC_L2_LD			0x29
#define	PMC_L2_ST			0x2a
#define	PMC_L2_LINES_IN			0x24
#define	PMC_L2_LINES_OUT		0x26
#define	PMC_L2_M_LINES_INM		0x25
#define	PMC_L2_M_LINES_OUTM		0x27
#define	PMC_L2_RQSTS			0x2e
#define	PMC_L2_ADS			0x21
#define	PMC_L2_DBUS_BUSY		0x22
#define	PMC_L2_DBUS_BUSY_RD		0x23
#define	PMC_BUS_DRDY_CLOCKS		0x62
#define	PMC_BUS_LOCK_CLOCKS		0x63
#define	PMC_BUS_REQ_OUTSTANDING		0x60
#define	PMC_BUS_TRAN_BRD		0x65
#define	PMC_BUS_TRAN_RFO		0x66
#define	PMC_BUS_TRANS_WB		0x67
#define	PMC_BUS_TRAN_IFETCH		0x68
#define	PMC_BUS_TRAN_INVAL		0x69
#define	PMC_BUS_TRAN_PWR		0x6a
#define	PMC_BUS_TRANS_P			0x6b
#define	PMC_BUS_TRANS_IO		0x6c
#define	PMC_BUS_TRAN_DEF		0x6d
#define	PMC_BUS_TRAN_BURST		0x6e
#define	PMC_BUS_TRAN_ANY		0x70
#define	PMC_BUS_TRAN_MEM		0x6f
#define	PMC_BUS_DATA_RCV		0x64
#define	PMC_BUS_BNR_DRV			0x61
#define	PMC_BUS_HIT_DRV			0x7a
#define	PMC_BUS_HITM_DRV		0x7b
#define	PMC_BUS_SNOOP_STALL		0x7e
#define	PMC_FLOPS			0xc1
#define	PMC_FP_COMP_OPS_EXE		0x10
#define	PMC_FP_ASSIST			0x11
#define	PMC_MUL				0x12
#define	PMC_DIV				0x13
#define	PMC_CYCLES_DIV_BUSY		0x14
#define	PMC_LD_BLOCKS			0x03
#define	PMC_SB_DRAINS			0x04
#define	PMC_MISALIGN_MEM_REF		0x05
#define	PMC_INST_RETIRED		0xc0
#define	PMC_UOPS_RETIRED		0xc2
#define	PMC_INST_DECODER		0xd0
#define	PMC_HW_INT_RX			0xc8
#define	PMC_CYCLES_INT_MASKED		0xc6
#define	PMC_CYCLES_INT_PENDING_AND_MASKED 0xc7
#define	PMC_BR_INST_RETIRED		0xc4
#define	PMC_BR_MISS_PRED_RETIRED	0xc5
#define	PMC_BR_TAKEN_RETIRED		0xc9
#define	PMC_BR_MISS_PRED_TAKEN_RETIRED	0xca
#define	PMC_BR_INST_DECODED		0xe0
#define	PMC_BTB_MISSES			0xe2
#define	PMC_BR_BOGUS			0xe4
#define	PMC_BACLEARS			0xe6
#define	PMC_RESOURCE_STALLS		0xa2
#define	PMC_PARTIAL_RAT_STALLS		0xd2
#define	PMC_SEGMENT_REG_LOADS		0x06
#define	PMC_CPU_CLK_UNHALTED		0x79
#define	PMC_UMASK_00			0x00
#define	PMC_UMASK_0F			0x0f
#define	PMC_UMASK_SELF			0x00
#define	PMC_UMASK_ANY			0x20

/* MTRR related defines */

#define MTRR_UC				0x00
#define MTRR_WC				0x01
#define MTRR_WT				0x04
#define MTRR_WP				0x05
#define MTRR_WB				0x06
#define MTRR_E				0x00000800
#define MTRR_FE				0x00000400
#define MTRR_VCNT			0x000000FF
#define MTRR_FIX_SUPPORT		0x00000100
#define MTRR_WC_SUPPORT			0x00000400

/* MCA related defines */

#define	MCG_CTL_P			0x00000100
#define	MCG_COUNT			0x000000ff
#define	MCG_MCIP			0x00000004
#define	MCG_EIPV			0x00000002
#define	MCG_RIPV			0x00000001
#define	MCI_VAL				0x80000000
#define	MCI_OVER			0x40000000
#define	MCI_UC				0x20000000
#define	MCI_EN				0x10000000
#define	MCI_MISCV			0x08000000
#define	MCI_ADDRV			0x04000000
#define	MCI_PCC				0x02000000

/* segment descriptor: types for application code, data segment */

#define	SEG_DATA_RO_U		0x00000000	/* read only */
#define	SEG_DATA_RW_U		0x00000200	/* read write */
#define	SEG_DATA_RO_D		0x00000400	/* read only expand down */
#define	SEG_DATA_RW_D		0x00000600	/* read write expand down */
#define	SEG_CODE_EO		0x00000800	/* exec only */
#define	SEG_CODE_ER		0x00000a00	/* exec read */
#define	SEG_CODE_EO_C		0x00000c00	/* exec only conform. */
#define	SEG_CODE_ERO_C		0x00000e00	/* exec read only conform. */

/* segment descriptor: types for system segment and gate */

#define SEG_LDT			0x00000200	/* LDT */
#define TASK_GATE		0x00000500	/* Task Gate */
#define TSS32			0x00000900	/* 32 bit TSS (available) */
#define CALL_GATE32		0x00000c00	/* 32 bit CALL gate */
#define INT_GATE32		0x00000e00	/* 32 bit INT  gate */
#define TRAP_GATE32		0x00000f00	/* 32 bit TRAP gate */

/* segment descriptor: descriptor type */

#define SYS_DESC		0x00000000	/* system descriptors */
#define APP_DESC		0x00001000	/* application descriptors */

/* segment descriptor: privilege level */

#define DPL0			0x00000000	/* privilege level 0 */
#define DPL1			0x00002000	/* privilege level 1 */
#define DPL2			0x00004000	/* privilege level 2 */
#define DPL3			0x00006000	/* privilege level 3 */

/* segment descriptor: privilege level */

#define SEG_P			0x00008000	/* present */

/* segment descriptor: default operation size */

#define DB_16			0x00000000	/* 16 bit segment */
#define DB_32			0x00400000	/* 32 bit segment */

/* segment descriptor: granularity */

#define G_BYTE			0x00000000	/* byte granularity */
#define G_4K			0x00800000	/* 4K byte granularity */

/* segment descriptor: mask bits for attribute */

#define SEG_ATTR_MASK		0x00f0ff00	/* mask bits */

/* segment descriptor: pSegdesc->type: present and busy bit */

#define SEG_PRESENT		0x80		/* present bit */
#define SEG_BUSY		0x02		/* busy bit */

/* GDT related macros */

#define GDT_ENTRIES		5

/* IDT related macros */

#define IDT_TRAP_GATE 		0x0000ef00 	/* trap gate */
#define IDT_INT_GATE  		0x0000ee00 	/* int gate */

/* TSS related macros */

#define TSS_LINK		  0	/* offset: link to previous task */
#define TSS_ESP0		  4	/* offset: privilege level 0 SP */
#define TSS_SS0	  		  8	/* offset:   ''              SS */
#define TSS_ESP1		 12	/* offset: privilege level 1 SP */
#define TSS_SS1	 		 16	/* offset:   ''              SS */
#define TSS_ESP2		 20	/* offset: privilege level 2 SP */
#define TSS_SS2			 24	/* offset:   ''              SS */
#define TSS_CR3			 28	/* offset: control register CR3 */
#define TSS_EIP			 32	/* offset: program counter  EIP */
#define TSS_EFLAGS		 36	/* offset: status register  EFLAGS */
#define TSS_EAX			 40	/* offset: general register EAX */
#define TSS_ECX			 44	/* offset: general register ECX */
#define TSS_EDX			 48	/* offset: general register EDX */
#define TSS_EBX			 52	/* offset: general register EBX */
#define TSS_ESP			 56	/* offset: stack pointer ESP */
#define TSS_EBP			 60	/* offset: frame pointer EBP */
#define TSS_ESI			 64	/* offset: general register ESI */
#define TSS_EDI			 68	/* offset: general register EDI */
#define TSS_ES			 72	/* offset: segment selector ES */
#define TSS_CS			 76	/* offset: segment selector CS */
#define TSS_SS			 80	/* offset: segment selector SS */
#define TSS_DS			 84	/* offset: segment selector DS */
#define TSS_FS			 88	/* offset: segment selector FS */
#define TSS_GS			 92	/* offset: segment selector GS */
#define TSS_LDT			 96	/* offset: segment selector LDT */
#define TSS_TFLAG		100	/* offset: debug trap flag T */
#define TSS_IOMAPB		102	/* offset: IO map base address */
#define TSS_IOBMAP		104	/* offset: IO bit map array */

#define TSS_BUSY_MASK	0xfffffdff	/* TSS descriptor BUSY bit mask */


/* inline version of intLock()/intUnlock() : used in mmuI86Lib.c */

#define INT_LOCK(oldLevel) \
    WRS_ASM ("pushf ; popl %0 ; andl $0x00000200, %0 ; cli" \
    : "=rm" (oldLevel) : /* no input */ : "memory")

#define INT_UNLOCK(oldLevel) \
    WRS_ASM ("testl $0x00000200, %0 ; jz 0f ; sti ; 0:" \
    : /* no output */ : "rm" (oldLevel) : "memory")


#ifdef __cplusplus
}
#endif

#endif	/* __INCregsI86h */
