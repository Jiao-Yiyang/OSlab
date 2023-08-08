;nasm boot.asm -o boot.bin
;dd if=boot.bin of=a.img bs=512 count=1 conv=notrunc
;bochs –f bochsrc

org 07c00h  ;告诉编译器程序加载到07c00h处
mov ax, cs
mov ds, ax
mov es, ax
call DispStr ;调用显示字符串的例程
jmp $ ;$ 表示当前行被汇编后的地址。

DispStr:
 mov ax, BootMessage ;把BootMessage的首地址传给寄存器ax
 mov bp, ax ;ES:BP = 串地址
 mov cx, 10 ;CX = 串长度
 mov ax, 01301h ; AH = 13, AL = 01h
 mov bx, 000ch ;
 mov dl, 0
 int 10h
 ret

 BootMessage: db "Hello, OS!"
 ;$$表示表示一个节的开始处被汇编后的地址。
 ;$-$$可能会被经常用到，它表示本行离程序开始处的相对距离
 times 510 - ($-$$) db 0  ;填充剩下空间， 使生成的二进制代码恰好为512字节
 dw 0xaa55