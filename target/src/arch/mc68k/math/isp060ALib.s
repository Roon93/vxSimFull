/* isp060ALib.s - MC68060 unimplemented integer instruction assembly library */

/* Copyright 1984-1994 Wind River Systems, Inc. */
        .data
        .globl  _copyright_wind_river
        .long   _copyright_wind_river

/*
modification history
--------------------
01c,09jul96,p_m replaced addql by addl since addql only support value
		between 0 and 8.
01b,21sep95,ms  moved data to data segement to fix SPR 4987
01a,24jun94,tpr	clean up following code review.
		written, handler source code from Motorola FPSP 060sp_B1.
*/

/*
DESCRIPTION

This library provides the exception handler to handle the "Unimplemented
Integer Instruction" exception vector #61. This exception is taken when any
of the integer instructions not implemented on the 68060 hardware are
encountered. The unimplemented integer instructions are:

	64-bit divide
	64-bit multiply
	movep
	cmp2
	chk2
	cas (with a misaligned effective address)
	cas2

The exception handler code is provided by the Motorola software package. The
ONLY format which is supported by Motorola is a hexadecimal image of the source
code. The hex code is divided into 2 parts: the entry point section and the code
section. A third section named "call out section" build by the host operating
system integrator is placed just before the Motorola hex code. The code
structure is 

        -----------------
        |               | - 128 byte-sized section
   (1)  |   Call-Out    | - 4 bytes per entry 
        |               | 
        -----------------
        |               | - 8 bytes per entry
   (2)  | Entry Point   |
        |               |
        -----------------
        |               | - code section
   (3)  ~     Code      ~
        |               |
        -----------------


The purpose of the call out section is to allow the handler to reference
external functions that must be provided by the host operating system (VxWorks).
This section is exactly 128 bytes in size. There are 32 fields, each 4
bytes in size. Each field entry contains the address of the corresponding
function RELATIVE to the starting address of the "call-out" section.
The "Call-out" section sits adjacent to the Motorola code. The function
address order in the call out section is fixed by Motorola and must never
change. Below are listed the functions used by the Motorola handler:

	_060_real_chk
	_060_real_divbyzero
	_060_real_trace
	_060_real_access
	_060_isp_done
	_060_real_cas
	_060_real_cas2
	_060_real_lock_page
	_060_real_unlock_page

	_060_imem_read
	_060_dmem_read
	_060_dmem_write
	_060_imem_read_word
	_060_imem_read_long
	_060_dmem_read_byte
	_060_dmem_read_word
	_060_dmem_read_long
	_060_dmem_write_byte
	_060_dmem_write_word
	_060_dmem_write_long

The first functions are only used by the handler, but the second functions are
used both by the umimplemented integer instruction handler and by the
unimplemented floating point instruction handler. The first functions are
defined in this file before the call out section and Motorola hex code. The
second functions are defined in the os060ALib.s file.

The call out section is initialized by the isp060COTblInit() function defined 
in the isp060ArchLib.c file. isp060COTblInit() function is called by
excVecInit().

The second section, the "Entry-point" section, can be used by the host
operating system to access the functions within the handler. This avoids the
overhead associated with taking the exception. The currently defined
entry-points are listed below:

	_060_isp_unimp
 
	_060_isp_cas
	_060_isp_cas2
	_060_isp_cas_finish
	_060_isp_cas2_finish
	_060_isp_cas_inrange
	_060_isp_cas_terminate
	_060_isp_cas_restart

The only entry point used by VxWorks is the _060_isp_unimp exception handler
entry point. This handler is connected to the exception number 61 by the
intVecSet() function called by intVecSet().

The third section is the Motorola code section.  


The Motorola files isp.sa and iskeleton.s was merged to obtain this file. All
"dc.l" from isp.sa file was replaced by ".long" and all "$" by "0x".
Only the call out functions was kept from the iskeleton.s. The code
was translated from Motorola to GNU style.

SEE ALSO: MC68060, MC68LC60, MC68EC60 MICROPROCESSORS USER'S MANUAL appendix C.

*/

	/* internals */

 	.global	__060_isp_done
	.global	__060_real_chk
 	.global	__060_real_divbyzero
 	.global	__060_real_cas
 	.global	__060_real_cas2
 	.global	__060_real_lock_page
 	.global	__060_real_unlock_page
 	.global	_ISP_060_CO_TBL

	/* externals */
	.global __060_real_trace

	.text
	.even

/*******************************************************************************
*
* _060_isp_done - main exit point for the Integer exception handler.
*
* This function is the main exit point for the Unimplemented Integer
* Instruction exception handler. The stack frame is the Unimplemented Integer
* Instruction stack frame with the PC pointing to the instruction following
* the instruction just emulated. Till now no special action is performed before
* to exit the exception handler.
*
*/
__060_isp_done:
	rte

/*******************************************************************************
*
* _060_real_chk - CHK instruction emulation exit point 
*
* This is an alternate exit point for the Unimplemented Integer
* Instruction exception handler. If the instruction was a "chk2"
* and the operand was out of bounds, then exception handler creates
* a CHK exception stack frame from the Unimplemented Integer Instrcution
* stack frame and branches to this routine which calls the exception handler
* connected to the vector number 6 (IV_CHK_INSTRUCTION).
*
* Remember that a trace exception may be pending. The code below performs
* no action associated with the "chk" exception. If tracing is enabled,
* then it create a Trace exception stack frame from the "chk" exception
* stack frame and branches to the _real_trace() entry point.
*
*/
__060_real_chk:
	tstb	(sp) 			/* is tracing enabled ? */
	jpl	real_chk_end		/* no */

