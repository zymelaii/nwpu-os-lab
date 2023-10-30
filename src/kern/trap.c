#include <assert.h>
#include <stdio.h>
#include <x86.h>

#include <kern/process.h>
#include <kern/time.h>
#include <kern/trap.h>
#include <kern/keymap.h>
#include <kern/keyboard.h>

/*
 * 当前内核需要处理中断的数量
 */
int k_reenter;

void (*irq_table[16])(int) = {
	clock_interrupt_handler,
	keyboard_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
};

/*
 * 开启对应外设中断（将掩码对应位置为0）
 */
void
enable_irq(int irq)
{
	u8 mask = 1 << (irq % 8);
	if (irq < 8)
		outb(INT_M_CTLMASK, inb(INT_M_CTLMASK) & ~mask);
	else
		outb(INT_S_CTLMASK, inb(INT_S_CTLMASK) & ~mask);
}

/*
 * 关闭对应外设中断（将掩码对应位置为1）
 */
void
disable_irq(int irq)
{
	u8 mask = 1 << (irq % 8);
	if (irq < 8)
		outb(INT_M_CTLMASK, inb(INT_M_CTLMASK) | mask);
	else
		outb(INT_S_CTLMASK, inb(INT_S_CTLMASK) | mask);
}
/*
 * 中断默认处理函数
 * 理论上是不支持的，所以会给个warning
 */
void
default_interrupt_handler(int irq)
{
	warn("unsupport interrupt! irq = %d", irq);
}

/*
 * 异常默认处理函数
 * 由于没有任何处理异常的手段，所以会给个panic
 */
void
exception_handler(int vec_no, int err_code, int eip, int cs, int eflags)
{
	char err_description[][64] = {	"#DE Divide Error",
					"#DB RESERVED",
					"—  NMI Interrupt",
					"#BP Breakpoint",
					"#OF Overflow",
					"#BR BOUND Range Exceeded",
					"#UD Invalid Opcode (Undefined Opcode)",
					"#NM Device Not Available (No Math Coprocessor)",
					"#DF Double Fault",
					"    Coprocessor Segment Overrun (reserved)",
					"#TS Invalid TSS",
					"#NP Segment Not Present",
					"#SS Stack-Segment Fault",
					"#GP General Protection",
					"#PF Page Fault",
					"—  (Intel reserved. Do not use.)",
					"#MF x87 FPU Floating-Point Error (Math Fault)",
					"#AC Alignment Check",
					"#MC Machine Check",
					"#XF SIMD Floating-Point Exception"
				};
	panic("\nException! --> %s\nEFLAGS: %x CS: %x EIP: %x\nError code: %x",
		err_description[vec_no], eflags, cs, eip, err_code);
}
/*
 * 时钟中断处理函数
 */
void
clock_interrupt_handler(int irq)
{
	kprintf("i%d", clock() + 1);
	timecounter_inc();

	//! keep 0x8 for thread 0, left for the others
	int flag = clock();
	if ((flag & 0xf) == 0x8) {
		p_proc_ready = proc_table;
	} else {
		int i = p_proc_ready - proc_table;
		p_proc_ready = &proc_table[i % (PCB_SIZE - 1) + 1];
	}
}

/*
 * 键盘中断处理函数
 */
void
keyboard_interrupt_handler(int irq)
{
	u8 scan_code = inb(0x60); //<! read the coming scan code
	if (scan_code & 0x80) { return; }
	u32 keycode = keymap[scan_code & 0x7f];
	//! only accept letters (ignore case)
	if (keycode >= 'A' && keycode <= 'Z') {
		keycode += 'a' - 'A';
	}
	if (!(keycode >= 'a' && keycode <= 'z')) {
		return;
	}
	add_keyboard_buf(keycode & 0x7f);
}
