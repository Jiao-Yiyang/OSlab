;nasm -f elf main.asm
;ld -m elf_i386 main.o -o main
;./main
SECTION .data
space db ' ',0h
num2: times 105 db 0h 
num1: times 105 db 0h
zero: db 48,0h
res1: times 105 db 0h;存储余数
temp: times 105 db 0h
count:db 0h
error_mes1: db 'ERROR:The first digit of the number is zero!!',0ah,0h;报错信息
error_mes2: db 'ERROR:The input is not a number!!',0ah,0h;报错信息
error_mes3: db 'ERROR:Please input two number seperated by space!!',0ah,0h;报错信息
error_mes4: db 'ERROR:The divisor cannot be zero!!',0ah,0h;报错信息
error_mes5: db 'ERROR:The number is too long!!',0ah,0h;报错信息
SECTION .bss
sinput:     resb    255   

SECTION .text
global _start
check_input:;输入检测
        push    edx			; 将EDX中的值保存于栈上
        push    ecx			; 将ECX中的值保存于栈上
        push    ebx			; 将EBX中的值保存于栈上
        push    eax			; 将EAX中的值保存于栈上
        mov eax,num1;检查第一个数字
        call check1
        cmp dh, 1
        jz error_mes11
        cmp dh, 2
        jz error_mes22
        cmp dh, 3
        jz error_mes33
        cmp dh, 5
        jz error_mes55
        mov edx, 0;
        mov eax,num2;检查第二个数字
        call check2
        cmp dh, 1
        jz error_mes11
        cmp dh, 2
        jz error_mes22
        cmp dh, 3
        jz error_mes33
        cmp dh, 4
        jz error_mes44
        cmp dh, 5
        jz error_mes55
        pop     eax
        pop     ebx			; 恢复原来EBX中的值
        pop     ecx			; 恢复原来ECX中的值
        pop     edx			; 恢复原来EDX中的值
        ret

check1:;检查除数指向数组是否为一个数字
        push eax
        push ebx
        push ecx
        mov bl, [eax]
        cmp bl, 48
        jz err1
        mov ecx,eax
        call slen
        cmp eax,100
        jg err5
        mov eax,ecx
        cmp byte [eax],0
        jz err3
        nextcharas:
        mov bl, [eax]
        cmp byte [eax],0
        jz finished21
        cmp bl,48
        jl err2
        cmp bl,57
        jg err2
        inc eax
        jmp nextcharas
        err3:
        mov dh,3
        jmp finished21
        err5:
        mov dh,5
        jmp finished21
        err2:
        mov dh,2
        jmp finished21
        err1:
        mov ecx,eax
        inc ecx
        mov cl,[ecx]
        cmp cl,0
        jz nextcharas
        mov dh,1
        jmp finished21
        finished21:
        pop ecx
        pop ebx
        pop eax
        ret

check2:;检查被除数指向数组是否为一个数字/区别为被除数不能为零
        push eax
        push ebx
        push ecx
        mov bl, [eax]
        cmp bl, 48
        jz err1se
        mov ecx,eax
        call slen
        cmp eax,100
        jg err5se
        mov eax,ecx
        cmp byte [eax],0
        jz err3se
        nextcharasse:
        mov bl, [eax]
        cmp byte [eax],0
        jz finished21se
        cmp bl,48
        jl err2se
        cmp bl,57
        jg err2se
        inc eax
        jmp nextcharasse
        err3se:
        mov dh,3
        jmp finished21se
        err5se:
        mov dh,5
        jmp finished21se
        err2se:
        mov dh,2
        jmp finished21se
        err1se:
        mov ecx,eax
        inc ecx
        mov cl,[ecx]
        cmp cl,0
        jz err4se
        mov dh,1
        jmp finished21se
        err4se:
        mov dh,4
        jmp finished21se
        finished21se:
        pop ecx
        pop ebx
        pop eax
        ret