/*
 *
 *	    CHK FRAME	  	   TRACE FRAME
 *	*****************	*****************
 *	*   Current PC	*	*   Current PC	*
 *	*****************	*****************
 *	* 0x2 *  0x018	*	* 0x2 *  0x024	*
 *	*****************	*****************
 *	*     Next	*	*     Next	*
 *	*      PC	*	*      PC	*
 *	*****************	*****************
 *	*      SR	*	*      SR	*
 *	*****************	*****************
 *
 */
	moveb	#0x24,0x7(sp)		/* set trace vecno */
	bral	__060_real_trace

real_chk_end:
	subql	#0x04,sp		/* save space for vector handler addr */
	movel	d0,sp@-			/* save d0 */
	movel	a0,sp@-			/* save a0 */
	jsr	_intVecBaseGet		/* get the interrupt vector base addr */
	addl	#0x18,d0		/* compute the vector number 6 addr */
	movel	d0,sp@-			/* move the vector number 6 addr */
	movel	sp@+,a0			/* into a0 */
	movel	a0@,sp@(8)		/* put into the stack the vector nb 6 */
					/* handler address */
	movel	sp@+,a0			/* restore a0 */
	movel	sp@+,d0			/* restore d0 */
	rts				/* jmp into the vector nb 6 handler */

/*******************************************************************************
*
* _060_real_divbyzero - divide by zero exit point
*
* This is an alternate exit point for the Unimplemented Integer 
* Instruction exception handler . If the instruction is a 64-bit
* integer divide where the source operand is a zero, then the  exception handler
* creates a Divide-by-zero exception stack frame from the Unimplemented
* Integer Instruction stack frame and branches to this routine. This
* routine calls the exception handler connected to the vector number 5
* (IV_ZERO_DIVIDE).
*
* Remember that a trace exception may be pending. The code below performs
* no action associated with the "chk" exception. If tracing is enabled,
* then it create a Trace exception stack frame from the "chk" exception
* stack frame and branches to the _real_trace() entry point.
* 
*/
__060_real_divbyzero:
 	tstb	(sp)			/* is tracing enabled */
	jpl	real_divbyzero_end	/* no */

/*
 *
 *	 DIVBYZERO FRAME	   TRACE FRAME
 *	*****************	*****************
 *	*   Current PC	*	*   Current PC	*
 *	*****************	*****************
 *	* 0x2 *  0x014	*	* 0x2 *  0x024	*
 *	*****************	*****************
 *	*     Next	*	*     Next	*
 *	*      PC	*	*      PC	*
 *	*****************	*****************
 *	*      SR	*	*      SR	*
 *	*****************	*****************
 *
 */
	moveb	#0x24,0x7(sp)		/* set trace vecno */
	bral	__060_real_trace

real_divbyzero_end:
	subql	#0x04,sp		/* save space for vector handler addr */
	movel	d0,sp@-			/* save d0 */
	movel	a0,sp@-			/* save a0 */
	jsr	_intVecBaseGet		/* get the interrupt vector base addr */
	addl	#0x14,d0		/* compute the vector number 5 addr */
	movel	d0,sp@-			/* move the vector number 5 addr */
	movel	sp@+,a0			/* into a0 */
	movel	a0@,sp@(8)		/* put into the stack the vector nb 5 */
					/* handler address */
	movel	sp@+,a0			/* restore a0 */
	movel	sp@+,d0			/* restore d0 */
	rts				/* jmp into the vector nb 5 handler */


/*******************************************************************************
*
* _060_real_cas - entry point for CAS instruction emulation
*
* This function is called when the CAS unimplemented instruction is encountered.
* The host OS can provide the emulation code or used the code provided by the
* Motorola library. 
*
* VxWorks uses the CAS emulation code provided by the Motorola library thus
* we jump to the CAS entry point.
*
*/
__060_real_cas:
	bral	_ISP_060_TOP + 0x08

/*******************************************************************************
*
* _060_real_cas2 - entry point for CAS2 instruction emulation
*
* This function is called when the CAS2 unimplemented instruction is
* encountered. The host OS can provide the emulation code or used the
* code provided by the Motorola library. 
*
* VxWorks uses the CAS emalulation code provided by the Motorola library thus
* we jump to the CAS2 entry point.
*
*/
__060_real_cas2:
	bral	_ISP_060_TOP + 0x10

/*******************************************************************************
*
* _060_lock_page - entry point to lock a page
*
* Entry point for the operating system's routine to "lock" a page
* from being paged out. This routine is needed by the cas/cas2
* algorithms so that no page faults occur within the "core" code
* region. Note: the routine must lock two pages if the operand 
* spans two pages.
* Arguments:
*	a0 = operand address
*	d0 = `xxxxxxff -> supervisor; `xxxxxx00 -> user
*	d1 = `xxxxxxff -> longword; `xxxxxx00 -> word
* Expected outputs:
*	d0 = 0 -> success; non-zero -> failure
*
* All VxWorks accessible pages are always present thus this function simply
* returns 0.
*/
__060_real_lock_page:
	clrl	d0
	rts

