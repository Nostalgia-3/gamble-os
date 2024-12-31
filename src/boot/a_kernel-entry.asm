[bits 32]
extern _start
sub esp, 8
push dx
push ecx
call _start
add esp, 8
jmp $