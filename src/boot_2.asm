    org    0x7c00
    mov    ax, cs
    mov    ds, ax
    mov    es, ax
    call   ClearScreen   ; ClearScreen()
    mov    ax, .LC0
    mov    cx, 4h
    mov    bx, 1fh
    mov    dx, 1326h
    call   DisplayString ; DisplayString("NWPU", 4, 0x1f, 19, 38)
    mov    ax, 0a0ah
    call   MoveCursorTo  ; MoveCursorTo(10, 10)
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
    db     "NWPU"
    times  510-($-$$) db 0
    dw     0xaa55

