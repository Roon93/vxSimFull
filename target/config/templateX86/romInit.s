/* romInit.s - templateX86 ROM initialization module */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
TODO -	Remove the template modification history and begin a new history
	starting with version 01a and growing the history upward with
	each revision.

modification history
--------------------
01c,08dec01,dat  update for pentium (pcPentium/romInit.s, ver 01o)
01b,27aug97,dat  code review comments, added X86 reset notes.
01a,27jan97,dat  written (from pc386/romInit.s, ver 01l).
*/

/*
TODO - Update documentation as necessary.

DESCRIPTION
This module contains the entry code for VxWorks images that start
running from ROM, such as 'bootrom' and 'vxWorks_rom'.
The entry point, romInit(), is the first code executed on power-up.
It performs the minimal setup needed to call
the generic C routine romStart() with parameter BOOT_COLD.

RomInit() typically masks interrupts in the processor, sets the initial
stack pointer (to STACK_ADRS which is defined in configAll.h), and
readies system memory by configuring the DRAM controller if necessary.
Other hardware and device initialization is performed later in the
BSP's sysHwInit() routine.

A second entry point in romInit.s is called romInitWarm(). It is called
by sysToMonitor() in sysLib.c to perform a warm boot.
The warm-start entry point must be written to allow a parameter on
the stack to be passed to romStart().  For X86 processors this is not
used. The processor is forced to restart by calling the routine sysReboot().

WARNING:
This code must be Position Independent Code (PIC).  This means that it
should not contain any absolute address references.  If an absolute address
must be used, it must be relocated by the macro ROM_ADRS(x).  This macro
will convert the absolute reference to the appropriate address within
ROM space no matter how the boot code was linked.  ROM_ADRS() is equivalent
to subtracting _romInit and adding ROM_TEXT_ADRS.

This code should not call out to subroutines declared in other modules,
specifically sysLib.o, and sysALib.o.  If an outside module is absolutely
necessary, it can be linked into the system by adding the module 
to the makefile variable BOOT_EXTRA.  If the same module is referenced by
other BSP code, then that module must be added to MACH_EXTRA as well.
Note that some C compilers can generate code with absolute addresses.
Such code should not be called from this module.  If absolute addresses
cannot be avoided, then only ROM resident code can be generated from this
module.  Compressed and uncompressed bootroms or VxWorks images will not
work if absolute addresses are not processed by the macro ROM_ADRS.

WARNING:
The most common mistake in BSP development is to attempt to do too much
in romInit.s.  This is not the main hardware initialization routine.
Only do enough device initialization to get memory functioning.  All other
device setup should be done in sysLib.c, as part of sysHwInit().

Unlike other RTOS systems, VxWorks does not use a single linear device
initialization phase.  It is common for inexperienced BSP writers to take
a BSP from another RTOS, extract the assembly language hardware setup
code and try to paste it into this file.  Because VxWorks provides 3
different memory configurations, compressed, uncompressed, and rom-resident,
this strategy will usually not work successfully.

WARNING:
The second most common mistake made by BSP writers is to assume that
hardware or CPU setup functions done by romInit.o do not need to be
repeated in sysALib.s or sysLib.c.  A vxWorks image needs only the following
from a boot program: The startType code, and the boot parameters string
in memory.  Each VxWorks image will completely reset the CPU and all
hardware upon startup.  The image should not rely on the boot program to
initialize any part of the system (it may assume that the memory controller
is initialized).

This means that all initialization done by romInit.s must be repeated in
either sysALib.s or sysLib.c.  The only exception here could be the
memory controller.  However, in most cases even that can be
reinitialized without harm.

Failure to follow this rule may require users to rebuild bootrom's for
minor changes in configuration.  It is WRS policy that bootroms and vxWorks
images should not be linked in this manner.


X86 CPU RESET NOTES:

The 80x86 will cold boot in REAL (i8086 compatible) mode. 
In this mode, the 80486 controls 20 address lines, 
A19-A0 (1MB).  The 80486 CS:EIP pair contains f000:0000fff000 upon 
initialization.  This is the linear address of 0xffff0.  
Upon reset, the processors instruction 
pointer points to the last 16 bytes of 1MB.  

However, upon reset, the 80486 processor actually holds 
the physical address lines, A31 through A20, at a high 
level until an intersegment jump instruction is executed. 
This is because the base address of the code segment in 
the segment descriptor cache register after processor 
reset is 0xffff0000) Holding A31-A20 high effectively 
logical OR's 0xfff00000 to everything applied to the 
local bus until an intersegment jump occurs.
This means that the reset vector of the processor 
is the physical (bus) address of 0xfffffff0.  

The processor, after reset, will therefore be using addresses 
within the uppermost 64KB of the 32bit (4GB) address space, 
from 0xffff0000 to 0xffffffff, until the intersegment jump
is used. 

Another point to note is that since the physical reset vector 
is 0xfffffff0, the last 16 bytes of physical address space, 
the very first instruction needs to be a (short) jump back to 
somewhere above 0xffff0000.
So, one needs a bootrom which will jump from the reset vector
(0xfffffff0) to somewhere in the uppermost 64KB (above 0xffff0000), 
then make an intersegment jump. 

Refer to /target/unsupported/config/frc386 for an example of how
to build a bootrom that does this.  The real solution to the
problem lies in a new object module format other than a.out for
the X86 architecture.
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "sysLib.h"
#include "asm.h"
#include "config.h"


	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)


	/* internals */

	.globl	romInit			/* start of system code */
	.globl	_romInit		/* start of system code */
	.globl	GTEXT(romWait)		/* wait routine */
	.globl	GTEXT(romA20on)		/* turn on A20 */

	.globl	GDATA(sdata)	/* start of data */


