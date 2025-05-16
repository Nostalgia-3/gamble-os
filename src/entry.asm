[bits 32]

section .multiboot.data

FLAGS       equ 0b11 | (1 << 2); | (1 << 16)
MAGIC       equ 0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

align 4
dd MAGIC
dd FLAGS
dd CHECKSUM
dd 0x01000000   ; header load location
dd 0x01000000   ; data load location
dd 0            ; data length (0 = the entire file)
dd 0            ; bss end (0 = none)
dd _mb_load     ; entry
dd 0            ; video type            (text mode)
dd 0            ; width of framebuffer  (no preference)
dd 0            ; height of framebuffer (no preference)
dd 32           ; depth of framebuffer  (32 bpp)

section .bss

stack_bottom:
    resb 131072
stack_top:
    align 4096
kernel_page_dir:
    resb 4096
boot_page_table1:
    resb 4096
data_page_table:
    resb 4096

section .multiboot.text

extern _start
extern _kernel_start
extern _kernel_end

global kernel_page_dir
global data_page_table
global boot_page_table1
global _mb_load

_mb_load:
    cli

    ; store information regarding multiboot for
    ; the kernel _start() method
    mov [0x500], eax
    mov [0x504], ebx

    ; eax = number of pages (max 1024)
    ; ebx = starting address (page-aligned)
    ; edx = page table address
    mov eax, 1023
    mov ebx, 0
    mov edx, boot_page_table1 - 0xC0000000
    call map_page_table

    mov eax, 1024
    mov ebx, 0x400000
    mov edx, data_page_table - 0xC0000000
    call map_page_table

    ; set 0xC03FF000 to 0x000B8000 (vga 80x25 text memory)
    mov edx, 0x000B8003
    mov [boot_page_table1 - 0xC0000000 + 1023 * 4], edx

    ; identity map the kernel
    mov DWORD [kernel_page_dir - 0xC0000000], boot_page_table1 - 0xC0000000 + 0x03
    ; and put it at 0xC0000000
    mov DWORD [kernel_page_dir - 0xC0000000 + 768*4], boot_page_table1 - 0xC0000000 + 0x03
    ; and put the data page table immediately after
    mov DWORD [kernel_page_dir - 0xC0000000 + 769*4], data_page_table - 0xC0000000 + 0x03

    ; set the page directory pointer
    mov ecx, kernel_page_dir - 0xC0000000
    mov cr3, ecx

    ; enable paging
    mov ecx, cr0
    or  ecx, 0x80000001
    mov cr0, ecx

    ; jump to the higher half page 
    lea ecx, enter_kernel
    jmp ecx

; Maps up to 1024 pages from the starting address to the page table address
; eax = number of pages (max 1024)
; ebx = starting address (page-aligned)
; edx = page table address
map_page_table:
    mov ecx, ebx
    or ecx, 3
    mov [edx], ecx
    add ebx, 4096
    add edx, 4
    dec eax
    cmp eax, 0
    jne map_page_table
    ret

section .text

enter_kernel:
    ; setup stack and
    mov esp, stack_top

    ; multiboot information
    mov eax, [0x500]
    push eax
    mov ebx, [0x504]
    push ebx

    ; remove identity paging
    mov ecx, 0
    mov [kernel_page_dir], ecx

    ; flush tlb
    mov ecx, cr3
    mov cr3, ecx

; setup the gdt
    lgdt [gdt_descriptor]
    jmp CODE_SEG:.post_gdt
.post_gdt:

    call _start
.done:
    cli
    hlt
    jmp .done

section .data

global gdt_start

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
gdt_code_ring3:
    dw 0xFFFF       ; segment limit     (bits  0-15)
    dw 0x0          ; segment base      (bits  16-31)
    db 0x0          ; segment base      (bits  32-39)
    db 0b11111011   ; flags             (8 bits)
    db 0b11001111   ; flags             (4 bits) + segment length, bits 16-19
    db 0x0          ; segment base      (bits 24-31)
gdt_data_ring3:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 0b11110010
    db 0b11001111
    db 0x0
gdt_tss:
    dq 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG    equ gdt_code - gdt_start
DATA_SEG    equ gdt_data - gdt_start