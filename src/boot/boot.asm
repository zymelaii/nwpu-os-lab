BaseOfBoot      equ     0x7c00

BS_OEMName      equ     (BaseOfBoot+0x03)
BPB_BytsPerSec  equ     (BaseOfBoot+0x0b)
BPB_SecPerClu   equ     (BaseOfBoot+0x0d)
BPB_RsvdSecCnt  equ     (BaseOfBoot+0x0e)
BPB_NumFATs     equ     (BaseOfBoot+0x10)
BPB_RootEntCnt  equ     (BaseOfBoot+0x11)
BPB_TotSec16    equ     (BaseOfBoot+0x13)
BPB_Media       equ     (BaseOfBoot+0x15)
BPB_FATSz16     equ     (BaseOfBoot+0x16)
BPB_SecPerTrk   equ     (BaseOfBoot+0x18)
BPB_NumHeads    equ     (BaseOfBoot+0x1a)
BPB_HiddSec     equ     (BaseOfBoot+0x1c)
BPB_TotSec32    equ     (BaseOfBoot+0x20)

BS_DrvNum       equ     (BaseOfBoot+36)
BS_Reserved1    equ     (BaseOfBoot+37)
BS_BootSig      equ     (BaseOfBoot+38)
BS_VolID        equ     (BaseOfBoot+39)
BS_VolLab       equ     (BaseOfBoot+43)
BS_FilSysType   equ     (BaseOfBoot+54)

; memory model
;
; [0x0000_0800~0x0000_0a00)     mbr
; [0x0000_0a00~0x0000_7c00)     stack
; [0x0000_7c00~0x0000_7e00)     boot
; [0x0009_0000~0x0009_0400)     buffer for loader
; [0x0009_0400~?)               loader
;
BaseOfStack         equ BaseOfBoot  ; stack base for loader
BaseOfLoader        equ 0x9000      ; phy base addr for loader
OffsetOfLoader      equ 0x0400      ; addr offset for loader
OffsetOfBuffer      equ 0x0000      ; addr offset of buffer for loader
BaseOfMBR           equ 0x0000      ; phy base addr for mbr
OffsetOfMBR         equ 0x0800      ; addr offset for mbr
OffsetOfPartTable   equ 446         ; offset of partion table in mbr
BaseOfVGA           equ 0xb800      ; base addr of vga memory

; fat12 constants
RootDirSectors            equ 14    ; total sectors in root dir
SectorNoOfRootDirectory   equ 19    ; offset of root dir
SectorNoOfFAT1            equ 1     ; offset of fat1 entry
DeltaSectorNo             equ 31    ; offset of data sectors

struc PTEntry
    .status     resb    1
    .startCHS   resb    3
    .type       resb    1
    .endCHS     resb    3
    .startLBA   resb    4
    .numSectors resb    4
endstruc

    org  0x7c3e
_entry:
boot_main:
    ; reset seg regs
    mov   ax, cs
    mov   ds, ax
    mov   ss, ax
    mov   sp, BaseOfStack
    mov   bp, sp
    ; read current partion from mbr segment
    ; NOTE: reset sp -> read partion index -> reset es
    mov   dl, [es:OffsetOfMBR+444]
    push  dx
    mov   ax, BaseOfLoader
    mov   es, ax
%ifndef NDEBUG
    ; reset vga addr
    mov   ax, BaseOfVGA
    mov	 gs, ax
    ; print index of current active primary partion
    mov   ax, [bp-2]                    ; get partion index
    add   al, '0'
    mov   ah, 0x0f
    mov   [gs:(80*1+0)*2], ax
    ; clear screen
    mov   ax, 0x0003
    int   10h
    ; print "BootFrom"
    mov   dh, 0
    call  display_msg
