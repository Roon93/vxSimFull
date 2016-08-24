/* dsmLib.c - 680X0 disassembler */

/* Copyright 1984-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
03o,31oct94,kdl  merge cleanup.
03n,26oct94,tmk  added MC68LC040 support
03m,30may94,tpr  added MC68060 cpu support.
03m,13sep94,ism  fixed bug in move.l (SPR #1311)
03l,16sep92,jmm  added default to case statement in prtArgs to fix warning msg
03k,18sep92,jmm  changed commas in register lists to slashes
03j,17sep92,jmm  fixed disassembly of floating point instructions (spr 1521)
                 fixed fmovem arg list, changed (f)movem to display ranges
03i,18jul92,smb  Changed errno.h to errnoLib.h.
03h,26may92,rrr  the tree shuffle
03g,20jan92,shl  ANSI cleanup.
03f,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed copyright notice
03e,28aug91,shl  added support for MC68040's new and modified instruction set.
03d,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
03c,01apr91,yao  fixed bug for confusion of ADDX and ADD.L instructions(same
		 with SUB.L and SUBX).
03b,06mar91,jaa	 documentation cleanup.
03a,25oct90,yao  added MMU instructions. document.
02p,10aug90,dnw  added forward declarations of void functions.
02o,30may90,yao  changed to compare the first two words of instructions.
		 added effective address mode check to dsmFind ().
		 added DIVSL and DIVUL.
		 deleted findTwoWord () and findFpp (), deleted table
		 instTwo [] and instFpp [].
		 fixed excess comma's for move multiple instructions.
		 consistently used 0x instead of $ for constants.
02n,14mar90,jdi  documentation cleanup.
02m,30may88,dnw  changed to v4 names.
02l,26oct87,hin  Fixed bug, when base displacement was used without base
		 register, instruction length was wrong.
                 Added 68881 instructions.
02k,14sep87,llk  Fixed bug.  itShift instructions weren't handled correctly.
		 Split itShift into itRegShift and itMemShift.
02j,30jul87,llk  Fixed bug.  "PC indirect with index" was not handled correctly
		 for cases where there was no memory indirection.
02i,23mar87,jlf  documentation.
02h,09feb87,llk  Fixed bug.  ASL (and ASR) was displaying "ASL 0" for "ASL 8".
02g,21dec86,dnw  changed to not get include files from default directories.
02f,19dec86,llk  Fixed bug.  Collision occurred between AND and EXG.
02e,03dec86,ecs  Fixed bug in dsmPrint that caused 5-word instructions to
		 take up 2 lines.
02d,26may86,llk	 Got rid of lint introduced in 02a.
02c,23may86,llk	 Fixed bug.  CMPA was disassembled as CMPM.
02b,08apr86,llk	 Fixed bug introduced in 02a.  Negative displacements
		 of branch instructions not printed correctly.
02a,03apr86,llk	 Added 68020 instructions, reordered inst [] table by the
		 number of bits in an instruction's mask.  Added two word
		 instruction handling.  Added the routines mode6And7Words,
		 prOffWid, prIndirectIndex, prDisplace, findTwoWord.
		 Added the extension parameter to modeNwords.  Enable
		 printing of unlimited number of words per instruction.
		 Corrected spelling of "dissasembler".
		 Still needs better handling of two word instructions.
01q,20jul85,jlf  documentation.
01p,12jun85,rdc  added 68010 instructions.
01o,11nov84,jlf  Fixed shift instructions to work right.
01n,11nov84,jlf  Made EXT be its own type.
01m,20sep84,ecs&jlf  Added dsmData.
		 Changed dsmPrint to print unknown instructions as data.
		 Fixed bug in printing cmpa (introduced in 01k).
		 Made CMPM be its own type.
01l,18sep84,jlf  Removed l from the description.
01k,17sep84,jlf  Separated out adda and suba from add and sub, to fix a bug
		 with printing adda's sometimes.
01j,10sep84,ecs  Removed l, lPrtAddress, and nxtToDsm to dbgLib.c.
01g,09sep84,jlf  Added dsmNbytes, got rid of GLOBAL.
01h,06aug84,jlf  Added copyright notice, and some comments.
01g,30aug84,jlf  Fixed bug that sometimes caused ADDX and ADD.L to get
		 confused (and same for SUBX).
01f,23aug84,ecs  Changed nxtToDsm to GLOBAL.
01e,16aug84,jlf  Fixed bug in printing DBcc and Scc instructions, by adding
		 new instruction types itDb and itScc.  Changed some
		 routines to accept the address of the instruction they
		 are disassembling as a parameter, rather than just looking
		 at the pointer to the instruction.  This will make it
		 easier to make this stuff work under unix.
01d,07aug84,ecs  Added call to setStatus to dsmFind.
		 Split instruction type definitions and definition of structure
		 INST off to dsmLib.h.
01c,24jul84,ecs&jlf  Appeased lint and fixed subq bug (didn't print dest.).
01b,16jul84,jlf  Minor bug fixes.  Made unlk instruction use A reg (bug
		 had D reg) and made long immediate operands print right,
		 even if low order word is 0.
01a,29jun84,jlf	 written
*/

/*
This library contains everything necessary to print 680x0 object code in
assembly language format.  The disassembly is done in Motorola format.

The programming interface is via dsmInst(), which prints a single disassembled
instruction, and dsmNbytes(), which reports the size of an instruction.

To disassemble from the shell, use l(), which calls this
library to do the actual work.  See dbgLib() for details.

INCLUDE FILE: dsmLib.h

SEE ALSO: dbgLib
*/

#include "vxWorks.h"
#include "dsmLib.h"
#include "symLib.h"
#include "string.h"
#include "stdio.h"
#include "errnoLib.h"

#define LONGINT	 	0
#define SINGLEREAL	1
#define EXTENDEDREAL	2
#define PACKEDDECIMAL	3
#define WORDINT		4
#define DOUBLEREAL	5
#define BYTEINT		6

/* forward static functions */

static INST *dsmFind (USHORT binInst [ ]);
static void dsmPrint (USHORT binInst [ ], INST *iPtr, int address, int
		nwords, FUNCPTR prtAddress);
static int dsmNwords (USHORT binInst [ ], INST *iPtr);
static int fppNwords (USHORT mode, USHORT reg, USHORT rm, USHORT src, USHORT
		extension [ ]);
static int modeNwords (USHORT mode, USHORT reg, int size, USHORT extension [
		]);
static int mode6And7Words (USHORT extension [ ]);
static void prtArgs (USHORT binInst [ ], INST *iPtr, int address, FUNCPTR
		prtAddress);
static void prContReg (USHORT contReg);
static void prEffAddr (USHORT mode, USHORT reg, USHORT extension [ ], int
		size, FUNCPTR prtAddress);
static void prMovemRegs (USHORT extension, USHORT mode);
static void prFmovemr (USHORT mode, USHORT rlist);
static void prFmovemcr (USHORT rlist);
static void prtSizeField (USHORT binInst [ ], INST *iPtr);
static void nPrtAddress (int address);
static void prOffWid (USHORT dO, USHORT offset, USHORT dW, USHORT width);
static void prIndirectIndex (USHORT extension [ ], USHORT mode, USHORT reg);
static void prDisplace (USHORT size, USHORT pDisp [ ]);


/*
This table is ordered by the number of bits in an instruction's
two word mask, beginning with the greatest number of bits in masks.
This scheme is used for avoiding conflicts between instructions
when matching bit patterns.  The instruction ops are arranged
sequentially within each group of instructions for a particular
mask so that uniqueness can be easily spotted.
*/

