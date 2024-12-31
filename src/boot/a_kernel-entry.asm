[bits 32]
extern _start
sub esp, 8
push DWORD ecx
push DWORD edx
call _start
add esp, 8
jmp $