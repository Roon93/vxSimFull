/* os060ALib.s - FPSP functions OS dependent library */

/* Copyright 1984-1996 Wind River Systems, Inc. */
        .data
        .globl  _copyright_wind_river
        .long   _copyright_wind_river

/*
modification history
--------------------
01b,09jul96,p_m	replaced addql by addl.
01a,24jun94,tpr	written, by modifying Motorola FPSP os.s file B1 version.
		removed %, changed & by #, 
		changed bxxx by jxx.
*/

/*
DESCRIPTION

This files handles the host operating system depend functions which
are used both by the unimplemented integer instruction handler and the
unimplemented floating point instruction handler.

This functions come from the os.s file provided by the Motorola software
package. Instructions was translated from Motorola to GNU style.
*/

	/* internals */
	.global	_intVecBaseGet

	/* externals */
	.global	__060_dmem_write
	.global __060_imem_read
	.global	__060_dmem_read
	.global	__060_dmem_read_word
	.global	__060_dmem_read_byte
	.global	__060_dmem_read_long
	.global	__060_dmem_write_byte
	.global	__060_dmem_write_word
	.global __060_dmem_write_long
	.global	__060_imem_read_word
	.global	__060_imem_read_long
	.global	__060_real_trace
	.global	__060_real_access

	.text
	.even

/*******************************************************************************
*
* _060_dmem_write - 
* 
* Each IO routine checks to see if the memory write/read is to/from user
* or supervisor application space. The examples below use simple "move"
* instructions for supervisor mode applications and call _copyin()/_copyout()
* for user mode applications.
* When installing the 060SP, the _copyin()/_copyout() equivalents for a 
* given operating system should be substituted.
*
* The addresses within the 060SP are guaranteed to be on the stack.
* The result is that Unix processes are allowed to sleep as a consequence
* of a page fault during a _copyout.
*
* Writes to data memory while in supervisor mode.
*
* INPUTS:
*	a0 - supervisor source address	
*	a1 - user destination address
*	d0 - number of bytes to write	
* 	0x4(%a6),bit5 - 1 = supervisor mode, 0 = user mode
* OUTPUTS:
*	d1 - 0 = success, !0 = failure
*
*/
__060_dmem_write:
	movb	(a0)+,(a1)+		/* copy 1 byte */
	subql	#0x1,d0			/* decr byte counter */
	jne	__060_dmem_write	/* quit if ctr = 0 */
	clrl	d1			/* return success */
	rts

/*******************************************************************************
*
* _060_imem_read, _060_dmem_read -
*
* Reads from data/instruction memory while in supervisor mode.
*
* INPUTS:
*	a0 - user source address
*	a1 - supervisor destination address
*	d0 - number of bytes to read
* 	0x4(%a6),bit5 - 1 = supervisor mode, 0 = user mode
* OUTPUTS:
*	d1 - 0 = success, !0 = failure
*
*/
__060_imem_read:
__060_dmem_read:
	movb	(a0)+,(a1)+		/* copy 1 byte */
	subql	#0x1,d0			/* decr byte counter */
	jne	__060_dmem_read		/* quit if ctr = 0 */
	clrl	d1			/* return success */
	rts

/*******************************************************************************
*
* _060_dmem_read_byte -
* 
* Read a data byte from user memory.
*
* INPUTS:
*	a0 - user source address
* 	0x4(%a6),bit5 - 1 = supervisor mode, 0 = user mode
* OUTPUTS:
*	d0 - data byte in d0
*	d1 - 0 = success, !0 = failure
*
*/
__060_dmem_read_byte:
	clrl	d0			/* clear whole longword */
	movb	(a0),d0		/* fetch super byte */
	clrl	d1			/* return success */
	rts

/*******************************************************************************
*
* _060_dmem_read_word -
* 
* Read a data word from user memory.
*
* INPUTS:
*	a0 - user source address
* 	0x4(%a6),bit5 - 1 = supervisor mode, 0 = user mode
* OUTPUTS:
*	d0 - data word in d0
*	d1 - 0 = success, !0 = failure
*
*/
__060_dmem_read_word:
	clrl	d0			/* clear whole longword */
	movw	(a0), d0		/* fetch super word */
	clrl	d1			/* return success */
	rts

/*******************************************************************************
*
* _060_dmem_read_long -
* 

*
* INPUTS:
*	a0 - user source address
* 	0x4(%a6),bit5 - 1 = supervisor mode, 0 = user mode
* OUTPUTS:
*	d0 - data longword in d0
*	d1 - 0 = success, !0 = failure
*
*/
__060_dmem_read_long:
	movl	(a0),d0			/* fetch super longword */
	clrl	d1			/* return success */
	rts

