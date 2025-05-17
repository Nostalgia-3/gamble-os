[bits 32]

section .text.start

global _start
_start:
    call main
    call exit

section .text

extern main

global exit
exit:
    mov eax, 0
    pop ebx
    int 0x80

global write
write:
    push    ebp
    mov     ebp, esp
    push    ebx

    mov     eax, 1
    mov     ebx, [ebp + 8]
    mov     ecx, [ebp + 12]
    mov     edx, [ebp + 16]
    int     0x80

    pop     ebx
    pop     ebp
    ret

global read
read:
    push    ebp
    mov     ebp, esp
    push    ebx

    mov     eax, 2
    mov     ebx, [ebp + 8]
    mov     ecx, [ebp + 12]
    mov     edx, [ebp + 16]
    int     0x80

    pop     ebx
    pop     ebp
    ret

global open
open:
    push    ebp
    mov     ebp, esp
    push    ebx

    mov     eax, 0x03
    mov     ebx, [ebp + 8]
    int     0x80

    pop     ebx
    pop     ebp
    ret

global close
close:
    push    ebp
    mov     ebp, esp
    push    ebx

    mov     eax, 0x04
    mov     ebx, [ebp + 8]
    int     0x80

    pop     ebx
    pop     ebp
    ret

global getdents
getdents:
    push    ebp
    mov     ebp, esp
    push    ebx

    mov     eax, 0x08
    mov     ebx, [ebp + 8]
    mov     ecx, [ebp + 12]
    mov     edx, [ebp + 16]
    int     0x80

    pop     ebx
    pop     ebp
    ret


global stat
stat:
    push    ebp
    mov     ebp, esp
    push    ebx

    mov     eax, 0x09
    mov     ebx, [ebp + 8]
    mov     ecx, [ebp + 12]
    int     0x80

    pop     ebx
    pop     ebp
    ret

global brk
brk:
    push    ebp
    mov     ebp, esp
    push    ebx

    mov     eax, 0x0A
    mov     ebx, [ebp + 8]
    int     0x80

    pop     ebx
    pop     ebp
    ret

global ioctl
ioctl:
    push    ebp
    mov     ebp, esp
    push    ebx

    mov     eax, 0x0B
    mov     ebx, [ebp + 8]
    mov     ecx, [ebp + 12]
    mov     edx, [ebp + 16]
    int     0x80

    pop     ebx
    pop     ebp
    ret