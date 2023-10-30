#include <assert.h>
#include <stdio.h>
#include <type.h>
#include <x86.h>

#include <kern/protect.h>
#include <kern/serial.h>
#include <kern/trap.h>

struct Pseudodesc gdt_ptr, idt_ptr;

Descriptor_t	gdt[GDT_SIZE];
Descriptor_t	ldt[LDT_SIZE];
Gate_t		idt[IDT_SIZE];
TSS_t		tss;

/*
 * 当发生不可挽回的错误时就打印错误信息并使CPU核休眠
 */
void
_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	// 确保CPU核不受外界中断的影响
	asm volatile("cli");
	asm volatile("cld");

	va_start(ap, fmt);
	kprintf("\x1b[0m\x1b[91mkernel panic at %s:%d: ", file, line);
	vkprintf(fmt, ap);
	kprintf("\n\x1b[0m");
	va_end(ap);
	// 休眠CPU核，直接罢工
	while(1)
		asm volatile("hlt");
}

/*
 * 很像panic，但是不会休眠CPU核，就是正常打印信息
 */
void
_warn(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	kprintf("\x1b[0m\x1b[93mkernel warning at %s:%d: ", file, line);
	vkprintf(fmt, ap);
	kprintf("\n\x1b[0m");
	va_end(ap);
}

/*
 * 再创建一遍gdt表，与loader中的gdt表不同的是
 * 新增了tss和ldt的两个的全局描述符
 */
static void
init_gdt()
{
	init_segment(&gdt[0], 0, 0, 0);
	// 代码段(cs)
	init_segment(&gdt[1], 0, 0xfffff, DA_CR | DA_32 | DA_LIMIT_4K);
	// 数据段(ds,es,fs,ss)
	init_segment(&gdt[2], 0, 0xfffff, DA_DRW | DA_32 | DA_LIMIT_4K);
	// 显存段(gs)
	init_segment(&gdt[3], 0xb8000, 0xffff, DA_DRW | DA_DPL3);
	tss.ss0 = SELECTOR_FLAT_RW;
	tss.iobase = sizeof(tss);	/* 没有I/O许可位图 */
	// tss段
	init_segment(&gdt[4], (u32)&tss, sizeof(tss) - 1, DA_386TSS);
	// ldt段
	init_segment(&gdt[5], (u32)ldt, sizeof(ldt) - 1, DA_LDT);

	gdt_ptr.pd_base = (u32)gdt;
	gdt_ptr.pd_lim = sizeof(gdt) - 1;
}
/*
 * 创建ldt表，ldt表用于用户态，用户进程的段寄存器不能存放内核权限的gdt
 * ，而是使用用户权限的ldt
 */
static void
init_ldt()
{
	init_segment(&ldt[0], 0, 0, 0);
	// 代码段(cs)
	init_segment(&ldt[1], 0, 0xfffff, 
			DA_CR | DA_32 | DA_LIMIT_4K | DA_DPL3);
	// 数据段(ds,es,fs,ss)
	init_segment(&ldt[2], 0, 0xfffff, 
			DA_DRW | DA_32 | DA_LIMIT_4K | DA_DPL3);
	// 用户态的gs继承自gdt的显存段
}

/*
 * 创建idt表，看起来挺吓人的，实际上全是按照手册抄的。
 */
