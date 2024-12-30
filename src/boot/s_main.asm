[bits 16]
[org 0x7c00]

KERNEL_OFFSET equ 0x1000

reset:
    push ax
    mov ax, 0
    mov ds, ax
    pop ax
    mov bp, 0x9000
    mov sp, bp

    mov [BOOT_DRIVE], dl

    mov bx, dx
    and bx, 0xFF
    call PrintDecimal

    call load_kernel
    call switch_to_32bit

    jmp $

partition_offset db 0
BOOT_DRIVE db 0

load_kernel:
    mov bx, KERNEL_OFFSET
    mov dh, 25 ; allocate ~12.5KiB
    mov dl, 0x80 ; [BOOT_DRIVE]
    call disk_load
    ret

disk_load:
    pusha
    push dx

    mov ah, 0x02    ; read(
    mov al, dh      ;   dh,   // # of sectors
    mov cl, 0x02    ;   0x02, // starting sector
    mov ch, 0x00    ;   0x00, // cylinder #
    mov dh, 0x00    ;   0x00  // head #
                    ;   dl    // drive #
    int 0x13        ; );
    jc disk_error

    pop dx          ; get original # of sectors to read
    cmp al, dh      ; check if # of sectors read == to the number of sectors
                    ; supposed to read
    jne sectors_error
    popa
    ret

disk_error:
    mov ah, 0x0e    ; tty mode
    add al, 'D'     ; converts to ASCII
    int 0x10        ; prints the remainder
    jmp disk_loop

sectors_error:
    ; output the number of sectors read
    push ax
    push bx
    and ax, 0xFF
    mov bx, ax
    call PrintDecimal
    pop ax

    jmp disk_loop

disk_loop:
    jmp $

PrintDecimal:
    mov dx, 0
    mov ax, bx
    mov bx, 10000
    div bx
    
    mov ah, 0
    call printDigit 
    mov bx, dx
    
    mov dx, 0
    mov ax, bx
    mov bx, 1000
    div bx
    
    mov ah, 0
    call printDigit 
    mov bx, dx
    
    
    mov dx, 0
    mov ax, bx
    mov bx, 100
    div bx
    
    mov ah, 0
    call printDigit 
    mov bx, dx
    
    mov dx, 0
    mov ax, bx
    mov bx, 10
    div bx
    mov ah, 0
    call printDigit 	
    
    mov ax, dx
    mov ah, 0
    call printDigit  
    

    ret

printDigit:
    mov ah, 0x0e 	; tty mode
    add al, 48 	; converts to ASCII
    int 0x10 	; prints the remainder
    ret

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

[bits 16]
switch_to_32bit:
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:init_32bit

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

    call BEGIN_32BIT

[bits 32]
BEGIN_32BIT:
    call KERNEL_OFFSET
    jmp $

times 510 - ($-$$) db 0
dw 0xAA55
