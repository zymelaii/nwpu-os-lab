#include <assert.h>
#include <x86.h>

#include <kern/stdio.h>
#include <kern/syscall.h>
#include <kern/protect.h>
#include <kern/process.h>
#include <kern/trap.h>

// 指向当前进程pcb的指针
proc_t *p_proc_ready;
// pcb表
proc_t	proc_table[PCB_SIZE];

/*
 * 内核的main函数
 * 用于初始化用户进程，然后将执行流交给用户进程
 */
void kernel_main(void)
{
	kprintf("---start kernel main---\n");

	p_proc_ready = proc_table;

	pcb_t *p_proc = &p_proc_ready->pcb;
	p_proc->statu = READY;
	// kern regs
	p_proc->kern_regs.esp = (u32)(p_proc_ready + 1) - 8;
	// 保证切换内核栈后执行流进入的是restart函数。
	*(u32 *)(p_proc->kern_regs.esp + 0) = (u32)restart;
	// 这里是因为restart要用`pop esp`确认esp该往哪里跳。
	*(u32 *)(p_proc->kern_regs.esp + 4) = (u32)p_proc;

	p_proc->priority = p_proc->ticks = 1;
	// 初始化进程树
	p_proc->fork_tree.p_fa = NULL;
	p_proc->fork_tree.sons = NULL;
	// 在实现了exec系统调用后main函数的负担就少了
	// 只需要调用一个函数接口就能实现进程的加载
	if (do_exec("initproc.bin") < 0)
		panic("init process failed");
	// 切换tss
	tss.esp0 = (u32)(&p_proc->user_regs + 1);
	lcr3(p_proc->cr3);
	
	// 开个无用的kern_context存当前执行流的寄存器上下文（之后就没用了，直接放在临时变量中）
	kern_context_t idle;
	switch_kern_context(&idle, &p_proc->kern_regs);
	assert(0);
}