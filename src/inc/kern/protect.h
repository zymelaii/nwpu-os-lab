#ifndef	MINIOS_KERN_PROTECT_H
#define	MINIOS_KERN_PROTECT_H

#include <type.h>

/* 存储段描述符/系统段描述符 */
struct s_descriptor {		/* 共 8 个字节 */
	u16	limit_low;		/* Limit */
	u16	base_low;		/* Base */
	u8	base_mid;		/* Base */
	u8	attr1;			/* P(1) DPL(2) DT(1) TYPE(4) */
	u8	limit_high_attr2;	/* G(1) D(1) 0(1) AVL(1) LimitHigh(4) */
	u8	base_high;		/* Base */
};
typedef struct s_descriptor descriptor_t;

/* 门描述符 */
struct s_gate {
	u16	offset_low;	/* Offset Low */
	u16	selector;	/* Selector */
	u8	dcount;		/* 该字段只在调用门描述符中有效。
				如果在利用调用门调用子程序时引起特权级的转换和堆栈的改变，需要将外层堆栈中的参数复制到内层堆栈。
				该双字计数字段就是用于说明这种情况发生时，要复制的双字参数的数量。 */
	u8	attr;		/* P(1) DPL(2) DT(1) TYPE(4) */
	u16	offset_high;	/* Offset High */
};
typedef struct s_gate gate_t;

/* 描述符选择子 */
#define	SELECTOR_DUMMY		0x00		// ┓
#define	SELECTOR_FLAT_C		0x08		// ┣ LOADER 里面已经确定了的.
#define	SELECTOR_FLAT_RW	0x10		// ┃
#define	SELECTOR_VIDEO		(0x18+3)	// ┛<-- RPL=3
#define	SELECTOR_TSS		0x20		// TSS. 从外层跳到内存时 SS 和 ESP 的值从里面获得.
#define SELECTOR_LDT		0x28
/* 描述符类型值说明 */
#define	DA_32			0x4000	/* 32 位段		*/
#define	DA_LIMIT_4K		0x8000	/* 段界限粒度为4K字节	*/
#define	DA_DPL0			0x00	/* DPL = 0		*/
#define	DA_DPL1			0x20	/* DPL = 1		*/
#define	DA_DPL2			0x40	/* DPL = 2		*/
#define	DA_DPL3			0x60	/* DPL = 3		*/
/* 存储段描述符类型值说明 */
#define	DA_DR			0x90	/* 存在的只读数据段类型值	*/
#define	DA_DRW			0x92	/* 存在的可读写数据段属性值	*/
#define	DA_DRWA			0x93	/* 存在的已访问可读写数据段类型值*/
#define	DA_C			0x98	/* 存在的只执行代码段属性值	*/
#define	DA_CR			0x9A	/* 存在的可执行可读代码段属性值	*/
#define	DA_CCO			0x9C	/* 存在的只执行一致代码段属性值	*/
#define	DA_CCOR			0x9E	/* 存在的可执行可读一致代码段属性值*/
/* 系统段描述符类型值说明 */
#define	DA_LDT			0x82	/* 局部描述符表段类型值		*/
#define	DA_TaskGate		0x85	/* 任务门类型值			*/
#define	DA_386TSS		0x89	/* 可用 386 任务状态段类型值	*/
#define	DA_386CGate		0x8C	/* 386 调用门类型值		*/
#define	DA_386IGate		0x8E	/* 386 中断门类型值		*/
#define	DA_386TGate		0x8F	/* 386 陷阱门类型值		*/

/* 选择子类型值说明 */
/* 其中, SA_ : Selector Attribute */
#define	SA_RPL_MASK	0xFFFC
#define	SA_RPL0		0
#define	SA_RPL1		1
#define	SA_RPL2		2
#define	SA_RPL3		3

#define	SA_TI_MASK	0xFFFB
#define	SA_TIG		0
#define	SA_TIL		4