%endif
    ; reset hard disk
    mov   ah, 0
    mov   dl, [BS_DrvNum]
    int   13h
    ; read mbr to buffer
    mov   ax, 0x0201
    mov   cx, 0x0001
    mov   dh, 0
    mov   dl, [BS_DrvNum]
    mov   bx, OffsetOfBuffer
    int   13h
    ; get start LBA
    pop   dx
    mov   si, OffsetOfBuffer+OffsetOfPartTable
    mov   ax, 16
    mul   dl
    add   si, ax
    mov   ax, [es:si+PTEntry.startLBA]
 .find.entry:
    mov   bx, OffsetOfLoader
    push  ax                            ; [bp-2] start lba
    add   ax, SectorNoOfRootDirectory
    push  ax                            ; [bp-4] current sector
    push  RootDirSectors                ; [bp-6] left sectors
 .find.loop:
    mov   ax, [bp-4]
    mov   cx, 1
    call  read_sector
    mov   si, LOADER_FILE
    mov   di, OffsetOfLoader
    mov   dx, 10h
 .cmp.loop:
    call  strcmp
    cmp   ax, 1
    je    .found
    dec   dx
    test  dx, dx
    jz    .next_sec
    add   di, 0x20
    jmp   .cmp.loop
 .next_sec:
    inc   word [bp-4]
    dec   word [bp-6]
    cmp   word [bp-6], 0
    je    .not_found
    jmp   .find.loop
 .not_found:
%ifndef NDEBUG
    mov   dh, 3
    call  display_msg
%endif
    jmp   $
 .found:
    add   di, 0x1a
    mov   dx, [es:di]
    mov   bx, OffsetOfLoader
 .load.loop:
    mov   ax, dx
    add   ax, DeltaSectorNo
    add   ax, [bp-2]
    mov   cx, 1
    call  read_sector
    mov   ax, dx
    mov   si, [bp-2]
    call  get_next_cluster
    mov   dx, ax
    and   ax, 0xfff8
    cmp   ax, 0x0ff8
    je    .load.done
    add   bx, [BPB_BytsPerSec]
    jmp   .load.loop
 .load.done:
    add   sp, 6
    jmp   BaseOfLoader:OffsetOfLoader

%ifndef NDEBUG
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
MSG_BASE:
    db "BootFrom"
    db "Ready.  "
    db "ReadFail"
    db "NoLoader"
%endif

; void read_sector(sec_no: ax, total_secs: cx, es:bx: buf_ptr)
read_sector:
    push  bp
    mov   bp, sp
    pusha
    mov   si, .packet
    mov   [si+2], cx
    mov   [si+4], bx
    mov   [si+8], ax
    mov   dl, [BS_DrvNum]
    mov   ah, 42h
    int   13h
    jc    .fail
    popa
    pop   bp
    ret
 .fail:
%ifndef NDEBUG
    mov   dh, 2
    call  display_msg
%endif
    jmp   $
 .packet:                       ; prefill struct to reduce total instrs
    dw 0x0010
    dw 0
    dw 0
    dw BaseOfLoader
    dd 0
    dd 0

; int get_next_cluster(cluster_id: ax, start_lba: si)
get_next_cluster:
    push  bp
    mov   bp, sp
    pusha
 .entry:
    ; dx:ax = dx:ax * 3 / 2
    ; {dx, ax} <- {odd flag, FAT entry offset}
    xor   dx, dx
    mov   cx, 3
    mul   cx
    mov   cx, 2
    div   cx
    mov   di, dx                ; save odd flag
    ; dx:ax = dx:ax / BPB_BytsPerSec
    ; {dx, ax} <- {addr offset, sector index}
    xor   dx, dx
    mov   bx, [BPB_BytsPerSec]
    div   bx
    add   ax, SectorNoOfFAT1
    add   ax, si
    mov   bx, OffsetOfBuffer
    call  read_sector           ; NOTE: here cx is 2
    mov   bx, dx
    mov   ax, [es:bx]           ; NOTE: here es is OffsetOfBuffer
    test  di, di
    jz    .even_case
 .odd_case:
    shr   ax, 4                 ; cut extra byte
 .even_case:
 .done:
    and   ax, 0xfff             ; clear unused high-bits
    mov   [bp-2], ax
    popa
    mov   sp, bp
    pop   bp
    ret

; bool strcmp(src: si, dst: di)
strcmp:
    push  bp
    mov   bp, sp
    pusha
    xor   ax, ax
 .loop:
    mov   bl, [ds:si]
    cmp   bl, [es:di]
    jne   .done
    test  bl, bl
    jz    .eq
    inc   si
    inc   di
    jmp   .loop
 .eq:
    mov   ax, 1
 .done:
    mov   word [bp-2], ax
    popa
    pop   bp
    ret

; str literal pool
LOADER_FILE:        db "LOADER  BIN",0x20,0

; padding
times 448-($-$$)    db 0
