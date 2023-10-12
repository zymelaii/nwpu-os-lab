
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               load.inc
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

BaseOfLoader              equ  09000h ; LOADER.BIN 被加载到的位置 ----  段地址
OffsetOfLoader            equ   0400h ; LOADER.BIN 被加载到的位置 ---- 偏移地址
LoaderSegPhyAddr          equ  90000h ; LOADER.BIN 被加载到的位置 ---- 物理地址 (= Base * 10h)

BaseOfKernelFile          equ  08000h ; KERNEL.BIN 被加载到的位置 ----  段地址
OffsetOfKernelFile        equ   0400h ; KERNEL.BIN 被加载到的位置 ---- 偏移地址
KernelSegPhyAddr          equ  80000h ; KERNEL.BIN 被加载到的位置 ---- 偏移地址 (= Base * 10h)

; 注意：1、必须与 MAKEFILE 中参数 -Ttext 的值相等!!
;      2、这是个地址而非仅仅是个偏移，如果 -Ttext 的值为 0x400400，则它的值也应该是 0x400400。
KernelEntryPointPhyAddr   equ 030400h

BaseOfStack               equ OffsetOfLoader

PageDirBase               equ 200000h ; 页目录开始地址:       2M
PageTblBase               equ 201000h ; 页表开始地址:         2M + 4K