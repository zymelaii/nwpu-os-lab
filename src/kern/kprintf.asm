[SECTION .text]

[BITS 32]

global kprintf
;===============================================
; void kprintf(u16 disp_pos, const char *format, ...)
; 参数说明：
; disp_pos: 开始打印的位置，0为0行0列，1为0行1列，80位1行0列
; format: 需要格式化输出的字符串，默认输出的字符颜色为黑底白字
; %c: 输出下一个参数的字符信息（保证参数范围在0~127），输出完打印的位置往下移动一位
; %b: 更改下一个输出的字符的背景色（保证参数范围在0~15）
; %f: 更改下一个输出的字符的前景色（保证参数范围在0~15）
; %s(提高内容): 参考inc/terminal.h，传进来的是一个结构体，结构体参数足够明确不复赘述，输出是独立的，输出完打印的位置不会往下移动一位
; 其余字符：按照字符输出（保证字符里不会有%，\n等奇奇怪怪的字符，都是常见字符，%后面必会跟上述三个参数之一），输出完打印的位置往下移动一位
kprintf:
	push  ebp
	mov   ebp, esp
	pusha
 .entry:
	; init cursor
	mov   ebx, 0
	mov   bx, [ebp+8]
	shl   bx, 1
	; init format str
	mov   esi, [ebp+12]
	mov   edi, ebp
	; init va-args cursor
	add   edi, 16
 .loop:
	; check terminator
	mov   cl, [esi]
	test  cl, cl
	jz    .done
	; check format char
	mov   ch, cl
	cmp   cl, '%'
	jne   .output
	; dispatch to formatter
	inc   esi
	mov   cl, [esi]
	cmp   cl, 'c'
	je    .fmt.char
	cmp   cl, 'b'
	je    .fmt.bg
	cmp   cl, 'f'
	je    .fmt.fg
	cmp   cl, 's'
	je    .fmt.struct
	; ignore unknown fmt flag
	jmp   .iter
 .fmt.char:
	mov   ch, [edi]
 	add   edi, 4
	jmp   .fmt.done
 .fmt.bg:
	mov   dl, [edi]
	shl   dl, 4
	and   ah, 0x0f
	or    ah, dl
	add   edi, 4
	jmp   .iter
 .fmt.fg:
	mov   dl, [edi]
	and   ah, 0xf0
	or    ah, dl
	add   edi, 4
	jmp   .iter
 .fmt.struct:
	; ignore
	jmp   .fmt.done
 .fmt.done:
 .output:
	mov   al, ch
	mov   [gs:ebx], ax
	add   ebx, 2
 .iter:
	inc   esi
	jmp   .loop
 .done:
	popa
	mov   esp, ebp
	pop   ebp
	ret