/*******************************************************************************
*
* _060_unlock_page - unlock a page
*
* Entry point for the operating system's routine to "unlock" a
* page that has been "locked" previously with _real_lock_page.
* Note: the routine must unlock two pages if the operand spans
* two pages.
* Arguments:
* 	a0 = operand address
*	d0 = `xxxxxxff -> supervisor; `xxxxxx00 -> user
*	d1 = `xxxxxxff -> longword; `xxxxxx00 -> word
* Expected outputs:
*	d0 = 0 -> success; non-zero -> failure
*
* All vxWorks accessible pages are always present thus this function simply
* returns 0. 
*/
__060_real_unlock_page:
	clrl	d0
	rts

/*
 * The following table handles the function relative addresses needed by the
 * unimplemented integer instruction exception handler. This handler called
 * the following functions:
 * 
 * 	_060_real_chk			/@ defined above @/
 *	_060_real_divbyzero		/@ defined above @/
 *	_060_real_trace			/@ defined above @/
 *	_060_real_access		/@ defined above @/
 *	_060_isp_done			/@ defined above @/
 *
 *	_060_real_cas			/@ defined above @/
 *	_060_real_cas2			/@ defined above @/
 *	_060_real_lock_page		/@ defined above @/
 *	_060_real_unlock_page		/@ defined above @/
 *
 *	_060_imem_read			/@ defined in os060ALib.s @/
 *	_060_dmem_read			/@ defined in os060ALib.s @/
 *	_060_dmem_write			/@ defined in os060ALib.s @/
 *	_060_imem_read_word		/@ defined in os060ALib.s @/
 *	_060_imem_read_long		/@ defined in os060ALib.s @/
 *	_060_dmem_read_byte		/@ defined in os060ALib.s @/
 *	_060_dmem_read_word		/@ defined in os060ALib.s @/
 *	_060_dmem_read_long		/@ defined in os060ALib.s @/
 *	_060_dmem_write_byte		/@ defined in os060ALib.s @/
 *	_060_dmem_write_word		/@ defined in os060ALib.s @/
 *	_060_dmem_write_long		/@ defined in os060ALib.s @/
 *
 * Table function order is fixed by MOTOROLA code and can be changed only
 * for handler update.
 *
 * Table table size MUST be exactly 128 bytes formed of 32 fields, each 4 bytes
 * in size.
 */

	.data
	.even

_ISP_060_CO_TBL:
	.fill	32,4,0	/* reserved 32 fields, each 4 bytes in size */

/*
 * The following data are extracted from the Motorola isp.sa file. 
 * All dc.l was replaced by .long and $ by 0x.
 */
 
