    org	 0400h

; ==========================================================================
; 0x0000_0500~0x0000_7c00   [UNUSED] stack
; 0x0000_7c00~0x0000_7e00   [UNUSED] boot sector
; 0x0009_0000~0x0009_0400   [UNUSED] buffer reserved for get_next_cluster
; 0x0009_0400~              memory for loader code

section .data

; constant
BPB_BytsPerSec          dw 512      ; bytes per sector
BS_DrvNum               db 80h      ; driver id for int 13
RootDirSectors          equ 14      ; total sectors of root dir
SectorNoOfRootDirectory equ 19      ; first sector of root dir
SectorNoOfFAT1          equ 1       ; sector of first FAT item
DeltaSectorNo           equ 31      ; offset for data sector

; variable
BufferPacket            times 10h db 0              ; buffer reserved for `int 13h`
SectorBuffer            times 400h db 0             ; buffer reserved for sector
TotalCompleted          dw 0                        ; [DEBUG] total handled files

LoaderFileName          db "LOADER  BIN",32,0
TextFileName            db "AA1     TXT",32,0
; ==========================================================================

section .text

_start:
 .reset:
    mov   ax, cs                 ; reset ds,ss to code segment addr
    mov   ds, ax
    mov   ss, ax
    mov	  ax, 0b800h             ; reset gs to video memory segment addr
    mov	  gs, ax
 .entry:
    mov   di, (80*10+30)*2
    mov   ah, 0fh
    mov   bx, .LC0
    call  direct_write_str

    ; reset hard disk
    mov   ah, 0
    mov   dl, [BS_DrvNum]
    int   13h

    mov   si, LoaderFileName
    call  print_file_clusters

    mov   si, TextFileName
    call  print_file_clusters

    mov   si, TextFileName
    call  find_first_cluster

    add   ax, DeltaSectorNo     ; convert cluster index to sector index
    mov   bx, SectorBuffer
    mov   cx, 1
    call  read_sector

    mov   ah, 0x0f
    mov   bx, .LC2
    mov   di, (80*4+0)*2
    call  direct_write_str
    shl   ax, 1
    add   di, ax
    mov   ah, 0x0f
    mov   cx, byte [SectorBuffer+10]
    mov   byte [SectorBuffer+10], 0
    mov   bx, SectorBuffer
    call  direct_write_str

    mov   byte [SectorBuffer+10], cx

    jmp   $
 .LC0:
    db    "This is xqd's loader",0
 .LC1:
    db    "DONE",0
 .LC2:
    db    "FIRST 10 CHARS IN aA1.txt: ",0

direct_write_int:               ; int direct_write_int(number: bx)
    push  bp
    mov   bp, sp
    sub   sp, 2
    pusha
    mov   ch, ah
    mov   cl, 0
    mov   dx, bx
 .loop.1:                       ; compute length of digits
    mov   ax, bx
    mov   bl, 10
    div   bl
    mov   bl, al
    add   cl, 1
    test  bl, bl
    jnz   .loop.1
 .exit.1:
    mov   bx, dx
    mov   dl, cl
    mov   [bp-2], dx
    sub   cl, 1
    sal   cl, 1
    mov   dx, 0
    mov   dl, cl
    add   di, dx
 .loop.2:                       ; output
    mov   ax, bx
    mov   bl, 10
    div   bl
    mov   bl, al
    mov   al, ah
    add   al, '0'
    mov   ah, ch
    mov   [gs:di], ax
    sub   di, 2
    test  bl, bl
    jnz   .loop.2
 .exit.2:
    popa
    mov   ax, [bp-2]
    mov   sp, bp
    pop   bp
    ret

direct_write_str:               ; int direct_write_str(str: bx)
    push  bp
    mov   bp, sp
    sub   sp, 2
    pusha
    mov   [bp-2], di
 .loop:
    mov   al, byte [bx]
    test  al, al
    jz    .exit
    mov   [gs:di], ax
    inc   bx
    add   di, 2                 ; move to next column/char-pos
    jmp   .loop
 .exit:
    mov   bx, [bp-2]
    sub   di, bx
    sar   di, 1
    mov   [bp-2], di
    popa
    mov   ax, [bp-2]
    mov   sp, bp
    pop   bp
    ret

strcmp:                         ; bool strcmp(src: si, dst: di)
    push  bp
    mov   bp, sp
    push  si
    push  di
 .loop:
    mov   al, byte [si]
    cmp   al, byte [di]
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

panic:                          ; void panic()
    mov   ah, 06h
    mov   di, 0
    mov   bx, .LC0
    call  direct_write_str
    jmp   $
 .LC0:
    db    "PANIC",0

read_sector:                    ; void read_sector(first_sector: ax, total_sectors: cx, buf: bx)
    push  bp
    mov   bp, sp
    pusha
 .entry:
    ; load sector data into BufferPacket
    mov   si, BufferPacket      ; use ds:si
    mov   byte [si+0], 0x10     ; size of DAP
    mov   byte [si+1], 0x00     ; unused
    mov   word [si+2], cx       ; number of sectors to be read
    mov   word [si+4], bx       ; offset of memory buffer
    mov   word [si+6], ds       ; segment of memory buffer
    mov   word [si+8], ax       ; absolute number of the start of the sectors to be read (LBA)
    mov   dl, [BS_DrvNum]       ; driver index (1st HDD is 80h)
    mov   ah, 42h               ; extended read
    int   13h
 .exit:
    jc    .failed               ; cf = 1 -> failed
    popa
    pop   bp
    ret
 .failed:
    call  panic

