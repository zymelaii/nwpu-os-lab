    org    0x7c00
    mov    ax, cs
    mov    ds, ax
    mov    es, ax
    call   ClearScreen
    mov    ax, .LC0
    mov    cx, 0eh
    mov    bx, 0fh
    mov    dx, 0000h
    call   DisplayString
    mov    ax, .LC1
    mov    cx, 19h
    mov    bx, 00f0h
    mov    dx, 0100h
    call   DisplayString
    mov    ax, 0200h
    call   MoveCursorTo
    jmp    $
ClearScreen:
    mov    ax, 0003h
    int    10h
    ret
DisplayString:
    mov    bp, ax
    mov    ax, 1301h
    mov    bh, 00h
    int    10h
    ret
MoveCursorTo:
    mov    dx, ax
    mov    ah, 02h
    int    10h
    ret
.LC0:
    db     "boot from USB."
.LC1:
    db     "This is xqd's bootloader!"
    times  510-($-$$) db 0
    dw     0xaa55