error_mes:;储存报错信息响应
        error_mes11:;调用错误1
        mov eax,error_mes1
        call sprint
        jmp end1

        error_mes22:;调用错误2
        mov eax,error_mes2
        call sprint
        jmp end1

        error_mes33:;调用错误3
        mov eax,error_mes3
        call sprint
        jmp end1
        
        error_mes44:;调用错误4
        mov eax,error_mes4
        call sprint
        jmp end1
        ret

        error_mes55:;调用错误5
        mov eax,error_mes5
        call sprint
        jmp end1
        ret   

split:;将读入的分为两个数组
        push    ecx
        push    ebx			; 将EBX中的值保存于栈上
        push    eax	
        mov eax, sinput
        mov ebx, num1
        mov ecx,0
        nextchar1:      
        cmp byte [eax], 20h
        jz nextchar2pre
        cmp byte [eax], 0
        jz error_mes33
        mov cl ,byte [eax]
        mov byte [ebx], cl
        inc ebx
        inc eax
        jmp nextchar1
        nextchar2pre:
        mov ecx, 0
        mov ebx, num2
        inc eax
        nextchar2:
        cmp byte [eax],0ah
        jz  finished1
        mov cl ,byte [eax]
        mov byte [ebx], cl
        inc eax
        inc ebx
        jmp nextchar2
        finished1:
        pop     eax
        pop     ebx			; 恢复原来EBX中的值
        pop     ecx
        ret

sprint:;打印
        push    edx			; 将EDX中的值保存于栈上
        push    ecx			; 将ECX中的值保存于栈上
        push    ebx			; 将EBX中的值保存于栈上
        push    eax			; 将EAX中的值保存于栈上，即参数string
        call    slen		; 计算EAX中字符串长度，保存在EA
        mov     edx, eax	; 将长度移入到EDX
        pop     eax			; 恢复EAX值，即参数string
        push eax
        mov     ecx, eax	; 将待打印string移入ECX
        mov     ebx, 1		; 表示写入到标准输出STDOUT
        mov     eax, 4		; 调用SYS_WRITE（操作码是4）
        int     80h
        pop     eax 
        pop     ebx			; 恢复原来EBX中的值
        pop     ecx			; 恢复原来ECX中的值
        pop     edx			; 恢复原来EDX中的值
        ret

slen:;计算eax中数组的长度
        push ebx
        mov ebx,eax
        nextchar:
        cmp byte [eax],0
        jz finished
        inc eax
        jmp nextchar
        finished:
        sub eax,ebx
        pop ebx
        ret

print_res:;打印结果
        push eax
        push ebx
        call sprint
        mov eax, space
        call sprint
        mov  eax, ebx
        call sprint
        pop ebx
        pop eax
        ret

sub_big:;eax指向的数组减去ebx指向的数组
        push    edx			; 将EDX中的值保存于栈上
        push    ecx			; 将ECX中的值保存于栈上
        push    ebx			; 将EBX中的值保存于栈上
        push    eax	                ; 将EAX中的值保存于栈上
        call reverse_big;将两个字符串都倒转
        mov ecx,eax
        mov eax,ebx
        call reverse_big
        mov ebx,eax
        mov eax,ecx

        sub11:;相减
        mov dl, [ebx]
        mov dh, [eax]
        cmp dl,0;被减数读完结束循环
        jz finish1122
        sub dh,dl
        add dh,48
        mov [eax],dh
        cmp dh,48;比较当前位大小，不够需要借位
        jnl no_br;相减小于零 借位
        mov ch,0
        br:;借位
        mov dh,[eax]
        add dh,10
        mov [eax],dh
        inc eax
        inc ch
        mov dh,[eax]
        sub dh,1
        mov [eax],dh
        cmp dh,48
        jnl br2
        jmp br
        br2:
        cmp ch,0
        jz no_br
        dec eax
        dec ch
        jmp br2

        no_br:;不借位的情况
        inc eax
        inc ebx
        jmp sub11
        
        finish1122:
        pop     eax
        pop     ebx			; 恢复原来EBX中的值
        call reverse_big
        mov ecx,eax
        mov eax,ebx
        call reverse_big
        mov ebx,eax
        mov eax,ecx
        pop     ecx			; 恢复原来ECX中的值
        pop     edx			; 恢复原来EDX中的值
        call remove_zero
        ret

