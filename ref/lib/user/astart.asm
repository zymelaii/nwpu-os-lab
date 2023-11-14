[section .text]

extern main
global _start

_start:			; 第一条指令
	call	main
	jmp	$