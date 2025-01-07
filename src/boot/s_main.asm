[bits 16]
[org 0x7C00]

start:
    cli
    mov ax, cs
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov bp, 0x7C00
    mov sp, bp

    push dx
load_second_stage:
    mov si, second_stage_dpack
    mov ah, 0x42
    int 0x13
    jc disk_error
    pop dx
    jmp 0x7E00
finish:
    jmp $

disk_error:
    mov ah, 0x0E
    mov al, 'd'
    int 0x10
    jmp $

second_stage_dpack:
    db 0x10     ; header
    db 0        ; header #2
    dw 2        ; blocks to read
    dw 0x7E00   ; write location
    dw 0        ; memory page
    dd 1        ; starting block
    dd 0        ; (only used for big reads)

times 510 - ($-$$) db 0
dw 0xAA55

;     mov [BOOT_DRIVE], dl
;     call do_e820
;     mov [NUM_MEMORY], bp
