[section .text]

extern main
extern exit
global _start

_start:			; 第一条指令
	call	main
	push	eax
	call	exit