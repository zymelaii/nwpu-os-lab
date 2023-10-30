
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               loader.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

BaseOfLoader              equ  01000h ; LOADER.BIN 被加载到的位置 ----  段地址
OffsetOfLoader            equ  05000h ; LOADER.BIN 被加载到的位置 ---- 偏移地址
LoaderSegPhyAddr          equ  10000h ; LOADER.BIN 被加载到的位置 ---- 物理地址 (= Base * 10h)

BaseOfStack               equ OffsetOfLoader

PageDirBase               equ 100000h ; 页目录开始地址:       1M
PageTblBase               equ 101000h ; 页表开始地址:         1M + 4K

[SECTION .text.s16]
ALIGN 16
[BITS 16]

global _start
_start:
    jmp    Main ; Start

%include   "./boot/inc/pm.inc"

ALIGN  4
; GDT ------------------------------------------------------------------------------------------------------------------------------------------------------------
;                                           段基址                段界限                                       属性
LABEL_GDT:              Descriptor             0,                    0, 0                                       ; 空描述符
LABEL_DESC_FLAT_C:      Descriptor             0,              0fffffh, DA_CR  | DA_32 | DA_LIMIT_4K            ; 0 ~ 4G
LABEL_DESC_FLAT_RW:     Descriptor             0,              0fffffh, DA_DRW | DA_32 | DA_LIMIT_4K            ; 0 ~ 4G
LABEL_DESC_VIDEO:       Descriptor       0B8000h,               0ffffh, DA_DRW                       | DA_DPL3  ; 显存首地址

; 准备lgdt必要的数据结构 --------------------------------------------------------------------------------------------------------------------------------------------
GdtLen              equ $ - LABEL_GDT
GdtPtr              dw  GdtLen - 1                    ; gdt界限
                    dd  LoaderSegPhyAddr + LABEL_GDT  ; gdt基地址

; GDT 选择子 ----------------------------------------------------------------------------------
SelectorFlatC       equ LABEL_DESC_FLAT_C   - LABEL_GDT
SelectorFlatRW      equ LABEL_DESC_FLAT_RW  - LABEL_GDT
SelectorVideo       equ LABEL_DESC_VIDEO    - LABEL_GDT + SA_RPL3

Main:            ; <--- 从这里开始 *************
    mov    ax, cs
    mov    ds, ax
    mov    es, ax
    mov    ss, ax
    mov    sp, BaseOfStack

    mov    cx, 02000h
    mov    ah, 01h
    int    10h

; 下面准备跳入保护模式 -------------------------------------------

; 加载 GDTR
    lgdt   [GdtPtr]

; 关中断
    cli

; 打开地址线A20
    in     al, 92h
    or     al, 00000010b
    out    92h, al

; 准备切换到保护模式
    mov    eax, cr0
    or     eax, 1
    mov    cr0, eax

; 真正进入保护模式
    jmp    dword SelectorFlatC:LABEL_PM_START

; 从此以后的代码在保护模式下执行 ----------------------------------------------------
; 32 位代码段. 由实模式跳入 ---------------------------------------------------------
[SECTION .text]
ALIGN    32
[BITS    32]

extern load_kernel

