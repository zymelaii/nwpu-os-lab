; Struct for Partition Table Entry
struc PTEntry
    .status     resb    1 ; bit 7 set = active
    .startCHS   resb    3
    .type       resb    1
    .endCHS     resb    3
    .startLBA   resb    4
    .numSectors resb    4
endstruc

PTOffset        equ 446
OSBOOTSegment   equ 0000h
OSBOOTOffset    equ 7c00h
MBROffset       equ 0800h
PART_TYPE_EXT   equ 5

org MBROffset

START:
    cli ; turn off interrupt for safety, though in most cases this is trivial
    xor     ax, ax
    mov     ds, ax
    mov     es, ax
    mov     ss, ax
    mov     sp, 7c00h
    ; copy self to another space, since the boot expects it to be at 0x7c00 where we are now
    mov     cx, 256
    mov     si, 7c00h
    mov     di, MBROffset
    rep movsw ; copy cx words from [si] to [di]
    jmp     0:REAL_START ; mbr is copied, use an explicit jump to switch the IP(Instruction Pointer)
REAL_START:
    sti ; turn on interrupt, or BIOS INTs cannot work
    ; Save boot drive number passed from BIOS
    mov     [BOOT_DRIVE_NUMBER], dl

    ; Clear screen
    mov     ax, 0600h
    mov     bx, 0700h
    mov     cx, 0       ; LeftTop(row, col) = (0, 0)
    mov     dx, 184fh   ; RightBottom(row, col) = (24, 79)
    int     10h
    ; Reset Cursor to (0, 0)
    mov     ax, 0200h
    mov     bx, 0
    mov     dx, 0000h
    int     10h
    ; Print "Find partition"
    mov    ax, 01301h                       ; AH = 13,  AL = 01h
    mov    bx, 000Fh                        ; 页号为0(BH = 0) 黑底白字(BL = 0Bh,高亮)
    mov    cx, MSG_FindingEnd-MSG_Finding   ; CX = 串长度
    xor    dx, dx                           ; DX = (起始行|列)
    mov    bp, MSG_Finding                  ; ES:BP = 串地址
    int    10h                              ; 10h 号中断
; TODO1.3&1.4: Add/Modify code below for parsing Partition Table & finding the first active primary partition
    mov     cx, MSG_NotFoundEnd-MSG_NotFound
    mov     bp, MSG_NotFound
    jmp     EXIT_FAIL
LOAD_OSBOOT:
; TODO1.4: You need to set INT13H's DAP to read the boot program into memory
    int     13h
    mov     cx, MSG_ReadFailureEnd-MSG_ReadFailure
    mov     bp, MSG_ReadFailure
    jc      EXIT_FAIL
; TODO1.4: You need to ensure that `DS:SI` points to the Partition Table Entry associated 
;       with the boot you are going to load at this point
    jmp     OSBOOTSegment:OSBOOTOffset

EXIT_FAIL:
    mov    ax, 01301h
    mov    bx, 000ch
    mov    dx, 0100h
    int    10h
    jmp $

; Data Address Packet
DAPS:   
    DB 0x10                     ; size of packet 
    DB 0                        ; Always 0
    D_CL    DW 1                ; number of sectors to transfer
    D_BX    DW OSBOOTOffset     ; transfer buffer (16 bit segment:16 bit offset) 
    D_ES    DW OSBOOTSegment    
    LBA_Lo  DD 1                ; lower 32-bits of 48-bit starting LBA
    LBA_Hi  DD 0                ; upper 32-bits of 48-bit starting LBAs

; -----------------
BOOT_DRIVE_NUMBER   db 00h

; -----------------
MSG_Finding:        db "FindPart"
MSG_FindingEnd:
MSG_NotFound:       db "NoPart"
MSG_NotFoundEnd:
MSG_ReadFailure:    db "RdFail"
MSG_ReadFailureEnd:
; ----------------- Padding ----------------
times 446 -($-$$) db 0  ; MBR[0:445] = Boot code
                        ; MBR[446:509] = Partition Table
                        ; MBR[510:511] = 0x55aa
