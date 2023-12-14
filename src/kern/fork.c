#include <assert.h>
#include <string.h>
#include <x86.h>
#include <errno.h>
#include <kern/pmap.h>
#include <kern/kmalloc.h>
#include <kern/sche.h>
#include <kern/exec.h>
#include <kern/process.h>
#include <kern/trap.h>
#include <kern/stdio.h>

extern void lin_mapping_phy(
    u32 cr3, page_node_t **page_list, uintptr_t laddr, phyaddr_t paddr, u32 pte_flag);

static u32 get_next_pid()
{
    static u32 lock = 0;
    static u32 next_pid = 1;
    //! WARNING: pid must be declared before schedule
    u32 pid = -1;
    while (xchg(&lock, 1) == 1) { schedule(); }
    pid = next_pid++;
    xchg(&lock, 0);
    assert(pid > 0 && "pid out of range");
    return pid;
}

static pcb_t *alloc_pcb()
{
    for (int i = 0; i < PCB_SIZE; ++i) {
        pcb_t *pcb = &proc_table[i].pcb;
        //! NOTE: a locked idle proc can be a potential availabel pcb,
        //> but it's too hard to decide, so simply ignore it. this strategy
        //> can cause an errno -EAGAIN even if pcb pool is not fully used
        if (pcb->statu == IDLE && pcb->lock == 0) {
            return pcb;
        }
    }
    return NULL;
}

static void copy_pcb(pcb_t *src, pcb_t *dst)
{
    //! copy regfile
    //! NOTE: some cpu model may not recog instrs generated from a direct
    //> struct copy operation, so call memcpy instead
    memcpy(&dst->user_regs, &src->user_regs, sizeof(user_context_t));

    //! sche attrs
    dst->priority = src->priority;
    dst->ticks = dst->priority;

    //! copy page mem, simply alloc & copy
    page_node_t *node = src->page_list;
    u8 pgbuf[PGSIZE] = {};
    DISABLE_INT();
    while (node != NULL) {
        do {
            //! skip invalid addr
            if (node->laddr == -1) { break; }
            //! skip kernel mem
            //! NOTE: not necessary actually (maybe, i'm not sure)
            if (node->laddr >= K_PHY2LIN(0) && node->laddr < K_PHY2LIN(128 * MB)) { break; }
            lin_mapping_phy(dst->cr3, &dst->page_list, node->laddr, -1, PTE_P | PTE_U | PTE_W);
            //! copy page mem via temp buffer
            //! NOTE: origin cr3 is from src
            memcpy(pgbuf, (void*)node->laddr, PGSIZE);
            lcr3(dst->cr3);
            //! NOTE: laddr is the same
            memcpy((void*)node->laddr, pgbuf, PGSIZE);
            lcr3(src->cr3);
        } while (0);
        node = node->nxt;
    }
    ENABLE_INT();

    //! copy kern stack -> conf kern stack & regfile

    //! NOTE: well, I have absolutely no idea how to copy the kernel stack, so I'm
    //> just going to lay out an execution flow and forget about the kernal fork.
    //> I think it would be better to separate user-mode fork and kernel-mode fork
    //> into two distinct implementations.

    //! detailed execution flow
    //! 1 schedule
    //! 2 switch_kern_context
    //! 2.1 switch_kern_context.inner_switch
    //! 2.2 push kern_regs of the old proc to its own kern stack
    //! 2.3 pop kern_regs of the new proc from its own kern stack
    //! 2.4 ret ~ jmp restart
    //! 3 restart
    //! 3.1 esp <- &user_regs
    //! 3.2 pop user_regs.{ gs, fs, es, ds }
    //! 3.3 pop user_regs.{ edi, esi, ebp, esp (ignore), ebx, edx, ecx, eax }
    //! 3.4 add esp, 4 ~ skip user_regs.retaddr
    //! 3.5 iret ~ pop user_regs.{ eip, cs, eflags } ~ continue at eip

    dst->kern_regs.esp = (u32)((void*)dst + KERN_STACKSIZE - 8);
    ((u32*)dst->kern_regs.esp)[0] = (u32)restart;   //<! direct jmp to restart in switch_kern_context
    ((u32*)dst->kern_regs.esp)[1] = (u32)dst;       //<! new esp for restart
    dst->user_regs.eflags = 0x1202;
}

static void join_proc_son(pcb_t *p_fa, pcb_t *p_ch)
{
    //! NOTE: assume that p_ch is an orphan proc
    son_node_t *son = (son_node_t*)kmalloc(sizeof(son_node_t));
    son->pre = NULL;
    son->nxt = NULL;
    son->p_son = p_ch;

    p_ch->fork_tree.p_fa = p_fa;
    p_ch->fork_tree.sons = NULL;

    if (p_fa->fork_tree.sons != NULL) {
        son_node_t *node = p_fa->fork_tree.sons;
        son->nxt = node;
        while (xchg(&node->p_son->lock, 1) == 1) { schedule(); }
        node->pre = son;
        xchg(&node->p_son->lock, 0);
    }
    p_fa->fork_tree.sons = son;
}

ssize_t
kern_fork(pcb_t *p_fa)
{
    int retval = 0;

    //! lock parent
    while (xchg(&p_fa->lock, 1) == 1) { schedule(); }

    do {
        pcb_t *p_ch = NULL;
        while (1) {
            //! pick an available pcb
            proc_t *child = (proc_t*)alloc_pcb();
            //! failed to fork: no res available
            if (child == NULL) {
                retval = -EAGAIN;
                break;
            }
            //! lock child
            //! NOTE: the lock action is expected to be useless for an idle proc,
            //> but take account of maintaining the proc tree, it's necessary
            //! NOTE: give up if failed to lock immediately to ensure sequential
            //> execution flow, it is helpful to avoid using an outdated pcb
            //> in the following fork procedure for the child
            if (xchg(&child->pcb.lock, 1) == 1) {
                schedule();
            } else if (child->pcb.statu != IDLE) {
                //! FIXME: unexpected branch
                xchg(&child->pcb.lock, 0);
                schedule();
            } else {
                p_ch = &child->pcb;
                assert(p_ch->statu == IDLE);
                break;
            }
        }
        if (p_ch == NULL) { break; }

        //! init page table, kernel page only
        init_pagetbl(p_ch);
        //! NOTE: the given init_pagetbl impl switches cr3, so rollback here!
        DISABLE_INT();
        //! FIXME: is incoming cr3 really from p_fa?
        lcr3(p_fa->cr3);
	    ENABLE_INT();

        //! copy context
        copy_pcb(p_fa, p_ch);

        //! conf fork result
        p_ch->pid = get_next_pid();
        //! NOTE: is it necessary to push it onto the kern stack?
        //> in the current impl, as child proc directly return to
        //> the prev user-mode eip, kern stack makes no effect on
        //> the execution flow.
        p_ch->user_regs.eax = 0; //<! fork retval for child
        retval = p_ch->pid;      //<! fork retval for parent

        //! conf proc family tree
        join_proc_son(p_fa, p_ch);

        //! finish fork & join child proc
        p_ch->statu = READY;

        //! unlock child
        xchg(&p_ch->lock, 0);
    } while (0);

    //! unlock parent
    xchg(&p_fa->lock, 0);

    return retval;
}

ssize_t
do_fork(void)
{
	return kern_fork(&p_proc_ready->pcb);
}
