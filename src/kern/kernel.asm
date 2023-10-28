SELECTOR_KERNEL_CS equ 8

extern clear_screen
extern cstart
extern kprintf_test
extern cmatrix_start
extern busy_sleep

extern gdt_ptr

[section .bss]

resb 4096
STACK_TOP: ; top of the stack for the kernel

[section .text]

    global _start
_start:
    ; reset stack pointer
    mov	  esp, STACK_TOP

    ; update gdt
    sgdt  [gdt_ptr]
    call  cstart
    lgdt  [gdt_ptr]

    ; display "kernel" in the center of screen
    call  clear_screen
    mov   esi, .str.data
    mov   di, (80*(25/2)+(80-6)/2)*2
    mov   ah, 0x0f
    mov   cx, 6
    jmp   .write.loop
 .str.data:
    db "kernel"
 .write.loop:
    mov   al, [esi]
    inc   esi
    mov   [gs:di], ax
    add   di, 2
    loop  .write.loop

    ; run kprintf test
    call  busy_sleep
    call  clear_screen
    call  kprintf_test

    ; run matrix demo
    call  busy_sleep
    call  clear_screen
    call  cmatrix_start

    ; NOTE: use jmp to force the changes into effect
    jmp	  SELECTOR_KERNEL_CS:csinit

    global csinit
csinit:
    ; TODO: further work from the kernel...
    hlt