reverse_big:;将exa寄存器指向的数组反转
        push    edx			; 将EDX中的值保存于栈上
        push    ecx			; 将ECX中的值保存于栈上
        push    ebx			; 将EBX中的值保存于栈上
        push    eax			; 将EAX中的值保存于栈上
        mov ecx , 0 
        call slen;计算eax中字符串长度
        mov ebx , eax;将长度存在ebx中
        pop eax;eax为字符串地址
    
        put111: 
        cmp byte [eax+ecx],0
        jz finished111
        dec ebx
        mov dl,[eax+ebx]
        mov [temp+ecx],dl
        inc ecx
    
        jmp put111
        finished111:
        mov ecx,0;
        put222:
        cmp byte [eax+ecx], 0
        jz finished222
        mov dl,[temp+ecx]
        mov [eax+ecx],dl
        inc ecx
        jmp put222
        finished222:
        pop     ebx			; 恢复原来EBX中的值
        pop     ecx			; 恢复原来ECX中的值
        pop     edx			; 恢复原来EDX中的值
        ret

inc_big:;大数自加
        push    edx			; 将EDX中的值保存于栈上
        push    ecx			; 将ECX中的值保存于栈上
        push    ebx			; 将EBX中的值保存于栈上
        push    eax			; 将EAX中的值保存于栈上
        call    reverse_big
        mov     dl,[eax]
       
        add     dl, 1
        mov     [eax],dl
        mov     ecx,0
        jinwei:
        cmp     byte [eax+ecx],58;判定输入的字符是否是0-9
        jl      finished333
        mov     dl,[eax+ecx]
        sub     dl,10
        mov     [eax+ecx],dl
        inc     ecx 
        mov     dl,[eax+ecx]
        cmp     dl,0   
        jnz     continue111
        add     dl,48
        continue111:     
        add     dl,1
        mov     [eax+ecx],dl
        jmp     jinwei
        finished333:
        call    reverse_big 
        pop     eax
        pop     ebx			; 恢复原来EBX中的值
        pop     ecx			; 恢复原来ECX中的值
        pop     edx			; 恢复原来EDX中的值
        ret

compare_big:;将eax与ebx数组比较,存在dl中，1则是eax>ebx,0为eax<ebx
        push    ecx			; 将ECX中的值保存于栈上
        push    ebx			; 将EBX中的值保存于栈上
        push    eax			; 将EAX中的值保存于栈上
    
        call slen
        mov ecx, eax
        mov eax, ebx
        call slen
        mov edx, eax;lenb存在edx
        pop eax
        push eax
        cmp ecx,edx
        je compa
        jg abiger
        jl bbiger
            
        compa:
        ;call print_res
        mov dh, [ebx]
        cmp dh, 0
        jz equal111
        cmp [eax],dh
        jg abiger
        jl bbiger 
        inc eax
        inc ebx
        jmp compa

        equal111:
        ;mov eax,error_mes1
        ;call sprint
        mov dl,2
        jmp finish22222
        
        abiger:
        mov dl, 1
        jmp finish22222
        
        bbiger:
        mov dl, 0
        jmp finish22222

        finish22222:
        pop     eax
        pop     ebx			; 恢复原来EBX中的值
        pop     ecx			; 恢复原来ECX中的值
        ret

remove_zero:;去除前置零
        push    edx			; 将EDX中的值保存于栈上
        push    ecx			; 将ECX中的值保存于栈上
        push    ebx			; 将EBX中的值保存于栈上
        mov ecx, eax
        mov ebx,0
        mov dl,[eax]
        cmp dl,48;如果没有前导零直接返回
        jnz finish1222
        remove1:
        mov dl, [ecx]
        cmp dl, 48
        jnz putkkk
        inc ecx
        jmp remove1
        
        putkkk:
        mov dl,[ecx]
        cmp dl, 0
        jz beforefinish
        mov [eax+ebx],dl
        inc ecx
        inc ebx
        jmp putkkk
        
        beforefinish:
        mov dh,[eax+ebx]
        cmp dh,0
        jz finish1222
        mov dl,0
        mov [eax+ebx], dl
        inc ebx
        jmp beforefinish

        finish1222:
        pop     ebx			; 恢复原来EBX中的值
        pop     ecx			; 恢复原来ECX中的值
        pop     edx			; 恢复原来EDX中的值
        ret

