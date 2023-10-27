; Memory model
; [0x0000_0800~0x0000_0a00)     mbr
; [0x0000_0a00~0x0000_7c00)     stack
; [0x0000_7c00~0x0000_7e00)     boot
; [0x0009_0000~0x0009_0400)     buffer for loader
; [0x0009_0400~?)               loader

BOOT_Offset         equ     0x7c00

BS_OEMName          equ     (BOOT_Offset+0x03)
BPB_BytsPerSec      equ     (BOOT_Offset+0x0b)
BPB_SecPerClu       equ     (BOOT_Offset+0x0d)
BPB_RsvdSecCnt      equ     (BOOT_Offset+0x0e)
BPB_NumFATs         equ     (BOOT_Offset+0x10)
BPB_RootEntCnt      equ     (BOOT_Offset+0x11)
BPB_TotSec16        equ     (BOOT_Offset+0x13)
BPB_Media           equ     (BOOT_Offset+0x15)
BPB_FATSz16         equ     (BOOT_Offset+0x16)
BPB_SecPerTrk       equ     (BOOT_Offset+0x18)
BPB_NumHeads        equ     (BOOT_Offset+0x1a)
BPB_HiddSec         equ     (BOOT_Offset+0x1c)
BPB_TotSec32        equ     (BOOT_Offset+0x20)

BS_DrvNum           equ     (BOOT_Offset+36)
BS_Reserved1        equ     (BOOT_Offset+37)
BS_BootSig          equ     (BOOT_Offset+38)
BS_VolID            equ     (BOOT_Offset+39)
BS_VolLab           equ     (BOOT_Offset+43)
BS_FilSysType       equ     (BOOT_Offset+54)

struc PTEntry
    .status     resb    1
    .startCHS   resb    3
    .type       resb    1
    .endCHS     resb    3
    .startLBA   resb    4
    .numSectors resb    4
endstruc

BaseOfStack               equ 07c00h
BaseOfLoader              equ 09000h
OffsetOfLoader            equ 0x0400
BufferForLoader           equ 0x0000

RootDirSectors            equ 14
SectorNoOfRootDirectory   equ 19
SectorNoOfFAT1            equ 1
DeltaSectorNo             equ 31

    org  0x7c3e
boot_main:
    ; reset seg regs
    mov   ax, cs
    mov   ds, ax
    mov   ss, ax
    mov   ax, BaseOfLoader
    mov   es, ax
    mov   sp, BaseOfStack
    ; reset vga addr
    mov   ax, 0xb800
    mov	  gs, ax
    ; clear screen
    mov   ax, 0x0003
    int   10h
    ; print "BootFrom"
    mov   dh, 0
    call  display_msg
    ; print index of current active primary partion
    mov   al, dl                ; recv from mbr
    push  dx                    ; save partion index
    add   al, '0'
    mov   ah, 0x0f
    mov   [gs:(80*1+9)*2], ax
    ; reset hard disk
    mov   ah, 0
    mov   dl, [BS_DrvNum]
    int   13h
    ; read mbr to buffer
    mov   ax, 0x0201
    mov   cx, 0x0001
    mov   dl, [BS_DrvNum]
    mov   bx, BufferForLoader
    int   13h
    ; get start LBA
    mov   si, BufferForLoader+446
    pop   dx
    mov   ax, 16
    mul   dl
    add   si, ax
    mov   ax, [es:si+PTEntry.startLBA]
    mov   [StartLBA], ax
    ; find loader
    mov   ax, [RootDirSectorNow]
    add   ax, [StartLBA]
    mov   bx, OffsetOfLoader
 .find.loop:
    mov   cx, 1
    call  read_sector
    mov   si, LoaderFileName
    mov   di, OffsetOfLoader
    mov   dx, 10h
 .cmp.loop:
    call  strcmp
    cmp   ax, 1
    jz    .found
    dec   dx
    cmp   dx, 0
    jz    .next_sec
    add   di, 0x20
    jmp   .cmp.loop
 .next_sec:
    inc   word [RootDirSectorNow]
    dec   word [LeftRootDirSectors]
    cmp   word [LeftRootDirSectors], 0
    jz    .not_found
    jmp   .find.loop
 .not_found:
    mov   dh, 3
    call  display_msg
    jmp   $
 .found:
    add   di, 0x1a
    mov   dx, word [es:di]
    mov   bx, OffsetOfLoader
 .load.loop:
    mov   ax, dx
    add   ax, DeltaSectorNo
    add   ax, [StartLBA]
    mov   cx, 1
    call  read_sector
    mov   ax, dx
    call  get_next_cluster
    mov   dx, ax
    and   dx, 0xfff8
    cmp   dx, 0x0ff8
    jne   .load.done
    add   bx, [BPB_BytsPerSec]
    jmp   .load.loop
 .load.done:
    jmp   BaseOfLoader:OffsetOfLoader

