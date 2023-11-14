#include <assert.h>
#include <mmu.h>
#include <string.h>
#include <type.h>
#include <x86.h>

#include <kern/fs.h>
#include <kern/kmalloc.h>
#include <kern/stdio.h>
#include <kern/pmap.h>
#include <kern/process.h>
#include <kern/protect.h>
#include <kern/trap.h>

// 指向当前进程pcb的指针
proc_t *p_proc_ready;
// pcb表
proc_t	proc_table[PCB_SIZE];

static inline void
init_segment_regs(proc_t *p_proc)
{
	p_proc->pcb.user_regs.cs = (SELECTOR_FLAT_C & SA_RPL_MASK & SA_TI_MASK)
		| SA_TIL | RPL_USER;
	p_proc->pcb.user_regs.ds = (SELECTOR_FLAT_RW & SA_RPL_MASK & SA_TI_MASK)
		| SA_TIL | RPL_USER;
	p_proc->pcb.user_regs.es = (SELECTOR_FLAT_RW & SA_RPL_MASK & SA_TI_MASK)
		| SA_TIL | RPL_USER;
	p_proc->pcb.user_regs.fs = (SELECTOR_FLAT_RW & SA_RPL_MASK & SA_TI_MASK)
		| SA_TIL | RPL_USER;
	p_proc->pcb.user_regs.ss = (SELECTOR_FLAT_RW & SA_RPL_MASK & SA_TI_MASK)
		| SA_TIL | RPL_USER;
	p_proc->pcb.user_regs.gs = (SELECTOR_VIDEO & SA_RPL_MASK & SA_TI_MASK)
		| RPL_USER;
}

/*
 * 内核的main函数
 * 用于初始化用户进程，然后将执行流交给用户进程
 */
void kernel_main(void)
{
	kprintf("---start kernel main---\n");

	proc_t *p_proc = proc_table;

	for (int i = 0 ; i < PCB_SIZE ; i++, p_proc++) {
		// 初始化进程段寄存器
		init_segment_regs(p_proc);
		// 为进程分配cr3物理内存
		p_proc->pcb.cr3 = phy_malloc_4k();
		// 将3GB~3GB+128MB的线性地址映射到0~128MB的物理地址
		map_kern(p_proc->pcb.cr3);
		// 在map_kern之后，就内核程序对应的页表已经被映射了
		// 就可以直接lcr3，于此同时执行流不会触发page fault
		// 如果不先map_kern，执行流会发现执行的代码的线性地址不存在爆出Page Fault
		// 当然选不选择看个人的想法，评价是都行，各有各的优缺点
		// lcr3(p_proc->pcb.cr3);
		static char filename[PCB_SIZE][12] = {
			"DELAY   BIN",
			"DELAY   BIN",	
		};
		// 从磁盘中将文件读出，需要注意的是要满足短目录项的文件名长度11，
		// 前八个为文件名，后三个为后缀名，跟BootLoader做法一致
		// 推荐将文件加载到3GB + 48MB处，应用程序保证不会有16MB那么大
		read_file(filename[i], (void *)K_PHY2LIN(48 * MB));
		// 现在你就需要将从磁盘中读出的ELF文件解析到用户进程的地址空间中
		panic("load elf");
		
		// 上一个实验中，我们开栈是用内核中的一个数组临时充当栈
		// 但是现在就不行了，用户是无法访问内核的地址空间（3GB ~ 3GB + 128MB）
		// 需要你自行处理给用户分配用户栈。
		panic("allocate user stack");
		// 初始化用户寄存器
		p_proc->pcb.user_regs.eflags = 0x1202; /* IF=1, IOPL=1 */
		
		// 接下来初始化内核寄存器，
		// 为什么需要初始化内核寄存器原因是加入了系统调用后
		// 非常有可能出现系统调用执行过程中插入其余中断的情况，
		// 如果跟之前一样所有进程共享一个内核栈会发生不可想象的结果，
		// 为了避免这种情况，就需要给每个进程分配一个进程栈。
		// 当触发时钟中断发生调度的时候，不再是简单的切换p_proc_ready，
		// 而是需要将内核栈进行切换，而且需要切换执行流到另一个进程的内核栈。
		// 所以需要一个地方存放当前进程的寄存器上下文，这是一个富有技巧性的活，深入研究会觉得很妙，
		// 如果需要深入了解，去查看kern/sche.c中的schedule函数了解切换细节。
		p_proc->pcb.kern_regs.esp = (u32)(p_proc + 1) - 8;
		// 保证切换内核栈后执行流进入的是restart函数。
		*(u32 *)(p_proc->pcb.kern_regs.esp + 0) = (u32)restart;
		// 这里是因为restart要用`pop esp`确认esp该往哪里跳。
		*(u32 *)(p_proc->pcb.kern_regs.esp + 4) = (u32)&p_proc->pcb;

		// 初始化其余量
		p_proc->pcb.pid = i;
		static int priority_table[PCB_SIZE] = {1, 2};
		// priority 预计给每个进程分配的时间片
		// ticks 进程剩余的进程片
		p_proc->pcb.priority = p_proc->pcb.ticks = priority_table[i];
	}

	p_proc_ready = proc_table;
	// 切换进程页表和tss
	lcr3(p_proc_ready->pcb.cr3);
	tss.esp0 = (u32)(&p_proc_ready->pcb.user_regs + 1);
	
	// 开个无用的kern_context存当前执行流的寄存器上下文（之后就没用了，直接放在临时变量中）
	struct s_kern_context idle;
	switch_kern_context(&idle, &p_proc_ready->pcb.kern_regs);
	assert(0);
}