mul_ten_big:;将ebx对应乘10
        push    edx			; 将EDX中的值保存于栈上
        push    ecx			; 将ECX中的值保存于栈上
        push    ebx			; 将EBX中的值保存于栈上
        push    eax			; 将EAX中的值保存于栈上
        mov eax,ebx
        call reverse_big
        mov ebx,eax
        call slen
        time20:
        cmp eax,0
        jz fini
        mov dl,[eax+ebx-1]
        mov [eax+ebx],dl
        dec eax
        jmp time20

        fini:
        mov dl,48
        mov [ebx],dl
        mov eax,ebx
        call reverse_big
        mov ebx , eax
        pop     eax
        pop     ebx			; 恢复原来EBX中的值
        pop     ecx			; 恢复原来ECX中的值
        pop     edx			; 恢复原来EDX中的值
        ret

div_ten_big:;将ebx对应除10
        push    edx			; 将EDX中的值保存于栈上
        push    ecx			; 将ECX中的值保存于栈上
        push    ebx			; 将EBX中的值保存于栈上
        push    eax			; 将EAX中的值保存于栈上
        mov eax,ebx
        call reverse_big
        mov ebx,eax
        call slen
        mov ecx,ebx
        div20:
        cmp eax,0
        jz fini1
        mov dl,[ebx+1]
        mov [ebx],dl
        dec eax
        inc ebx
        jmp div20

        fini1:
        mov ebx , ecx
        mov eax,ebx
        call reverse_big
        ;call sprint
        mov ebx , eax
        pop     eax
        pop     ebx			; 恢复原来EBX中的值
        pop     ecx			; 恢复原来ECX中的值
        pop     edx			; 恢复原来EDX中的值
        ret
 
div_big:
        push    edx			; 将EDX中的值保存于栈上
        push    ecx			; 将ECX中的值保存于栈上
        push    ebx			; 将EBX中的值保存于栈上
        push    eax			; 将EAX中的值保存于栈上
        ;先对齐
        mov dh,[count]
        count1: 
        call mul_ten_big
        mov dh,[count]
        inc dh
        mov [count],dh
        call compare_big
        cmp dl,0
        jz ok
        jmp count1
        
        ok:
        call div_ten_big
        mov dh,[count]
        dec dh
        mov [count],dh

        sub111:;先减，不够减除10，除不了10结束
        call compare_big
        cmp dl,0
        jz count2
        call sub_big
        mov ecx,eax
        mov eax,res1
        call inc_big
        mov eax,ecx
        jmp sub111

        count2:
        mov dh,[count]
        cmp dh,0
        jz finish0
        mov ecx,ebx
        mov ebx,res1
        call mul_ten_big;商*10
        mov ebx,ecx
        jmp ok

        finish0:
        ;call sprint
        mov ebx,eax
        mov eax,res1
        mov dh,[ebx]
        cmp dh,0
        jnz print11
        mov dh,48
        mov [ebx],dh
        print11:
        call    print_res
        pop     eax
        pop     ebx			; 恢复原来EBX中的值
        pop     ecx			; 恢复原来ECX中的值
        pop     edx			; 恢复原来EDX中的值
        ret

_start:;起始函数
        mov     edx, 255        ; 要读取的字符串字节数
        mov     ecx, sinput     ; 缓冲变量地址 
        mov     ebx, 0          ; write to the STDIN file
        mov     eax, 3          ; 调用SYS_READ 
        int     80h

        mov edx,0
        call split
        call check_input
        mov eax, num2
        mov ebx, num1
        call compare_big
        cmp dl, 1
        jnz num1_biger;若除数小于被除数，商为0，余数为除数
        mov eax,zero
        mov ebx,num1
        call print_res
        jmp end1

        num1_biger:
        mov eax,num1
        mov ebx,num2
        mov dl,48
        mov [res1],dl
        call div_big
        jmp end1

        end1:
        mov ebx,0
        mov eax,1
        int 80h
