#ifndef MINIOS_PROCESS_H
#define MINIOS_PROCESS_H

#include <type.h>

/* pcb中存储用户态进程的寄存器信息 */
struct s_StackFrame {		/* proc_ptr points here				↑ Low			*/
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
typedef struct s_StackFrame StackFrame_t;

/* pcb */
struct s_Process {
	// process' registers saved in stack frame
	StackFrame_t	user_regs;
	// process id passed in from MM
	u32		pid;
};
typedef struct s_Process Process_t;

#define PCB_SIZE 3
/* 指向当前进程pcb的指针 */
extern Process_t	*p_proc_ready;
/* pcb表 */
extern Process_t	proc_table[PCB_SIZE];

#endif /* MINIOS_PROCESS_H */