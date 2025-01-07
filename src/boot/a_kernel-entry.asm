[bits 32]
extern _start

mov eax, _start
cmp eax, 0
jne boot
jmp $

boot:
    call _start
done:
    jmp $