        org     10000h

        mov     ax,     cs
        mov     ds,     ax
        mov     es,     ax
        mov     ax,     0x00
        mov     ss,     ax
        mov     sp,     0x7c00

        mov     ax,     1301h
        mov     bx,     0047h
        mov     dx,     0810h        
        mov     cx,     13
        push    ax
        mov     ax,     ds
        mov     es,     ax
        pop     ax
        mov     bp,    Loadmessage
        int     10h

        jmp     $

   Loadmessage:     db     "Hello Loader!"