LOCAL INST inst [] =
    {
    /*   26 bit mask */
    {"DIVU",	itDivL, 0x4c40, 0x0000,	  0xffc0, 0xffff,    0x02, 0x00},
    {"DIVS",	itDivL, 0x4c40, 0x0800,	  0xffc0, 0xffff,    0x02, 0x00},
    {"DIVU",	itDivL, 0x4c40, 0x1001,	  0xffc0, 0xffff,    0x02, 0x00},
    {"DIVS",	itDivL, 0x4c40, 0x1801,	  0xffc0, 0xffff,    0x02, 0x00},
    {"DIVU",	itDivL, 0x4c40, 0x2002,	  0xffc0, 0xffff,    0x02, 0x00},
    {"DIVS",	itDivL, 0x4c40, 0x2802,	  0xffc0, 0xffff,    0x02, 0x00},
    {"DIVU",	itDivL, 0x4c40, 0x3003,	  0xffc0, 0xffff,    0x02, 0x00},
    {"DIVS",	itDivL, 0x4c40, 0x3803,	  0xffc0, 0xffff,    0x02, 0x00},
    {"DIVU",	itDivL, 0x4c40, 0x4004,	  0xffc0, 0xffff,    0x02, 0x00},
    {"DIVS",	itDivL, 0x4c40, 0x4804,	  0xffc0, 0xffff,    0x02, 0x00},
    {"DIVU",	itDivL, 0x4c40, 0x5005,	  0xffc0, 0xffff,    0x02, 0x00},
    {"DIVS",	itDivL, 0x4c40, 0x5805,	  0xffc0, 0xffff,    0x02, 0x00},
    {"DIVU",	itDivL, 0x4c40, 0x6006,	  0xffc0, 0xffff,    0x02, 0x00},
    {"DIVS",	itDivL, 0x4c40, 0x6806,	  0xffc0, 0xffff,    0x02, 0x00},
    {"DIVU",	itDivL, 0x4c40, 0x7007,	  0xffc0, 0xffff,    0x02, 0x00},
    {"DIVS",	itDivL, 0x4c40, 0x7807,	  0xffc0, 0xffff,    0x02, 0x00},

    /*   24 bit mask */
    {"ORI",	itImmCCR, 0x003c, 0x0000, 0xffff, 0xff00,    0x00, 0x00},
								    /* to CCR */
    {"ANDI",	itImmCCR, 0x023c, 0x0000, 0xffff, 0xff00,    0x00, 0x00},
								    /* to CCR */
    {"EORI",	itImmCCR, 0x0a3c, 0x0000, 0xffff, 0xff00,    0x00, 0x00},
								   /* to CCR */
    {"CAS2",	itCas2,   0x0cfc, 0x0000, 0xffff, 0x0e38,    0x00, 0x00},
    {"CAS2",	itCas2,   0x0efc, 0x0000, 0xffff, 0x0e38,    0x00, 0x00},

    /*     22 bit mask	*/
    {"CHK2",	itChk2, 0x00c0, 0x0800,	  0xffc0, 0x0fff,    0x9b, 0x10},
    {"CMP2",	itChk2, 0x00c0, 0x0000,	  0xffc0, 0x0fff,    0x9b, 0x10},
    {"CHK2",	itChk2, 0x02c0, 0x0800,   0xffc0, 0x0fff,    0x9b, 0x10},
    {"CMP2",	itChk2, 0x02c0, 0x0000,	  0xffc0, 0x0fff,    0x9b, 0x10},
    {"CHK2",	itChk2, 0x04c0, 0x0800,	  0xffc0, 0x0fff,    0x9b, 0x10},
    {"CMP2",	itChk2, 0x04c0, 0x0000,	  0xffc0, 0x0fff,    0x9b, 0x10},
    {"MOVES",	itMoves,0x0e00, 0x0000,   0xffc0, 0x0fff,    0x83, 0x1c},
    {"MOVES",	itMoves,0x0e40, 0x0000,   0xffc0, 0x0fff,    0x83, 0x1c},
    {"MOVES",	itMoves,0x0e80, 0x0000,   0xffc0, 0x0fff,    0x83, 0x1c},

    /*     21 bit mask	*/
    {"CAS",	itCas, 0x0ac0, 0x0000,	  0xffc0, 0xfe38,    0x83, 0x1c},
    {"CAS",	itCas, 0x0cc0, 0x0000,	  0xffc0, 0xfe38,    0x83, 0x1c},
    {"CAS",	itCas, 0x0ec0, 0x0000,	  0xffc0, 0xfe38,    0x83, 0x1c},

    /*   Fpp instructions */
    {"FABS",	itFabs,   0xf200, 0x0018,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FACOS",   itFacos,  0xf200, 0x001c,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FADD",    itFadd,   0xf200, 0x0022,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FASIN",	itFasin,  0xf200, 0x000c,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FATAN",	itFatan,  0xf200, 0x000a,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FATANH",	itFatanh, 0xf200, 0x000b,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FCMP",	itFcmp,   0xf200, 0x0038,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FCOS",	itFcos,   0xf200, 0x001d,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FCOSH",	itFcosh,  0xf200, 0x0019,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FDB",	itFdb,    0xf248, 0x0000,   0xfff8, 0xffc0,    0x00, 0x00},
    {"FDIV",	itFdiv,   0xf200, 0x0020,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FETOX",	itFetox,  0xf200, 0x0010,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FETOXM1",	itFetoxm1,0xf200, 0x0008,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FGETEXP",	itFgetexp,0xf200, 0x001e,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FGETMAN",	itFgetman,0xf200, 0x001f,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FINT",	itFint,   0xf200, 0x0001,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FINTRZ",	itFintrz, 0xf200, 0x0003,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FLOG10",	itFlog10, 0xf200, 0x0015,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FLOG2",	itFlog2,  0xf200, 0x0016,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FLOGN",	itFlogn,  0xf200, 0x0014,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FLOGNP1",	itFlognp1,0xf200, 0x0006,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FMOD",	itFmod,   0xf200, 0x0021,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FMOVE",	itFmove,  0xf200, 0x0000,   0xffc0, 0xffff,    0x00, 0x00},
    {"FMOVE",	itFmove,  0xf200, 0x4000,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FMOVE",	itFmovek, 0xf200, 0x6000,   0xffc0, 0xe000,    0x82, 0x1c},
    {"FMOVE",	itFmovel, 0xf200, 0x8000,   0xffc0, 0xe3ff,    0x00, 0x00},
    {"FMOVE",	itFmovel, 0xf200, 0xa000,   0xffc0, 0xe3ff,    0x80, 0x1c},
    {"FMOVECR",	itFmovecr,0xf200, 0x5c00,   0xffc0, 0xfc00,    0x00, 0x00},
    {"FMOVEM",	itFmovem, 0xf200, 0xc000,   0xffc0, 0xe700,    0x83, 0x10},
    {"FMOVEM",	itFmovem, 0xf200, 0xe000,   0xffc0, 0xe700,    0x83, 0x1c},
    {"FMOVEM",	itFmovemc,0xf200, 0xc000,   0xffc0, 0xe3ff,    0x00, 0x00},
    {"FMOVEM",	itFmovemc,0xf200, 0xe000,   0xffc0, 0xe3ff,    0x80, 0x1c},
    {"FMUL",	itFmul,   0xf200, 0x0023,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FMUL",	itFmul,   0xf200, 0x4023,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FNEG",	itFneg,   0xf200, 0x001a,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FNEG",	itFneg,   0xf200, 0x401a,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FNOP",	itFnop,   0xf200, 0x0000,   0xffc0, 0xffff,    0x00, 0x00},
    {"FREM",	itFrem,   0xf200, 0x0025,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FREM",	itFrem,   0xf200, 0x4025,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FSCALE",	itFscale, 0xf200, 0x0026,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FSCALE",	itFscale, 0xf200, 0x4026,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FS",	itFs,     0xf200, 0x0000,   0xffc0, 0xffc0,    0x82, 0x1c},
    {"FSGLDIV",	itFsgldiv,0xf200, 0x0024,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FSGLDIV",	itFsgldiv,0xf200, 0x4024,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FSGLMUL",	itFsglmul,0xf200, 0x0027,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FSGLMUL",	itFsglmul,0xf200, 0x4027,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FSIN",	itFsin,   0xf200, 0x000e,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FSIN",	itFsin,   0xf200, 0x400e,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FSINCOS",	itFsincos,0xf200, 0x0030,   0xffc0, 0xe078,    0x00, 0x00},
    {"FSINCOS",	itFsincos,0xf200, 0x4030,   0xffc0, 0xe078,    0x00, 0x00},
    {"FSINH",	itFsinh,  0xf200, 0x0002,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FSINH",	itFsinh,  0xf200, 0x4002,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FSQRT",	itFsqrt,  0xf200, 0x0004,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FSQRT",	itFsqrt,  0xf200, 0x4004,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FSUB",	itFsub,   0xf200, 0x0028,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FSUB",	itFsub,   0xf200, 0x4028,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FTAN",	itFtan,   0xf200, 0x000f,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FTAN",	itFtan,   0xf200, 0x400f,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FTANH",	itFtanh,  0xf200, 0x0009,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FTANH",	itFtanh,  0xf200, 0x4009,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FTENTOX",	itFtentox,0xf200, 0x0012,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FTENTOX",	itFtentox,0xf200, 0x4012,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FTRAP",	itFtrap,  0xf200, 0x0000,   0xffc0, 0xffc0,    0x00, 0x00},
    {"FTST",	itFtst,   0xf200, 0x003a,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FTST",	itFtst,   0xf200, 0x403a,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FTWOTOX",	itFtwotox,0xf200, 0x0011,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FTWOTOX",	itFtwotox,0xf200, 0x4011,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FB",	itFb,     0xf280, 0x0000,   0xff80, 0x0000,    0x00, 0x00},
    {"FRESTORE",itFrestore,0xf340,0x0000,   0xffc0, 0x0000,    0x93, 0x10},
    {"FSAVE",	itFsave,  0xf300, 0x0000,   0xffc0, 0x0000,    0x8b, 0x1c},

#if (CPU==MC68040 || CPU==MC68060 || CPU == MC68LC040)
    {"PFLUSHN",itPflush,  0xf500, 0x0000, 0xfff8, 0x0000,    0x00, 0x00},
    {"PFLUSH", itPflush,  0xf508, 0x0000, 0xfff8, 0x0000,    0x00, 0x00},
    {"PFLUSHAN",itPflush, 0xf510, 0x0000, 0xfff8, 0x0000,    0x00, 0x00},
    {"PFLUSHA",itPflush,  0xf518, 0x0000, 0xfff8, 0x0000,    0x00, 0x00},
    {"PTESTW", itPtest,   0xf548, 0x0000, 0xfff8, 0x0000,    0x00, 0x00},
    {"PTESTR", itPtest,   0xf568, 0x0000, 0xfff8, 0x0000,    0x00, 0x00},

    {"CINVL",  itCinv,    0xf408, 0x0000, 0xff38, 0x0000,    0x00, 0x00},
    {"CINVP",  itCinv,    0xf410, 0x0000, 0xff38, 0x0000,    0x00, 0x00},
    {"CINVA",  itCinva,   0xf418, 0x0000, 0xff38, 0x0000,    0x00, 0x00},

    {"CPUSHL", itCpush,	  0xf428, 0x0000, 0xff38, 0x0000,    0x00, 0x00},
    {"CPUSHP", itCpush,	  0xf430, 0x0000, 0xff38, 0x0000,    0x00, 0x00},
    {"CPUSHA", itCpusha,  0xf438, 0x0000, 0xff38, 0x0000,    0x00, 0x00},

    {"MOVE16", itMove16,  0xf620, 0x8000, 0xfff8, 0x8fff,    0x00, 0x00},
    {"MOVE16", itMove16L, 0xf600, 0x0000, 0xfff8, 0x0000,    0x00, 0x00},
    {"MOVE16", itMove16L, 0xf608, 0x0000, 0xfff8, 0x0000,    0x00, 0x00},
    {"MOVE16", itMove16L, 0xf610, 0x0000, 0xfff8, 0x0000,    0x00, 0x00},
    {"MOVE16", itMove16L, 0xf618, 0x0000, 0xfff8, 0x0000,    0x00, 0x00},

    /* 68040 alternations to the 68881/2 instruction set */

    /* XXX bug in Motorola manual */
    /*
    {"FSABS",	itFabs,   0xf200, 0x00??,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FDABS",	itFabs,   0xf200, 0x00??,   0xffc0, 0xa07f,    0x02, 0x00},
    */

    {"FSADD",   itFadd,   0xf200, 0x0062,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FDADD",   itFadd,   0xf200, 0x0066,   0xffc0, 0xa07f,    0x02, 0x00},

    {"FSDIV",   itFdiv,   0xf200, 0x0060,   0xffc0, 0xa07f,    0x02, 0x00},
    {"FDDIV",   itFdiv,   0xf200, 0x0064,   0xffc0, 0xa07f,    0x02, 0x00},

    {"FSMOVE",  itFmove,  0xf200, 0x0040,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FDMOVE",  itFmove,  0xf200, 0x0044,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FSMOVE",  itFmove,  0xf200, 0x4040,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FDMOVE",  itFmove,  0xf200, 0x4044,   0xffc0, 0xe07f,    0x02, 0x00},

    {"FSMUL",   itFmul,   0xf200, 0x0063,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FDMUL",   itFmul,   0xf200, 0x0067,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FSMUL",   itFmul,   0xf200, 0x4063,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FDMUL",   itFmul,   0xf200, 0x4067,   0xffc0, 0xe07f,    0x02, 0x00},

    {"FSNEG",   itFneg,   0xf200, 0x005a,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FDNEG",   itFneg,   0xf200, 0x005e,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FSNEG",   itFneg,   0xf200, 0x405a,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FDNEG",   itFneg,   0xf200, 0x405e,   0xffc0, 0xe07f,    0x02, 0x00},

    {"FSSQRT",  itFsqrt,  0xf200, 0x0041,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FDSQRT",  itFsqrt,  0xf200, 0x0045,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FSSQRT",  itFsqrt,  0xf200, 0x4041,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FDSQRT",  itFsqrt,  0xf200, 0x4045,   0xffc0, 0xe07f,    0x02, 0x00},

    {"FSSUB",   itFsub,   0xf200, 0x0068,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FDSUB",   itFsub,   0xf200, 0x006c,   0xffc0, 0xe07f,    0x00, 0x00},
    {"FSSUB",   itFsub,   0xf200, 0x4068,   0xffc0, 0xe07f,    0x02, 0x00},
    {"FDSUB",   itFsub,   0xf200, 0x406c,   0xffc0, 0xe07f,    0x02, 0x00},

#endif	/* (CPU==MC68040) || (CPU == MC68060) || (CPU == MC68LC040)) */

#if (CPU == MC68020 || CPU == MC68030)
/* MMU instructions */

    {"PFLUSHA",	itPflush, 0xf000, 0x2400, 0xffc0, 0xffff,    0x00, 0x00},
    {"PFLUSH",	itPflush, 0xf000, 0x3000, 0xffc0, 0xff1f,    0x00, 0x00},
    {"PMOVEFD",	itPmove,  0xf000, 0x0800, 0xffc0, 0xfdcf,    0x9b, 0x1c},
    {"PMOVE",	itPmove,  0xf000, 0x0900, 0xffc0, 0xfdcf,    0x9b, 0x1c},
    {"PMOVEFD",	itPmove,  0xf000, 0x0c00, 0xffc0, 0xfdcf,    0x9b, 0x1c},
    {"PMOVE",	itPmove,  0xf000, 0x0d00, 0xffc0, 0xfdcf,    0x9b, 0x1c},

    {"PLOADW",	itPload,  0xf000, 0x2000, 0xffc0, 0xffff,    0xc9, 0x1c},
    {"PLOADW",	itPload,  0xf000, 0x2001, 0xffc0, 0xffff,    0xc9, 0x1c},
    {"PLOADW",	itPload,  0xf000, 0x2008, 0xffc0, 0xfff8,    0xc9, 0x1c},
    {"PLOADW",	itPload,  0xf000, 0x2010, 0xffc0, 0xfff8,    0xc9, 0x1c},
    {"PLOADR",	itPload,  0xf000, 0x2200, 0xffc0, 0xffff,    0xc9, 0x1c},
    {"PLOADR",	itPload,  0xf000, 0x2201, 0xffc0, 0xffff,    0xc9, 0x1c},
    {"PLOADR",	itPload,  0xf000, 0x2208, 0xffc0, 0xfff8,    0xc9, 0x1c},
    {"PLOADR",	itPload,  0xf000, 0x2210, 0xffc0, 0xfff8,    0xc9, 0x1c},

    {"PFLUSH",	itPflush, 0xf000, 0x3001, 0xffc0, 0xff1f,    0x00, 0x00},
    {"PFLUSH",	itPflush, 0xf000, 0x3008, 0xffc0, 0xff18,    0x00, 0x00},
    {"PFLUSH",	itPflush, 0xf000, 0x3010, 0xffc0, 0xff18,    0x00, 0x00},
    {"PFLUSH",	itPflush, 0xf000, 0x3800, 0xffc0, 0xff1f,    0x8b, 0x1c},
    {"PFLUSH",	itPflush, 0xf000, 0x3801, 0xffc0, 0xff1f,    0x8b, 0x1c},
    {"PFLUSH",	itPflush, 0xf000, 0x3808, 0xffc0, 0xff18,    0x00, 0x00},
    {"PFLUSH",	itPflush, 0xf000, 0x3810, 0xffc0, 0xff18,    0x00, 0x00},

    {"PMOVEFD",	itPmove,  0xf000, 0x4000, 0xffc0, 0xfdcf,    0x9b, 0x1c},
    {"PMOVE",	itPmove,  0xf000, 0x4100, 0xffc0, 0xfdcf,    0x9b, 0x1c},
    {"PMOVEFD",	itPmove,  0xf000, 0x4800, 0xffc0, 0xfdcf,    0x9b, 0x1c},
    {"PMOVE",	itPmove,  0xf000, 0x4900, 0xffc0, 0xfdcf,    0x9b, 0x1c},
    {"PMOVEFD",	itPmove,  0xf000, 0x4c00, 0xffc0, 0xfdcf,    0x9b, 0x1c},
    {"PMOVE",	itPmove,  0xf000, 0x4d00, 0xffc0, 0xfdcf,    0x9b, 0x1c},
    {"PMOVE",	itPmove,  0xf000, 0x6000, 0xffc0, 0xfdff,    0x9b, 0x1c},

    {"PTESTW",	itPtest,  0xf000, 0x8000, 0xffc0, 0xe21f,    0x8b, 0x1c},
    {"PTESTW",	itPtest,  0xf000, 0x8001, 0xffc0, 0xe21f,    0x8b, 0x1c},
    {"PTESTW",	itPtest,  0xf000, 0x8008, 0xffc0, 0xe218,    0x8b, 0x1c},
    {"PTESTW",	itPtest,  0xf000, 0x8010, 0xffc0, 0xe218,    0x8b, 0x1c},
    {"PTESTR",	itPtest,  0xf000, 0x8200, 0xffc0, 0xe21f,    0x8b, 0x1c},
    {"PTESTR",	itPtest,  0xf000, 0x8201, 0xffc0, 0xe21f,    0x8b, 0x1c},
    {"PTESTR",	itPtest,  0xf000, 0x8208, 0xffc0, 0xe218,    0x8b, 0x1c},
    {"PTESTR",	itPtest,  0xf000, 0x8210, 0xffc0, 0xe218,    0x8b, 0x1c},
#endif	/* CPU == MC68020 || CPU == MC68030) */

    /*     20 bit mask	*/
    {"DIVUL",	itDivL,    0x4c40, 0x0000,   0xffc0, 0x8ff8,    0x02, 0x00},
    {"DIVU",	itDivL,    0x4c40, 0x0400,   0xffc0, 0x8ff8,    0x02, 0x00},
    {"DIVSL",	itDivL,    0x4c40, 0x0800,   0xffc0, 0x8ff8,    0x02, 0x00},
    {"DIVS",	itDivL,    0x4c40, 0x0c00,   0xffc0, 0x8ff8,    0x02, 0x00},
    {"cpDBcc",	itCpDbcc,  0xf048, 0x0000,   0xf1f8, 0xffc0,    0x00, 0x00},
    {"cpTRAP",	itCpTrapcc,0xf078, 0x0000,   0xf1f8, 0xffc0,    0x00, 0x00},

    /*    19 bit mask	*/
    {"MULS",	itDivL, 0x4c00, 0x0800,	  0xffc0, 0x8bf8,    0x02, 0x00},
    {"MULU",	itDivL, 0x4c00, 0x0000,	  0xffc0, 0x8bf8,    0x02, 0x00},

    /*    18 bit mask	*/
    {"BTST",	itStatBit, 0x0800,0x0000,   0xffc0, 0xff00,    0x82, 0x10},
    {"BCHG",	itStatBit, 0x0840,0x0000,   0xffc0, 0xff00,    0x82, 0x1c},
    {"BCLR",	itStatBit, 0x0880,0x0000,   0xffc0, 0xff00,    0x82, 0x1c},
    {"CALLM",	itCallm,   0x06c0,0x0000,   0xffc0, 0xff00,    0x9b, 0x10},

    /*    17 bit mask	*/
    {"BSET",	itStatBit,  0x08c0, 0x0000,  0xffc0, 0xfe00,    0x82, 0x1c},

    /*    16 bit mask */
    {"ANDI",	itImmTSR,   0x027c, 0x0000,  0xffff, 0x0000,    0x00, 0x00},
								    /* to SR */
    {"ORI",	itImmTSR,   0x007c, 0x0000,  0xffff, 0x0000,    0x00, 0x00},
								    /* to SR */
    {"EORI",	itImmTSR,   0x0a7c, 0x0000,  0xffff, 0x0000,    0x00, 0x00},
								    /* to SR */
    {"ILL",	itComplete, 0x4afc, 0x0000,  0xffff, 0x0000,    0x00, 0x00},
    {"RESET",	itComplete, 0x4e70, 0x0000,  0xffff, 0x0000,    0x00, 0x00},
    {"NOP",	itComplete, 0x4e71, 0x0000,  0xffff, 0x0000,    0x00, 0x00},
    {"STOP",	itStop,     0x4e72, 0x0000,  0xffff, 0x0000,    0x00, 0x00},
    {"RTE",	itComplete, 0x4e73, 0x0000,  0xffff, 0x0000,    0x00, 0x00},
    {"RTD",	itRTD, 	    0x4e74, 0x0000,  0xffff, 0x0000,    0x00, 0x00},
    {"RTS",	itComplete, 0x4e75, 0x0000,  0xffff, 0x0000,    0x00, 0x00},
    {"TRAPV",	itComplete, 0x4e76, 0x0000,  0xffff, 0x0000,    0x00, 0x00},
    {"RTR",	itComplete, 0x4e77, 0x0000,  0xffff, 0x0000,    0x00, 0x00},

    /*     15 bit mask	*/
    {"MOVEC",	itMovec,    0x4e7a, 0x0000,  0xfffe, 0x0000,    0x00, 0x00},

    /*     14 bit mask	*/
    {"BFCHG",	itBfchg,    0xeac0, 0x0000,  0xffc0, 0xf000,    0x9a, 0x1c},
    {"BFCLR",	itBfchg,    0xecc0, 0x0000,  0xffc0, 0xf000,    0x9a, 0x1c},
    {"BFSET",	itBfchg,    0xeec0, 0x0000,  0xffc0, 0xf000,    0x9a, 0x1c},

    /*    13 bit mask	*/
    {"LINK",	itLinkL,    0x4808, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"SWAP",	itSwap,     0x4840, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"BKPT",	itBkpt,     0x4848, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"EXT",	itExt,      0x4880, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"EXT",	itExt,      0x48c0, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"EXTB",	itExt,      0x49c0, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"LINK",	itLink,     0x4e50, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"UNLK",	itUnlk,     0x4e58, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"DBT",	itDb,       0x50c8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"TRAPT",	itTrapcc,   0x50f8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"DBF",	itDb,       0x51c8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"TRAPF",	itTrapcc,   0x51f8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"DBHI",	itDb,       0x52c8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"TRAPHI",	itTrapcc,   0x52f8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"DBLS",	itDb,       0x53c8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"TRAPLS",	itTrapcc,   0x53f8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"DBCC",	itDb,       0x54c8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"TRAPCC",	itTrapcc,   0x54f8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"DBCS",	itDb,       0x55c8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"TRAPCS",	itTrapcc,   0x55f8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"DBNE",	itDb,       0x56c8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"TRAPNE",	itTrapcc,   0x56f8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"DBEQ",	itDb,       0x57c8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"TRAPEQ",	itTrapcc,   0x57f8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"DBVC",	itDb,       0x58c8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"TRAPVC",	itTrapcc,   0x58f8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"DBVS",	itDb,       0x59c8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"TRAPVS",	itTrapcc,   0x59f8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"DBPL",	itDb,       0x5ac8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"TRAPPL",	itTrapcc,   0x5af8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"DBMI",	itDb,       0x5bc8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"TRAPMI",	itTrapcc,   0x5bf8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"DBGE",	itDb,       0x5cc8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"TRAPGE",	itTrapcc,   0x5cf8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"DBLT",	itDb,       0x5dc8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"TRAPLT",	itTrapcc,   0x5df8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"DBGT",	itDb, 	    0x5ec8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"TRAPGT",	itTrapcc,   0x5ef8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"DBLE",	itDb,       0x5fc8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"TRAPLE",	itTrapcc,   0x5ff8, 0x0000,  0xfff8, 0x0000,    0x00, 0x00},
    {"BFTST",	itBfchg,    0xe8c0, 0x0000,  0xffc0, 0xe000,    0x9a, 0x10},


    /*     12 bit mask	*/
    {"RTM",	itRtm, 	    0x06c0, 0x0000,  0xfff0, 0x0000,    0x00, 0x00},
    {"TRAP",	itTrap,     0x4e40, 0x0000,  0xfff0, 0x0000,    0x00, 0x00},
    {"MOVE",	itMoveUSP,  0x4e60, 0x0000,  0xfff0, 0x0000,    0x00, 0x00},
								    /* USP */
    /*    11 bit mask	*/
    {"BFEXTU",	itBfext,    0xe9c0, 0x0000,  0xffc0, 0x8000,    0x9a, 0x10},
    {"BFEXTS",	itBfext,    0xebc0, 0x0000,  0xffc0, 0x8000,    0x9a, 0x10},
    {"BFFFO",	itBfext,    0xedc0, 0x0000,  0xffc0, 0x8000,    0x9a, 0x10},
    {"BFINS",	itBfins,    0xefc0, 0x0000,  0xffc0, 0x8000,    0x9a, 0x1c},

    /*     10 bit mask	*/
    {"ORI",	itImm,      0x0000, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"ORI",	itImm,      0x0040, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"ORI",	itImm,      0x0080, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"ANDI",	itImm,      0x0200, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"ANDI",	itImm,      0x0240, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"ANDI",	itImm,      0x0280, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SUBI",	itImm,      0x0400, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SUBI",	itImm,      0x0440, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SUBI",	itImm,      0x0480, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"ADDI",	itImm,      0x0600, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"ADDI",	itImm,      0x0640, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"ADDI",	itImm,      0x0680, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"EORI",	itImm,      0x0a00, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"EORI",	itImm,      0x0a40, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"EORI",	itImm,      0x0a80, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"CMPI",	itImm,      0x0c00, 0x0000,  0xffc0, 0x0000,    0x82, 0x10},
    {"CMPI",	itImm,      0x0c40, 0x0000,  0xffc0, 0x0000,    0x82, 0x10},
    {"CMPI",	itImm,      0x0c80, 0x0000,  0xffc0, 0x0000,    0x82, 0x10},
    {"MOVE",	itMoveFSR,  0x40c0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
					 		    /* from SR*/
    {"MOVE",	itMoveFCCR, 0x42c0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    								    /*from CCR*/
    {"MOVE",	itMoveCCR,  0x44c0, 0x0000,  0xffc0, 0x0000,    0x02, 0x00},
								    /* to CCR */
    {"MOVE",	itMoveTSR,  0x46c0, 0x0000,  0xffc0, 0x0000,    0x02, 0x00},
								    /* to SR */
    {"NBCD",	itNbcd,     0x4800, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"PEA",	itNbcd,     0x4840, 0x0000,  0xffc0, 0x0000,    0x9b, 0x10},
    {"TAS",	itNbcd,     0x4ac0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"JSR",	itNbcd,     0x4e80, 0x0000,  0xffc0, 0x0000,    0x9b, 0x10},
    {"JMP",	itNbcd,     0x4ec0, 0x0000,  0xffc0, 0x0000,    0x9b, 0x10},
    {"ST",	itScc,      0x50c0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SF",	itScc,      0x51c0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SHI",	itScc,      0x52c0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SLS",	itScc,      0x53c0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SCC",	itScc,      0x54c0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SCS",	itScc,      0x55c0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SNE",	itScc,      0x56c0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SEQ",	itScc,      0x57c0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SVC",	itScc,      0x58c0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SVS",	itScc,      0x59c0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SPL",	itScc,      0x5ac0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SMI",	itScc,      0x5bc0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SGE",	itScc,      0x5cc0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SLT",	itScc,      0x5dc0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SGT",	itScc,      0x5ec0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"SLE",	itScc,      0x5fc0, 0x0000,  0xffc0, 0x0000,    0x82, 0x1c},
    {"ASR",	itMemShift, 0xe0c0, 0x0000,  0xffc0, 0x0000,    0x83, 0x1c},
    {"ASL",	itMemShift, 0xe1c0, 0x0000,  0xffc0, 0x0000,    0x83, 0x1a},
    {"LSR",	itMemShift, 0xe2c0, 0x0000,  0xffc0, 0x0000,    0x83, 0x1c},
    {"LSL",	itMemShift, 0xe3c0, 0x0000,  0xffc0, 0x0000,    0x83, 0x1c},
    {"ROXR",	itMemShift, 0xe4c0, 0x0000,  0xffc0, 0x0000,    0x83, 0x1c},
    {"ROXL",	itMemShift, 0xe5c0, 0x0000,  0xffc0, 0x0000,    0x83, 0x1c},
    {"ROR",	itMemShift, 0xe6c0, 0x0000,  0xffc0, 0x0000,    0x83, 0x1c},
    {"ROL",	itMemShift, 0xe7c0, 0x0000,  0xffc0, 0x0000,    0x83, 0x1c},

    /*     10 bit mask	*/
    {"CMPM",	itCmpm, 0xb108, 0x0000,	  0xf1f8, 0x0000,    0x00, 0x00},
    {"CMPM",	itCmpm, 0xb148,	0x0000,   0xf1f8, 0x0000,    0x00, 0x00},
    {"CMPM",	itCmpm, 0xb188, 0x0000,	  0xf1f8, 0x0000,    0x00, 0x00},
    {"EXG",	itExg,  0xc140,	0x0000,   0xf1f8, 0x0000,    0x00, 0x00},
    {"EXG",	itExg,  0xc148,	0x0000,   0xf1f8, 0x0000,    0x00, 0x00},
    {"EXG",	itExg,  0xc188,	0x0000,   0xf1f8, 0x0000,    0x00, 0x00},

    /*     9 bit mask	*/
    {"MOVEM",	itMovem, 0x4880, 0x0000, 0xff80, 0x0000,    0x8b, 0x1c},
    {"MOVEM",	itMovem, 0x4c80, 0x0000, 0xff80, 0x0000,    0x93, 0x10},
    {"SBCD",	itBcd,   0x8100, 0x0000, 0xf1f0, 0x0000,    0x00, 0x00},
    {"PACK",	itPack,  0x8140, 0x0000, 0xf1f0, 0x0000,    0x00, 0x00},
    {"UNPK",	itPack,  0x8180, 0x0000, 0xf1f0, 0x0000,    0x00, 0x00},
    {"ABCD",	itBcd,   0xc100, 0x0000, 0xf1f0, 0x0000,    0x00, 0x00},
    {"SUBX",	itX,     0x9100, 0x0000, 0xf1f0, 0x0000,    0x00, 0x00},
    {"SUBX",	itX,     0x9140, 0x0000, 0xf1f0, 0x0000,    0x00, 0x00},
    {"SUBX",	itX,     0x9180, 0x0000, 0xf1f0, 0x0000,    0x00, 0x00},
    {"ADDX",	itX,     0xd100, 0x0000, 0xf1f0, 0x0000,    0x00, 0x00},
    {"ADDX",	itX,     0xd140, 0x0000, 0xf1f0, 0x0000,    0x00, 0x00},
    {"ADDX",	itX,     0xd180, 0x0000, 0xf1f0, 0x0000,    0x00, 0x00},


    /*     8 bit mask	*/
    {"NEGX",	itNegx, 0x4000,	0x0000,   0xff00, 0x0000,    0x82, 0x1c},
    {"CLR",	itNegx, 0x4200,	0x0000,   0xff00, 0x0000,    0x82, 0x1c},
    {"NEG",	itNegx, 0x4400,	0x0000,   0xff00, 0x0000,    0x82, 0x1c},
    {"NOT",	itNegx, 0x4600,	0x0000,   0xff00, 0x0000,    0x82, 0x1c},
    {"TST",	itNegx, 0x4a00,	0x0000,   0xff00, 0x0000,    0x80, 0x10},
    {"BRA",	itBra,  0x6000,	0x0000,   0xff00, 0x0000,    0x00, 0x00},
    {"BSR",	itBra,  0x6100,	0x0000,   0xff00, 0x0000,    0x00, 0x00},
    {"BHI",	itBra,  0x6200, 0x0000,	  0xff00, 0x0000,    0x00, 0x00},
    {"BLS",	itBra,  0x6300,	0x0000,   0xff00, 0x0000,    0x00, 0x00},
    {"BCC",	itBra,  0x6400,	0x0000,   0xff00, 0x0000,    0x00, 0x00},
    {"BCS",	itBra,  0x6500,	0x0000,   0xff00, 0x0000,    0x00, 0x00},
    {"BNE",	itBra,  0x6600,	0x0000,   0xff00, 0x0000,    0x00, 0x00},
    {"BEQ",	itBra,  0x6700,	0x0000,   0xff00, 0x0000,    0x00, 0x00},
    {"BVC",	itBra,  0x6800,	0x0000,   0xff00, 0x0000,    0x00, 0x00},
    {"BVS",	itBra,  0x6900,	0x0000,   0xff00, 0x0000,    0x00, 0x00},
    {"BPL",	itBra,  0x6a00,	0x0000,   0xff00, 0x0000,    0x00, 0x00},
    {"BMI",	itBra,  0x6b00,	0x0000,   0xff00, 0x0000,    0x00, 0x00},
    {"BGE",	itBra,  0x6c00,	0x0000,   0xff00, 0x0000,    0x00, 0x00},
    {"BLT",	itBra,  0x6d00, 0x0000,	  0xff00, 0x0000,    0x00, 0x00},
    {"BGT",	itBra,  0x6e00,	0x0000,   0xff00, 0x0000,    0x00, 0x00},
    {"BLE",	itBra,  0x6f00,	0x0000,   0xff00, 0x0000,    0x00, 0x00},

    /*     7 bit mask	*/
    {"BTST",	itDynBit, 0x0100, 0x0000,  0xf1c0, 0x0000,    0x02, 0x00},
    {"BCHG",	itDynBit, 0x0140, 0x0000,  0xf1c0, 0x0000,    0x82, 0x1c},
    {"BCLR",	itDynBit, 0x0180, 0x0000,  0xf1c0, 0x0000,    0x82, 0x1c},
    {"BSET",	itDynBit, 0x01c0, 0x0000,  0xf1c0, 0x0000,    0x82, 0x1c},
    {"CHK",	itChk,    0x4100, 0x0000,  0xf1c0, 0x0000,    0x02, 0x00},
    {"CHK",	itChk,    0x4180, 0x0000,  0xf1c0, 0x0000,    0x02, 0x00},
    {"LEA",	itLea,    0x41c0, 0x0000,  0xf1c0, 0x0000,    0x9b, 0x10},
    {"DIVU",	itDivW,   0x80c0, 0x0000,  0xf1c0, 0x0000,    0x02, 0x00},
    {"DIVS",	itDivW,   0x81c0, 0x0000,  0xf1c0, 0x0000,    0x02, 0x00},
    {"SUB",	itOr,     0x9000, 0x0000,  0xf1c0, 0x0000,    0x00, 0x00},
    {"SUB",	itOr,     0x9040, 0x0000,  0xf1c0, 0x0000,    0x00, 0x00},
    {"SUB",	itOr,     0x9080, 0x0000,  0xf1c0, 0x0000,    0x00, 0x00},
    {"SUBA",	itAdda,   0x90c0, 0x0000,  0xf1c0, 0x0000,    0x00, 0x00},
    {"SUB",	itOr,     0x9100, 0x0000,  0xf1c0, 0x0000,    0x82, 0x1c},
    {"SUB",	itOr,     0x9140, 0x0000,  0xf1c0, 0x0000,    0x82, 0x1c},
    {"SUB",	itOr,     0x9180, 0x0000,  0xf1c0, 0x0000,    0x82, 0x1c},
    {"SUBA",	itAdda,   0x91c0, 0x0000,  0xf1c0, 0x0000,    0x00, 0x00},
    {"CMP",	itOr,     0xb000, 0x0000,  0xf1c0, 0x0000,    0x00, 0x00},
    {"CMP",	itOr,     0xb040, 0x0000,  0xf1c0, 0x0000,    0x00, 0x00},
    {"CMP",	itOr,     0xb080, 0x0000,  0xf1c0, 0x0000,    0x00, 0x00},
    {"CMPA",	itAdda,   0xb0c0, 0x0000,  0xf1c0, 0x0000,    0x00, 0x00},
    {"CMPA",	itAdda,   0xb1c0, 0x0000,  0xf1c0, 0x0000,    0x00, 0x00},
    {"MULU",	itDivW,   0xc0c0, 0x0000,  0xf1c0, 0x0000,    0x02, 0x00},
    {"MULS",	itDivW,   0xc1c0, 0x0000,  0xf1c0, 0x0000,    0x02, 0x00},
    {"ADD",	itOr,     0xd000, 0x0000,  0xf1c0, 0x0000,    0x00, 0x00},
    {"ADD",	itOr,     0xd040, 0x0000,  0xf1c0, 0x0000,    0x00, 0x00},
    {"ADD",	itOr,     0xd080, 0x0000,  0xf1c0, 0x0000,    0x00, 0x00},
    {"ADDA",	itAdda,   0xd0c0, 0x0000,  0xf1c0, 0x0000,    0x00, 0x00},
    {"ADD",	itOr,     0xd100, 0x0000,  0xf1c0, 0x0000,    0x82, 0x1c},
    {"ADD",	itOr,     0xd140, 0x0000,  0xf1c0, 0x0000,    0x82, 0x1c},
    {"ADD",	itOr,     0xd180, 0x0000,  0xf1c0, 0x0000,    0x82, 0x1c},
    {"ADDA",	itAdda,   0xd1c0, 0x0000,  0xf1c0, 0x0000,    0x00, 0x00},

    /*     7 bit mask	*/
    {"ASR",	itRegShift, 0xe000,0x0000,   0xf118, 0x0000,    0x00, 0x00},
    {"LSR",	itRegShift, 0xe008,0x0000,   0xf118, 0x0000,    0x00, 0x00},
    {"ROXR",	itRegShift, 0xe010,0x0000,   0xf118, 0x0000,    0x00, 0x00},
    {"ROR",	itRegShift, 0xe018,0x0000,   0xf118, 0x0000,    0x00, 0x00},
    {"ASL",	itRegShift, 0xe100,0x0000,   0xf118, 0x0000,    0x00, 0x00},
    {"LSL",	itRegShift, 0xe108,0x0000,   0xf118, 0x0000,    0x00, 0x00},
    {"ROXL",	itRegShift, 0xe110,0x0000,   0xf118, 0x0000,    0x00, 0x00},
    {"ROL",	itRegShift, 0xe118,0x0000,   0xf118, 0x0000,    0x00, 0x00},

    /*     7 bit mask	*/
    {"MOVEP",	itMovep, 0x0080, 0x0000,   0xf038, 0x0000,    0x00, 0x00},

    /*     6 bit mask	*/
    {"cpBcc",	itCpBcc, 0xf080, 0x0000,   0xf180, 0x0000,    0x00, 0x00},

    /*     5 bit mask	*/
    {"MOVEA",	itMoveA, 0x0040, 0x0000,   0xc1c0, 0x0000,    0x00, 0x00},

    /*     5 bit mask	*/
    {"ADDQ",	itQuick, 0x5000, 0x0000,  0xf100, 0x0000,    0x80, 0x1c},
    {"SUBQ",	itQuick, 0x5100, 0x0000,  0xf100, 0x0000,    0x80, 0x1c},
    {"MOVEQ",	itMoveq, 0x7000, 0x0000,  0xf100, 0x0000,    0x00, 0x00},

    /*     4 bit mask	*/
    {"MOVE",	itMoveB, 0x1000, 0x0000,  0xf000, 0x0000,    0x82, 0x1c},
    {"MOVE",	itMoveL, 0x2000, 0x0000,  0xf000, 0x0000,    0x82, 0x1c},
    {"MOVE",	itMoveW, 0x3000, 0x0000,  0xf000, 0x0000,    0x82, 0x1c},
    {"OR",	itOr,    0x8000, 0x0000,  0xf100, 0x0000,    0x02, 0x00},
    {"OR",	itOr,    0x8100, 0x0000,  0xf100, 0x0000,    0x83, 0x1c},
    {"EOR",	itOr,    0xb000, 0x0000,  0xf000, 0x0000,    0x82, 0x1c},
    {"AND",	itOr,    0xc000, 0x0000,  0xf100, 0x0000,    0x02, 0x00},
    {"AND",	itOr,    0xc100, 0x0000,  0xf100, 0x0000,    0x83, 0x1c},
    {"",	0,	 NULL,	 NULL,   NULL,	  NULL,      NULL, NULL}
    };



/*******************************************************************************
*
* dsmFind - disassemble one instruction
*
* This routine figures out which instruction is pointed to by binInst,
* and returns a pointer to the INST which describes it.
*
* RETURNS: pointer to instruction or NULL if unknown instruction.
*/

LOCAL INST *dsmFind
    (
    USHORT binInst []
    )
    {
    FAST INST *iPtr;
    UINT8 instMode;
    UINT8 instReg;

    /* Find out which instruction it is */

    for (iPtr = &inst [0]; iPtr->mask1 != NULL; iPtr++)

	if ((binInst [0] & iPtr->mask1) == iPtr->op1
	     && (binInst[1] & iPtr->mask2) == iPtr->op2)
	    {
	    /* get address mode */

	    if (strcmp (iPtr->name, "MOVE") == 0)
		{
		instMode = (binInst[0] & 0x01c0) >> 6;
		instReg  = (binInst[0] & 0x0e00) >> 9;
		}
	    else
		{
	        instMode = (binInst[0] & 0x0038) >> 3;
	        instReg = binInst [0] & 0x0007;
		}

	    /* check effective address mode */

	    if (((1 << instMode ) & iPtr->modemask) == 0x00)
		{
	        return (iPtr);
		}
	    if ((((1 << instMode ) & iPtr->modemask) == 0x80) &&
		(((1 << instReg) & iPtr->regmask) == 0x00))
	        {
	        return (iPtr);
		}

	    }

    /* If we're here, we couldn't find it */

    errnoSet (S_dsmLib_UNKNOWN_INSTRUCTION);
    return (NULL);
    }

/*******************************************************************************
*
* dsmPrint - print a disassembled instruction
*
* This routine prints an instruction in disassembled form.  It takes
* as input a pointer to the instruction, a pointer to the INST
* that describes it (as found by dsmFind()), and an address with which to
* prepend the instruction.
*/

LOCAL void dsmPrint
    (
    USHORT binInst [],  /* Pointer to instructin */
    FAST INST *iPtr,            /* Pointer to INST returned by dsmFind */
    int address,                /* Address with which to prepend instructin */
    int nwords,                 /* Instruction length, in words */
    FUNCPTR prtAddress          /* Address printing function */
    )
    {
    FAST int ix;		/* index into binInst */
    FAST int wordsToPrint;	/* # of 5-char areas to reserve for printing
				   of hex version of instruction */
    wordsToPrint = (((nwords - 1) / 5) + 1) * 5;

    /* Print the address and the instruction, in hex */

    printf ("%06x  ", address);
    for (ix = 0; ix < wordsToPrint; ++ix)
	/* print lines in multiples of 5 words */
	{
	if ((ix > 0) && (ix % 5) == 0)		/* print words on next line */
	    printf ("\n        ");
	printf ((ix < nwords) ? "%04x " : "     ", binInst [ix]);
	}

    if (iPtr == NULL)
	{
	printf ("DC.W        0x%04x\n", binInst[0]);
	return;
	}

    /* Print the instruction mnemonic, the size code (.w, or whatever), and
       the arguments */

    printf ("%-6s", iPtr->name);
    prtSizeField (binInst, iPtr);
    printf ("    ");
    prtArgs (binInst, iPtr, address, prtAddress);
    }
/*******************************************************************************
*
* dsmNwords - return the length (in words) of an instruction
*/

LOCAL int dsmNwords
    (
    USHORT binInst [],
    FAST INST *iPtr
    )
    {
    int frstArg;	/* length of first argument */

    if (iPtr == NULL)
	return (1);			/* not an instruction */

    switch (iPtr->type)
	{
	case itLea:
	case itBcd:
	case itNbcd:
	case itDynBit:
	case itMemShift:
	case itMoveTSR:
	case itMoveFSR:
	case itMoveCCR:
	case itQuick:
	case itScc:
	case itMoveFCCR:
	case itDivW:
	case itCpSave:
	    return (1 + modeNwords ((binInst [0] & 0x0038) >> 3,
				    binInst [0] & 7, 1, &binInst [1]));

	case itMoves:
	case itStatBit:
	case itMovem:
	case itCallm:
	case itBfchg:
	case itBfext:
	case itBfins:
	case itCpGen:
	case itCpScc:
	    return (2 + modeNwords ((binInst [0] & 0x0038) >> 3,
				    binInst [0] & 7, 0, &binInst [2]));

	case itDivL:
	    return (2 + modeNwords ((binInst [0] & 0x0038) >> 3,
				    binInst [0] & 7, 2, &binInst [2]));

	case itAdda:
	    return (1 + modeNwords ((binInst [0] & 0x0038) >> 3,
				    binInst [0] & 7,
				    ((binInst [0] & 0x0100) == 0) ? 1 : 2,
				    &binInst [1]));
	case itMoveA:
	    return (1 + modeNwords ((binInst [0] & 0x0038) >> 3,
				    binInst [0] & 7,
				    ((binInst [0] & 0x1000) == 0) ? 2 : 1,
				    &binInst [1]));
	case itNegx:
	case itOr:
	    return (1 + modeNwords ((binInst [0] & 0x0038) >> 3,
				    binInst [0] & 7,
				    ((binInst [0] & 0x0080) == 0) ? 1 : 2,
				    &binInst [1]));
	case itChk:
	    return (1 + modeNwords ((binInst [0] & 0x0038) >> 3,
				    binInst [0] & 7,
				    ((binInst [0] & 0x0080) == 0) ? 2 : 1,
				    &binInst [1]));

	case itChk2:
	    return (2 + modeNwords ((binInst [0] & 0x0038) >> 3,
				    binInst [0] & 7,
				    ((binInst [0] & 0x0400) == 0) ? 1 : 2,
				    &binInst [2]));

	case itCas:
	    return (2 + modeNwords ((binInst [0] & 0x0038) >> 3,
				    binInst [0] & 7,
				    ((binInst [0] & 0x0600) == 3) ? 2 : 1,
				    &binInst [2]));

	case itComplete:
	case itMoveq:
	case itExg:
	case itSwap:
	case itTrap:
	case itX:
	case itMoveUSP:
	case itUnlk:
	case itCmpm:
	case itExt:
	case itBkpt:
	case itRtm:
	case itRegShift:
#if ((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040))
	case itCinv:
	case itCinva:
	case itCpush:
	case itCpusha:
#endif	/* ((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040)) */
	    return (1);

	case itMovep:
	case itStop:
	case itLink:
	case itDb:
	case itRTD:
	case itMovec:
	case itPack:
	case itImmCCR:
	case itImmTSR:
#if ((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040))
	case itMove16:
#endif	/* ((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040)) */
	    return (2);

	case itLinkL:
	case itCas2:
	case itCpDbcc:
#if ((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040))
	case itMove16L:
#endif	/* ((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040)) */
	    return (3);

	case itBra:
	    switch (binInst [0] & 0xff)
		{
		case 0: 	return (2);
		case 0xff:	return (3);
		default:	return (1);
		}

	case itTrapcc:
	    return (((binInst [0] & 7) == 4) ? 1 : binInst [0] & 7);

	case itImm:
	    return ((((binInst [0] & 0x0080) == 0) ? 2 : 3) +
		modeNwords ((binInst [0] & 0x0038) >> 3, binInst [0] & 7, 0,
			((binInst [0] & 0x0080) == 0) ? &binInst [2]
			: &binInst [3]));

	case itMoveB:
	case itMoveW:
	    {
	    frstArg = modeNwords ((binInst [0] & 0x0038) >> 3,
				   binInst [0] & 7, 1, &binInst [1]);
	    return (1 + frstArg + modeNwords ((binInst [0] & 0x01c0) >> 6,
			    		      (binInst [0] & 0x0e00) >> 9, 1,
			    		      &binInst [1 + frstArg]));
	    }
	case itMoveL:
	    {
	    frstArg = modeNwords ((binInst [0] & 0x0038) >> 3,
				    binInst [0] & 7, 2, &binInst [1]);
	    return (1 + frstArg + modeNwords ((binInst [0] & 0x01c0) >> 6,
				    	      (binInst [0] & 0x0e00) >> 9, 2,
					      &binInst [1 + frstArg]));
	    }
	case itCpBcc:
	    return ((binInst [0] & 0x40) ? 3 : 2);

	case itCpTrapcc:
	    switch (binInst [0] & 7)
		{
		case 2:		return (3);
		case 3:		return (4);
		case 4:		return (2);
		}

	/* fpp instructions */
	case itFb:
	    if (binInst[0] & 0x0040)
		return(3);
	    else
		return(2);

	case itFrestore:
	case itFsave:
	    return(1 + modeNwords((binInst[0] & 0x0038) >> 3,
			    binInst[0] & 0x7, 0, &binInst[1]));

	case itFmovek:
	case itFmovel:
	case itFmovem:
	case itFmovemc:
	    return(2 + fppNwords((binInst[0] & 0x0038) >> 3,
			    binInst[0] & 0x7, 1, 0, &binInst[2]));

	case itFmovecr:
		return(2);

	case itFabs:
	case itFacos:
	case itFadd:
	case itFasin:
	case itFatan:
	case itFatanh:
	case itFcmp:
	case itFcos:
	case itFcosh:
	case itFdb:
	case itFdiv:
	case itFetox:
	case itFetoxm1:
	case itFgetexp:
	case itFgetman:
	case itFint:
	case itFintrz:
	case itFlog10:
	case itFlog2:
	case itFlogn:
	case itFlognp1:
	case itFmod:
	case itFmove:
	case itFmul:
	case itFneg:
	case itFnop:
	case itFrem:
	case itFscale:
	case itFs:
	case itFsgldiv:
	case itFsglmul:
	case itFsin:
	case itFsincos:
	case itFsinh:
	case itFsqrt:
	case itFsub:
	case itFtan:
	case itFtanh:
	case itFtentox:
	case itFtrap:
	case itFtst:
	case itFtwotox:
	    return (2 + fppNwords ((binInst[0] & 0x0038) >> 3,
		    binInst[0] & 0x7, (binInst[1] & 0x4000) >> 14,
		    (binInst[1] & 0x1c00) >> 10,
		    /* (binInst[1] & 0x0380) >> 7, XXX eliminate? */
		    &binInst[2]));

#if	(CPU == MC68020 || CPU == MC68030)
	case itPflush:
	case itPload:
	case itPmove:
	case itPtest:
	    return (2);
#endif	/* CPU == MC68020 || CPU == MC68030 */

#if	((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040))
	case itPflush:
	case itPtest:
	    return (1);
#endif	/* ((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040)) */
	}

    /* We never get here, but just for lint ... */
    return (0);
    }
/*******************************************************************************
*
* fppNwords - number of words of extension used by the mode for fpp instrutions
*/

LOCAL int fppNwords
    (
    FAST USHORT mode,
    USHORT reg,
    USHORT rm,
    USHORT src,
    USHORT extension []         /* extension array for effective address */
    )
    {
    if (rm != 0)
	{
	if (mode == 0x7 && reg == 0x4)
	    {
	    switch (src)
		{
		case 0:
		case 1:
		    return (2);
		case 2:
		case 3:
		    return (6);
		case 4:
		    return (1);
		case 5:
		    return (4);
		case 6:
		    return (1);
		}
	    }
	else
	    return (modeNwords (mode, reg, 0, extension));
	}

    return (0);
    }
/*******************************************************************************
*
* modeNwords - Figure out the number of words of extension needed for a mode
*/

LOCAL int modeNwords
    (
    FAST USHORT mode,
    USHORT reg,
    int size,           /* the size of an immediate operand in words */
    USHORT extension [] /* extension array for effective address */
    )
    {
    switch (mode)
	{
	case 0x00:    				/* Dn */
	case 0x01:				/* An */
	case 0x02:				/* (An) */
	case 0x03:				/* (An)+ */
	case 0x04:				/* -(An) */

	    return (0);

	case 0x05:				/* (d16,An) */

	    return (1);

	case 0x06:				/* reg indirect w.index modes */

	    return (mode6And7Words (&extension [0]));

	case 0x07:				/* memory indirect */
	    switch (reg)
		{
		/* With mode 7, sub-modes are determined by the
		   register number */

		case 0x0:			/* abs.short */
		case 0x2:			/* PC + off */

		     return (1);

		case 0x3:			/* PC + ind + off */

		    return (mode6And7Words (&extension [0]));

		case 0x1:				/* abs.long */

		    return (2);

		case 0x4:				/* imm. */
		    return (size);
		}
	}
    /* We never get here, but just for lint ... */
    return (0);
    }
/*******************************************************************************
*
* mode6And7Words - number of words of extension needed for modes 6 and 7
*/

LOCAL int mode6And7Words
    (
    FAST USHORT extension []
    )
    {
    int count = 1;	/* number of words in extension */

    if ((extension [0] & 0x0100) == 0)		/* (An) + (Xn) + d8 */
	return (count);

    switch ((extension [0] & 0x30) >> 4)	/* base displacement size */
	{
	case 0:		/* reserved or NULL displacement */
	case 1:
	    break;

	case 2:		/* word displacement */
	    count += 1;
	    break;

	case 3:		/* long displacement */
	    count += 2;
	    break;
	}

    if ((extension [0] & 0x40) == 0)		/* index operand added */
	{
	switch (extension [0] & 3)
	    {
	    case 0:		/* reserved or NULL displacement */
	    case 1:
		break;

	    case 2:		/* word displacement */
		count += 1;
		break;

	    case 3:		/* long displacement */
		count += 2;
		break;
	    }
	}

    return (count);
    }
/*******************************************************************************
*
* prtArgs - Print the arguments for an instruction
*/

LOCAL void prtArgs
    (
    USHORT binInst [],  /* Pointer to the binary instruction */
    FAST INST *iPtr,    /* Pointer to the INST describing binInst */
    int address,        /* Address at which the instruction resides */
    FUNCPTR prtAddress  /* routine to print addresses. */
    )
    {
    int frstArg;
    int displacement;
    int sizeData;

    switch (iPtr->type)
	{
	case itBra:
	    switch (binInst [0] & 0xff)
		{
		case 0:
		    displacement = (short) binInst[1];
		    break;
		case 0xff:
		    displacement = (((short) binInst [1] << 16) | binInst [2]);
		    break;
		default:
		    displacement = (char) (binInst [0] & 0xff);
		    break;
		}

	    (*prtAddress) (address + 2 + displacement);
	    printf ("\n");
	    break;

	case itDivW:
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 7,
			&binInst [1], 2, prtAddress);
	    printf (",D%x\n", (binInst [0] & 0x0e00) >> 9);
	    break;

	case itChk:
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 7,
			&binInst [1], (binInst [0] & 0x80) ? 2 : 4, prtAddress);
	    printf (",D%x\n", (binInst [0] & 0x0e00) >> 9);
	    break;

	case itChk2:
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 7,
			&binInst [2], 0, prtAddress);
	    printf ((binInst [1] & 0x8000) ? ",A%x\n" : ",D%x\n",
		    (binInst [1] & 0x7000) >> 12);
	    break;

	case itLea:
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [1], 2, prtAddress);
	    printf (",A%x\n", (binInst [0] & 0x0e00) >> 9);
	    break;

	case itComplete:
	    /* No arguments */
	    printf ("\n");
	    break;

	case itStatBit:
	case itCallm:
	    printf ("#%#x,", binInst [1]);
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [2], 0, prtAddress);
	    printf ("\n");
	    break;

	case itDynBit:
	    printf ("D%x,", (binInst [0] & 0x0e00) >> 9);
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [1], 0, prtAddress);
	    printf ("\n");
	    break;

	case itExg:
	    switch ((binInst [0] & 0x00f8) >> 3)
		{
		case 0x08:
		    printf ("D%x,D%x\n", (binInst [0] & 0x0e00) >> 9,
					 binInst [0] & 0x0007);
		    break;
		case 0x09:
		    printf ("A%x,A%x\n", (binInst [0] & 0x0e00) >> 9,
					 binInst [0] & 0x0007);
		    break;
		case 0x11:
		    printf ("D%x,A%x\n", (binInst [0] & 0x0e00) >> 9,
					 binInst [0] & 0x0007);
		    break;
		}
	    break;

	case itImm:
	    switch ((binInst [0] & 0x00c0) >> 6)	/* size */
		{
		case 0:					/* byte */
		    printf ("#%#x,", binInst [1] & 0x00ff);
		    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
				&binInst [2], 0, prtAddress);
		    break;
		case 1:					/* word */
		    printf ("#%#x,", binInst [1]);
		    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
				&binInst [2], 0, prtAddress);
		    break;
		case 2:					/* long */
		    printf ("#%#x%04x,", binInst [1], binInst [2]);
		    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
				&binInst [3], 0, prtAddress);
		    break;
		}
	    printf ("\n");
	    break;

	case itMoveB:
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [1], 1, prtAddress);
	    printf (",");
	    frstArg = modeNwords ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
				  1, &binInst [1]);
	    prEffAddr ((binInst [0] & 0x01c0) >> 6, (binInst [0] & 0x0e00) >> 9,
			&binInst [1 + frstArg], 1, prtAddress);
	    printf ("\n");
	    break;

	case itMoveW:
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [1], 2, prtAddress);
	    printf (",");
	    frstArg = modeNwords ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
				  1, &binInst [1]);
	    prEffAddr ((binInst [0] & 0x01c0) >> 6, (binInst [0] & 0x0e00) >> 9,
			&binInst [1 + frstArg], 2, prtAddress);
	    printf ("\n");
	    break;

	case itMoveL:
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [1], 4, prtAddress);
	    printf (",");
	    frstArg = modeNwords ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
				  2, &binInst [1]);
	    prEffAddr ((binInst [0] & 0x01c0) >> 6, (binInst [0] & 0x0e00) >> 9,
			&binInst [1 + frstArg], 4, prtAddress);
	    printf ("\n");
	    break;

	case itImmCCR:
	    printf ("#%#x,CCR\n", binInst [1] & 0xff);
	    break;

	case itImmTSR:
	    printf ("#%#x,SR\n", binInst [1]);
	    break;

	case itMoveCCR:
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [1], 2, prtAddress);
	    printf (",CCR\n");
	    break;

	case itMoveTSR:
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [1], 2, prtAddress);
	    printf (",SR\n");
	    break;

	case itMoveFSR:
	    printf ("SR,");
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [1], 0, prtAddress);
	    printf ("\n");
	    break;

	case itMoveUSP:

	    printf (((binInst [0] & 0x8) == 0) ? "A%x,USP\n" : "USP,A%x\n",
		    binInst [0] & 0x07);
	    break;

