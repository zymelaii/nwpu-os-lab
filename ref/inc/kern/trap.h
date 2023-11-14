#ifndef MINIOS_KERN_TRAP_H
#define MINIOS_KERN_TRAP_H

#define	INT_M_CTL	0x20	/* I/O port for interrupt controller         <Master> */
#define	INT_M_CTLMASK	0x21	/* setting bits in this port disables ints   <Master> */
#define	INT_S_CTL	0xA0	/* I/O port for second interrupt controller  <Slave>  */
#define	INT_S_CTLMASK	0xA1	/* setting bits in this port disables ints   <Slave>  */

/* Hardware interrupts */
#define	NR_IRQ		16	/* Number of IRQs */
#define	CLOCK_IRQ	0
#define	KEYBOARD_IRQ	1
#define	CASCADE_IRQ	2	/* cascade enable for 2nd AT controller */
#define	ETHER_IRQ	3	/* default ethernet interrupt vector */
#define	SECONDARY_IRQ	3	/* RS232 interrupt vector for port 2 */
#define	RS232_IRQ	4	/* RS232 interrupt vector for port 1 */
#define	XT_WINI_IRQ	5	/* xt winchester */
#define	FLOPPY_IRQ	6	/* floppy disk */
#define	PRINTER_IRQ	7
#define	AT_WINI_IRQ	14	/* at winchester */

/* 执行用户进程的入口(汇编接口) */
void	restart();

/* 导入中断处理函数(汇编接口) */
// 系统调用
void	int_syscall();
// 异常
void	divide_error();
void	single_step_exception();
void	nmi();
void	breakpoint_exception();
void	overflow();
void	bounds_check();
void	inval_opcode();
void	copr_not_available();
void	double_fault();
void	copr_seg_overrun();
void	inval_tss();
void	segment_not_present();
void	stack_exception();
void	general_protection();
void	page_fault();
void	copr_error();
// 外设（时钟、键盘等）
void	hwint00();
void	hwint01();
void	hwint02();
void	hwint03();
void	hwint04();
void	hwint05();
void	hwint06();
void	hwint07();
void	hwint08();
void	hwint09();
void	hwint10();
void	hwint11();
void	hwint12();
void	hwint13();
void	hwint14();
void	hwint15();

/* 中断开关 */
void	enable_irq(int irq);
void	disable_irq(int irq);


static inline void
disable_int()
{
	asm volatile("cli");
}

static inline void
enable_int()
{
	asm volatile("sti");
}

#include <mmu.h>

#define DISABLE_INT()				\
	{					\
	u32 IF_BIT = read_eflags() & FL_IF;	\
	if (IF_BIT != 0)			\
		disable_int();			

#define ENABLE_INT()				\
	if (IF_BIT != 0)			\
		enable_int();			\
	}

/* 系统调用实际处理函数(C接口) */
void	syscall_handler(void);

/* 异常中断实际处理函数(C接口) */
void	pagefault_handler(int vec_no, int err_code, int eip, 
						int cs, int eflags);
void	exception_handler(int vec_no, int err_code, int eip, 
						int cs, int eflags);

/* 外设中断实际处理函数(C接口) */
void	default_interrupt_handler(int irq);
void	clock_interrupt_handler(int irq);
void	kb_interrupt_handler(int irq);
/* 外设中断实际处理函数表 */
// kern/trap.c
extern void (*irq_table[])(int);

#endif /* MINIOS_KERN_TRAP_H */