FUNC_LABEL(sdata)
	.asciz	"start of data"


	.text
	.balign 16

/*******************************************************************************
*
* romInit - entry point for VxWorks in ROM
*

* romInit (startType)
*     int startType;	/@ only used by 2nd entry point @/

*/

	/* cold start entry point in REAL MODE(16 bits) */

romInit:
_romInit:
	cli				/* LOCK INTERRUPT */
	jmp	cold			/* offset must be less than 128 */


	/* warm start entry point in PROTECTED MODE(32 bits) */

	.balign 16,0x90
romWarmHigh:				/* ROM_WARM_HIGH(0x10) is offset */
	cli				/* LOCK INTERRUPT */
	movl    SP_ARG1(%esp),%ebx	/* %ebx has the startType */
	jmp	warm

	/* warm start entry point in PROTECTED MODE(32 bits) */
	
	.balign 16,0x90
romWarmLow:				/* ROM_WARM_LOW(0x20) is offset */
	cli				/* LOCK INTERRUPT */
	cld				/* copy itself to ROM_TEXT_ADRS */
	movl	$ RAM_LOW_ADRS,%esi	/* get src addr (RAM_LOW_ADRS) */
	movl	$ ROM_TEXT_ADRS,%edi	/* get dst addr (ROM_TEXT_ADRS) */
	movl	$ ROM_SIZE,%ecx		/* get nBytes to copy */
	shrl	$2,%ecx			/* get nLongs to copy */
	rep				/* repeat next inst ECX time */
	movsl				/* copy nLongs from src to dst */
	movl    SP_ARG1(%esp),%ebx	/* %ebx has the startType */
	jmp	warm			/* jump to warm */

	/* copyright notice appears at beginning of ROM (in TEXT segment) */

	.ascii   "Copyright 1984-2001 Wind River Systems, Inc."


	/* cold start code in REAL MODE(16 bits) */

	.balign 16,0x90
cold:
	.byte	0x67, 0x66		/* next inst has 32bit operand */
	lidt	%cs:(romIdtr - romInit)	/* load temporary IDT */

	.byte	0x67, 0x66		/* next inst has 32bit operand */
	lgdt	%cs:(romGdtr - romInit)	/* load temporary GDT */

	/* switch to protected mode */

	mov	%cr0,%eax		/* move CR0 to EAX */
	.byte	0x66			/* next inst has 32bit operand */
	or	$0x00000001,%eax	/* set the PE bit */
	mov	%eax,%cr0		/* move EAX to CR0 */
	jmp	romInit1		/* near jump to flush a inst queue */

romInit1:
	.byte	0x66			/* next inst has 32bit operand */
	mov	$0x0010,%eax		/* set data segment 0x10 is 3rd one */
	mov	%ax,%ds			/* set DS */
	mov	%ax,%es			/* set ES */
	mov	%ax,%fs			/* set FS */
	mov	%ax,%gs			/* set GS */
	mov	%ax,%ss			/* set SS */
	.byte	0x66			/* next inst has 32bit operand */
	mov	$ ROM_STACK,%esp 	/* set lower mem stack pointer */
	.byte	0x67, 0x66		/* next inst has 32bit operand */
	ljmp	$0x08, $ ROM_TEXT_ADRS + romInit2 - romInit


	/* temporary IDTR stored in code segment in ROM */

