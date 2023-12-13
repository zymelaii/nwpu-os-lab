#ifndef MINIOS_KERN_PROCESS_H
#define MINIOS_KERN_PROCESS_H

#include <type.h>
#include <mmu.h>

/* pcb中存储用户态进程的寄存器信息 */
struct s_user_context {	/* proc_ptr points here				↑ Low			*/
	u32	gs;		/* ┓						│			*/
	u32	fs;		/* ┃						│			*/
	u32	es;		/* ┃						│			*/
	u32	ds;		/* ┃						│			*/
	u32	edi;		/* ┃						│			*/
	u32	esi;		/* ┣ pushed by save()				│			*/
	u32	ebp;		/* ┃						│			*/
	u32	kernel_esp;	/* <- 'popad' will ignore it			│			*/
	u32	ebx;		/* ┃						↑栈从高地址往低地址增长     */
	u32	edx;		/* ┃						│			*/
	u32	ecx;		/* ┃						│			*/
	u32	eax;		/* ┛						│			*/
	u32	retaddr;	/* return address for assembly code save()	│			*/
	u32	eip;		/*  ┓						│			*/
	u32	cs;		/*  ┃						│			*/
	u32	eflags;		/*  ┣ these are pushed by CPU during interrupt	│			*/
	u32	esp;		/*  ┃						│			*/
	u32	ss;		/*  ┛						┷High			*/
};
typedef struct s_user_context user_context_t;

struct s_kern_context {
	u32	eflags;
	u32	edi;
	u32	esi;
	u32	ebp;
	u32	ebx;
	u32	edx;
	u32	ecx;
	u32	eax;
	u32	esp;
};
typedef struct s_kern_context kern_context_t;

enum proc_statu {IDLE, READY, SLEEPING, ZOMBIE, INITING};

typedef struct s_proc pcb_t;

/*
 * 记录进程所申请的所有物理页面，用单向链表维护
 * 对于非物理页面（页表，页目录表cr3）的那些页面，其laddr被置为-1
 * 对于物理页面，会维护物理页面对应的线性地址首地址
 */
typedef struct s_page_node page_node_t;
struct s_page_node {
	page_node_t	*nxt;
	phyaddr_t	paddr;
	uintptr_t	laddr;
};

/*
 * 维护进程树儿子的信息，由双向链表维护
 */
typedef struct s_son_node son_node_t;
struct s_son_node {
	son_node_t	*pre;
	son_node_t	*nxt;
	pcb_t		*p_son;
};

typedef struct s_tree_node tree_node_t;
struct s_tree_node {
	pcb_t		*p_fa;
	son_node_t	*sons;
};

/* pcb */
struct s_proc {
	user_context_t		user_regs;
	kern_context_t		kern_regs;
	u32			lock;
	enum proc_statu		statu;
	u32			pid;
	phyaddr_t		cr3;
	page_node_t		*page_list;
	int			exit_code;
	int			priority;
	int			ticks;
	tree_node_t		fork_tree;
};

#define KERN_STACKSIZE	(8 * KB)

union u_proc {
	pcb_t pcb;			// 内核pcb
	char _pad[KERN_STACKSIZE];	// pcb往上剩下的空间用于内核栈
};					// 通过union控制每个进程占8kb空间
typedef union u_proc proc_t;

/* 指向当前进程pcb的指针 */
// kern/main.c
extern proc_t	*p_proc_ready;
/* pcb表 */
#define PCB_SIZE	20
// kern/main.c
extern proc_t	proc_table[];

// 内核栈切换上下文函数(汇编接口)
void	switch_kern_context(
	kern_context_t *current_context,
	kern_context_t *next_context
);

// 处理函数
u32	kern_get_pid(pcb_t *p_proc);

#endif /* MINIOS_KERN_PROCESS_H */