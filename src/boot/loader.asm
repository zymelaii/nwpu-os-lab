; memory model
BaseOfLoader        equ 0x9000              ; seg base addr for loader
OffsetOfLoader      equ 0x0400              ; addr offset for loader
LoaderSegPhyAddr    equ BaseOfLoader<<4     ; phy base addr for loader
BaseOfKernelFile    equ 0x8000              ; seg base addr for kernel
OffsetOfKernelFile  equ 0x0400              ; addr offset for kernel
KernelSegPhyAddr    equ BaseOfKernelFile<<4 ; phy base addr for kernel
BaseOfStack         equ OffsetOfLoader      ; stack base for loader
KernelEntry         equ 0x30400             ; phy addr of kernel entry point

; NOTE: KernelEntry is processed at link time to ensure that the address corresponds correctly

; page model
PageDirBase equ 0x20_0000 ; [2M   ] base addr of page dir
PageTblBase equ 0x20_1000 ; [2M+4K] base addr of page table

[section .text.s16]
    align 16

[bits 16]

    global _start
_start:
    jmp   loader_main

%include   "./boot/inc/fat12.inc"
%include   "./boot/inc/pm.inc"

    align 4

; GDT table <- {(base addr, limit, attr)}
LABEL_GDT:          Descriptor 0, 0, 0                              ; empty descriptor
LABEL_DESC_FLAT_C:  Descriptor 0, 0fffffh, DA_CR|DA_32|DA_LIMIT_4K  ; 0~4G
LABEL_DESC_FLAT_RW: Descriptor 0, 0fffffh, DA_DRW|DA_32|DA_LIMIT_4K ; 0~4G
LABEL_DESC_VIDEO:   Descriptor 0B8000h, 0ffffh, DA_DRW|DA_DPL3      ; vga memory
; pointer to gdt
GdtPtr: dw  $-LABEL_GDT-1               ; limit of gdt
        dd  LoaderSegPhyAddr+LABEL_GDT  ; base addr of gdt

; gdt selector
SelectorFlatC   equ LABEL_DESC_FLAT_C-LABEL_GDT
SelectorFlatRW  equ LABEL_DESC_FLAT_RW-LABEL_GDT
SelectorVideo   equ LABEL_DESC_VIDEO-LABEL_GDT+SA_RPL3

    align 16

loader_main:
    mov   ax, cs
    mov   ds, ax
    mov   es, ax
    mov   ss, ax
    mov   sp, BaseOfStack
    ; clear screen
    mov   cx, 02000h
    mov   ah, 01h
    int   10h
    ; load kernel
    call  load_kernel_file
    ; prepare to entry protected mode
    ; 1. lgdt
    lgdt  [GdtPtr]
    ; 2. turn off ints
    cli
    ; 3. enable A20
    in    al, 92h
    or    al, 0b0000_0010
    out   92h, al
    ; 4. modify cr0
    mov   eax, cr0
    or    eax, 1
    mov   cr0, eax
    ; 5. jmp to the entry point
    jmp   dword SelectorFlatC:pm_main

[section .text]
    align 32

[bits 32]

extern load_kernel

pm_main:
    ; reset seg regs
    mov   ax, SelectorVideo
    mov   gs, ax
    mov   ax, SelectorFlatRW
    mov   ds, ax
    mov   es, ax
    mov   fs, ax
    mov   ss, ax
    mov   esp, STACK_TOP
    ; enable page table
    call  setup_paging
    ; transfer control to kernel
    jmp   SelectorFlatC:load_kernel

setup_paging:
    ; compute required num of page table and PDE
    ; NOTE: IDT is not setup yet, so assume that qemu provide memory of 128M at here
    mov   eax, 0x800_0000
    xor   edx, edx
    mov   ebx, 0x40_0000        ; 4M per page table
    div   ebx
    mov   ecx, eax              ; ecx <- num of page table, or PDE
    test  edx, edx
    jz    .done
    inc   ecx                   ; align to size of page table
 .done:
    push  ecx
    ; NOTE: maps addrs to phy addrs linearly and equivalently, regardless of memory holes for convenience
    ; init page dir
    mov   ax, SelectorFlatRW
    mov   es, ax
    mov   edi, PageDirBase
    xor   eax, eax
    mov   eax, PageTblBase|PG_P|PG_USU|PG_RWW
 .loop.1:
    stosd
    ; NOTE: make page tables contiguous in memory for convenience
    add   eax, 4096
    loop  .loop.1
 .init:
    ; init all page tables
    pop   eax
    mov   ebx, 1024          ; 1024 PTE per page table
    mul   ebx
    mov   ecx, eax           ; num of PTE = num of page table * 1024
    mov   edi, PageTblBase
    xor   eax, eax
    mov   eax, PG_P|PG_USU|PG_RWW
 .loop.2:
    stosd
    add   eax, 4096          ; 4K mem per page
    loop  .loop.2
 .complete:
    mov   eax, PageDirBase
    mov   cr3, eax
    mov   eax, cr0
    or    eax, 0x8000_0000
    mov   cr0, eax
    ret

[section .data]
    align 32

[bits 32]

times 4096 db 0
STACK_TOP: ; top of the stack for the loader
