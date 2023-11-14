[section .data]
error_code	dd 0

[section .text]

; 导入
extern	StackTop		; kern/astart.asm
extern	tss			; inc/kern/protect.h
extern	pagefault_handler	; inc/kern/trap.h
extern	exception_handler	; inc/kern/trap.h
extern	irq_table		; inc/kern/trap.h
extern	syscall_handler		; inc/kern/trap.h
extern	p_proc_ready		; inc/kern/process.h

; 具体看inc/kern/process.h pcb维护的寄存器表
P_STACKBASE	equ	0
GSREG		equ	P_STACKBASE
FSREG		equ	GSREG		+ 4
ESREG		equ	FSREG		+ 4
DSREG		equ	ESREG		+ 4
EDIREG		equ	DSREG		+ 4
ESIREG		equ	EDIREG		+ 4
EBPREG		equ	ESIREG		+ 4
KERNELESPREG	equ	EBPREG		+ 4
EBXREG		equ	KERNELESPREG	+ 4
EDXREG		equ	EBXREG		+ 4
ECXREG		equ	EDXREG		+ 4
EAXREG		equ	ECXREG		+ 4
RETADR		equ	EAXREG		+ 4
EIPREG		equ	RETADR		+ 4
CSREG		equ	EIPREG		+ 4
EFLAGSREG	equ	CSREG		+ 4
ESPREG		equ	EFLAGSREG	+ 4
SSREG		equ	ESPREG		+ 4
P_STACKTOP	equ	SSREG		+ 4

extern	to_kern_stack	; kern/process.c

