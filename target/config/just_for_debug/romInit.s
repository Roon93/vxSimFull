#define _ASMLANGUAGE  /*定义_ASMLANGUAGE。GNU汇编器GAS看到这个定义后，会按照C的语法进行预处理，所以GAS预处理器能够认识C头文件中定义的类型和宏。如果不定义_ASMLANGUAGE，以下的#include语句将无法编译。*/
#include "vxWorks.h"  /*系统头文件*/
#include "sysLib.h"	/*系统提供给BSP的头文件 */
#include "asm.h"	/*汇编头*/
#include "config.h" /*BSP的头文件*/

/*multiboot header*/
#define MULTIBOOT_HEADER_MAGIC          0x1BADB002
#define MULTIBOOT_HEADER_FLAGS         0x00000002

	.data /*定义数据段*/
	.globl	FUNC(copyright_wind_river) /*申明全局变量_copyright_wind_river并使用它定义一个新变量*/
	.long	0xdeadbeaf /*定义一个32-bit的全局变量，变量的初始值为_copyright_wind_river的地址。由于在Makefile中规定了romInit.o为第一个链接的模块，所以这个无名变量将出现在数据段的最开始（没有变量名）*/


	/* internals */

	.globl	romInit			/* start of system code */
	.globl	_romInit		/* start of system code */
	.globl	GTEXT(romWait)		/* wait routine */
	.globl	GTEXT(romA20on)		/* turn on A20 */

	.globl	sdata			/* start of data */
	.globl	_sdata			/* start of data */


 sdata:
_sdata:
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

	/* cold start entry point in REAL MODE(16 bits) 以下执行的是VxWorks系统的第一条指令。此时CPU还处于实模式，程序只能访问1M(16位)内存空间，缺省的指令为16-bit代码。需要尽快将CPU切换到保护模式*/

romInit:
_romInit: /*同时定义_romInit和romInit的原因是，有些编译器产生的对外部符号的调用不加下划线，而有些加。*/
	cli				/* LOCK INTERRUPT 禁止中断发生*/
	jmp	cold			/* offset must be less than 128 */

        .align  4
        /* Multiboot header*/
multiboot_header:
        /* magic */
        .long   MULTIBOOT_HEADER_MAGIC
        /* flags */
        .long   MULTIBOOT_HEADER_FLAGS
        /* checksum */
        .long   -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
	/* cold start code in REAL MODE(16 bits) */

	.balign 16,0x90
cold:
#if 0
	.byte	0x67, 0x66		/* next inst has 32bit operand 0X66用于反转默认的操作数大小! 0X67用于反转默认的寻址方式 16位模式下采用32位操作数和寻址方式*/
#endif
	lidtw	%cs:(romIdtr - romInit + ROM_TEXT_ADRS)	/* load temporary IDT to IDTR*/
#if 0
	.byte	0x67, 0x66		/* next inst has 32bit operand */
#endif
	lgdtw	%cs:(romGdtr - romInit + ROM_TEXT_ADRS)	/* load temporary GDT */

	/* switch to protected mode 切换到保护模式*/

	mov	%cr0,%eax		/* move CR0 to EAX CR0中含有控制处理器操作模式和状态的系统控制标志*/
#if 0
	.byte	0x66			/* next inst has 32bit operand */
#endif
	or	$0x00000001,%eax	/* set the PE bit */
	mov	%eax,%cr0		/* move EAX to CR0 */
	jmp	romInit1		/* near jump to flush a inst queue */

romInit1:
#if 0
	.byte	0x66			/* next inst has 32bit operand */
#endif
	mov	$0x0010,%eax		/* set data segment 0x10 is 3rd one */
	mov	%ax,%ds			/* set DS */
	mov	%ax,%es			/* set ES */
	mov	%ax,%fs			/* set FS */
	mov	%ax,%gs			/* set GS */
	mov	%ax,%ss			/* set SS */
#if 0
	.byte	0x66			/* next inst has 32bit operand */
#endif
	mov	$ ROM_STACK,%esp 	/* set lower mem stack pointer 现在已进入保护模式。然而各个段寄存器的值，以及它们的高速缓存寄存器中的值还是老的。把DS, ES, FS, GS, SS寄存器   *设为0x0010，即指向GDT的第2项（从0开始），DPL=0。它们都指向一个段。把堆栈指针esp设为ROM_STACK。*/
#if 0	
	.byte	0x67, 0x66		/* next inst has 32bit operand */
#endif
	ljmp	$0x08, $ ROM_TEXT_ADRS + romInit2 - romInit


	/* temporary IDTR stored in code segment in ROM */

romIdtr:
	.word	0x0000			/*IDT size   : 0  16bits*/
	.long	0x00000000		/*IDT address: 0 32bits*/


	/* temporary GDTR stored in code segment in ROM */

romGdtr:
	.word	0x0027			/*GDT size   : 39(8 * 5 - 1) bytes 16bits*/
	.long	(romGdt	- romInit + ROM_TEXT_ADRS) /*GDT address: romGdt 32bits*/


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

#if	defined (TGT_CPU) && defined (SYMMETRIC_IO_MODE)

        movl    $ MP_N_CPU, %eax
        lock
        incl    (%eax)

#endif	/* defined (TGT_CPU) && defined (SYMMETRIC_IO_MODE) */

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
	movl	$ FUNC(romStart),%eax	/* jump to romStart $ FUNC(romStart)*/
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
	pushl	(%eax)
	pushl	(%edx)
	movl	$0x0,(%eax)
	movl	$0x0,(%edx)
	movl	$0x01234567,(%eax)
	cmpl	$0x01234567,(%edx)
	popl	(%edx)
	popl	(%eax)
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
	pushl	(%eax)
	pushl	(%edx)
	movl	$0x0,(%eax)
	movl	$0x0,(%edx)
	movl	$0x01234567,(%eax)
	cmpl	$0x01234567,(%edx)
	popl	(%edx)
	popl	(%eax)
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