#if ((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040))
	case itMove16:

	    printf ("(A%x)+,(A%x)+\n", binInst [0] &0x07,
		    (binInst [1] & 0x7000) >> 12);
	    break;

	case itMove16L:
	    switch ((binInst [0] & 0x0018) >> 3)	/* op-mode */
		{
		case 0:
		    printf ("(A%x)+,0x%x%x", binInst[0] & 0x07, binInst[1],
			    binInst[2]);
		    break;

		case 1:
		    printf ("0x%x%x,(A%x)-", binInst[1], binInst[2],
			    binInst[0] & 0x07);
		    break;

		case 2:
		    printf ("(A%x),0x%x%x", binInst[0] & 0x07, binInst[1],
			    binInst[2]);
		    break;

		case 3:
		    printf ("0x%x%x,(A%x)", binInst[1], binInst[2],
			    binInst[0] & 0x07);
		    break;
		}
	    printf ("\n");
	    break;


	case itCinv:
	case itCpush:
	    switch ((binInst [0] & 0x00c0) >>6)
		{
		case 1:
		    printf ("DC,(A%x)", binInst [0] & 0x07);
		    break;

		case 2:
		    printf ("IC,(A%x)", binInst [0] & 0x07);
		    break;

		case 3:
		    printf ("BC,(A%x)", binInst [0] & 0x07);
		    break;
		}
	    printf ("\n");
	    break;

	case itCinva:
	case itCpusha:
	    switch ((binInst [0] & 0x00c0) >>6)
		{
		case 1:
		    printf ("DC");
		    break;

		case 2:
		    printf ("IC");
		    break;

		case 3:
		    printf ("BC");
		    break;
		}
	    printf ("\n");
	    break;

	case itPtest:
	    printf ("(A%x)\n", binInst [0] & 0x07);
	    break;

	case itPflush:
	    switch ((binInst [0] & 0x0018) >> 3)
		{
		case 0:
		    printf ("(A%x)", binInst [0] & 0x07);
		    break;

		case 1:
		    printf ("(A%x)", binInst [0] & 0x07);
		    break;

		case 2:
		case 3:
		    break;
		}
	    printf ("\n");
	    break;