write_sector:                    ; void write_sector(sector: ax, buf: bx)
    push  bp
    mov   bp, sp
    pusha
 .entry:
    mov   si, BufferPacket
    mov   al, 0
    mov   dl, [BS_DrvNum]
    mov   ah, 0x43              ; extended write

    mov   byte [si+0], 0x10     ; size of DAP
    mov   byte [si+1], 0x00     ; unused
    mov   word [si+2], cx       ; number of sectors to be read
    mov   word [si+4], bx       ; offset of memory buffer
    mov   word [si+6], ds       ; segment of memory buffer
    mov   word [si+8], ax       ; absolute number of the start of the sectors to be read (LBA)
    mov   dl, [BS_DrvNum]       ; driver index (1st HDD is 80h)
    mov   ah, 42h               ; extended read
    int   13h
 .exit:
    jc    .failed               ; cf = 1 -> failed
    popa
    pop   bp
    ret
 .failed:
    call  panic

find_first_cluster:             ; int find_first_cluster(file: si)
    push  bp
    mov   bp, sp
    sub   sp, 6
    pusha
 .reset:
    mov   word [bp-2], 19       ; current root dir sector index
    mov   word [bp-4], 14       ; left root dir sectors
 .find:
   ; read next sector
    mov   ax, [bp-2]
    mov   bx, SectorBuffer
    mov   cx, 1
    call  read_sector
 .cmp.entry:
    mov   di, SectorBuffer
    mov   dx, 10h               ; each sector has a maximum of 16 dir entries
 .cmp:
    call  strcmp
    cmp   ax, 1
    je    .found
    dec   dx
    test  dx, dx
    jz    .next_sector
    add   di, 20h               ; 32 bytes per dir entry
    jmp   .cmp
 .next_sector:
    inc   word [bp-2]
    dec   word [bp-4]
    cmp   word [bp-4], 0
    jz    .failed
    jmp   .find
 .failed:
    mov   word [bp-2], 0
    jmp   .done
 .found:
    add   di, 1ah               ; offset 0x1a to get the first cluster index
    mov   dx, word [di]         ; dx <- first cluster index
    mov   word [bp-2], dx
 .done:
    popa
    mov   ax, word [bp-2]
    mov   sp, bp
    pop   bp
    ret

print_file_clusters:            ; int print_file_clusters(file: si)
    push  bp
    mov   bp, sp
    sub   sp, 8
    pusha
 .reset:
    mov   word [bp-2], 0        ; total clusters
    mov   word [bp-4], 19       ; current root dir sector index
    mov   word [bp-6], 14       ; left root dir sectors
 .find:
    ; read next sector
    mov   ax, [bp-4]
    mov   bx, SectorBuffer
    mov   cx, 1
    call  read_sector
 .cmp.entry:
    mov   di, SectorBuffer
    mov   dx, 10h               ; each sector has a maximum of 16 dir entries
 .cmp:
    call  strcmp
    cmp   ax, 1
    je    .found
    dec   dx
    test  dx, dx
    jz    .next_sector
    add   di, 20h               ; 32 bytes per dir entry
    jmp   .cmp
 .next_sector:
    inc   word [bp-4]
    dec   word [bp-6]
    cmp   word [bp-6], 0
    jz    .failed
    jmp   .find
 .failed:
    ;;;
    pusha
    push  di
    mov   di, (80*2+0)*2
    mov   ax, [TotalCompleted]
    inc   word [TotalCompleted]
    mov   bx, 160
    mul   bx
    add   di, ax
    mov   ah, 06h
    mov   bx, .LC1
    call  direct_write_str
    inc   ax
    sal   ax, 1
    add   di, ax
    mov   ah, 06h
    mov   bx, si
    call  direct_write_str
    pop   di
    popa
    ;;;
    jmp   .done
 .found:
    ;;;
    pusha
    push  di
    mov   di, (80*2+0)*2
    mov   ax, [TotalCompleted]
    inc   word [TotalCompleted]
    mov   bx, 160
    mul   bx
    add   di, ax
    mov   ah, 06h
    mov   bx, .LC0
    call  direct_write_str
    inc   ax
    sal   ax, 1
    add   di, ax
    mov   ah, 06h
    mov   bx, si
    call  direct_write_str
    sal   ax, 1
    add   di, ax
    mov   [bp-8], di            ; start pos to display cluster index
    pop   di
    popa
    ;;;
    add   di, 1ah               ; offset 0x1a to get the first cluster index
    mov   dx, word [di]         ; dx <- first cluster index
 .tranverse:
 .display:
    mov   di, [bp-8]
    mov   ah, 08h
    mov   bx, dx
    call  direct_write_int
    inc   ax
    sal   ax, 1
    add   [bp-8], ax
 .tranverse.body:
    inc   word [bp-2]
    mov   ax, dx
    add   ax, DeltaSectorNo     ; offset cluster index with DeltaSectorNo to get the sector index
    mov   bx, SectorBuffer
    mov   cx, 1
    call  read_sector
    mov   ax, dx
    call  get_next_cluster
    mov   dx, ax
    mov   ax, dx
    and   ax, 0xfff8            ; cluster index should be of 0x0ff8~0x0fff
    cmp   ax, 0x0ff8
    je    .tranverse.done
    jmp   .tranverse
 .tranverse.done:
 .done:
    popa
    mov   ax, word [bp-2]
    mov   sp, bp
    pop   bp
    ret
 .LC0:
    db    "FOUND",0
 .LC1:
    db    "NOT FOUND",0

get_next_cluster:               ; int get_next_cluster(cluster_id: dx)
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
    mov   bx, SectorBuffer
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