/*******************************************************************************
*
* _060_dmem_write_byte -
*
* Write a data byte to user memory.
*
* INPUTS:
*	a0 - user destination address
* 	d0 - data byte in d0
* 	0x4(%a6),bit5 - 1 = supervisor mode, 0 = user mode
* OUTPUTS:
*	d1 - 0 = success, !0 = failure
*
*/
__060_dmem_write_byte:
	movb	d0,(a0)			/* store super byte */
	clrl	d1			/* return success */
	rts

/*******************************************************************************
*
* _060_dmem_write_word -
*
* Write a data word to user memory.
*
* INPUTS:
*	a0 - user destination address
* 	d0 - data word in d0
* 	0x4(%a6),bit5 - 1 = supervisor mode, 0 = user mode
* OUTPUTS:
*	d1 - 0 = success, !0 = failure
*
*/
__060_dmem_write_word:
	movw	d0,(a0)			/* store super word */
	clrl	d1			/* return success */
	rts

/*******************************************************************************
*
* _060_dmem_write_long -
*
* Write a data longword to user memory.
*
* INPUTS:
*	a0 - user destination address
* 	d0 - data longword in d0
* 	0x4(%a6),bit5 - 1 = supervisor mode, 0 = user mode
* OUTPUTS:
*	d1 - 0 = success, !0 = failure
*
*/
__060_dmem_write_long:
	movl	d0,(a0)			/* store super longword */
	clrl	d1			/* return success */
	rts

/*******************************************************************************
*
* _060_imem_read_word -
* 
* Read an instruction word from user memory.
*
* INPUTS:
*	a0 - user source address
* 	0x4(%a6),bit5 - 1 = supervisor mode, 0 = user mode
* OUTPUTS:
*	d0 - instruction word in d0
*	d1 - 0 = success, !0 = failure
*
*/
__060_imem_read_word:
	movw	(a0),d0			/* fetch super word */
	clrl	d1			/* return success */
	rts

/*******************************************************************************
*
* _060_imem_read_long -
* 
* Read an instruction longword from user memory.
*
* INPUTS:
*	a0 - user source address
* 	0x4(%a6),bit5 - 1 = supervisor mode, 0 = user mode
* OUTPUTS:
*	d0 - instruction longword in d0
*	d1 - 0 = success, !0 = failure
*
*/
__060_imem_read_long:
	movl	(a0),d0			/* fetch super longword */
	clrl	d1			/* return success */
	rts

/*******************************************************************************
*
* _060_real_trace - 
*
* This is the exit point for the 060FPSP when an instruction is being traced
* and there are no other higher priority exceptions pending for this instruction
* or they have already been processed.
*
* The sample code below simply executes an "rte".
*
*/
__060_real_trace:
	subql	#0x04,sp		/* save space for vector handler addr */
	movel	d0,sp@-			/* save d0 */
	movel	a0,sp@-			/* save a0 */
	jsr	_intVecBaseGet		/* get the interrupt vector base addr */
	addl	#0x24,d0		/* compute the vector number 9 addr */
	movel	d0,sp@-			/* move the vector number 9 addr */
	movel	sp@+,a0			/* into a0 */
	movel	a0@,sp@(8)		/* put into the stack the vector nb 9 */
					/* handler address */
	movel	sp@+,a0			/* restore a0 */
	movel	sp@+,d0			/* restore d0 */
	rts				/* jmp into the vector nb 9 handler */

/*******************************************************************************
*
* _060_real_access - 
*
* This is the exit point for the 060FPSP when an access error exception
* is encountered. The routine below should point to the operating system
* handler for access error exceptions. The exception stack frame is an
* 8-word access error frame.
*
* The sample routine below simply executes an "rte" instruction which
* is most likely the incorrect thing to do and could put the system
* into an infinite loop.
*
*/
__060_real_access:
	subql	#0x04,sp		/* save space for vector handler addr */
	movel	d0,sp@-			/* save d0 */
	movel	a0,sp@-			/* save a0 */
	jsr	_intVecBaseGet		/* get the interrupt vector base addr */
	addql	#0x08,d0		/* compute the vector number 2 addr */
	movel	d0,sp@-			/* move the vector number 2 addr */
	movel	sp@+,a0			/* into a0 */
	movel	a0@,sp@(8)		/* put into the stack the vector nb 2 */
					/* handler address */
	movel	sp@+,a0			/* restore a0 */
	movel	sp@+,d0			/* restore d0 */
	rts				/* jmp into the vector nb 2 handler */
