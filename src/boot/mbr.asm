[section .data]

struc PTEntry
    .status     resb    1
    .startCHS   resb    3
    .type       resb    1
    .endCHS     resb    3
    .startLBA   resb    4
    .numSectors resb    4
endstruc

PART_TABLE_OFFSET   equ 446
MBR_SEGMENT         equ 0x0000
BOOT_SEGMENT        equ 0x0000
BOOT_OFFSET         equ 0x7c00
MBR_OFFSET          equ 0x0800
PART_TYPE_EXT       equ 5
TOTAL_PRI_PART      equ 4

[section .text]

    org MBR_OFFSET
_entry:
    ; turn off ints
    cli
    ; reset
    xor   ax, ax
    mov   ds, ax
    mov   es, ax
    mov   ss, ax
    mov   sp, BOOT_OFFSET
    ; copy self to boot addr, 512B in total
    mov   cx, 256
    mov   si, BOOT_OFFSET
    mov   di, MBR_OFFSET
    rep movsw
    jmp   MBR_SEGMENT:mbr_main

mbr_main:
    ; turn on ints
    sti
    ; save boot drive index
    mov [BOOT_DRIVE], dl
    ; clear screen
    mov   ax, 0x0600
    mov   bx, 0x0700
    mov   cx, 0
    mov   dx, 0x184f
    int   10h
    ; move cursor to (0,0)
    mov   ax, 0x0200
    mov   bx, 0
    mov   dx, 0
    int   10h
    ; print "Find partition"
    push  MSG_FOUND
    push  0x0f
    push  0
    call  direct_write_str
    add   sp, 6
 .visit_main_part.entry:
    ; read mbr to memory
    mov   ah, 02h
    mov   al, 1                 ; total sectors
    mov   ch, 0                 ; cylinder 0
    mov   dh, 0                 ; head 0
    mov   cl, 1                 ; sector 1
    xor   dx, dx
    mov   dl, [BOOT_DRIVE]      ; boot drive -> dl
    mov   bx, MBR_OFFSET+512    ; use 0x0a00~??? as buffer
    int   13h
    ; move to partion table addr
    add   bx, PART_TABLE_OFFSET
    mov   si, bx
    mov   cl, 0
    mov   dx, 0x0300
    ; print table head
    push  MSG_TABLE_HEAD
    push  0x02
    push  0x0200
    call  direct_write_str
    add   sp, 6
 .visit_main_part.loop:
    ; print index
    xor   ch, ch
    push  cx
    push  0
    push  0x0e
    push  dx
    call  direct_write_u32
    add   sp, 8
    add   dl, 3
    ; handle PTEntry.status
    mov   bl, byte [si]
    xor   bh, bh
    push  bx
    push  0
    push  0x0f
    push  dx
    call  direct_write_u32
    add   sp, 8
    add   dl, 7
    ; handle first active partion
    cmp   byte [FIRST_ACTIVE_PART], -1  ; first parition not found
    jne   .skip
    and   bl, 0x80                      ; is bootable
    test  bl, bl
    jz    .skip
    mov   bl, [si+PTEntry.type]         ; is primary
    cmp   bl, PART_TYPE_EXT
    je    .skip
    mov   [FIRST_ACTIVE_PART], cl       ; found
 .skip:
    ; handle PTEntry.startLBA
    mov   ax, word [si+PTEntry.startLBA]
    push  ax
    mov   ax, word [si+PTEntry.startLBA+2]
    push  ax
    push  0x0f
    push  dx
    call  direct_write_u32
    add   sp, 8
    xor   dl, dl
    inc   dh
 .visit_main_part.iter:
    add   si, 16
    inc   cl
    cmp   cl, TOTAL_PRI_PART
    jne   .visit_main_part.loop
 .visit_main_part.done:
    mov   dx, 0x0800
    push  MSG_BOOT_PART
    push  0x0f
    push  dx
    call  direct_write_str
    add   sp, 6
    add   dl, al
    xor   ax, ax
    mov   al, [FIRST_ACTIVE_PART]
    push  ax
    push  0
    push  0x0f
    push  dx
    call  direct_write_u32
    add   sp, 8
 .load:
    ; TODO: read boot to memory
    jmp   $
    jc    .fail

 .done:
    jmp   BOOT_SEGMENT:BOOT_OFFSET

 .fail:
    jmp    $

; i16 direct_write_u32(i16 pos, i16 style, i16 hi_val, i16 lo_val)
; WARNING: hi_val:lo_val must le 0x9ffff
direct_write_u32:
    push  bp
    mov   bp, sp
    sub   sp, 12
    pusha
 .entry:
    mov   di, bp
    sub   di, 2
    mov   cx, di
    mov   dx, [bp+8]
    mov   ax, [bp+10]
    mov   bx, 10
 .loop:
    div   bx
    dec   di
    add   dl, '0'
    mov   [di], dl
    xor   dx, dx
    test  ax, ax
    jnz   .loop
 .done:
    mov   ah, 13h
    sub   cx, di
    mov   [bp-2], cx
    mov   bh, 0
    mov   bl, [bp+6]
    mov   dx, [bp+4]
    mov   bp, di
    int   10h
 .exit:
    popa
    mov   ax, [bp-2]
    mov   sp, bp
    pop   bp
    ret

; i16 direct_write_str(i16 pos, i16 style, i16 str)
direct_write_str:
    push  bp
    mov   bp, sp
    sub   sp, 2
    pusha
 .entry:
    mov   cx, 0
    mov   si, [bp+8]
 .loop:
    mov   al, [si]
    test  al, al
    jz    .done
    inc   si
    inc   cx
    jmp   .loop
 .done:
    mov   [bp-2], cx
    mov   ah, 13h
    mov   bh, 0
    mov   bl, [bp+6]
    mov   dx, [bp+4]
    mov   bp, [bp+8]
    int   10h
 .exit:
    popa
    mov   ax, [bp-2]
    mov   sp, bp
    pop   bp
    ret

; save first active partion
FIRST_ACTIVE_PART:  db -1
BOOT_DRIVE:         db 0

; string literals
MSG_FOUND:          db "FindPart",0
MSG_NOT_FOUND:      db "NoPart",0
MSG_RDFAIL:         db "RdFail",0
MSG_TABLE_HEAD:     db "NO STATUS START_LBA",0
MSG_BOOT_PART:      db "FIRST BOOTABLE PRIMARY PARTION: ",0

; padding
times 446-($-$$)    db 0    ; [  0,446) = Boot code
                            ; [446,510) = Partition Table
                            ; [510,512) = 0x55aa