#endif	/* ((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040)) */

	case itMovep:
	    if ((binInst [0] & 0x0040) == 0)

		printf ("%x(A%x),D%x\n", binInst [1], binInst [0] & 0x07,
			(binInst [0] & 0x0c00) >> 9);

	    else

		printf ("D%x,%x(A%x)\n", (binInst [0] & 0x0c00) >> 9,
			binInst [1], binInst [0] & 0x07);

	    break;

	case itMovem:
	    if ((binInst [0] & 0x0400) == 0)
		{
		prMovemRegs (binInst [1], (binInst [0] & 0x38) >> 3);
		printf (",");
		prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			    &binInst [2], 0, prtAddress);
		printf ("\n");
		}
	    else
		{
		prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			    &binInst [2], 0, prtAddress);
		printf (",");
		prMovemRegs (binInst [1], (binInst [0] & 0x38) >> 3);
		printf ("\n");
		}
	    break;

	case itMoveq:
	    printf ("#%#x,D%x\n", binInst [0] & 0x00ff,
		    (binInst [0] & 0x0e00) >> 9);
	    break;

	case itNbcd:
	case itNegx:
	case itCpSave:
	case itScc:
	case itMemShift:
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [1], 0, prtAddress);
	    printf ("\n");
	    break;

	case itCpGen:
	case itCpScc:
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [2], 0, prtAddress);
	    printf ("\n");
	    break;

	case itMoveA:
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
		&binInst [1], ((binInst [0] & 0x1000) == 0) ? 4 : 2,
		prtAddress);
	    printf (",A%x\n", (binInst [0] & 0x0e00) >> 9);
	    break;

	case itAdda:
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
		&binInst [1], ((binInst [0] & 0x0100) == 0) ? 2 : 4,
		prtAddress);
	    printf (",A%x\n", (binInst [0] & 0x0e00) >> 9);
	    break;

	case itCmpm:
	    printf ("(A%x)+,(A%x)+\n", (binInst [0] & 0x0007),
		(binInst [0] & 0x0e00) >> 9);
	    break;

	case itOr:
	    switch ((binInst [0] & 0x01c0) >> 6)	/* op-mode */
		{
		case 0:				/* byte, to D reg */
		    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [1], 1, prtAddress);
		    printf (",D%x\n", (binInst [0] & 0x0e00) >> 9);
		    break;

		case 1:				/* word to D reg */
		    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [1], 2, prtAddress);
		    printf (",D%x\n", (binInst [0] & 0x0e00) >> 9);
		    break;

		case 2:				/* long, to D reg */
		    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [1], 4, prtAddress);
		    printf (",D%x\n", (binInst [0] & 0x0e00) >> 9);
		    break;

		case 4:				/* byte, to eff address */
		case 5:				/* word, to eff address */
		case 6:				/* long, to eff address */
		    printf ("D%x,", (binInst [0] & 0x0e00) >> 9);
		    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [1], 0, prtAddress);
		    printf ("\n");
		    break;

		}
	    break;

	case itQuick:

	    /* If the data field is 0, it really means 8 */

	    if ((binInst [0] & 0x0e00) == 0)
		printf ("#0x8,");
	    else
		printf ("#%#x,", (binInst [0] & 0x0e00) >> 9);

	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [1], 0, prtAddress);
	    printf ("\n");
	    break;

	case itBcd:
	    if ((binInst [0] & 0x0004) == 0)
		printf ("D%x,D%x\n", (binInst [0] & 0x0e00) >> 9,
				    binInst [0] & 0x0007);
	    else
		printf ("-(A%x),-(A%x)\n", (binInst [0] & 0x0e00) >> 9,
				    binInst [0] & 0x0007);
	    break;

	case itRegShift:
	    if ((binInst [0] & 0x0e20) == 0)
		printf ("#0x8,");
	    else if ((binInst [0] & 0x0020) == 0)
		printf ("#%#x,", (binInst [0] & 0x0e00) >> 9);
	    else
		printf ("D%x,", (binInst [0] & 0x0e00) >> 9);

	    printf ("D%x\n", binInst [0] & 0x07);
	    break;

	case itTrapcc:
	    if ((binInst [0] & 7) == 2)
		printf ("#%#x", binInst [1]);
	    else if ((binInst [0] & 7) == 3)
		printf ("#%#x%04x", binInst [1], binInst [2]);
	    printf ("\n");
	    break;

	case itStop:
	    printf ("%#x\n", binInst [1]);
	    break;

	case itSwap:
	case itExt:
	    printf ("D%x\n", binInst [0] & 0x07);
	    break;

	case itUnlk:
	    printf ("A%x\n", binInst [0] & 0x07);
	    break;

	case itLink:
	    printf ("A%x,#%#x\n", binInst [0] & 0x07, binInst [1]);
	    break;

	case itLinkL:
	    printf ("A%x,#%#x%04x\n", binInst [0] & 7, binInst [1], binInst [2]);
	    break;

	case itRtm:
	    printf ((binInst [0] & 8) ? "A%x\n" : "D%x\n", binInst [0] & 7);
	    break;

	case itTrap:
	    printf ("#%#x\n", binInst [0] & 0x0f);
	    break;

	case itBkpt:
	    printf ("#%#x\n", binInst [0] & 7);
	    break;

	case itX:
	    printf (((binInst [0] & 0x08) == 0) ?
		    "D%x,D%x\n" : "-(A%x),-(A%x)\n",
		    binInst [0] & 0x07, (binInst [0] & 0x0e00) >> 9);
	    break;

	case itPack:
	    printf (((binInst [0] & 0x08) == 0) ?
		    "D%x,D%x," : "-(A%x),-(A%x),",
		    binInst [0] & 0x07, (binInst [0] & 0x0e00) >> 9);
	    printf ("#%#x\n", binInst [1]);
	    break;

	case itDb:
	    printf ("D%x,", binInst [0] & 0x07);
	    (*prtAddress) (address + 2 + (short) binInst [1]);
	    printf ("\n");
	    break;

	case itRTD:
	    printf ("#%#x\n", binInst [1]);
	    break;

	case itBfchg:
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 7, &binInst [2],
	              0, prtAddress);
	    prOffWid ((binInst [1] & 0x0800) >> 11, (binInst [1] & 0x07c0) >> 6,
		      (binInst [1] & 0x20) >> 5, binInst [1] & 0x1f);
	    printf ("\n");
	    break;

	case itBfext:
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 7, &binInst [2],
	              0, prtAddress);
	    prOffWid ((binInst [1] & 0x0800) >> 11, (binInst [1] & 0x07c0) >> 6,
		      (binInst [1] & 0x20) >> 5, binInst [1] & 0x1f);
	    printf (",D%x\n", (binInst [1] & 0x7000) >> 12);
	    break;

	case itBfins:
	    printf ("D%x,", (binInst [1] & 0x7000) >> 12);
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 7, &binInst [2],
	              0, prtAddress);
	    prOffWid ((binInst [1] & 0x0800) >> 11, (binInst [1] & 0x07c0) >> 6,
		      (binInst [1] & 0x20) >> 5, binInst [1] & 0x1f);
	    printf ("\n");
	    break;

	case itDivL:
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 7, &binInst [2],
			4, prtAddress);
	    printf (",");
	    if (binInst [1] & 0x0400
		|| ((binInst [1] & 0x0007) != ((binInst [1] & 0x7000) >> 12)))
		{
		printf ("D%x:", binInst [1] & 7);
		}
	    printf ("D%x\n", (binInst [1] & 0x7000) >> 12);
	    break;

	case itCas:
	    printf ("D%x,D%x,", binInst [1] & 7, (binInst [1] & 0x01c0) >> 6);
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 7, &binInst [2],
			0, prtAddress);
	    printf ("\n");
	    break;

	case itCas2:
	    printf ("D%x:D%x,D%x:D%x,", binInst [1] & 7, binInst [2] & 7,
		     (binInst [1] & 0x01c0) >> 6, (binInst [2] & 0x01c0) >> 6 );
	    printf (binInst [1] & 0x8000 ? "(A%x):" : "(D%x):",
		     (binInst [1] & 0x7000) >> 12);
	    printf (binInst [2] & 0x8000 ? "(A%x)\n" : "(D%x)\n",
		     (binInst [2] & 0x7000) >> 12);
	    break;

	case itMoves:
	    if ((binInst[1] & 0x0800) == 0)
		{
		/* from <ea> to general reg */

		prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			    &binInst [2], 0, prtAddress);
		if ((binInst[1] & 0x8000) == 0)
		    printf (",D%x\n",(binInst[1] & 0x7000) >> 12);
		else
		    printf (",A%x\n",(binInst[1] & 0x7000) >> 12);
		}
	    else
		{
		/* from general reg to <ea> */
		if ( (binInst[1] & 0x8000) == 0)
		    printf ("D%x, ",(binInst[1] & 0x7000) >> 12);
		else
		    printf ("A%x, ",(binInst[1] & 0x7000) >> 12);
		prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			    &binInst [2], 0, prtAddress);
		printf ("\n");
		}
	    break;

	case itMovec:
	    if ((binInst[0] & 1) == 0)
		{
		/* control reg to general reg */
		prContReg (binInst[1] & 0x0fff);
		if ((binInst[1] & 0x8000) == 0)
		    printf (",D%x\n",(binInst[1] & 0x7000) >> 12);
		else
		    printf (",A%x\n",(binInst[1] & 0x7000) >> 12);
		}
	    else
		{
		/* general reg to control reg */
		if ((binInst[1] & 0x8000) == 0)
		    printf ("D%x,",(binInst[1] & 0x7000) >> 12);
		else
		    printf ("A%x,",(binInst[1] & 0x7000) >> 12);
		prContReg (binInst[1] & 0x0fff);
		printf ("\n");
		}
	    break;

	case itMoveFCCR:
	    printf ("CCR, ");
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [1], 0, prtAddress);
	    printf ("\n");
	    break;

	case itCpBcc:
	    if ((binInst [0] & 0x40) == 0)
		printf ("#%#x%04x\n", binInst [1], binInst [2]); /* xxx 0x */
	    else
		printf ("#%#x\n", binInst [1]);
	    break;

	case itCpDbcc:
	    printf("D%x,#%#x\n", binInst [0] & 7, binInst [2]);
	    break;

	case itCpTrapcc:
	    switch (binInst [0] & 7)
		{
		case 2:
		    printf ("#%#x\n", binInst [2]);
		    break;
		case 3:
		    printf ("#%#x%04x\n", binInst [2], binInst [3]);
		    break;
		case 4:
		    printf ("#0\n");
		    break;
		}
	    break;

	/* fpp instructions */
	case itFb:
		if(binInst[0] & 0x40)
			printf ("#%#x\n", binInst[1]);
		else
			printf ("#%#x%04x\n", binInst[1], binInst[2]);
		break;

	case itFrestore:
	case itFsave:
	case itFs:
		prEffAddr((binInst[0] & 0x38)>>3, binInst[0] & 0x7,
			&binInst[1], 0, prtAddress);
		printf ("\n");
		break;

	case itFdb:
		printf ("D%x,#%#x\n",binInst[0] & 0x7, binInst[2]);
		break;

	case itFtrap:
		switch(binInst[0] & 7)
		    {
		    case 2:
			    printf ("#%#x\n",binInst[2]);
			    break;
		    case 3:
			    printf ("#%#x%04x\n", binInst[2], binInst[3]);
			    break;
		    case 4:
			    printf ("#0\n"); break;
		    }
		break;
	case itFtst:
		if (binInst[1] & 0x4000)
			prEffAddr((binInst[0] & 0x38)>>3, binInst[0] & 0x7,
				&binInst[2], 0, prtAddress);
		else
			printf ("F%x", (binInst[1] & 0x0380)>>7);
		printf ("\n");
		break;

	case itFmovek:
		printf ("F%x,", (binInst[1] & 0x0380)>>7);
		prEffAddr((binInst[0] & 0x38) >> 3, binInst[0] & 0x7,
			&binInst[2], 0, prtAddress);
		printf ("\n");
		break;

	case itFmovel:
		if (binInst[1] & 0x2000)
		    {
		    prFmovemcr ((binInst[1] & 0x1c00) >> 10);
		    printf (",");
		    prEffAddr((binInst[0] & 0x38) >> 3, binInst[0] & 0x7,
			    &binInst[2], 0, prtAddress);
		    }
		else
		    {
		    prEffAddr((binInst[0] & 0x38) >> 3, binInst[0] & 0x7,
			    &binInst[2], 0, prtAddress);
		    printf (",");
		    prFmovemcr ((binInst[1] & 0x1c00) >> 10);
		    }
		printf ("\n");
		break;

	case itFmovecr:
		printf ("#%#x,", binInst[1] & 0x7f);
		printf ("F%x\n",(binInst[1] & 0x0380) >> 7);
		break;

	case itFmovem:
		if (binInst[1] & 0x2000)
		    {
		    prFmovemr((binInst[1] & 0x1800) >> 11, binInst[1] & 0xff);
		    printf (",");
		    prEffAddr((binInst[0] & 0x38) >> 3, binInst[0] & 0x7,
			    &binInst[2], 0, prtAddress);
		    }
		else
		    {
		    prEffAddr((binInst[0] & 0x38) >> 3, binInst[0] & 0x7,
			    &binInst[2], 0, prtAddress);
		    printf (",");
		    prFmovemr ((binInst[1] & 0x1800) >> 11, binInst[1] & 0xff);
		    }
		printf ("\n");
		break;

	case itFmovemc:
		if (binInst[1] & 0x2000)
		    {
		    prFmovemcr ((binInst[1] & 0x1c00) >> 10);
		    printf (",");
		    prEffAddr((binInst[0] & 0x38) >> 3, binInst[0] & 0x7,
			    &binInst[2], 0, prtAddress);
		    }
		else
		    {
		    prEffAddr ((binInst[0] & 0x38) >> 3, binInst[0] & 0x7,
			    &binInst[2], 0, prtAddress);
		    printf (",");
		    prFmovemcr ((binInst[1] & 0x1c00) >> 10);
		    }
		printf ("\n");
		break;

	case itFsincos:
		if (binInst[1] & 0x4000)
		    prEffAddr((binInst[0] & 0x38) >> 3, binInst[0] & 0x7,
			    &binInst[2], 0, prtAddress);
		else
		    printf ("F%x,", (binInst[1] & 0x1c00) >> 10);
		printf (",F%x:F%x\n", (binInst[1] & 0x7),
				(binInst[1] & 0x380) >> 7);
		break;

	case itFabs:
	case itFacos:
	case itFadd:
	case itFasin:
	case itFatan:
	case itFatanh:
	case itFcmp:
	case itFcos:
	case itFcosh:
	case itFdiv:
	case itFetox:
	case itFetoxm1:
	case itFgetexp:
	case itFgetman:
	case itFint:
	case itFintrz:
	case itFlog10:
	case itFlog2:
	case itFlogn:
	case itFlognp1:
	case itFmod:
	case itFmove:
	case itFmul:
	case itFneg:
	case itFnop:
	case itFrem:
	case itFscale:
	case itFsgldiv:
	case itFsglmul:
	case itFsin:
	case itFsinh:
	case itFsqrt:
	case itFsub:
	case itFtan:
	case itFtanh:
	case itFtentox:
	case itFtwotox:
		if (binInst[1] & 0x4000)
		    {
		    switch ((binInst [1] & 0x1c00) >> 10)
			{
			case LONGINT: 		sizeData = 4; break;
			case SINGLEREAL:	sizeData = 4; break;
			case EXTENDEDREAL:	sizeData = 12; break;
			case PACKEDDECIMAL:	sizeData = 12; break;
			case WORDINT: 		sizeData = 2; break;
			case DOUBLEREAL:	sizeData = 8; break;
			case BYTEINT: 		sizeData = 2; break;
			default: 		sizeData = 4; break;
			}
		    
		    prEffAddr((binInst[0] & 0x38) >> 3, binInst[0] & 0x7,
			    &binInst[2], sizeData, prtAddress);
		    printf (",F%x", (binInst[1] & 0x0380) >> 7);
		    }
		else
		    printf ("F%x,F%x", (binInst[1] & 0x1c00) >> 10,
			    (binInst[1] & 0x0380) >> 7);
		printf ("\n");
		break;

