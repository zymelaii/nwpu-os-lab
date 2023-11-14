#include <assert.h>
#include <x86.h>

#include <kern/keyboard.h>
#include <kern/keymap.h>
#include <kern/process.h>
#include <kern/protect.h>
#include <kern/sche.h>
#include <kern/stdio.h>
#include <kern/syscall.h>
#include <kern/time.h>
#include <kern/trap.h>

/*
 * 当前内核需要处理中断的数量
 */
int k_reenter;

void (*irq_table[16])(int) = {
	clock_interrupt_handler,
	kb_interrupt_handler,
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
 * 页面异常处理函数
 * 替大家处理页面异常信息并打印
 */
void
pagefault_handler(int vec_no, int err_code, int eip, int cs, int eflags)
{
	static char fmtstr[256];
	char *p_str = fmtstr, *p_end = fmtstr + sizeof(fmtstr) - 1;
	p_str += snprintf(p_str, p_end - p_str,
		"\nOh, you receive a page fault!\n");
	p_str += snprintf(p_str, p_end - p_str,
		"CS: 0x%%x EIP: 0x%%x You tried to access the address: 0x%%x\n");
	if ((err_code & FEC_PR) == 0) {
		p_str += snprintf(p_str, p_end - p_str,
		"You tried to access a nonexistent page!\n");
	}
	if ((err_code & FEC_WR) != 0) {
		p_str += snprintf(p_str, p_end - p_str,
		"You tried to write in this page!\n");
	} else {
		p_str += snprintf(p_str, p_end - p_str,
		"You tried to read in this page!\n");
	}
	if ((err_code & FEC_U) != 0) {
		p_str += snprintf(p_str, p_end - p_str,
		"You tried to access a page in user mode!\n");
	}
	*p_str = '\0';
	panic(fmtstr, cs, eip, rcr2());
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
 * 时钟中断处理函数，
 * 需要注意的是，在这次实验中时钟中断全程是
 * 关!中!断!
 * 关!中!断!
 * 关!中!断!
 * 不允许任何其他中断打扰。
 */
void
clock_interrupt_handler(int irq)
{
	timecounter_inc();
	// 当一个函数的时间片全用完时
	if (--p_proc_ready->pcb.ticks == 0) {
		p_proc_ready->pcb.ticks = p_proc_ready->pcb.priority;
		schedule();
	}
}

/*
 * 键盘中断处理函数
 */
void
kb_interrupt_handler(int irq)
{
	u8 ch = inb(0x60);
	if ((ch & 0x80) != 0)
		return;
	ch = keymap[ch];
	if ('a' <= ch && ch <= 'z')
		add_keyboard_buf(ch);
}

/*
 * 系统调用处理函数
 */
void
syscall_handler(void)
{
	int syscall_id = p_proc_ready->pcb.user_regs.eax;
	ssize_t ret = (*syscall_table[syscall_id])();
	p_proc_ready->pcb.user_regs.eax = ret;
}

