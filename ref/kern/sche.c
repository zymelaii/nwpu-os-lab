#include <x86.h>

#include <kern/process.h>
#include <kern/protect.h>
#include <kern/trap.h>

/*
 * 调度函数
 */
void
schedule(void)
{
	// 如果中断没关，一定！一定！一定！要关中断保证当前执行流操作的原子性
	DISABLE_INT();
		proc_t *p_cur_proc = p_proc_ready;
		proc_t *p_next_proc = p_proc_ready;

		asm volatile ("":::"memory");

		static bool in_sche;
		if (in_sche)
			goto free;
		in_sche = true;

		do {
			if (++p_next_proc >= proc_table + PCB_SIZE)
				p_next_proc = proc_table;
			if (p_cur_proc == p_next_proc && 
					p_next_proc->pcb.statu != READY) {
				asm volatile("sti\n\thlt\n\tcli":::"memory");
			}
		} while (p_next_proc->pcb.statu != READY);
		
		p_proc_ready = p_next_proc;
		// 切换进程页表和tss
		lcr3(p_proc_ready->pcb.cr3);
		tss.esp0 = (u32)(&p_proc_ready->pcb.user_regs + 1);

		in_sche = false;

		switch_kern_context(
			&p_cur_proc->pcb.kern_regs, 
			&p_next_proc->pcb.kern_regs);
		
		asm volatile ("mfence":::"memory");
free:
	ENABLE_INT();
}