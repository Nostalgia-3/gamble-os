[bits 32]

org 0x1000

%macro print 2
    mov eax, 1
    mov ebx, 1
    mov ecx, %1
    mov edx, %2
    int 0x80
%endmacro

_start:

key_check:
    mov eax, 2          ; read(
    mov ebx, 0          ;   STDIN,
    mov ecx, buffer     ;   buffer,
    mov edx, buffer_len ;   sizeof(buffer)
    int 0x80            ; );

    cmp eax, 0          ; amount read
    je key_check
handle_keypress:
    cmp BYTE [buffer], 'a'
    je  print_a
    
    print hello, hello_len

    jmp key_check
print_a:
    print buffer, 1
    jmp key_check
exit:
    mov eax, 0          ; exit(
    mov ebx, 0          ;   0
    int 0x80            ; );

    jmp $

hello: db "hello", 10
hello_len equ $ - hello

buffer_len  equ 16
buffer:     times buffer_len db 0