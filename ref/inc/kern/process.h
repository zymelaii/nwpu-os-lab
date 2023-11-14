#ifndef MINIOS_KERN_PROCESS_H
#define MINIOS_KERN_PROCESS_H

#include <type.h>

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

/* pcb */
struct s_proc {
	struct s_user_context	user_regs;
	struct s_kern_context	kern_regs;
	u32			pid;
	phyaddr_t		cr3;
	int			priority;
	int			ticks;
};
typedef struct s_proc __proc_t;

#define KERN_STACKSIZE	(8 * 1024)

union u_proc {
	__proc_t pcb;			// 内核pcb
	char _pad[KERN_STACKSIZE];	// pcb往上剩下的空间用于内核栈
};					// 通过union控制每个进程占8kb空间
typedef union u_proc proc_t;

/* 指向当前进程pcb的指针 */
// kern/main.c
extern proc_t *p_proc_ready;
/* pcb表 */
#define PCB_SIZE	2
// kern/main.c
extern proc_t	proc_table[];

// 内核栈切换上下文函数(汇编接口)
void	switch_kern_context(
	struct s_kern_context *current_context,
	struct s_kern_context *next_context
);

// 处理函数
u32	kern_get_pid(proc_t *p_proc);

ssize_t	do_delay_ticks(int ticks);

#endif /* MINIOS_KERN_PROCESS_H */