; void display_msg(msg_no: dh)
display_msg:
    push  bp
    mov   bp, sp
    pusha
    push  es
    mov   ax, 8
    mul   dh
    add   ax, MSG_BASE
    mov   bp, ax
    mov   ax, ds
    mov   es, ax
    mov   cx, 8
    mov   ax, 0x1301
    mov   bx, 0x0007
    mov   dl, 0
    int   10h
    pop   es
    popa
    pop   bp
    ret

; bool strcmp(src: si, dst: di)
strcmp:
    push  bp
    mov   bp, sp
    push  si
    push  di
 .loop:
    mov   al, byte [ds:si]
    cmp   al, byte [es:di]
    jne   .ne
    test  al, al
    jz    .eq
    inc   si
    inc   di
    jmp   .loop
 .ne:
    mov   ax, 0
    jmp   .exit
 .eq:
    mov   ax, 1
 .exit:
    pop   di
    pop   si
    pop   bp
    ret

; void read_sector(sec_no: ax, total_secs: cx, es:bx: buf_ptr)
read_sector:
    push   bp
    mov    bp, sp
    pusha
    mov    si, BufferPacket
    mov    word [si+2], cx
    mov    word [si+4], bx
    mov    word [si+8], ax
    mov    dl, [BS_DrvNum]
    mov    ah, 42h
    int    13h
    jc     .fail
    popa
    pop    bp
    ret
 .fail:
    mov    dh, 2
    call   display_msg
    jmp    $

; int get_next_cluster(cluster_id: dx)
get_next_cluster:
    push  bp
    mov   bp, sp
    sub   sp, 2
    pusha
 .entry:
    mov   bx, 3
    mul   bx                    ; dx:ax = dx:ax * 3 / 2
    mov   bx, 2                 ; ax <- FAT entry offset
    div   bx                    ; dx <- odd flag
    mov   word [bp-2], dx
    mov   dx, 0                 ; dx:ax = dx:ax / BPB_BytsPerSec
    mov   bx, [BPB_BytsPerSec]  ; ax <- sector index
    div   bx                    ; dx <- addr offset
    add   ax, SectorNoOfFAT1
    mov   bx, BufferForLoader
    mov   cx, 2
    call  read_sector
    mov   bx, dx
    mov   ax, [bx]
    mov   bx, word [bp-2]
    test  bx, bx
    jz    .even_case
 .odd_case:
    shr   ax, 4                 ; cut extra byte
 .even_case:
 .done:
    and   ax, 0fffh             ; clear unused high-bits
    mov   word [bp-2], ax
    popa
    mov   ax, [bp-2]
    mov   sp, bp
    pop   bp
    ret

BufferPacket:
    dw 0x0010
    dw 0
    dw 0
    dw BaseOfLoader
    dd 0
    dd 0

; tmp var
LeftRootDirSectors: dw RootDirSectors
RootDirSectorNow:   dw SectorNoOfRootDirectory
StartLBA:           dw 0

; str literal
LoaderFileName:     db "LOADER  BIN",32,0

; msg pool
MSG_BASE:
MSG_INDEX_0:        db "BootFrom"
MSG_INDEX_1:        db "Ready.  "
MSG_INDEX_2:        db "ReadFail"
MSG_INDEX_3:        db "NoLoader"

; padding
times 448-($-$$) db 0
