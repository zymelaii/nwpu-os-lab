#include <assert.h>
#include <errno.h>
#include <elf.h>
#include <string.h>
#include <x86.h>

#include <kern/fs.h>
#include <kern/exec.h>
#include <kern/kmalloc.h>
#include <kern/pmap.h>
#include <kern/protect.h>
#include <kern/sche.h>
#include <kern/syscall.h>
#include <kern/trap.h>

static inline void
init_segment_regs(pcb_t *p_proc)
{
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
}

static inline void
init_pagetbl(pcb_t *p_proc)
{
	phyaddr_t new_cr3 = phy_malloc_4k();
	memset((void *)K_PHY2LIN(new_cr3), 0, PGSIZE);
	page_node_t *new_page_list = kmalloc(sizeof(page_node_t));
	new_page_list->nxt = NULL;
	new_page_list->paddr = new_cr3;
	new_page_list->laddr = -1;
	map_kern(new_cr3, &new_page_list);
	// 这里需要特别注意的是，替换用户页表这种危险行为
	// 无论如何都是要关中断的，不允许中间有任何调度
	// 否则很有可能换到一半，啪，一个中断进来调度了
	// 调度回来时要加载cr3，然后惊喜地发现page fault了
	page_node_t *old_page_list;
	DISABLE_INT();
		old_page_list = p_proc->page_list;
		p_proc->cr3 = new_cr3;
		p_proc->page_list = new_page_list;
		lcr3(p_proc->cr3);
	ENABLE_INT();
	// 最后记得回收进程页面资源
	recycle_pages(old_page_list);
}

ssize_t
kern_exec(pcb_t *p_proc, const char *pathname)
{
	ssize_t ret = 0;

	// 路径名的地址必须在内核中的地址
	// 因为exec会回收用户程序的页表
	// 这会导致程序中的地址发生缺页触发page fault
	assert((uintptr_t)pathname >= K_PHY2LIN(0));

	// 进来先给自己上个锁
	while (xchg(&p_proc->lock, 1) == 1)
		schedule();
	
	if ((ret = read_file(pathname, (void *)K_PHY2LIN(48 * MB))) < 0)
		goto free;
	
	// 这里有一个特判，如果不是elf文件会返回ENOEXEC
	if (*(uintptr_t *)K_PHY2LIN(48 * MB) != ELF_MAGIC) {
		ret = -ENOEXEC;
		goto free;
	}
	
	assert(p_proc->statu == READY);
	
	// 初始化用户寄存器
	memset(&p_proc->user_regs, 0, sizeof(p_proc->user_regs));
	init_segment_regs(p_proc);
	p_proc->user_regs.eflags = 0x1202; /* IF=1, IOPL=1 */
	// 初始化页表
	init_pagetbl(p_proc);
	// 将elf加载到文件指定的地址
	map_elf(p_proc, (void *)K_PHY2LIN(48 * MB));
	// 初始化用户栈空间
	map_stack(p_proc);
free:
	// 最后记得释放锁
	xchg(&p_proc->lock, 0);
	return ret;
}

/*
 * 将src路径名转换为段目录项名到dst
 * 如果不能转换会返回-ENOENT
 */
static inline ssize_t
translate_pathname(char *dst, const char *src)
{
	assert(strlen(dst) == 11);
	
	char *st = (char *)src;
	char *ed = st + strlen(st);
	char *dot = ed;
	for (char *c = st ; *c ; c++) {
		if (*c == '.')
			dot = c;
	}

	if (dot - st > 8)
		return -ENOENT;
	memcpy(dst, st, dot - st);

	if (ed - dot - 1 > 3)
		return -ENOENT;
	memcpy(dst + 8, dot + 1, ed == dot ? 0 : ed - dot - 1);
	
	for (char *c = dst ; *c ; c++) {
		if ('a' <= *c && *c <= 'z')
			*c += 'A' - 'a';
	}

	return 0;
}

ssize_t
do_exec(const char *pathname)
{
	ssize_t ret = 0;
	char *name_cpy;

	// 原来路径名转短目录项名的工作可以交给do_exec来做
	name_cpy = kmalloc(12);
	memset(name_cpy, ' ', 11);
	name_cpy[11] = '\0';

	// 当然如果转文件名失败了会返回-ENOENT
	if ((ret = translate_pathname(name_cpy, pathname)) < 0)
		goto free;

	ret = kern_exec(&p_proc_ready->pcb, name_cpy);
free:
	if (name_cpy)
		kfree(name_cpy);
	return ret;
}
