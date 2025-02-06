[bits 32]

section .entry.text

extern _start
extern boot_setup_paging

global _load
_load:
    cli                     ; clear interrupts
    push eax
    push ebx

    mov ah, 'K'
    mov [0xB8000], ah
    mov ah, 'e'
    mov [0xB8002], ah
    
    lgdt [gdt_descriptor]   ; setup the GDT
    jmp CODE_SEG:_newgdt
_newgdt:
    call boot_setup_paging
    mov eax, kernel_page_dir
    mov cr3, eax
    mov eax, cr4
    or eax, 0x00000010
    mov cr4, eax
    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax

    call _start             ; jump to the kernel _start function
    jmp $                   ; and if it ever returns, halt

section .entry.data

extern kernel_page_dir
extern kernel_page_tables

align 4096
kernel_page_dir:
    times 1024 dd 0

align 4096
kernel_page_tables:
    times 64*1024 dd 0

section .entry.rodata

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

FLAGS       equ 0b11; | (1 << 16)
MAGIC       equ 0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

; multiboot header
section .multiboot
align 4
dd MAGIC
dd FLAGS
dd CHECKSUM
dd 0x01000000 ; header load location
dd 0x01000000 ; data load location
dd 0          ; data length (0 = the entire file)
dd 0          ; bss end (0 = none)
dd _load