/* 异常中断向量 */
#define	INT_VECTOR_DIVIDE		0x0
#define	INT_VECTOR_DEBUG		0x1
#define	INT_VECTOR_NMI			0x2
#define	INT_VECTOR_BREAKPOINT		0x3
#define	INT_VECTOR_OVERFLOW		0x4
#define	INT_VECTOR_BOUNDS		0x5
#define	INT_VECTOR_INVAL_OP		0x6
#define	INT_VECTOR_COPROC_NOT		0x7
#define	INT_VECTOR_DOUBLE_FAULT		0x8
#define	INT_VECTOR_COPROC_SEG		0x9
#define	INT_VECTOR_INVAL_TSS		0xA
#define	INT_VECTOR_SEG_NOT		0xB
#define	INT_VECTOR_STACK_FAULT		0xC
#define	INT_VECTOR_PROTECTION		0xD
#define	INT_VECTOR_PAGE_FAULT		0xE
#define	INT_VECTOR_COPROC_ERR		0x10

/* 外设中断向量，时钟中断，键盘中断等都在这里 */
#define	INT_VECTOR_IRQ0			0x20
#define	INT_VECTOR_IRQ8			0x28

/* 系统调用 */
#define INT_VECTOR_SYSCALL		0x80

/* 权限 */
#define	PRIVILEGE_KRNL	0
#define	PRIVILEGE_TASK	1
#define	PRIVILEGE_USER	3

#define	RPL_KRNL	SA_RPL0
#define	RPL_TASK	SA_RPL1
#define	RPL_USER	SA_RPL3

/* 初始化描述符函数 */
static void
init_segment(descriptor_t *p_desc, u32 base, u32 limit, u16 attribute)
{
	p_desc->limit_low		= limit & 0x0FFFF;		// 段界限1
	p_desc->base_low		= base & 0x0FFFF;		// 段基址1
	p_desc->base_mid		= (base >> 16) & 0x0FF;		// 段基址2
	p_desc->attr1			= attribute & 0xFF;		// 属性1
	p_desc->limit_high_attr2	= ((limit >> 16) & 0x0F) |	// 段界限2
					((attribute >> 8) & 0xF0);	// 属性2
	p_desc->base_high		= (base >> 24) & 0x0FF;		// 段基址3
}

static void
init_gate(gate_t *p_gate, u8 desc_type, void *handler, u8 privilage)
{
	p_gate->offset_low	= (u32)handler & 0xffff;	//中断处理地址的低16位
	p_gate->selector	= SELECTOR_FLAT_C;		//内核代码段选择子
	p_gate->dcount		= 0;				//不需要复制参数
	p_gate->attr		= desc_type | (privilage << 5);	//属性
	p_gate->offset_high	= ((u32)handler >> 16) & 0xffff;//中断处理地址的高16位
}

/* GDT 、 LDT 和 IDT 中描述符的个数 */
#define	GDT_SIZE	128
#define LDT_SIZE	GDT_SIZE
#define IDT_SIZE	256

// kern/start.c
extern descriptor_t	gdt[GDT_SIZE];
extern descriptor_t	ldt[LDT_SIZE];
extern gate_t		idt[IDT_SIZE];

/* TSS(Taskstate) */
struct s_tss {
	u32	backlink;
	u32	esp0;		/* 当发生中断的时候esp就会变成esp0 */
	u32	ss0;		/* 当发生中断的时候ss就会变成ss0，由于ss0存储的是内核态权限段，于是顺利进入内核态 */
	u32	esp1;
	u32	ss1;
	u32	esp2;
	u32	ss2;
	u32	cr3;
	u32	eip;
	u32	flags;
	u32	eax;
	u32	ecx;
	u32	edx;
	u32	ebx;
	u32	esp;
	u32	ebp;
	u32	esi;
	u32	edi;
	u32	es;
	u32	cs;
	u32	ss;
	u32	ds;
	u32	fs;
	u32	gs;
	u32	ldt;
	u16	trap;
	u16	iobase;		/* I/O位图基址大于或等于TSS段界限，就表示没有I/O许可位图 */
	/*u8	iomap[2];*/
};
typedef struct s_tss tss_t;

/* 全局的tss数据结构 */
// kern/start.c
extern tss_t tss;

/* 描述符表 */
struct Pseudodesc {
	u16 pd_lim;		// Limit
	u32 pd_base;		// Base address
} __attribute__ ((packed));
// kern/start.c
extern struct Pseudodesc gdt_ptr, idt_ptr;

#endif /* MINIOS_KERN_PROTECT_H */
