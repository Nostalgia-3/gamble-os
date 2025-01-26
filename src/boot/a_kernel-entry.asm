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
    cli                     ; clear interrupts
    lgdt [gdt_descriptor]
    mov esp, stack_top      ; setup stack
    mov ebp, esp
    call _start             ; jump to the kernel _start function
    jmp $                   ; and if it ever returns, halt

gdt_start:
    dq 0x0
gdt_code:
    dw 0xFFFF       ; segment limit     (bits  0-15)
    dw 0x0          ; segment base      (bits  16-31)
    db 0x0          ; segment base      (bits  32-39)
    db 0b10011010   ; flags             (8 bits)
    db 0b11001111   ; flags             (4 bits) + segment length, bits 16-19
    db 0x0          ; segment base      (bits 24-31)
gdt_data:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 0b10010010
    db 0b11001111
    db 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start