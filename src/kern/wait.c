#include <assert.h>
#include <string.h>
#include <mmu.h>
#include <type.h>
#include <x86.h>
#include <errno.h>
#include <kern/trap.h>
#include <kern/sche.h>
#include <kern/process.h>
#include <kern/kmalloc.h>
#include <kern/pmap.h>

static son_node_t* find_zombie_proc(son_node_t *node)
{
	static u32 lock_pool[16] = {};
	assert(node->p_son->fork_tree.p_fa != NULL);
	int index = ((proc_t*)node->p_son->fork_tree.p_fa - proc_table) % 16;
	while (xchg(&lock_pool[index], 1) == 1) { schedule(); }
	assert(node != NULL);
	do {
		if (node->p_son->statu == ZOMBIE) {
			if (xchg(&node->p_son->lock, 1) == 0) {
				break;
			}
		}
		node = node->nxt;
	} while (node != NULL);
	xchg(&lock_pool[index], 0);
	return node;
}

static void remove_proc_link(pcb_t *fa, son_node_t *son)
{
	assert(son != NULL);
	assert(son->p_son != NULL);
	son_node_t *pre = son->pre;
	son_node_t *nxt = son->nxt;
	//! FIXME: lock is required
	son->pre = NULL;
	son->nxt = NULL;
	if (pre != NULL) {
		pre->nxt = nxt;
	}
	if (nxt != NULL) {
		nxt->pre = pre;
	}
	if (fa != NULL && fa->fork_tree.sons == son) {
		assert(pre == NULL);
		fa->fork_tree.sons = nxt;
	}
}

ssize_t
kern_wait(int *wstatus)
{
	ssize_t retval = 0;

	pcb_t *fa = &p_proc_ready->pcb;
	son_node_t *node = NULL;
	while (xchg(&fa->lock, 1) == 1) { schedule(); }

	do {
		//! NOTE: proc family tree can changed externally, so
		//> it's necessary to check it every time
		if (fa->fork_tree.sons == NULL) {
			retval = -ECHILD;
			break;
		}

		//! NOTE: kern_wait is special for 0 proc since it will
		//> share its lock with kern_exec, inner procedure of
		//> kern_exec requires lock 0 proc which may be held by
		//> kern_wait.
		//> a very special condition is that in a round sche
		//> between kern_wait and kern_exec, kern_wait depends on
		//> kern_exec to ctor an unlocked zombie proc, while
		//> kern_exec also requires the lock held by kern_wait to
		//> continue the zombie ctor procedure. so there's a dead
		//> lock now.
		//> to avoid the case, disable int until the lock was
		//> released so that when switch to kern_exec, lock is
		//> always acquireable as long as it is used to be held by
		//> kern_wait
		DISABLE_INT();

		//! find a zombine proc and try to lock it
		node = find_zombie_proc(fa->fork_tree.sons);
		if (node != NULL) {
			assert(node->p_son->fork_tree.p_fa == fa);
			break;
		}

		//! sleep until awake
		fa->statu = SLEEPING;
		//! NOTE: the current proc can be waken up by an exited
		//> zombie child, but there're still some other cases,
		//> e.g. transfer_orphans in kern_exit will always wake
		//> up the 0 proc, so a loop is necessary here
		//! NOTE: give up the lock since awake from child may
		//> acquire the lock to modify proc statu
		xchg(&fa->lock, 0);

		ENABLE_INT();

		schedule();
		while (xchg(&fa->lock, 1) == 1) { schedule(); }
	} while (1);

	if (node != NULL) {
		pcb_t *ch = node->p_son;

		assert(ch->fork_tree.p_fa == fa);
		assert(ch->fork_tree.sons == NULL);

		retval = ch->pid;

		if (wstatus != NULL) {
			*wstatus = ch->exit_code;
		}

		remove_proc_link(fa, node);
		ch->fork_tree.p_fa = NULL;
		assert(ch->fork_tree.sons == NULL);

		recycle_pages(ch->page_list);

		ch->page_list = NULL;
		memset(&ch->user_regs, 0, sizeof(user_context_t));
		memset(&ch->kern_regs, 0, sizeof(kern_context_t));
		memset(&ch->fork_tree, 0, sizeof(tree_node_t));

		kfree(node);
		node = NULL;

		ch->statu = IDLE;

		xchg(&ch->lock, 0);
	}

	xchg(&fa->lock, 0);

	return retval;
}

ssize_t
do_wait(int *wstatus)
{
	assert((uintptr_t)wstatus < KERNBASE);
	assert((uintptr_t)wstatus + sizeof(wstatus) < KERNBASE);
	return kern_wait(wstatus);
}