#if 	(CPU == MC68020 || CPU == MC68030)
	case itPflush:
	    if ((binInst[1] & 0x1c00) == 0x0400)
		{
		printf ("\n");
		break;
		}

	    prContReg (binInst[1] & 0x001f);
	    printf (",#%#x", (binInst[1] & 0x00e0) >> 5);
	    if ((binInst[1] & 0x1c00) == 0x1800)
		{
		printf (",");
		prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			    &binInst [1], 0, prtAddress);
		}
	    printf ("\n");
	    break;

	case itPload:
	    prContReg (binInst[1] & 0x001f);
	    printf (",");
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
		        &binInst [1], 0, prtAddress);
	    printf ("\n");
	    break;

	case itPmove:
	    if (binInst[1] & 0x0200)
		{
		prContReg ((binInst[1] & 0xfc00) >> 4);
		printf (",");
	        prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
		            &binInst [1], 0, prtAddress);
		}
	    else
		{
	        prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
		            &binInst [1], 0, prtAddress);
		printf (",");
		prContReg ((binInst[1] & 0xfc00) >> 4);
		}
	    printf ("\n");
	    break;

	case itPtest:
	    prContReg (binInst[1] & 0x001f);
	    printf (",");
	    prEffAddr ((binInst [0] & 0x38) >> 3, binInst [0] & 0x7,
			&binInst [2], 0, prtAddress);
	    printf (",#%#x", (binInst[1] & 0x1c00) >> 10);
	    if (binInst[1] & 0x0100)
	       printf (", A%x", (binInst[1] & 0x00e0) >> 5);
	    printf ("\n");
	    break;
