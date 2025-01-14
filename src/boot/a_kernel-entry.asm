[bits 32]

FLAGS       equ 0b11
MAGIC       equ 0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

; multiboot header
section .multiboot
align 4
dd MAGIC
dd FLAGS
dd CHECKSUM

section .bss
align 16
stack_bottom:
resb 16384
stack_top:

section .text

extern _start

global _load
_load:
    mov esp, stack_top
    cli
    jmp _start