[bits 32]

org 0x1000

_start:
    mov eax, 1          ; write(
    mov ebx, 1          ;   STDOUT,
    mov ecx, hello      ;   "Hello, world!\n",
    mov edx, hello_len  ;   hello_len
    int 0x80            ; );

    mov eax, 0          ; exit(
    mov ebx, 0          ;   0
    int 0x80            ; );

    jmp $

hello: db "Hello, world!", 10
hello_len equ $ - hello