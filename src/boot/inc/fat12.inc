; memory model
BaseOfLoader        equ 0x9000      ; phy base addr for loader
OffsetOfLoader      equ 0x0400      ; addr offset for loader
OffsetOfBuffer      equ 0x0000      ; addr offset of buffer for loader
BaseOfMBR           equ 0x0000      ; phy base addr for mbr
OffsetOfMBR         equ 0x0800      ; addr offset for mbr
OffsetOfPartTable   equ 446         ; offset of partion table in mbr

; fat12 constants
RootDirSectors          equ 14      ; total sectors in root dir
SectorNoOfRootDirectory equ 19      ; offset of root dir
SectorNoOfFAT1          equ 1       ; offset of fat1 entry
DeltaSectorNo           equ 31      ; offset of data sectors
BPB_BytsPerSec          equ 512     ; size of sector
BS_DrvNum               equ 80h     ; drive number

KERNEL_FILE     db  "KERNEL  BIN",0x20,0

load_kernel_file:
    pusha
    push   es
 .prepare:
    ; read current partion from mbr segment
    mov   ax, BaseOfMBR
    mov   es, ax
    mov   dl, [es:OffsetOfMBR+444]
    push  dx
    ; reset es seg
    mov   ax, BaseOfKernelFile
    mov   es, ax
    ; reset hard disk
    mov   ah, 0
    mov   dl, BS_DrvNum
    int   13h
    ; read mbr to buffer
    mov   ax, 0x0201
    mov   cx, 0x0001
    mov   dh, 0
    mov   dl, 0x80
    mov   bx, OffsetOfBuffer
    int   13h
    ; get start LBA
    pop   dx
    mov   si, OffsetOfBuffer+OffsetOfPartTable
    mov   ax, 16
    mul   dl
    add   si, ax
    mov   ax, [es:si+8]
    mov   [.start_lba], ax
    add   [.cur_sector], ax
%ifndef NDEBUG
    ; clear screen
    mov   ax, 0x0003
    int   10h
    ; print ""Booting  ""
    mov   dh, 0
    call  display_msg
%endif
 .find.entry:
    mov   bx, OffsetOfKernelFile
    jmp   .find.loop
 .start_lba:
    dw 0
 .cur_sector:
    dw SectorNoOfRootDirectory
 .left_secs:
    dw RootDirSectors
 .find.loop:
    mov   ax, [.cur_sector]
    mov   cx, 1
    call  read_sector
    mov   si, KERNEL_FILE
    mov   di, OffsetOfKernelFile
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
    inc   word [.cur_sector]
    dec   word [.left_secs]
    cmp   word [.left_secs], 0
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
    mov   bx, OffsetOfKernelFile
 .load.loop:
%ifndef NDEBUG
    call   put_dot
%endif
    mov   ax, dx
    add   ax, DeltaSectorNo
    add   ax, [.start_lba]
    mov   cx, 1
    call  read_sector
    mov   ax, dx
    mov   si, [.start_lba]
    call  get_next_cluster
    mov   dx, ax
    and   ax, 0xfff8
    cmp   ax, 0x0ff8
    je    .load.done
    add   bx, BPB_BytsPerSec
    jmp   .load.loop
 .load.done:
%ifndef NDEBUG
    mov   dh, 1
    call  display_msg
%endif
    pop   es
    popa
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

%ifndef NDEBUG
; void put_dot()
put_dot:
    pusha
    mov   ah, 0eh
    mov   al, '.'
    mov   bl, 0x0f
    int   10h
    popa
    ret
%endif

%ifndef NDEBUG
; void display_msg(msg_no: dh)
display_msg:
    push  bp
    mov   bp, sp
    pusha
    push  es
    mov   ax, 9
    mul   dh
    add   ax, MSG_BASE
    mov   bp, ax
    mov   ax, ds
    mov   es, ax
    mov   cx, 9
    mov   ax, 0x1301
    mov   bx, 0x0007
    mov   dl, 0
    int   10h
    pop   es
    popa
    pop   bp
    ret
MSG_BASE:
    db "Booting  "
    db "Ready.   "
    db "No KERNEL"
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
    mov   dl, BS_DrvNum
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
    dw BaseOfKernelFile
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
    mov   bx, BPB_BytsPerSec
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
