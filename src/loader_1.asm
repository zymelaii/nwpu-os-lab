    org	 0400h

    mov	 ax, 0b800h
    mov	 gs, ax

    mov  di, (80*10+30)*2       ; row 10, column 30

    mov  ah, 0fh                ; fg white, bg black
    mov  si, .LC0               ; load str

.L0:
    mov  al, byte [es:si]
    test al, al
    jz   .L1
    mov  [gs:di], ax
    inc  si
    add  di, 2                  ; move to next column/char-pos
    jmp  .L0

.L1:
    jmp	 $

.LC0:
    db   "This is xqd's loader"