_ISP_060_TOP:
	.long	0x60ff0000,0x02360000,0x60ff0000,0x13560000
	.long	0x60ff0000,0x10080000,0x60ff0000,0x0f260000
	.long	0x60ff0000,0x0e1a0000,0x60ff0000,0x0fd00000
	.long	0x60ff0000,0x0f920000,0x60ff0000,0x0f660000
	.long	0x51fc51fc,0x51fc51fc,0x51fc51fc,0x51fc51fc
	.long	0x51fc51fc,0x51fc51fc,0x51fc51fc,0x51fc51fc
	.long	0x51fc51fc,0x51fc51fc,0x51fc51fc,0x51fc51fc
	.long	0x51fc51fc,0x51fc51fc,0x51fc51fc,0x51fc51fc
	.long	0x2f00203a,0xfefc487b,0x0930ffff,0xfef8202f
	.long	0x00044e74,0x00042f00,0x203afeea,0x487b0930
	.long	0xfffffee2,0x202f0004,0x4e740004,0x2f00203a
	.long	0xfed8487b,0x0930ffff,0xfecc202f,0x00044e74
	.long	0x00042f00,0x203afec6,0x487b0930,0xfffffeb6
	.long	0x202f0004,0x4e740004,0x2f00203a,0xfeb4487b
	.long	0x0930ffff,0xfea0202f,0x00044e74,0x00042f00
	.long	0x203afea2,0x487b0930,0xfffffe8a,0x202f0004
	.long	0x4e740004,0x2f00203a,0xfe90487b,0x0930ffff
	.long	0xfe74202f,0x00044e74,0x00042f00,0x203afe7e
	.long	0x487b0930,0xfffffe5e,0x202f0004,0x4e740004
	.long	0x2f00203a,0xfe6c487b,0x0930ffff,0xfe48202f
	.long	0x00044e74,0x00042f00,0x203afe76,0x487b0930
	.long	0xfffffe32,0x202f0004,0x4e740004,0x2f00203a
	.long	0xfe64487b,0x0930ffff,0xfe1c202f,0x00044e74
	.long	0x00042f00,0x203afe52,0x487b0930,0xfffffe06
	.long	0x202f0004,0x4e740004,0x2f00203a,0xfe40487b
	.long	0x0930ffff,0xfdf0202f,0x00044e74,0x00042f00
	.long	0x203afe2e,0x487b0930,0xfffffdda,0x202f0004
	.long	0x4e740004,0x2f00203a,0xfe1c487b,0x0930ffff
	.long	0xfdc4202f,0x00044e74,0x00042f00,0x203afe0a
	.long	0x487b0930,0xfffffdae,0x202f0004,0x4e740004
	.long	0x2f00203a,0xfdf8487b,0x0930ffff,0xfd98202f
	.long	0x00044e74,0x00042f00,0x203afde6,0x487b0930
	.long	0xfffffd82,0x202f0004,0x4e740004,0x2f00203a
	.long	0xfdd4487b,0x0930ffff,0xfd6c202f,0x00044e74
	.long	0x00042f00,0x203afdc2,0x487b0930,0xfffffd56
	.long	0x202f0004,0x4e740004,0x4e56ffa0,0x48ee3fff
	.long	0xffc02d56,0xfff8082e,0x00050004,0x66084e68
	.long	0x2d48fffc,0x600841ee,0x000c2d48,0xfffc422e
	.long	0xffaa3d6e,0x0004ffa8,0x2d6e0006,0xffa4206e
	.long	0xffa458ae,0xffa461ff,0xffffff26,0x2d40ffa0
	.long	0x0800001e,0x67680800,0x00166628,0x61ff0000
	.long	0x0a40082e,0x00050004,0x670000a6,0x082e0002
	.long	0xffaa6700,0x009c082e,0x00070004,0x66000180
	.long	0x600001aa,0x61ff0000,0x07f6082e,0x0002ffaa
	.long	0x660e082e,0x0005ffaa,0x66000104,0x60000072
	.long	0x082e0005,0x000467ea,0x082e0005,0xffaa6600
	.long	0x01204a2e,0x00046b00,0x01466000,0x01700800
	.long	0x0018670a,0x61ff0000,0x05fe6000,0x00440800
	.long	0x001b672a,0x48400c00,0x00fc670a,0x61ff0000
	.long	0x0bce6000,0x002c206e,0xffa454ae,0xffa461ff
	.long	0xfffffe68,0x61ff0000,0x0a766000,0x001461ff
	.long	0x000006d0,0x0c2e0010,0xffaa6600,0x0004605c
	.long	0x1d6effa9,0x0005082e,0x00050004,0x6606206e
	.long	0xfffc4e60,0x4cee3fff,0xffc0082e,0x00070004
	.long	0x66122d6e,0xffa40006,0x2caefff8,0x4e5e60ff
	.long	0xfffffd68,0x2d6efff8,0xfffc3d6e,0x00040000
	.long	0x2d6e0006,0x00082d6e,0xffa40002,0x3d7c2024
	.long	0x0006598e,0x4e5e60ff,0xfffffd14,0x1d6effa9
	.long	0x00054cee,0x3fffffc0,0x3cae0004,0x2d6e0006
	.long	0x00082d6e,0xffa40002,0x3d7c2018,0x00062c6e
	.long	0xfff8dffc,0x00000060,0x60ffffff,0xfcb61d6e
	.long	0xffa90005,0x4cee3fff,0xffc03cae,0x00042d6e
	.long	0x00060008,0x2d6effa4,0x00023d7c,0x20140006
	.long	0x2c6efff8,0xdffc0000,0x006060ff,0xfffffc9a
	.long	0x1d6effa9,0x00054cee,0x3fffffc0,0x2d6e0006
	.long	0x000c3d7c,0x2014000a,0x2d6effa4,0x00062c6e
	.long	0xfff8dffc,0x00000064,0x60ffffff,0xfc6c1d6e
	.long	0xffa90005,0x4cee3fff,0xffc02d6e,0x0006000c
	.long	0x3d7c2024,0x000a2d6e,0xffa40006,0x2c6efff8
	.long	0xdffc0000,0x006460ff,0xfffffc54,0x1d6effa9
	.long	0x00054cee,0x3fffffc0,0x3d7c00f4,0x000e2d6e
	.long	0xffa4000a,0x3d6e0004,0x00082c6e,0xfff8dffc
	.long	0x00000068,0x60ffffff,0xfc52203c,0xf1f1f1f1
	.long	0x4ac82040,0x302effa0,0x32000240,0x003f0281
	.long	0x00000007,0x303b020a,0x4efb0006,0x4afc0040
	.long	0xffdaffda,0xffdaffda,0xffdaffda,0xffdaffda
	.long	0xffdaffda,0xffdaffda,0xffdaffda,0xffdaffda
	.long	0x00800086,0x008c0092,0x0098009e,0x00a400aa
	.long	0x00b000c0,0x00d000e0,0x00f00100,0x01100120
	.long	0x0140014e,0x015c016a,0x01780186,0x019401a2
	.long	0x01c001d6,0x01ec0202,0x0218022e,0x0244025a
	.long	0x02700270,0x02700270,0x02700270,0x02700270
	.long	0x02f40306,0x03180330,0x02ccffda,0xffdaffda
	.long	0x206effe0,0x4e75206e,0xffe44e75,0x206effe8
	.long	0x4e75206e,0xffec4e75,0x206efff0,0x4e75206e
	.long	0xfff44e75,0x206efff8,0x4e75206e,0xfffc4e75
	.long	0x202effe0,0x2200d288,0x2d41ffe0,0x20404e75
	.long	0x202effe4,0x2200d288,0x2d41ffe4,0x20404e75
	.long	0x202effe8,0x2200d288,0x2d41ffe8,0x20404e75
	.long	0x202effec,0x2200d288,0x2d41ffec,0x20404e75
	.long	0x202efff0,0x2200d288,0x2d41fff0,0x20404e75
	.long	0x202efff4,0x2200d288,0x2d41fff4,0x20404e75
	.long	0x202efff8,0x2200d288,0x2d41fff8,0x20404e75
	.long	0x1d7c0004,0xffaab0fc,0x00016710,0x202efffc
	.long	0x2200d288,0x2d41fffc,0x20404e75,0x524860ec
	.long	0x202effe0,0x90882d40,0xffe02040,0x4e75202e
	.long	0xffe49088,0x2d40ffe4,0x20404e75,0x202effe8
	.long	0x90882d40,0xffe82040,0x4e75202e,0xffec9088
	.long	0x2d40ffec,0x20404e75,0x202efff0,0x90882d40
	.long	0xfff02040,0x4e75202e,0xfff49088,0x2d40fff4
	.long	0x20404e75,0x202efff8,0x90882d40,0xfff82040
	.long	0x4e751d7c,0x0008ffaa,0xb0fc0001,0x670e202e
	.long	0xfffc9088,0x2d40fffc,0x20404e75,0x524860ee
	.long	0x206effa4,0x54aeffa4,0x61ffffff,0xfb0e3040
	.long	0xd1eeffe0,0x4e75206e,0xffa454ae,0xffa461ff
	.long	0xfffffaf8,0x3040d1ee,0xffe44e75,0x206effa4
	.long	0x54aeffa4,0x61ffffff,0xfae23040,0xd1eeffe8
	.long	0x4e75206e,0xffa454ae,0xffa461ff,0xfffffacc
	.long	0x3040d1ee,0xffec4e75,0x206effa4,0x54aeffa4
	.long	0x61ffffff,0xfab63040,0xd1eefff0,0x4e75206e
	.long	0xffa454ae,0xffa461ff,0xfffffaa0,0x3040d1ee
	.long	0xfff44e75,0x206effa4,0x54aeffa4,0x61ffffff
	.long	0xfa8a3040,0xd1eefff8,0x4e75206e,0xffa454ae
	.long	0xffa461ff,0xfffffa74,0x3040d1ee,0xfffc4e75
	.long	0x2f01206e,0xffa454ae,0xffa461ff,0xfffffa5c
	.long	0x221f0281,0x00000007,0x207614e0,0x08000008
	.long	0x670e48e7,0x3c002a00,0x260860ff,0x000000e8
	.long	0x2f022200,0xe9590241,0x000f2236,0x14c00800
	.long	0x000b6602,0x48c12400,0xef5a0282,0x00000003
	.long	0xe5a949c0,0xd081d1c0,0x241f4e75,0xb0fc0001
	.long	0x6710202e,0xffa42200,0xd2882d41,0xffa42040
	.long	0x4e75202e,0xffa42200,0x54812d41,0xffa45280
	.long	0x20404e75,0x206effa4,0x54aeffa4,0x61ffffff
	.long	0xf9da3040,0x4e75206e,0xffa458ae,0xffa461ff
	.long	0xfffff9de,0x20404e75,0x206effa4,0x54aeffa4
	.long	0x61ffffff,0xf9b63040,0xd1eeffa4,0x55884e75
	.long	0x206effa4,0x54aeffa4,0x61ffffff,0xf99e206e
	.long	0xffa45588,0x08000008,0x670e48e7,0x3c002a00
	.long	0x260860ff,0x00000030,0x2f022200,0xe9590241
	.long	0x000f2236,0x14c00800,0x000b6602,0x48c12400
	.long	0xef5a0282,0x00000003,0xe5a949c0,0xd081d1c0
	.long	0x241f4e75,0x08050006,0x67044282,0x6016e9c5
	.long	0x24042436,0x24c00805,0x000b6602,0x48c2e9c5
	.long	0x0542e1aa,0x08050007,0x67024283,0xe9c50682
	.long	0x67ffffff,0xfc280c00,0x00026d24,0x6710206e
	.long	0xffa458ae,0xffa461ff,0xfffff926,0x6010206e
	.long	0xffa454ae,0xffa461ff,0xfffff900,0x48c0d680
	.long	0xe9c50782,0x67000052,0x0c000002,0x6d246710
	.long	0x206effa4,0x58aeffa4,0x61ffffff,0xf8f46014
	.long	0x206effa4,0x54aeffa4,0x61ffffff,0xf8ce48c0
	.long	0x60024280,0x28000805,0x0002670e,0x204361ff
	.long	0xfffff910,0xd082d084,0x6012d682,0x204361ff
	.long	0xfffff900,0xd0846004,0xd6822003,0x20404cdf
	.long	0x003c4e75,0x322effa0,0x10010240,0x00072076
	.long	0x04e0302e,0xffa2d0c0,0x08010007,0x67ff0000
	.long	0x008e3001,0xef580240,0x00072036,0x04c00801
	.long	0x000667ff,0x0000004e,0x2d40ffb0,0x2f0a2448
	.long	0x700143d2,0x41eeffb0,0x61ffffff,0xf8387001
	.long	0x43ea0002,0x41eeffb1,0x61ffffff,0xf8287001
	.long	0x43ea0004,0x41eeffb2,0x61ffffff,0xf8187001
	.long	0x43ea0006,0x41eeffb3,0x61ffffff,0xf808245f
	.long	0x4e752d40,0xffb02f0a,0x24487001,0x43d241ee
	.long	0xffb261ff,0xfffff7ee,0x700143ea,0x000241ee
	.long	0xffb361ff,0xfffff7de,0x245f4e75,0x08010006
	.long	0x67000036,0x700743ee,0xffb061ff,0xfffff79a
	.long	0x41eeffb0,0x1010e188,0x10280002,0xe1881028
	.long	0x0004e188,0x10280006,0x122effa0,0xe2090241
	.long	0x00072d80,0x14c04e75,0x700343ee,0xffb061ff
	.long	0xfffff766,0x41eeffb0,0x1010e188,0x10280002
	.long	0x122effa0,0xe2090241,0x00073d80,0x14c24e75
	.long	0x61ffffff,0xfa90102e,0xffa2e918,0x0240000f
	.long	0x243604c0,0x0c2e0002,0xffa06d3c,0x671c2448
	.long	0x61ffffff,0xf7be2600,0x588a204a,0x61ffffff
	.long	0xf7b22200,0x20036000,0x003861ff,0xfffff7a4
	.long	0x32004840,0x48c048c1,0x082e0007,0xffa26600
	.long	0x002048c2,0x6000001a,0x61ffffff,0xf7701200
	.long	0xe04849c0,0x49c1082e,0x0007ffa2,0x660249c2
	.long	0x948042c3,0x02030004,0x9280b282,0x42c48604
	.long	0x02030005,0x382effa8,0x0204001a,0x88033d44
	.long	0xffa8082e,0x0003ffa2,0x66024e75,0x08040000
	.long	0x66024e75,0x1d7c0010,0xffaa4e75,0x102effa1
	.long	0x02000038,0x660e102e,0xffa10240,0x00072e36
	.long	0x04c06010,0x700461ff,0xfffff9ca,0x61ffffff
	.long	0xf7122e00,0x670000c0,0x102effa3,0x122effa2
	.long	0x02400007,0xe8090241,0x00073d40,0xffb23d41
	.long	0xffb42a36,0x04c02c36,0x14c0082e,0x0003ffa2
	.long	0x671a4a87,0x5deeffb0,0x6a024487,0x4a855dee
	.long	0xffb16a08,0x44fc0000,0x40864085,0x4a856616
	.long	0x4a866700,0x0048be86,0x6306cb46,0x60000012
	.long	0x4c476005,0x600abe85,0x634e61ff,0x00000068
	.long	0x082e0003,0xffa26724,0x4a2effb1,0x67024485
	.long	0x102effb0,0xb12effb1,0x670c0c86,0x80000000
	.long	0x62264486,0x60060806,0x001f661c,0x44eeffa8
	.long	0x4a8642ee,0xffa8302e,0xffb2322e,0xffb42d85
	.long	0x04c02d86,0x14c04e75,0x08ee0001,0xffa908ae
	.long	0x0000ffa9,0x4e75022e,0x001effa9,0x002e0020
	.long	0xffaa4e75,0x0c870000,0xffff621e,0x42814845
	.long	0x48463a06,0x8ac73205,0x48463a06,0x8ac74841
	.long	0x32054245,0x48452c01,0x4e7542ae,0xffbc422e
	.long	0xffb64281,0x0807001f,0x660e52ae,0xffbce38f
	.long	0xe38ee395,0x6000ffee,0x26072405,0x48424843
	.long	0xb4436606,0x323cffff,0x600a2205,0x82c30281
	.long	0x0000ffff,0x2f064246,0x48462607,0x2401c4c7
	.long	0x4843c6c1,0x28059883,0x48443004,0x38064a40
	.long	0x6600000a,0xb4846304,0x538160de,0x2f052c01
	.long	0x48462a07,0x61ff0000,0x006a2405,0x26062a1f
	.long	0x2c1f9c83,0x9b8264ff,0x0000001a,0x53814282
	.long	0x26074843,0x4243dc83,0xdb822607,0x42434843
	.long	0xda834a2e,0xffb66616,0x3d41ffb8,0x42814845
	.long	0x48463a06,0x424650ee,0xffb66000,0xff6c3d41
	.long	0xffba3c05,0x48464845,0x2e2effbc,0x670a5387
	.long	0xe28de296,0x51cffffa,0x2a062c2e,0xffb84e75
	.long	0x24062606,0x28054843,0x4844ccc5,0xcac3c4c4
	.long	0xc6c44284,0x4846dc45,0xd744dc42,0xd7444846
	.long	0x42454242,0x48454842,0xda82da83,0x4e75102e
	.long	0xffa10c00,0x00076e0a,0x02400007,0x263604c0
	.long	0x60107004,0x61ffffff,0xf7ac61ff,0xfffff4f4
	.long	0x2600342e,0xffa24241,0x1202e95a,0x02420007
	.long	0x283624c0,0x4a846700,0x00884a83,0x67000082
	.long	0x422effb0,0x082e0003,0xffa26718,0x4a836c08
	.long	0x4483002e,0x0001ffb0,0x4a846c08,0x44840a2e
	.long	0x0001ffb0,0x2a032c03,0x2e044846,0x4847c6c4
	.long	0xc8c6cac7,0xccc74287,0x4843d644,0xdd87d645
	.long	0xdd874843,0x42444245,0x48444845,0xd885d886
	.long	0x4a2effb0,0x67084683,0x46845283,0xd9872d83
	.long	0x24c044fc,0x00002d84,0x14c042c7,0x02070008
	.long	0x1c2effa9,0x02060010,0x8c071d46,0xffa94e75
	.long	0x42b624c0,0x42b614c0,0x7e0460e4,0x2d40ffb4
	.long	0x2200e958,0x0240000f,0x227604c0,0x2d49ffb0
	.long	0x2001ec49,0x02410007,0x2a3614c0,0x02400007
	.long	0x263604c0,0x3d40ffba,0x302effa2,0x2200e958
	.long	0x0240000f,0x207604c0,0x2d48ffbc,0x2001ec49
	.long	0x02410007,0x283614c0,0x02400007,0x243604c0
	.long	0x3d40ffb8,0x082e0001,0xffa056c7,0x082e0005
	.long	0x000456c6,0x24482649,0x22072006,0x61ffffff
	.long	0xf30c204a,0x4a8066ff,0x000001b4,0x22072006
	.long	0x204b61ff,0xfffff2f6,0x204b4a80,0x66ff0000
	.long	0x019e204a,0x224b60ff,0xfffff2cc,0x082e0001
	.long	0xffa06648,0x44eeffa8,0xb0426602,0xb24342ee
	.long	0xffa84a04,0x6610362e,0xffba3d81,0x34c2342e
	.long	0xffb83d80,0x24c2082e,0x00050004,0x56c22002
	.long	0x51c1206e,0xffbc61ff,0xfffff2b8,0x200251c1
	.long	0x206effb0,0x61ffffff,0xf2aa4e75,0x44eeffa8
	.long	0xb0826602,0xb28342ee,0xffa84a04,0x6610362e
	.long	0xffba2d81,0x34c0342e,0xffb82d80,0x24c0082e
	.long	0x00050004,0x56c22002,0x50c1206e,0xffbc61ff
	.long	0xfffff270,0x200250c1,0x206effb0,0x61ffffff
	.long	0xf2624e75,0x202effb4,0x6000fec2,0x082e0001
	.long	0xffa06610,0x700261ff,0xfffff5aa,0x2d48ffb4
	.long	0x51c7600e,0x700461ff,0xfffff59a,0x2d48ffb4
	.long	0x50c7302e,0xffa22200,0xec480240,0x00072436
	.long	0x04c00241,0x00072836,0x14c03d41,0xffb8082e
	.long	0x00050004,0x56c62448,0x22072006,0x61ffffff
	.long	0xf1ec4a80,0x66000096,0x204a60ff,0xfffff1b2
	.long	0x082e0001,0xffa0662c,0x44eeffa8,0xb04442ee
	.long	0xffa84a01,0x6608362e,0xffb83d80,0x34c2206e
	.long	0xffb451c1,0x082e0005,0x000456c0,0x61ffffff
	.long	0xf1c24e75,0x44eeffa8,0xb08442ee,0xffa84a01
	.long	0x6608362e,0xffb82d80,0x34c0206e,0xffb450c1
	.long	0x082e0005,0x000456c0,0x61ffffff,0xf1964e75
	.long	0x4e7b6000,0x4e7b6001,0x0c2e00fc,0xffa167ff
	.long	0xffffff24,0x206effb4,0x082e0001,0xffa056c7
	.long	0x6000ff40,0x4e7b6000,0x4e7b6001,0x2448588f
	.long	0x518f518e,0x723341ef,0x000843ef,0x000022d8
	.long	0x51c9fffc,0x2d4a000c,0x2d400010,0x4cee3fff
	.long	0xffc04e5e,0x60ffffff,0xf0cc4280,0x43fb0170
	.long	0x000005b2,0xb3c86d0e,0x43fb0170,0x00000010
	.long	0xb1c96d02,0x4e7570ff,0x4e754a06,0x66047001
	.long	0x60027005,0x4a076700,0x01e82448,0x26492848
	.long	0x2a49568c,0x568d220a,0x40c7007c,0x07004e7a
	.long	0x60004e7b,0x00004e7b,0x0001f58a,0xf58cf58b
	.long	0xf58df46a,0xf46cf46b,0xf46d2441,0x56812841
	.long	0xf5caf5cc,0x247c8000,0x0000267c,0xa0000000
	.long	0x287c0000,0x00002008,0x02000003,0x67200c00
	.long	0x00026700,0x009a6000,0x010651fc,0x51fc51fc
	.long	0x4e7ba008,0x0e911000,0x0e900000,0x6002600e
	.long	0xb082661c,0xb2836618,0x0e915800,0x6002600e
	.long	0x4e7bb008,0x0e904800,0x4e7bc008,0x6034600e
	.long	0x4e7bb008,0x0e900800,0x4e7bc008,0x6012600e
	.long	0x4e714e71,0x4e714e71,0x4e714e71,0x4e7160b0
	.long	0x4e7b6000,0x4e7b6001,0x46c751c4,0x60ffffff
	.long	0xfd4e4e7b,0x60004e7b,0x600146c7,0x50c460ff
	.long	0xfffffd3c,0x51fc51fc,0x51fc51fc,0x51fc51fc
	.long	0x4e7ba008,0x0e911000,0x0e900000,0x6002600e
	.long	0xb082662c,0xb2836628,0x0e915800,0x6002600e
	.long	0x48440e58,0x48004e7b,0xb0084844,0x6002600e
	.long	0x0e504800,0x4e7bc008,0x6000ffa8,0x4e71600e
	.long	0x48400e58,0x08004e7b,0xb0084840,0x6002600e
	.long	0x0e500800,0x4e7bc008,0x6000ff76,0x4e71600e
	.long	0x4e714e71,0x4e714e71,0x4e714e71,0x4e716090
	.long	0x4e7ba008,0x0e911000,0x0e900000,0x6002600e
	.long	0xb082663c,0xb2836638,0x0e915800,0x6002600e
	.long	0xe19c0e18,0x48004844,0x0e584800,0x6002600e
	.long	0xe19c4e7b,0xb0080e10,0x48006004,0x4e71600e
	.long	0x4e7bc008,0x6000ff2c,0x4e714e71,0x4e71600e
	.long	0xe1980e18,0x08004840,0x0e580800,0x6002600e
	.long	0xe1984e7b,0xb0080e10,0x08006004,0x4e71600e
	.long	0x4e7bc008,0x6000feea,0x4e714e71,0x4e71600c
	.long	0x4e714e71,0x4e714e71,0x4e714e71,0x6000ff72
	.long	0x24482649,0x28482a49,0x528c528d,0x220a40c7
	.long	0x007c0700,0x4e7a6000,0x4e7b0000,0x4e7b0001
	.long	0xf58af58c,0xf58bf58d,0xf46af46c,0xf46bf46d
	.long	0x24415681,0x2841f5ca,0xf5cc247c,0x80000000
	.long	0x267ca000,0x0000287c,0x00000000,0x20080800
	.long	0x00006600,0x009a6016,0x51fc51fc,0x51fc51fc
	.long	0x4e7ba008,0x0e511000,0x0e500000,0x6002600e
	.long	0xb042661c,0xb2436618,0x0e515800,0x6002600e
	.long	0x4e7bb008,0x0e504800,0x4e7bc008,0x6034600e
	.long	0x4e7bb008,0x0e500800,0x4e7bc008,0x6012600e
	.long	0x4e714e71,0x4e714e71,0x4e714e71,0x4e7160b0
	.long	0x4e7b6000,0x4e7b6001,0x46c751c4,0x60ffffff
	.long	0xfb6e4e7b,0x60004e7b,0x600146c7,0x50c460ff
	.long	0xfffffb5c,0x51fc51fc,0x51fc51fc,0x51fc51fc
	.long	0x4e7ba008,0x0e511000,0x0e500000,0x6002600e
	.long	0xb042662c,0xb2436628,0x0e515800,0x6002600e
	.long	0xe09c0e18,0x48004e7b,0xb008e19c,0x6002600e
	.long	0x0e104800,0x4e7bc008,0x6000ffa8,0x4e71600e
	.long	0xe0980e18,0x08004e7b,0xb008e198,0x6002600e
	.long	0x0e100800,0x4e7bc008,0x6000ff76,0x4e71600e
	.long	0x4e714e71,0x4e714e71,0x4e714e71,0x4e716090
	.long	0x4a066604,0x70016002,0x70054a07,0x660000c6
	.long	0x22482448,0x528a2602,0xe04a40c7,0x00470700
	.long	0x4e7a6000,0x4e7b0000,0x4e7b0001,0xf589f58a
	.long	0xf469f46a,0x227c8000,0x0000247c,0xa0000000
	.long	0x267c0000,0x00006016,0x51fc51fc,0x51fc51fc
	.long	0x4e7b9008,0x0e500000,0xb0446624,0x6002600e
	.long	0x0e182800,0x4e7ba008,0x0e103800,0x6002600e
	.long	0x4e7bb008,0x604c4e71,0x4e714e71,0x4e71600e
	.long	0xe0980e18,0x08004e7b,0xa008e198,0x6002600e
	.long	0x0e100800,0x4e7bb008,0x60164e71,0x4e71600e
	.long	0x4e714e71,0x4e714e71,0x4e714e71,0x4e7160a0
	.long	0x4e7b6000,0x4e7b6001,0x46c751c1,0x60ffffff
	.long	0xfb224e7b,0x60004e7b,0x600146c7,0x50c160ff
	.long	0xfffffb10,0x22482448,0x568a2208,0x08010000
	.long	0x660000c2,0x26024842,0x40c7007c,0x07004e7a
	.long	0x60004e7b,0x00004e7b,0x0001f589,0xf58af469
	.long	0xf46a227c,0x80000000,0x247ca000,0x0000267c
	.long	0x00000000,0x601851fc,0x51fc51fc,0x51fc51fc
	.long	0x4e7b9008,0x0e900000,0xb0846624,0x6002600e
	.long	0x0e582800,0x4e7ba008,0x0e503800,0x6002600e
	.long	0x4e7bb008,0x604c4e71,0x4e714e71,0x4e71600e
	.long	0x48400e58,0x08004840,0x4e7ba008,0x6002600e
	.long	0x0e500800,0x4e7bb008,0x60164e71,0x4e71600e
	.long	0x4e714e71,0x4e714e71,0x4e714e71,0x4e7160a0
	.long	0x4e7b6000,0x4e7b6001,0x46c751c1,0x60ffffff
	.long	0xfa524e7b,0x60004e7b,0x600146c7,0x50c160ff
	.long	0xfffffa40,0x2a02e08a,0x26024842,0x40c7007c
	.long	0x07004e7a,0x60004e7b,0x00004e7b,0x0001f589
	.long	0xf58af469,0xf46a227c,0x80000000,0x247ca000
	.long	0x0000267c,0x00000000,0x601451fc,0x51fc51fc
	.long	0x4e7b9008,0x0e900000,0xb0846624,0x6002600e
	.long	0x0e182800,0x0e583800,0x4e7ba008,0x6002600e
	.long	0x0e105800,0x4e7bb008,0x6000ff88,0x4e71600e
	.long	0xe1980e18,0x08004840,0x0e580800,0x6002600e
	.long	0xe1984e7b,0xa0080e10,0x08006004,0x4e71600e
	.long	0x4e7bb008,0x6000ff4a,0x4e714e71,0x4e71600e
	.long	0x4e714e71,0x4e714e71,0x4e714e71,0x4e716090
