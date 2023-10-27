; NOTE: some ops are combined to reduce the length of the mbr prog
; NOTE: a more detailed version can be found at commit 146b240

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
    mov   [BOOT_DRIVE], dl
    ; clear screen
    mov   ax, 0x0003
    int   10h
    ; print "Find partition"
    push  MSG_FOUND
    xor   dx, dx
    call  direct_write_str
 .visit_main_part.entry:
    ; read mbr to memory
    mov   ax, 0x0201
    mov   cx, 0x0001
    mov   dl, [BOOT_DRIVE]      ; boot drive -> dl
    mov   bx, MBR_OFFSET+512    ; [unsafe] use 0x0a00~??? as buffer
    int   13h
    ; move to partion table addr
    add   bx, PART_TABLE_OFFSET
    mov   si, bx
    mov   cl, 0
    mov   dx, 0x0300
    ; print table head
    push  MSG_TABLE_HEAD
    mov   dx, 0x0200
    call  direct_write_str
    inc   dh
 .visit_main_part.loop:
    ; print index
    xor   ch, ch
    push  cx
    push  0
    push  dx
    call  direct_write_u32
    add   dl, 3
    ; handle PTEntry.status
    mov   bl, byte [si]
    ; handle PTEntry.type
    mov   al, [si+PTEntry.type]
    xor   ah, ah
    push  ax
    push  0
    push  dx
    call  direct_write_u32
    add   dl, 5
    ; handle PTEntry.startLBA
    push  word [si+PTEntry.startLBA]
    push  0                             ; [unsafe] ignore hi-word
    push  dx
    call  direct_write_u32
    xor   dl, dl
    inc   dh
    ; handle first active partion
    cmp   byte [FIRST_ACTIVE_PART], -1  ; first parition not found
    jne   .visit_main_part.iter
    and   bl, 0x80                      ; is bootable
    test  bl, bl
    jz    .visit_main_part.iter
    cmp   al, PART_TYPE_EXT             ; is primary
    je    .visit_main_part.iter
    mov   [FIRST_ACTIVE_PART], cl       ; found
    mov   ax, [si+PTEntry.startLBA]     ; update start LBA
    mov   [PACKET+8], ax                ; [unsafe] ignore hi-word
 .visit_main_part.iter:
    add   si, 16
    inc   cl
    cmp   cl, TOTAL_PRI_PART
    jne   .visit_main_part.loop
 .visit_main_part.done:
    mov   dx, 0x0800
    push  MSG_BOOT_PART
    call  direct_write_str
    add   dl, al
    xor   ax, ax
    mov   al, [FIRST_ACTIVE_PART]
    push  ax
    push  0
    push  dx
    call  direct_write_u32
    add   sp, 2*2+6*3*4+2+6             ; clean-up str*3+u32*13
 .load:
    mov   si, PACKET
    mov   dl, [BOOT_DRIVE]
    mov   ah, 42h
    int   13h
    mov   dx, 0x0a00
    jc    .fail
 .done:
    push  MSG_HINT
    call  direct_write_str              ; [unsafe] ignore sp restore
    call  wait_for_key
    mov   dl, [FIRST_ACTIVE_PART]       ; pass primary partion no. to boot
    jmp   BOOT_SEGMENT:BOOT_OFFSET
 .fail:
    push   MSG_RDFAIL
    call   direct_write_str             ; [unsafe] ignore sp restore
    jmp    $

; i16 direct_write_u32(i16 pos, i16 hi_val, i16 lo_val)
; WARNING: hi_val:lo_val must le 0x9ffff
direct_write_u32:
    push  bp
    mov   bp, sp
    sub   sp, 10            ; reserve char[10]
    pusha
 .entry:
    mov   di, bp
    mov   cx, di
    mov   dx, [bp+6]
    mov   ax, [bp+8]
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
    mov   [bp-12], cx       ; rewrite ret ax
    mov   bx, 0x000f
    mov   dx, [bp+4]
    mov   bp, di
    int   10h
 .exit:
    popa
    mov   sp, bp
    pop   bp
    ret

; i16 direct_write_str(i16 str)
direct_write_str:
    push  bp
    mov   bp, sp
    sub   sp, 2
    pusha
.entry:
    mov   cx, 0
    mov   si, [bp+4]
.loop:
    mov   al, [si]
    test  al, al
    jz    .done
    inc   si
    inc   cx
    jmp   .loop
.done:
    mov   [bp-4], cx        ; rewrite ret ax
    mov   ah, 13h
    mov   bx, 0x0f
    mov   bp, [bp+4]
    int   10h
.exit:
    popa
    mov   sp, bp
    pop   bp
    ret

; void wait_for_key()
wait_for_key:
   mov ah, 0
   int 16h
   xor ah, ah
   ret

; save first active partion
FIRST_ACTIVE_PART:  db -1
BOOT_DRIVE:         db 0

; data address packet
PACKET:
    db 0x10                 ; size of packet
    db 0                    ; always zero
    dw 1                    ; total sectors
    dw BOOT_OFFSET          ; buffer offset
    dw BOOT_SEGMENT         ; buffer segment
    dd 0                    ; lower 32-bits of 48-bit starting LBA
    dd 0                    ; upper 32-bits of 48-bit starting LBA

; string literals
MSG_FOUND:          db "FindPart",0
MSG_NOT_FOUND:      db "NoPart",0
MSG_RDFAIL:         db "RdFail",0
MSG_TABLE_HEAD:     db "NO TYPE START_LBA",0
MSG_BOOT_PART:      db "1st ACTIVE PRIMARY PARTION: ",0
MSG_HINT:           db "PRESS TO CONTINUE",0

; padding
times 446-($-$$)    db 0    ; [  0,446) = Boot code
                            ; [446,510) = Partition Table
                            ; [510,512) = 0x55aa