#endif	/* CPU == MC68020 || CPU == MC68030 */

	default:
	    break;
	}
    }
/*******************************************************************************
*
* prContReg - print the name of a control register
*/

LOCAL void prContReg
    (
    USHORT contReg
    )
    {
    char *buf;

#if	(CPU == MC68020 || CPU == MC68030)
    if ((contReg & 0x18) == FCDATA)	/* immediate function code */
	{
	printf ("#%#x", contReg & 0x07);
	return;
	}
    else
	if ((contReg & 0x18) == FCREG) /* function code in data register */
	    {
	    printf ("D%d", contReg & 0x07);
	    return;
	    }
#endif	/* (CPU == MC68020 || CPU == MC68030) */

    switch (contReg)
	{
	case SFC:	buf = "SFC";	break;
	case DFC:	buf = "DFC";	break;
	case USP:	buf = "USP";	break;
	case VBR:	buf = "VBR";	break;

#if	(CPU==MC68020 || CPU==MC68030 || CPU==MC68040 || CPU==MC68060 || \
	 CPU==MC68LC040)
	case CACR:	buf = "CACR";	break;
	case TC:	buf = "TC";	break;
	case MSP:	buf = "MSP";	break;
	case ISP:	buf = "ISP";	break;
	case MMUSR:	buf = "MMUSR"; break;
	case SRP:	buf = "SRP";	break;
#endif	/* (CPU==MC68020 || CPU==MC68030 || CPU==MC68040 || CPU==MC68060 \
         *  CPU==MC68LC040) */