romIdtr:
	.word	0x0000			/* size   : 0 */
	.long	0x00000000		/* address: 0 */


	/* temporary GDTR stored in code segment in ROM */

romGdtr:
	.word	0x0027			/* size   : 39(8 * 5 - 1) bytes */
	.long	(romGdt	- romInit + ROM_TEXT_ADRS) /* address: romGdt */


	/* temporary GDT stored in code segment in ROM */

	.balign 16,0x90
romGdt:
	/* 0(selector=0x0000): Null descriptor */
	.word	0x0000
	.word	0x0000
	.byte	0x00
	.byte	0x00
	.byte	0x00
	.byte	0x00

	/* 1(selector=0x0008): Code descriptor */
	.word	0xffff			/* limit: xffff */
	.word	0x0000			/* base : xxxx0000 */
	.byte	0x00			/* base : xx00xxxx */
	.byte	0x9a			/* Code e/r, Present, DPL0 */
	.byte	0xcf			/* limit: fxxxx, Page Gra, 32bit */
	.byte	0x00			/* base : 00xxxxxx */

	/* 2(selector=0x0010): Data descriptor */
	.word	0xffff			/* limit: xffff */
	.word	0x0000			/* base : xxxx0000 */
	.byte	0x00			/* base : xx00xxxx */
	.byte	0x92			/* Data r/w, Present, DPL0 */
	.byte	0xcf			/* limit: fxxxx, Page Gra, 32bit */
	.byte	0x00			/* base : 00xxxxxx */

	/* 3(selector=0x0018): Code descriptor, for the nesting interrupt */
	.word	0xffff			/* limit: xffff */
	.word	0x0000			/* base : xxxx0000 */
	.byte	0x00			/* base : xx00xxxx */
	.byte	0x9a			/* Code e/r, Present, DPL0 */
	.byte	0xcf			/* limit: fxxxx, Page Gra, 32bit */
	.byte	0x00			/* base : 00xxxxxx */

	/* 4(selector=0x0020): Code descriptor, for the nesting interrupt */
	.word	0xffff			/* limit: xffff */
	.word	0x0000			/* base : xxxx0000 */
	.byte	0x00			/* base : xx00xxxx */
	.byte	0x9a			/* Code e/r, Present, DPL0 */
	.byte	0xcf			/* limit: fxxxx, Page Gra, 32bit */
	.byte	0x00			/* base : 00xxxxxx */


	/* cold start code in PROTECTED MODE(32 bits) */

	.balign 16,0x90
romInit2:
	cli				/* LOCK INTERRUPT */
	movl	$ ROM_STACK,%esp	/* set a stack pointer */

	/* WindML + VesaBIOS initialization */
	
#ifdef	INCLUDE_WINDML	
	movl	$ VESA_BIOS_DATA_PREFIX,%ebx	/* move BIOS prefix addr to EBX */
	movl	$ VESA_BIOS_KEY_1,(%ebx)	/* store "BIOS" */
	addl	$4,%ebx				/* increment EBX */
	movl	$ VESA_BIOS_KEY_2,(%ebx)	/* store "DATA" */
	movl	$ VESA_BIOS_DATA_SIZE,%ecx	/* load ECX with nBytes to copy */
	shrl    $2,%ecx				/* get nLongs to copy */
	movl	$0,%esi				/* load ESI with source addr */
	movl	$ VESA_BIOS_DATA_ADDRESS,%edi	/* load EDI with dest addr */
	rep
	movsl					/* copy BIOS data to VRAM */
#endif	/* INCLUDE_WINDML */

	/*
         * Don't call romA20on if booted through SFL. romA20on does ISA
         * I/O port accesses to turn A20 on. In case of SFL boot, ISA
         * I/O address space will not be initialized by properly by the
         * time this code gets executed. Also, A20 comes ON when booted
         * through SFL.
         */

#ifndef INCLUDE_IACSFL	
	call	FUNC(romA20on)		/* enable A20 */
	cmpl	$0, %eax		/* is A20 enabled? */
	jne	romInitHlt		/*   no: jump romInitHlt */
#endif	/* INCLUDE_IACSFL */

	movl    $ BOOT_COLD,%ebx	/* %ebx has the startType */

	/* copy bootrom image to dst addr if (romInit != ROM_TEXT_ADRS) */