LABEL_PM_START:
    mov    ax, SelectorVideo
    mov    gs, ax
    mov    ax, SelectorFlatRW
    mov    ds, ax
    mov    es, ax
    mov    fs, ax
    mov    ss, ax
    mov    esp, TopOfStack

    call   SetupPaging; 启动分页机制，不过在这个实验大家可以不用管，就当是一个能够扩展访存范围的牛逼玩意
    jmp    SelectorFlatC:load_kernel

    ;***************************************************************
    ; 内存看上去是这样的：
    ;              ┃                                    ┃
    ;              ┃                 .                  ┃
    ;              ┃                 .                  ┃
    ;              ┃                 .                  ┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃■■■■■■■■■■■■■■■■■■┃
    ;              ┃■■■■■■Page  Tables■■■■■■┃
    ;              ┃■■■■■(大小由LOADER决定)■■■■┃
    ;    00101000h ┃■■■■■■■■■■■■■■■■■■┃ PageTblBase
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃■■■■■■■■■■■■■■■■■■┃
    ;    00100000h ┃■■■■Page Directory Table■■■■┃ PageDirBase  <- 1M
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃□□□□□□□□□□□□□□□□□□┃
    ;       F0000h ┃□□□□□□□System ROM□□□□□□┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃□□□□□□□□□□□□□□□□□□┃
    ;       E0000h ┃□□□□Expansion of system ROM □□┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃□□□□□□□□□□□□□□□□□□┃
    ;       C0000h ┃□□□Reserved for ROM expansion□□┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃□□□□□□□□□□□□□□□□□□┃ B8000h ← gs
    ;       A0000h ┃□□□Display adapter reserved□□□┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃□□□□□□□□□□□□□□□□□□┃
    ;       9FC00h ┃□□extended BIOS data area (EBDA)□┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃■■■■■■■■■■■■■■■■■■┃
    ;       90000h ┃■■■■■■■LOADER.BIN■■■■■■┃ somewhere in LOADER ← esp
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃■■■■■■■■■■■■■■■■■■┃
    ;       80000h ┃■■■■■■■KERNEL.BIN■■■■■■┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃■■■■■■■■■■■■■■■■■■┃
    ;       30000h ┃■■■■■■■■KERNEL■■■■■■■┃ 30400h ← KERNEL 入口 (KernelEntryPointPhyAddr)
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃                                    ┃
    ;        7E00h ┃              F  R  E  E            ┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃■■■■■■■■■■■■■■■■■■┃
    ;        7C00h ┃■■■■■■BOOT  SECTOR■■■■■■┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃                                    ┃
    ;         500h ┃              F  R  E  E            ┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃□□□□□□□□□□□□□□□□□□┃
    ;         400h ┃□□□□ROM BIOS parameter area □□┃
    ;              ┣━━━━━━━━━━━━━━━━━━┫
    ;              ┃◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇┃
    ;           0h ┃◇◇◇◇◇◇Int  Vectors◇◇◇◇◇◇┃
    ;              ┗━━━━━━━━━━━━━━━━━━┛ ← cs, ds, es, fs, ss
    ;
    ;
    ;        ┏━━━┓        ┏━━━┓
    ;        ┃■■■┃ 我们使用     ┃□□□┃ 不能使用的内存
    ;        ┗━━━┛        ┗━━━┛
    ;        ┏━━━┓        ┏━━━┓
    ;        ┃      ┃ 未使用空间    ┃◇◇◇┃ 可以覆盖的内存
    ;        ┗━━━┛        ┗━━━┛
    ;
    ; 注：KERNEL 的位置实际上是很灵活的，可以通过同时改变 LOAD.INC 中的 KernelEntryPointPhyAddr 和 MAKEFILE 中参数 -Ttext 的值来改变。
    ;     比如，如果把 KernelEntryPointPhyAddr 和 -Ttext 的值都改为 0x400400，则 KERNEL 就会被加载到内存 0x400000(4M) 处，入口在 0x400400。
    ;

; 启动分页机制 --------------------------------------------------------------
SetupPaging:
    ; 根据内存大小计算应初始化多少PDE以及多少页表
    xor    edx, edx
    mov    eax, 8000000h      ; qemu虚拟机默认内存128MB，这里简化实现省去了实模式代码里中断向BIOS询问可用内存大小的步骤
    mov    ebx, 400000h       ; 400000h = 4M = 4096 * 1024, 一个页表对应的内存大小
    div    ebx
    mov    ecx, eax           ; 此时 ecx 为页表的个数，也即 PDE 应该的个数
    test   edx, edx
    jz     .no_remainder
    inc    ecx                ; 如果余数不为 0 就需增加一个页表
.no_remainder:
    push   ecx                ; 暂存页表个数

    ; 为简化处理, 所有线性地址对应相等的物理地址. 并且不考虑内存空洞.

    ; 首先初始化页目录
    mov    ax, SelectorFlatRW
    mov    es, ax
    mov    edi, PageDirBase   ; 此段首地址为 PageDirBase
    xor    eax, eax
    mov    eax, PageTblBase | PG_P  | PG_USU | PG_RWW
.1:
    stosd
    add    eax, 4096          ; 为了简化, 所有页表在内存中是连续的.
    loop   .1

    ; 再初始化所有页表
    pop    eax                ; 页表个数
    mov    ebx, 1024          ; 每个页表 1024 个 PTE
    mul    ebx
    mov    ecx, eax           ; PTE个数 = 页表个数 * 1024
    mov    edi, PageTblBase   ; 此段首地址为 PageTblBase
    xor    eax, eax
    mov    eax, PG_P  | PG_USU | PG_RWW
.2:
    stosd
    add    eax, 4096          ; 每一页指向 4K 的空间
    loop   .2

    mov    eax, PageDirBase
    mov    cr3, eax
    mov    eax, cr0
    or     eax, 80000000h
    mov    cr0, eax
    jmp    short .3
.3:
    nop

    ret
; 分页机制启动完毕 ----------------------------------------------------------


; SECTION .data 之开始 ---------------------------------------------------------------------------------------------
[SECTION .data]
ALIGN    32
[BITS    32]
; 堆栈就在数据段的末尾
StackSpace:    times    1000h    db    0
TopOfStack     equ      $    ; 栈顶