#if	(CPU == MC68020 || CPU == MC68030)
	case TT0:	buf = "TT0";	break;
	case TT1:	buf = "TT1";	break;
	case CRP:	buf = "CRP";	break;
	case CAAR:	buf = "CAAR";	break;
#endif	/* CPU == MC68020 || CPU == MC68030 */

#if	((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040))
	case ITT0:	buf = "ITT0";	break;
	case ITT1:	buf = "ITT1";	break;
	case DTT0:	buf = "DTT0";	break;
	case DTT1:	buf = "DTT1";	break;
	case URP:	buf = "URP";	break;
#endif	/* ((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040)) */

	default:
	    buf = "";	/* make sure nothing is printed */
	    break;
	}

    printf (buf);
    }
/*******************************************************************************
*
* prEffAddr - print the argument for an effective address
*/

LOCAL void prEffAddr
    (
    FAST USHORT mode,   /* mode indicator */
    FAST USHORT reg,    /* register number (or special if mode = 7) */
    USHORT extension [],/* extension data, if required */
    int size,           /* size of extension, in bytes, for immediate */
    FUNCPTR prtAddress  /* Function to print addresses */
    )
    {
    switch (mode)	/* Effective mode */
	{
	case 0x00:    				/* Dn */
	    printf ("D%x", reg);
	    break;

	case 0x01:				/* An */
	    printf ("A%x", reg);
	    break;

	case 0x02:				/* (An) */
	    printf ("(A%x)", reg);
	    break;

	case 0x03:				/* (An)+ */
	    printf ("(A%x)+", reg);
	    break;

	case 0x04:				/* -(An) */
	    printf ("-(A%x)", reg);
	    break;

	case 0x05:				/* (d16,An) */
	    printf ("(%#x,A%x)", extension [0], reg);
	    break;

	case 0x06:				/* addr reg + index + offset */
	    prIndirectIndex (&extension [0], mode, reg);
	    break;

	case 0x07:
	    switch (reg)
		{
		/* With mode 7, sub-modes are determined by the
		   register number */

		case 0x0:				/* abs.short */
		    (*prtAddress) (extension [0]);
		    break;

		case 0x1:				/* abs.long */
		    (*prtAddress) (extension [1] + (extension [0] << 16));
		    break;

		case 0x2:				/* PC + displacement */
		    printf ("(%#x,PC)", extension [0]);
		    break;

		case 0x3:				/* rel + ind + off */
		    prIndirectIndex (&extension [0], mode, reg);
		    break;

		case 0x4:				/* imm. */
		    switch (size)
			{
			case 1:			/* 1 byte */
			    printf ("#%#x", extension [0] & 0xff);
			    break;
			case 2:			/* 2 bytes */
			    printf ("#%#x", extension [0]);
			    break;
			case 4:
			    printf ("#%#x%04x", extension [0], extension [1]);
			    break;
		        case 8:
			    printf ("#%#x%04x%04x%04x", extension [0],
				    extension [1], extension [2],
				    extension [3]);
			    break;
			}
		    break;
		}
	}
    }
/*******************************************************************************
*
* prMovemRegs - print the regs for a movem instruction
*/

LOCAL void prMovemRegs
    (
    FAST USHORT extension,
    FAST USHORT mode
    )
    {
    FAST int ix;
    BOOL slash = FALSE;		/* keep track of slash's between registers */
    BOOL forward  = mode == 4;
    int  repeatCount;
    char regChar  = 'D';
    BOOL isRange  = FALSE;	/* printing range of registers*/
    BOOL nextSet  = FALSE;	/* set to TRUE if next bit is set */

    /* If the mode is predecrement, the extension word has the bits in
       the opposite order, which explains all the weird conditionals below. */

    for (repeatCount = 0; repeatCount < 2; repeatCount++)
	{
	for (ix = 0; ix <= 7; ix++)
	    {
	    if ((extension & (forward ? 0x8000 : 1)) != 0)
		{
		/* see if the next bit is set */

		nextSet = (((forward ? extension << 1 : extension >> 1) &
			   (forward ? 0x8000 : 1)) && ix != 7);

		/*
		 * If we're not printing a range of registers, just print ",DX"
		 * If we're at the end of a range, print -DX.  If neither 
		 * applies, don't print anything.
		 */

		if (!isRange)
		    {
		    if (slash)
			printf ("/");
		    printf ("%c%x", regChar, ix);
		    }
		else
		    {
		    if (!nextSet)
			printf ("-%c%x", regChar, ix);
		    }

		/*
		 * If the next bit is set, then we're in a range of registers.
		 * Set isRange and slash appropriately.
		 */

		if (nextSet)
		    {
		    slash = FALSE;
		    isRange = TRUE;
		    }
		else
		    {
		    slash = TRUE;
		    isRange = FALSE;
		    }
		}

	    extension = forward ? extension << 1 : extension >> 1;
	    }

	regChar = 'A';
	}
    }
/*******************************************************************************
*
* prFmovemr - print the registers for a fmovemr instruction
*/

LOCAL void prFmovemr
    (
    FAST USHORT mode,
    FAST USHORT rlist
    )
    {
    FAST int ix;
    BOOL slash = FALSE;		/* keep track of slash's between registers */
    BOOL postincr = mode == 2 || mode == 3;
    BOOL isRange  = FALSE;	/* printing range of registers*/
    BOOL nextSet  = FALSE;	/* set to TRUE if next bit is set */

    for (ix = 0; ix <= 7; ix++)
	{
	if (rlist & (postincr ? 0x80 : 1))
	    {
	    /* see if the next bit is set */

	    nextSet = (((postincr ? rlist << 1 : rlist >> 1) &
		       (postincr ? 0x80 : 1)) && ix != 7);

	    /*
	     * If we're not printing a range of registers, just print ",FX"
	     * If we're at the end of a range, print -FX.  If neither 
	     * applies, don't print anything.
	     */

	    if (!isRange)
		{
		if (slash)
		    printf ("/");
		printf ("F%x", ix);
		}
	    else
		{
		if (!nextSet)
		    printf ("-F%x", ix);
		}

	    /*
	     * If the next bit is set, then we're in a range of registers.
	     * Set isRange and slash appropriately.
	     */

	    if (nextSet)
		{
		slash = FALSE;
		isRange = TRUE;
		}
	    else
		{
		slash = TRUE;
		isRange = FALSE;
		}
	    }
	rlist = postincr ? rlist << 1 : rlist >> 1;
	}
    }

/*******************************************************************************
*
* prFmovemcr - print the regs for a fmovemcr instruction
*/

LOCAL void prFmovemcr
    (
    FAST USHORT rlist
    )
    {
    printf ("#<");
    if (rlist & 1)
	printf ("FPIAR,");
    if (rlist & 2)
	printf ("FPSR,");
    if (rlist & 4)
	printf ("FPCR");
    printf (">");
    }
/*******************************************************************************
*
* prtSizeField - print the size field of an instruction (.S, .W, or .L)
*/

