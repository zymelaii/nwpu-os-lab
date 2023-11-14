#include <elf.h>
#include <mmu.h>
#include <string.h>
#include <x86.h>

#include <kern/time.h>
#include <kern/trap.h>
#include <kern/pmap.h>
#include <kern/process.h>
#include <kern/protect.h>
#include <kern/sche.h>
#include <kern/syscall.h>

/*
 * 这个函数是写给汇编函数用，
 * 在汇编函数调用save函数保存完用户寄存器上下文后，
 * 需要切到进程内核栈中，minios的进程栈是与pcb是绑定的。
 * 相当于每个进程分配了一个大空间，前面一小部分给了pcb，
 * 后面的全可以给进程栈使用，目前进程占8KB，进程栈占7KB多。
 * 
 * 这个函数的目的就是切换到进程栈处，由于每个进程的空间只有C语言这知道，
 * 所以直接在C语言里面写一个内联汇编实现，这样更方便动态开内核栈空间不用顾忌到汇编。
 */
void
to_kern_stack(u32 ret_esp)
{
	asm volatile (
		"add %1, %0\n\t"
		"mov %0, %%esp\n\t"
		"push %2\n\t"
		"jmp *4(%%ebp)\n\t":
		: "a"((u32)p_proc_ready->_pad)
		, "b"((u32)sizeof(p_proc_ready->_pad))
		, "c"(ret_esp)
	);
}

u32
kern_get_pid(proc_t *p_proc)
{
	return p_proc->pcb.pid;
}

ssize_t
do_get_pid(void)
{
	return (ssize_t)kern_get_pid(p_proc_ready);
}