#include <assert.h>
#include <x86.h>

#include <kern/exit.h>
#include <kern/kmalloc.h>
#include <kern/pmap.h>
#include <kern/sche.h>
#include <kern/syscall.h>
#include <kern/trap.h>

static void
awake_father_and_become_zombie(pcb_t *p_proc)
{	
	// 由于我们已经协商了上锁的顺序，上锁顺序是从父亲到儿子
	// 但是这里我们必须锁了自己才能知道父进程是谁
	// 所以这里我们换了一种做法，不再一个劲的等锁
	// 如果父进程上锁失败，就直接将自己的锁释放拱手让人
	// 这样做有个好处，要么两个锁同时被上掉，要么两个锁同时被释放
	// 这也是一个非常有趣的实现方法
	// 而真实情况是将大锁拆小锁，不可能一个pcb就一个大锁保着，这样又浪费效率又难写
	pcb_t *p_fa;
	while (1) {
		if (xchg(&p_proc->lock, 1) == 1)
			goto loop;
		p_fa = p_proc->fork_tree.p_fa;
		if (xchg(&p_fa->lock, 1) == 1)
			goto free;
		break;
free:
		xchg(&p_proc->lock, 0);
loop:
		schedule();
	}
	// 这两句assert防止其他奇奇怪怪的状态出现
	assert(p_proc->statu == READY);
	assert(p_fa->statu == READY || p_fa->statu == SLEEPING);
	p_proc->statu = ZOMBIE;
	p_fa->statu = READY;
	
	xchg(&p_fa->lock, 0);
	xchg(&p_proc->lock, 0);
}

static void
transfer_orphans(pcb_t *p_proc)
{
	pcb_t *p_init = &proc_table[0].pcb;
	
	// 上锁顺序为：初始进程->当前进程->子进程
	while (xchg(&p_init->lock, 1) == 1)
		schedule();
	while (xchg(&p_proc->lock, 1) == 1)
		schedule();

	for (son_node_t *p = p_proc->fork_tree.sons ; p ;) {
		pcb_t *p_son = p->p_son;
		son_node_t *p_nxt = p->nxt;
		// 上子进程的锁，因为需要修改子进程的父进程信息（移到初始进程下）
		while (xchg(&p_son->lock, 1) == 1)
			schedule();
		// 将子进程的进程树信息做修改
		// 将节点node移到初始进程的链表头处
		p_son->fork_tree.p_fa = p_init;
		// 确保这个是双向链表头
		assert(p->pre == NULL);
		// 接下来就是一坨又臭又长的链表操作部分
		if (p->nxt != NULL)
			p->nxt->pre = p->pre;
		p->nxt = p_init->fork_tree.sons;
		if (p->nxt != NULL)
			p->nxt->pre = p;
		p_init->fork_tree.sons = p; 
		// 最后释放子进程的锁
		xchg(&p_son->lock, 0);
		p = p_nxt;
	}
	// 在移交完后当前进程的子进程信息会被清空
	p_proc->fork_tree.sons = NULL;
	// 初始进程可能在休眠，而且子进程可能是僵尸进程，需要将初始进程唤醒
	// 初始进程会一直调用wait系统调用回收僵尸子进程
	assert(p_init->statu == READY || p_init->statu == SLEEPING);
	p_init->statu = READY;
free:
	xchg(&p_proc->lock, 0);
	xchg(&p_init->lock, 0);
}

ssize_t
kern_exit(pcb_t *p_proc, int exit_code)
{
	// 托孤，将所有子进程转移到初始进程下
	transfer_orphans(p_proc);
	
	// 上锁修改exit code
	while (xchg(&p_proc->lock, 1) == 1)
		schedule();
	p_proc->exit_code = exit_code;
	xchg(&p_proc->lock, 0);

	// 下面两个操作会修改进程的状态，
	// 这是非常危险的，最好用开关中断保护上
	DISABLE_INT();
	// 这个函数干了两件事，唤醒父进程，将自己状态置为僵尸进程
	// 关中断就相当于两件事同时干了
	awake_father_and_become_zombie(p_proc);
	// 在触发了调度之后这个进程在被回收之前永远无法被调度到
	schedule();

	ENABLE_INT();
	panic("exit failed!");
}

ssize_t
do_exit(int status)
{
	// 为什么这个参数这么奇怪？你可能需要读读手册
	return kern_exit(&p_proc_ready->pcb, (status & 0xFF) << 8);
}