static void
init_idt()
{
	// 异常处理
	init_gate(idt + INT_VECTOR_DIVIDE,	DA_386IGate, 
				divide_error,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_DEBUG,	DA_386IGate, 
				single_step_exception,	PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_NMI,		DA_386IGate, 
				nmi,			PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_BREAKPOINT,	DA_386IGate, 
				breakpoint_exception,	PRIVILEGE_USER);
	init_gate(idt + INT_VECTOR_OVERFLOW,	DA_386IGate, 
				overflow,		PRIVILEGE_USER);
	init_gate(idt + INT_VECTOR_BOUNDS,	DA_386IGate, 
				bounds_check,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_INVAL_OP,	DA_386IGate, 
				inval_opcode,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_COPROC_NOT,	DA_386IGate, 
				copr_not_available,	PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_DOUBLE_FAULT,DA_386IGate, 
				double_fault,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_COPROC_SEG,	DA_386IGate, 
				copr_seg_overrun,	PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_INVAL_TSS,	DA_386IGate, 
				inval_tss,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_SEG_NOT,	DA_386IGate, 
				segment_not_present,	PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_STACK_FAULT,	DA_386IGate, 
				stack_exception,	PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_PROTECTION,	DA_386IGate, 
				general_protection,	PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_PAGE_FAULT,	DA_386IGate, 
				page_fault,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_COPROC_ERR,	DA_386IGate, 
				copr_error,		PRIVILEGE_KRNL);
	// 外设中断
	init_gate(idt + INT_VECTOR_IRQ0 + 0,	DA_386IGate, 
				hwint00,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_IRQ0 + 1,	DA_386IGate, 
				hwint01,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_IRQ0 + 2,	DA_386IGate, 
				hwint02,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_IRQ0 + 3,	DA_386IGate, 
				hwint03,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_IRQ0 + 4,	DA_386IGate, 
				hwint04,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_IRQ0 + 5,	DA_386IGate, 
				hwint05,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_IRQ0 + 6,	DA_386IGate, 
				hwint06,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_IRQ0 + 7,	DA_386IGate, 
				hwint07,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_IRQ8 + 0,	DA_386IGate, 
				hwint08,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_IRQ8 + 1,	DA_386IGate, 
				hwint09,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_IRQ8 + 2,	DA_386IGate, 
				hwint10,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_IRQ8 + 3,	DA_386IGate, 
				hwint11,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_IRQ8 + 4,	DA_386IGate, 
				hwint12,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_IRQ8 + 5,	DA_386IGate, 
				hwint13,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_IRQ8 + 6,	DA_386IGate, 
				hwint14,		PRIVILEGE_KRNL);
	init_gate(idt + INT_VECTOR_IRQ8 + 7,	DA_386IGate, 
				hwint15,		PRIVILEGE_KRNL);
			
	idt_ptr.pd_base = (u32)idt;
	idt_ptr.pd_lim = sizeof(idt) - 1;
}

/*
 * 初始化8259A，设置外设中断，看起来挺吓人的，实际上全是按照手册抄的。
 */
static void
init_8259A()
{
	outb(INT_M_CTL, 0x11);		// Master 8259, ICW1.
	outb(INT_S_CTL, 0x11);		// Slave  8259, ICW1.
	
	// Master 8259, ICW2. 设置 '主8259' 的中断入口地址为 0x20.
	outb(INT_M_CTLMASK, INT_VECTOR_IRQ0);
	// Slave  8259, ICW2. 设置 '从8259' 的中断入口地址为 0x28
	outb(INT_S_CTLMASK, INT_VECTOR_IRQ8);
	// Master 8259, ICW3. IR2 对应 '从8259'.
	outb(INT_M_CTLMASK, 0x4);
	// Slave  8259, ICW3. 对应 '主8259' 的 IR2.
	outb(INT_S_CTLMASK, 0x2);

	outb(INT_M_CTLMASK, 0x1);	// Master 8259, ICW4.
	outb(INT_S_CTLMASK, 0x1);	// Slave  8259, ICW4.

	outb(INT_M_CTLMASK, 0xFF);	// Master 8259, OCW1. 
	outb(INT_S_CTLMASK, 0xFF);	// Slave  8259, OCW1.
}

/*
 * 内核初始化函数
 */
void cstart()
{
	// 清屏并将光标置为开头
	// ANSI转义序列，由\x1b(ascii码27，为Esc)开头的序列
	// 能够控制光标控制终端（linux上也在用这个控制终端）
	// 目前支持的功能在inc/terminal.c中的kprintf函数有写
	// \x1b[2J 清屏
	// \x1b[H 将光标放置到左上角
	kprintf("\x1b[2J\x1b[H");

	kprintf("---loading gdt ");
	init_gdt();
	kprintf("ldt ");
	init_ldt();
	kprintf("8259A ");
	init_8259A();
	kprintf("idt ");
	init_idt();
	kprintf("serial ");
	assert(init_serial() == 0);
	kprintf("---\n");
}
