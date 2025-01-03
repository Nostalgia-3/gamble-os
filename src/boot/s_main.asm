[bits 16]
[org 0x7c00]

; KERNEL_OFFSET   equ 0x1000
KERNEL_OFFSET   equ 0x8000
SECS_TO_READ    equ 80

reset:
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov bp, 0x7C00
    mov sp, bp

    mov [BOOT_DRIVE], dl
    call do_e820
    mov [NUM_MEMORY], bp
load_kernel:
    mov si, DAPACK
    mov ax, 0x4200
    mov dl, 0x80
    int 0x13
    jc disk_error
set_vid_mode:
    ; set mode to 320x200
    mov ax, 13h
    ; mov ax, 0x12
    ; int 0x10
switch_to_32bit:
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:init_32bit
    jmp $

DAPACK:
    db 0x10
    db 0
blkcnt:
    dw SECS_TO_READ
db_add: ; 0:KERNEL_OFFSET
    dw KERNEL_OFFSET
    dw 0
d_lba:
    dd 1 ; read from
    dd 0

BOOT_DRIVE: db 0
NUM_MEMORY: dd 0

do_e820:
    mov di, 0x7E00          ; memory starting
    xor ebx, ebx            ; ebx must be 0 to start
    xor bp, bp              ; keep an entry count in bp
    mov edx, 0x0534D4150    ; Place "SMAP" into edx
    mov eax, 0xe820
    mov [es:di + 20], dword 1    ; force a valid ACPI 3.X entry
    mov ecx, 24             ; ask for 24 bytes
    int 0x15
    jc short .failed        ; carry set on first call means "unsupported function"
    mov edx, 0x0534D4150    ; Some BIOSes apparently trash this register?
    cmp eax, edx            ; on success, eax must have been reset to "SMAP"
    jne short .failed
    test ebx, ebx           ; ebx = 0 implies list is only 1 entry long (worthless)
    je short .failed
    jmp short .jmpin
.e820lp:
    mov eax, 0xe820         ; eax, ecx get trashed on every int 0x15 call
    mov [es:di + 20], dword 1    ; force a valid ACPI 3.X entry
    mov ecx, 24             ; ask for 24 bytes again
    int 0x15
    jc short .e820f         ; carry set means "end of list already reached"
    mov edx, 0x0534D4150    ; repair potentially trashed register
.jmpin:
    jcxz .skipent        ; skip any 0 length entries
    cmp cl, 20        ; got a 24 byte ACPI 3.X response?
    jbe short .notext
    test byte [es:di + 20], 1    ; if so: is the "ignore this data" bit clear?
    je short .skipent
.notext:
    mov ecx, [es:di + 8]    ; get lower uint32_t of memory region length
    or ecx, [es:di + 12]    ; "or" it with upper uint32_t to test for zero
    jz .skipent        ; if length uint64_t is 0, skip entry
    inc bp            ; got a good entry: ++count, move to next storage spot
    add di, 24
.skipent:
    test ebx, ebx        ; if ebx resets to 0, list is complete
    jne short .e820lp
.e820f:
    clc            ; there is "jc" on end of list to this point, so the carry must be cleared
    ret
.failed:
    stc
    ret
disk_error:
    mov ah, 0x0E
    mov al, 'd'
    int 0x10
sectors_error:
    mov ah, 0x0E
    mov al, 's'
    int 0x10
    jmp $

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

[bits 32]
init_32bit:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000
    mov esp, ebp
    
    mov dx, [BOOT_DRIVE]
    mov ecx, [NUM_MEMORY]

    call KERNEL_OFFSET
    jmp $

times 510 - ($-$$) db 0
dw 0xAA55