save:
        pushad          ; `.
        push	ds      ;  |
        push	es      ;  | 保存原寄存器值
        push	fs      ;  |
        push	gs      ; /
        mov	dx, ss
        mov	ds, dx
        mov	es, dx

        mov	eax, esp
	test	dword [eax + CSREG - P_STACKBASE], 3
        jz	.1			; 根据段寄存器的状态信息判断是否发生内核重入
        mov	esp, StackTop		; 如果没有发生内核重入就意味着要到用户栈了，先将esp移入临时内核栈
	push	eax			; 传入进程表首地址信息
	call	to_kern_stack		; 这个函数之后esp变为进程内核栈
	pop	eax			; 恢复进程表首地址信息（这个是restart第一句话要用的）
.1:
	push	eax			; 将restart要用的，如果在内核栈重入，pop esp不会有任何变化
        push	restart			; 否则eax在进程表首地址，pop esp会使esp移动到用户栈栈底
        jmp	[eax + RETADR - P_STACKBASE]

restart:
	pop	esp			; 获悉当前的esp该到哪，不用管现在是否要回用户态，语义在save中有解释
	pop	gs
	pop	fs
	pop	es
	pop	ds
	popad
	add	esp, 4
	iretd

; 系统调用
int_syscall:
	call	save
	sti
	call	syscall_handler
	cli
	ret

EOI		equ	0x20
INT_M_CTL	equ	0x20	; I/O port for interrupt controller         <Master>
INT_M_CTLMASK	equ	0x21	; setting bits in this port disables ints   <Master>
INT_S_CTL	equ	0xA0	; I/O port for second interrupt controller  <Slave>
INT_S_CTLMASK	equ	0xA1	; setting bits in this port disables ints   <Slave>

; 中断和异常 -- 硬件中断
; ---------------------------------
%macro	hwint_master	1
	call	save
	in	al, INT_M_CTLMASK	; `.
	or	al, (1 << %1)		;  | 屏蔽当前中断
	out	INT_M_CTLMASK, al	; /
	mov	al, EOI			; `. 置EOI位
	out	INT_M_CTL, al		; /
	sti	; CPU在响应中断的过程中会自动关中断，这句之后就允许响应新的中断
	push	%1			; `.
	call	[irq_table + 4 * %1]	;  | 中断处理程序
	pop	ecx			; /
	cli
	in	al, INT_M_CTLMASK	; `.
	and	al, ~(1 << %1)		;  | 恢复接受当前中断
	out	INT_M_CTLMASK, al	; /
	ret
%endmacro

; 时钟中断采取全程关中断的模式，时钟中断相当重要，不允许被打扰
ALIGN	16
hwint00:		; Interrupt routine for irq 0 (the clock).
	call	save
	mov	al, EOI
	out	INT_M_CTL, al

	push	0
	call	[irq_table + 0]
	pop	ecx

	ret
	
ALIGN	16
hwint01:		; Interrupt routine for irq 1 (keyboard)
	hwint_master	1

ALIGN	16
hwint02:		; Interrupt routine for irq 2 (cascade!)
	hwint_master	2

ALIGN	16
hwint03:		; Interrupt routine for irq 3 (second serial)
	hwint_master	3

ALIGN	16
hwint04:		; Interrupt routine for irq 4 (first serial)
	hwint_master	4

ALIGN	16
hwint05:		; Interrupt routine for irq 5 (XT winchester)
	hwint_master	5

ALIGN	16
hwint06:		; Interrupt routine for irq 6 (floppy)
	hwint_master	6

ALIGN	16
hwint07:		; Interrupt routine for irq 7 (printer)
	hwint_master	7

; ---------------------------------
%macro	hwint_slave	1
	hlt		; 后面的8个外设中断暂时不需要，先hlt休眠核
%endmacro
; ---------------------------------

ALIGN	16
hwint08:		; Interrupt routine for irq 8 (realtime clock).
	hwint_slave	8

ALIGN	16
hwint09:		; Interrupt routine for irq 9 (irq 2 redirected)
	hwint_slave	9

ALIGN	16
hwint10:		; Interrupt routine for irq 10
	hwint_slave	10

ALIGN	16
hwint11:		; Interrupt routine for irq 11
	hwint_slave	11

ALIGN	16
hwint12:		; Interrupt routine for irq 12
	hwint_slave	12

ALIGN	16
hwint13:		; Interrupt routine for irq 13 (FPU exception)
	hwint_slave	13

ALIGN	16
hwint14:		; Interrupt routine for irq 14 (AT winchester)
	hwint_slave	14

ALIGN	16
hwint15:		; Interrupt routine for irq 15
	hwint_slave	15

%macro exception_push	0
	mov	eax, [esp + 4]		; 旧的esp
	mov	ebx, [eax + EFLAGSREG - P_STACKBASE]
	push	ebx
	mov	ebx, [eax + CSREG - P_STACKBASE]
	push	ebx
	mov	ebx, [eax + EIPREG - P_STACKBASE]
	push	ebx
%endmacro

%macro exception_with_err 2
	xchg	eax, [esp]		; 这个时候eax对应是error code
	mov	[error_code], eax	; 暂存一下
	pop	eax
	call	save

	exception_push
	mov	eax, [error_code]	; 压入error code和向量标识
	push	eax
	push	%1
	call	%2			; 处理函数
	add	esp, 8

	ret
%endmacro

%macro exception_without_err 2
	call	save

	exception_push
	push	0xFFFFFFFF		; 压入error code和向量标识
	push	%1
	call	%2			; 处理函数
	add	esp, 8

	ret
%endmacro

; 中断和异常 -- 异常
divide_error:
	exception_without_err 0, exception_handler

single_step_exception:
	exception_without_err 1, exception_handler

nmi:
	exception_without_err 2, exception_handler

breakpoint_exception:
	exception_without_err 3, exception_handler

overflow:
	exception_without_err 4, exception_handler

bounds_check:
	exception_without_err 5, exception_handler

inval_opcode:
	exception_without_err 6, exception_handler

copr_not_available:
	exception_without_err 7, exception_handler

double_fault:
	exception_with_err 8, exception_handler

copr_seg_overrun:
	exception_without_err 9, exception_handler

inval_tss:
	exception_with_err 10, exception_handler

segment_not_present:
	exception_with_err 11, exception_handler

stack_exception:
	exception_with_err 12, exception_handler

general_protection:
	exception_with_err 13, exception_handler

page_fault:
	exception_with_err 14, pagefault_handler
	
copr_error:
	exception_without_err 16, exception_handler

; 一堆符号导出，没别的
global restart
; 系统调用
global int_syscall
; 异常处理
global	divide_error
global	single_step_exception
global	nmi
global	breakpoint_exception
global	overflow
global	bounds_check
global	inval_opcode
global	copr_not_available
global	double_fault
global	copr_seg_overrun
global	inval_tss
global	segment_not_present
global	stack_exception
global	general_protection
global	page_fault
global	copr_error
; 外设中断
global	hwint00
global	hwint01
global	hwint02
global	hwint03
global	hwint04
global	hwint05
global	hwint06
global	hwint07
global	hwint08
global	hwint09
global	hwint10
global	hwint11
global	hwint12
global	hwint13
global	hwint14
global	hwint15