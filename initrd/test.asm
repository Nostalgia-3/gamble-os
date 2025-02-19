[bits 32]
section .text

extern _start
_start:
    mov eax, 0
    mov ebx, 1
    int 0x80
    ret