LOCAL void prtSizeField
    (
    USHORT binInst [],
    FAST INST *iPtr
    )
    {
    switch (iPtr->type)
	{
	case itLea:
	case itBcd:
	case itNbcd:
	case itMoveTSR:
	case itMoveFSR:
	case itMoveCCR:
	case itComplete:
	case itMoveq:
	case itExg:
	case itSwap:
	case itUnlk:
	case itTrap:
	case itMoveUSP:
	case itStop:
	case itBra:
	case itBfchg:
	case itBfext:
	case itBfins:
	case itDb:
	case itScc:
	case itRTD:
	case itMovec:
	case itMoveFCCR:
	case itBkpt:
	case itCallm:
	case itPack:
	case itRtm:
	case itImmCCR:
	case itImmTSR:
	case itCpGen:
	case itCpSave:
	case itMemShift:
#if ((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040))
	case itCpush:
	case itCpusha:
	case itCinv:
	case itCinva:
	case itMove16:
	case itMove16L:
#endif	/* ((CPU==MC68040) || (CPU==MC68060) || (CPU==MC68LC040)) */
	    printf ("  ");
	    break;

	case itDynBit:
	case itStatBit:
	    printf (((binInst [0] & 0x0038) == 0) ? ".L" : ".B");
	    break;

	case itNegx:
	case itQuick:
	case itX:
	case itImm:
	case itMoves:
	    switch ((binInst [0] & 0x00c0) >> 6)
		{
		case 0:
		    printf (".B");
		    break;
		case 1:
		case 3:
		    printf (".W");
		    break;
		case 2:
		    printf (".L");
		    break;
		}
	    break;

	case itOr:
	case itCmpm:
	case itRegShift:
	    switch ((binInst [0] & 0x00c0) >> 6)
		{
		case 0:
		    printf (".B");
		    break;
		case 1:
		    printf (".W");
		    break;
		case 2:
		    printf (".L");
		    break;
		}
	    break;

	case itCas:
	case itCas2:
	    switch ((binInst [0] & 0x0600) >> 9)
		{
		case 1:
		    printf (".B");
		    break;
		case 2:
		    printf (".W");
		    break;
		case 3:
		    printf (".L");
		    break;
		}
	    break;

	case itChk2:
	    switch ((binInst [0] & 0x0600) >> 9)
		{
		case 0:
		    printf (".B");
		    break;
		case 1:
		    printf (".W");
		    break;
		case 2:
		    printf (".L");
		    break;
		}
	    break;

	case itTrapcc:
	    switch (binInst [0] & 7)
		{
		case 2:
		    printf (".W");
		    break;

		case 3:
		    printf (".L");
		    break;

		case 4:
		    printf ("  ");
		    break;
		}
	    break;

	case itChk:
	    printf (((binInst [0] & 0x0080) == 0) ? ".W" : ".L");
	    break;

	case itMoveA:
	    printf (((binInst [0] & 0x1000) == 0) ? ".L" : ".W");
	    break;

	case itAdda:
	    printf (((binInst [0] & 0x0100) == 0) ? ".W" : ".L");
	    break;

	case itMovem:
	case itMovep:
	case itExt:
	case itCpBcc:
	    printf (((binInst [0] & 0x0040) == 0) ?  ".W" : ".L");
	    break;

	case itCpTrapcc:
	    switch (binInst [0] & 7)
		{
		case 2:
		    printf (".W");
		    break;
		case 3:
		    printf (".L");
		    break;
		case 4:
		    printf ("  ");
		    break;
		}
	    break;

	case itMoveB:
	case itCpScc:
	    printf (".B");
	    break;

	case itMoveW:
	case itDivW:
	case itLink:
	case itCpDbcc:
	    printf (".W");
	    break;

	case itMoveL:
	case itDivL:
	case itLinkL:
	    printf (".L");
	    break;

	/* fpp instructions */
	case itFb:
		if (binInst[0] & 0x0040)
			printf (".L");
		else
			printf (".W");
		break;

	case itFrestore:
	case itFsave:
	case itFdb:
	case itFnop:
		printf ("  ");
		break;

	case itFtrap:
		if ((binInst[0] & 0x7) == 0x2)
			printf (".W");
		else if ((binInst[0] & 0x7) == 0x3)
			printf (".L");
		break;

	case itFmovek:
		switch ((binInst[1] & 0x1c00) >> 10)
		    {
		    case 0:		printf (".L"); break;
		    case 1:		printf (".S"); break;
		    case 2:		printf (".X"); break;
		    case 3:		printf (".P"); break;
		    case 4:		printf (".W"); break;
		    case 5:		printf (".D"); break;
		    case 6:		printf (".B"); break;
		    case 7:		printf (".P"); break;
		    }
		break;

	case itFmovel:
	case itFmovemc:
		printf (".L");
		break;

	case itFmovecr:
	case itFmovem:
		printf (".X");
		break;

	case itFabs:
	case itFacos:
	case itFadd:
	case itFasin:
	case itFatan:
	case itFatanh:
	case itFcmp:
	case itFcos:
	case itFcosh:
	case itFdiv:
	case itFetox:
	case itFetoxm1:
	case itFgetexp:
	case itFgetman:
	case itFint:
	case itFintrz:
	case itFlog10:
	case itFlog2:
	case itFlogn:
	case itFlognp1:
	case itFmod:
	case itFmove:
	case itFmul:
	case itFneg:
	case itFrem:
	case itFscale:
	case itFs:
	case itFsgldiv:
	case itFsglmul:
	case itFsin:
	case itFsincos:
	case itFsinh:
	case itFsqrt:
	case itFsub:
	case itFtan:
	case itFtanh:
	case itFtentox:
	case itFtst:
	case itFtwotox:
	    if (binInst[1] & 0x4000)
		{
		switch ((binInst[1] & 0x1c00)>>10)
		    {
		    case 0:
			    printf (".L"); break;
		    case 1:
			    printf (".S"); break;
		    case 2:
			    printf (".X"); break;
		    case 3:
			    printf (".P"); break;
		    case 4:
			    printf (".W"); break;
		    case 5:
			    printf (".D"); break;
		    case 6:
			    printf (".B"); break;
		    }
		}
	    else
	        printf (".X"); break;

	    break;
	}
    }
/*******************************************************************************
*
* nPrtAddress - print addresses as numbers
*/

LOCAL void nPrtAddress
    (
    int address
    )
    {
    printf ("%#x", address);
    }
/*******************************************************************************
*
* prOffWid - print the offset and width fields of an instruction
*/

LOCAL void prOffWid
    (
    FAST USHORT dO,             /* if dO is true, offset in data reg */
    FAST USHORT offset,         /* field offset */
    FAST USHORT dW,             /* if dW is true, width in data reg */
    FAST USHORT width           /* field width */
    )
    {
    printf (dO ? "{D%x:" : "{#%d:", offset);
    printf (dW ? "D%x}" : "#%d}", width);
    }
/*******************************************************************************
*
* prIndirectIndex - print memory indirect with index, address register or
* 	program counter
*
* Assumes only modes 6 and 7 of effective addressing
*/

LOCAL void prIndirectIndex
    (
    USHORT extension [],
    USHORT mode,
    USHORT reg
    )
    {
    USHORT scale;		/* scaling factor */
    USHORT bdSize;		/* base displacement size */
    USHORT iS;			/* index suppress bit */
    USHORT indexReg;		/* index register number */
    USHORT indexIndirect;	/* index/indirect selection */

    scale 	= (extension [0] & 0x0600) >> 9;
    bdSize 	= (extension [0] & 0x0030) >> 4;
    iS		= (extension [0] & 0x0040) >> 6;
    indexReg	= (extension [0] & 0x7000) >> 12;
    indexIndirect = extension [0] & 7;

    if ((extension [0] & 0x0100) == 0)
	{
    	/* address register indirect with index (8-bit displacement) */

	printf ("(%#x,", extension [0] & 0xff); /* print displacement */

	/* print address register */

	if (mode == 7)
	    printf ("PC,");
	else
	    printf ("A%x,", reg);

	/* print the index register.  The high order bit of the
	   extension word determines whether it's a D or A reg */

	printf (extension [0] & 0x8000 ? "A%x" : "D%x", indexReg);

	/* determine whether the index register is a word or a long,
	   also determined by a bit in the extension word */

	printf (extension [0] & 0x0800 ? ".L" : ".W");

	/* print the scaling factor */

	printf ("*%d)", 1 << scale);
	}

    /*else if ((iS == 1) || ((iS == 0) && (indexIndirect == 0))) --ism */
    else if ((iS == 0) && (indexIndirect == 0))
	{
    	/* address register indirect with index (base displacement)
	 * or PC indirect with index (base displacement)
	 */

	printf ("(");

	/* print the base displacement, address register, index register,
	   length, and scaling factor */

	prDisplace (bdSize, &extension [1]);
	if (mode == 7)
	    printf (",PC,");
	else if ((extension[0] & 0x80) == 0)
	    printf (",A%x,", reg);
	else
	    printf (",");
	printf (extension [0] & 0x8000 ? "A%x" : "D%x", indexReg);
	printf (extension [0] & 0x0800 ? ".L" : ".W");
	printf ("*%d)", 1 << scale);
	}

    /*else if ((iS == 0) && ((indexIndirect > 4) && (indexIndirect < 8)))--ism*/
    else if ((iS == 1) || ((iS == 0) && ((indexIndirect > 4) &&
		(indexIndirect < 8))))
    	/* memory indirect post-indexed */
	{
	printf ("([");

	/* print the base displacement, address register, index register,
	   length, and scaling factor */

	prDisplace (bdSize, &extension [1]);
	if (mode == 7)
	    printf (",PC],");
	else
	    printf (",A%x],", reg);

	if (iS==0) /* no suppression of index */
		{
		printf (extension [0] & 0x8000 ? "A%x" : "D%x", indexReg);
		printf (extension [0] & 0x0800 ? ".L" : ".W");
		printf ("*%d,", 1 << scale);
		}

	/* print the outer displacement */

	prDisplace (extension [0] & 7, &extension [0] + bdSize);
	printf (")");
	}

    else if ((iS == 0) && (indexIndirect >= 1) && (indexIndirect <= 3))
    	/* memory indirect pre-indexed */
	{
	printf ("([");

	/* print the base displacement, address register, index register,
	   length, and scaling factor */

	prDisplace (bdSize, &extension [1]);
	if (mode == 7)
	    printf (",PC,");
	else
	    printf (",A%x,", reg);

	printf (extension [0] & 0x8000 ? "A%x" : "D%x", indexReg);
	printf (extension [0] & 0x0800 ? ".L" : ".W");
	printf ("*%d],", 1 << scale);

	/* print the outer displacement */

	prDisplace (extension [0] & 7, &extension [0] + bdSize);
	printf (")");
	}
    }
/*******************************************************************************
*
* prDisplace - print displacement
*
* Used for printing base and outer displacements.  Only looks at two least
* significant bits of the size.
*/

LOCAL void prDisplace
    (
    USHORT size,
    USHORT pDisp []
    )
    {
    switch (size & 3)
	{
	case 1:				/* NULL displacement */
	    printf ("0");
	    break;

	case 2:				/* word displacement */
	    printf ("%#x", pDisp [0]);
	    break;

	case 3:				/* long displacement */
	    printf ("%#x%04x", pDisp [0], pDisp [1]);
	    break;
	}
    }
/**************************************************************************
*
* dsmData - disassemble and print a word as data
*
* This routine disassembles and prints a single 16-bit word as data (that is,
* as a DC.W assembler directive) on standard output.  The disassembled data
* will be prepended with the address passed as a parameter.
*
* RETURNS: The number of words occupied by the data (always 1).
*/

int dsmData
    (
    USHORT *binInst,    /* Pointer to the data */
    int address         /* Address prepended to data */
    )
    {
    dsmPrint (binInst, (INST *)NULL, address, 1, (FUNCPTR) nPrtAddress);
    return (1);
    }
/*******************************************************************************
*
* dsmInst - disassemble and print a single instruction
*
* This routine disassembles and prints a single instruction on standard
* output.  The function passed as parameter <prtAddress> is used to print any
* operands that might be construed as addresses.  The function could be a
* subroutine that prints a number or looks up the address in a symbol table.
* The disassembled instruction will be prepended with the address passed as
* a parameter.
*
* If <prtAddress> is zero, dsmInst() will use a default routine that prints
* addresses as hex numbers.
*
* ADDRESS-PRINTING ROUTINE
* Many assembly language operands are addresses.  In order to print these
* addresses symbolically, dsmInst() calls a user-supplied routine, passed as a
* parameter, to do the actual printing.  The routine should be declared as:
* .CS
*    void prtAddress (address)
*        int address;	/@ address to print @/
* .CE
*
* When called, the routine prints the address on standard output in either
* numeric or symbolic form.  For example, the address-printing routine used
* by l() looks up the address in the system symbol table and prints the
* symbol associated with it, if there is one.  If not, the routine prints the
* address as a hex number.
*
* If the <prtAddress> argument to dsmInst() is NULL, a default print routine is
* used, which prints the address as a hexadecimal number.
*
* The directive DC.W (declare word) is printed for unrecognized instructions.
*
* RETURNS : The number of 16-bit words occupied by the instruction.
*/

int dsmInst
    (
    FAST USHORT *binInst,       /* Pointer to the instruction */
    int address,                /* Address prepended to instruction */
    VOIDFUNCPTR prtAddress      /* Address printing function */
    )
    {
    FAST INST *iPtr;
    FAST int size;

    if (prtAddress == NULL)
	prtAddress = nPrtAddress;

    iPtr = dsmFind (binInst);
    size = dsmNwords (binInst, iPtr);
    dsmPrint (binInst, iPtr, address, size, (FUNCPTR) prtAddress);
    return (size);
    }
/*******************************************************************************
*
* dsmNbytes - determine the size of an instruction
*
* This routine reports the size, in bytes, of an instruction.
*
* RETURNS:
* The size of the instruction, or
* 0 if the instruction is unrecognized.
*/

int dsmNbytes
    (
    FAST USHORT *binInst        /* Pointer to the instruction */
    )
    {
    FAST INST *iPtr = dsmFind (binInst);

    return ((iPtr == NULL) ? 0 : 2 * dsmNwords (binInst, iPtr));
    }
