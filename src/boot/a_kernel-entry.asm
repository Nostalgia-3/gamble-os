[bits 32]
extern _start
push DWORD ecx
push DWORD edx
call _start
; add esp, 8
jmp $