warm:
	ARCH_REGS_INIT			/* initialize DR[0-7] CR[04] EFLAGS */

	movl	$romGdtr,%eax		/* load the original GDT */
	subl	$ FUNC(romInit),%eax
	addl	$ ROM_TEXT_ADRS,%eax
	pushl	%eax	
	call	FUNC(romLoadGdt)

	movl	$ STACK_ADRS, %esp	/* initialise the stack pointer */
	movl    $ ROM_TEXT_ADRS, %esi	/* get src addr(ROM_TEXT_ADRS) */
	movl    $ romInit, %edi		/* get dst addr(romInit) */
	cmpl	%esi, %edi		/* is src and dst same? */
	je	romInit4		/*   yes: skip copying */
	movl    $ FUNC(end), %ecx	/* get "end" addr */
	subl    %edi, %ecx		/* get nBytes to copy */
	shrl    $2, %ecx		/* get nLongs to copy */
	cld 				/* clear the direction bit */
	rep				/* repeat next inst ECX time */
	movsl                           /* copy itself to the entry point */

	/* jump to romStart(absolute address, position dependent) */

romInit4:
	xorl	%ebp, %ebp		/* initialize the frame pointer */
	pushl	$0			/* initialise the EFLAGS */
	popfl
	pushl	%ebx			/* push the startType */
	movl	$ FUNC(romStart),%eax	/* jump to romStart */
	call	*%eax

	/* just in case, if there's a problem in romStart or romA20on */

romInitHlt:
	pushl	%eax
	call	FUNC(romEaxShow)	/* show EAX on your display device */
	hlt	
	jmp	romInitHlt

/*******************************************************************************
*
* romA20on - enable A20
*
* enable A20
*
* RETURNS: N/A

* void romA20on (void)
 
*/

	.balign 16,0x90
FUNC_LABEL(romA20on)
	call	FUNC(romWait)
	movl	$0xd1,%eax		/* Write command */
	outb	%al,$0x64
	call	FUNC(romWait)

	movl	$0xdf,%eax		/* Enable A20 */
	outb	%al,$0x60
	call	FUNC(romWait)

	movl	$0xff,%eax		/* NULL command */
	outb	%al,$0x64
	call	FUNC(romWait)

	movl	$0x000000,%eax		/* Check if it worked */
	movl	$0x100000,%edx
	movl	$0x0,(%eax)
	movl	$0x0,(%edx)
	movl	$0x01234567,(%eax)
	cmpl	$0x01234567,(%edx)
	jne	romA20on0

	/* another way to enable A20 */

	movl	$0x02,%eax
	outb	%al,$0x92

	xorl	%ecx,%ecx
romA20on1:
	inb	$0x92,%al
	andb	$0x02,%al
	loopz	romA20on1

	movl	$0x000000,%eax		/* Check if it worked */
	movl	$0x100000,%edx
	movl	$0x0,(%eax)
	movl	$0x0,(%edx)
	movl	$0x01234567,(%eax)
	cmpl	$0x01234567,(%edx)
	jne	romA20on0

	movl	$ 0xdeaddead,%eax	/* error, can't enable A20 */
	ret

romA20on0:
	xorl	%eax,%eax
	ret

/*******************************************************************************
*
* romLoadGdt - load the global descriptor table.
*
* RETURNS: N/A
*
* NOMANUAL

* void romLoadGdt (char *romGdtr)
 
*/

        .balign 16,0x90
FUNC_LABEL(romLoadGdt)
	movl	SP_ARG1(%esp),%eax
	lgdt	(%eax)
	movw	$0x0010,%ax		/* a selector 0x10 is 3rd one */
	movw	%ax,%ds	
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss
	ret

/*******************************************************************************
*
* romWait - wait until the input buffer become empty
*
* wait until the input buffer become empty
*
* RETURNS: N/A

* void romWait (void)
 
*/

	.balign 16,0x90
FUNC_LABEL(romWait)
	xorl	%ecx,%ecx
romWait0:
	movl	$0x64,%edx		/* Check if it is ready to write */
	inb	%dx,%al
	andb	$2,%al
	loopnz	romWait0
	ret

/*******************************************************************************
*
* romEaxShow - show EAX register 
*
* show EAX register in your display device 
*
* RETURNS: N/A

* void romEaxShow (void)
 
*/

	.balign 16,0x90
FUNC_LABEL(romEaxShow)

	/* show EAX register in your display device available */

	ret

