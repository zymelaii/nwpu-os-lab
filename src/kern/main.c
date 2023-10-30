#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <type.h>
#include <x86.h>

#include <kern/process.h>
#include <kern/protect.h>
#include <kern/trap.h>

/*
 * 三个测试函数，用户进程的执行流
 */
void TestA()
{
	int i = 0;
	while(1){
		kprintf("A%d.",i++);
		for (int j = 0 ; j < 5e7 ; j++)
			;//do nothing
	}
}

void TestB()
{
	int i = 0;
	while(1){
		kprintf("B%d.",i++);
		for (int j = 0 ; j < 5e7 ; j++)
			;//do nothing
	}
}

void TestC()
{
	int i = 0;
	while(1){
		kprintf("C%d.",i++);
		for (int j = 0 ; j < 5e7 ; j++)
			;//do nothing
	}
}

const char* color_print_str[] = { "\x1b[031m%c %d %d;", "\x1b[032m%c %d %d;", "\x1b[034m%c %d %d;"};
/*
fmt: format string for kprintf, you'd better use the strings provided above in the string array
id: just like the TestA/B/C functions above, you should designate a character to mark different processes
my_pcb: the PCB(process control block) of this process
*/
void TestABC(const char * fmt, char id, Process_t* my_pcb) {
	int i = 0;
	while(1) {
		kprintf(fmt,id,i++,my_pcb->pid);
		for (int j = 0; j < 5e7; j++)
			;//do nothing
	}
}

void cb_getch_test() {
	while (1) {
		u8 ch = getch();
		if (ch == 0xff) { continue; }
		kprintf("%c ", ch);
	}
}

// 每个栈空间有4kb大
#define STACK_PREPROCESS	0x1000
#define STACK_TOTSIZE		STACK_PREPROCESS * PCB_SIZE
// 用户栈（直接在内核开一个临时数组充当）
char process_stack[STACK_TOTSIZE];
// 指向当前进程pcb的指针
Process_t	*p_proc_ready;
// pcb表
Process_t	proc_table[PCB_SIZE];
void (*entry[]) = {
	TestABC,
	TestABC,
	//TestABC,
	cb_getch_test,
};

/*
 * 内核的main函数
 * 用于初始化用户进程，然后将执行流交给用户进程
 */
void kernel_main()
{
	//! reset timer frequency
	outb(0x43, 0x34);
	outb(0x40, (u8)(((1193182 / 1000) >> 0) & 0xff));
	outb(0x40, (u8)(((1193182 / 1000) >> 8) & 0xff));

	kprintf("---start kernel main---\n");

	Process_t *p_proc = proc_table;
	char *p_stack = process_stack;

	for (int i = 0 ; i < PCB_SIZE ; i++, p_proc++) {
		p_proc->user_regs.cs = (SELECTOR_FLAT_C & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_USER;
		p_proc->user_regs.ds = (SELECTOR_FLAT_RW & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_USER;
		p_proc->user_regs.es = (SELECTOR_FLAT_RW & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_USER;
		p_proc->user_regs.fs = (SELECTOR_FLAT_RW & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_USER;
		p_proc->user_regs.ss = (SELECTOR_FLAT_RW & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_USER;
		p_proc->user_regs.gs = (SELECTOR_VIDEO & SA_RPL_MASK & SA_TI_MASK)
			| RPL_USER;
		p_proc->user_regs.eip = (u32)entry[i];
		p_stack += STACK_PREPROCESS;

		//! | ret addr | param 1 | param 2 | param... |
		u32 *params = (u32*)p_stack - 3;
		params[0] = (uintptr_t)(void*)color_print_str[i];
		params[1] = 'A' + i;
		params[2] = (uintptr_t)(void*)p_proc;
		p_proc->user_regs.esp = (u32)(params - 1);

		p_proc->pid = i+1;
		p_proc->user_regs.eflags = 0x1202; /* IF=1, IOPL=1 */
	}

	p_proc_ready = proc_table;

	enable_irq(CLOCK_IRQ);
	enable_irq(KEYBOARD_IRQ);

	restart();
	assert(0);
}
