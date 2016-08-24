/* elfI86.h - i86-specific ELF loader header */

/* Copyright 2001 Wind River Systems, Inc. */
/*
modification history
--------------------
01a,12sep01,pad  Backported from TAE 3.1 to T2.2 with necessary adaptations
                 (base version: elfI86.h@@/main/tor3_x/1).
*/

#ifndef __INCelfI86h
#define __INCelfI86h

#ifdef __cplusplus
extern "C" {
#endif

#define EM_ARCH_MACHINE		EM_386
#define EM_ARCH_MACH_ALT	EM_486    	/* not used */

/*
 * Relocation Type Definitions 
 *   The only ones that appear in vxWorks archives are "1" and "2"
 */

#define	R_386_NONE       0  /* No reloc */
#define R_386_32         1  /* Direct 32 bit  */
#define R_386_PC32       2  /* PC relative 32 bit */
#define R_386_GOT32      3  /* 32 bit GOT entry */
#define R_386_PLT32      4  /* 32 bit PLT address */
#define R_386_COPY       5  /* Copy symbol at runtime */
#define R_386_GLOB_DAT   6  /* Create GOT entry */
#define R_386_JUMP_SLOT  7  /* Create PLT entry */
#define R_386_RELATIVE   8  /* Adjust by program base */
#define R_386_GOTOFF     9  /* 32 bit offset to GOT */
#define R_386_GOTPC     10  /* 32 bit PC relative offset to GOT */

/* These 16-bit and 8-bit relocs should never show up */

#define R_386_16        20
#define R_386_PC16      21
#define R_386_8         22
#define R_386_PC8       23
#define R_386_max       24

/* These are GNU extensions to enable C++ vtable garbage collection.  */

#define R_386_GNU_VTINHERIT 250
#define R_386_GNU_VTENTRY 251

#ifdef __cplusplus
}
#endif

#endif /* __INCelfI86h */
