; the second stage has two main purposes:
;  1. be more verbose than the original bootloader
;  2. setup and run the kernel

KERNEL_OFF  equ 0x9000
KERNEL_LEN  equ 80

; this is where stuff the kernel cares about is stored
;  0x00: bios boot drive (1 byte)
;  0x01: selected video mode (1 byte)
;  0x02: memory segment count (2 bytes)
KERNEL_DATA equ 0x500
curbuf      equ 0x9000

[bits 16]
[org 0x7E00]

second_stage:
    mov [KERNEL_DATA], dl
select_video:
    mov si, hello
    call print
get_input: ; make this better at some point
    mov ah, 0x00
    int 16h
    cmp al, '1'
    je vga_text_mode
    cmp al, '2'
    je mode_13h
    cmp al, '3'
    je mode_12h

    jmp vga_text_mode ; default
mode_12h:
    mov ax, 12h
    mov bl, 2
preload:
    int 0x10
    mov [KERNEL_DATA+1], bl
load_kernel:
    ; call do_e820
    ; mov [KERNEL_DATA+2], bp

    ; Read the PVD in to memory. Bytes 156-189 contain the root directory entry.
    ; Load the root directory by reading the LBA and Length values in this root directory entry.
    ; Scan the directory entry identifiers for 'SYS'.
    ; If found, use the LBA and length values to load the 'SYS' directory in to memory.
    ; Scan the 'SYS' directory for 'KERNEL.BIN;1'. If found, you can now use the LBA value to load your file in to memory.

    xor eax, eax
    clc
    mov si, iso_boot_dapack
    mov ah, 0x42
    mov dl, 0x80 ; don't hardcode this
    int 0x13
    jc disk_error

    ; check version and vdv
    xor eax, eax
    mov al, [curbuf]
    add al, [curbuf+6]
    cmp al, 2
    jne load_kernel_err

    ; get the root directory
    mov si, curbuf+156
    ; "open" the directory by loading the contents
.load_root_dir:
    add si, 2               ; skip length and extra attribute record length
    mov eax, [si]           ; get the location of the internal directory
    shl eax, 2              ; multiply by four to get 512-byte sectors
    mov [iso_start], eax    ; put the starting address into the data pack
    xor eax, eax
    mov si, iso_boot_dapack
    mov ah, 0x42
    mov dl, 0x80            ; don't hardcode this
    int 0x13
    jc disk_error
    xor ax, ax
    mov si, curbuf
.search_for_sys:
    mov al, [si]            ; check length
    cmp al, 0               ; if zero...
    je .failed              ; then we couldn't find it
    add si, 31

    xor bx, bx
    mov bl, [si]            ; check name length
    add si, bx
    push bx
    and bl, 1
    cmp bl, 0
    jne .padding_bit
.check_name:
    pop bx
    cmp bl, 3               ; check if the name is 3 bytes long
    jne .next_entry

    mov si, yay
    call print
    jmp $

.padding_bit:
    add si, 1
    jmp .check_name
.next_entry:
    add si, 1
    jmp .search_for_sys

.failed:
    jmp $

boot_kernel:
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SEG:init_32bit
done:
    jmp $

disk_error:
    mov si, failed_disk
    call print
    jmp $

load_kernel_err:
    mov si, failed_loading_kernel
    call print
    jmp $

; do_e820:
;     mov di, 0x1000          ; memory starting
;     xor ebx, ebx            ; ebx must be 0 to start
;     xor bp, bp              ; keep an entry count in bp
;     mov edx, 0x0534D4150    ; Place "SMAP" into edx
;     mov eax, 0xe820
;     mov [es:di + 20], dword 1    ; force a valid ACPI 3.X entry
;     mov ecx, 24             ; ask for 24 bytes
;     int 0x15
;     jc short .failed        ; carry set on first call means "unsupported function"
;     mov edx, 0x0534D4150    ; Some BIOSes apparently trash this register?
;     cmp eax, edx            ; on success, eax must have been reset to "SMAP"
;     jne short .failed
;     test ebx, ebx           ; ebx = 0 implies list is only 1 entry long (worthless)
;     je short .failed
;     jmp short .jmpin
; .e820lp:
;     mov eax, 0xe820         ; eax, ecx get trashed on every int 0x15 call
;     mov [es:di + 20], dword 1    ; force a valid ACPI 3.X entry
;     mov ecx, 24             ; ask for 24 bytes again
;     int 0x15
;     jc short .e820f         ; carry set means "end of list already reached"
;     mov edx, 0x0534D4150    ; repair potentially trashed register
; .jmpin:
;     jcxz .skipent        ; skip any 0 length entries
;     cmp cl, 20        ; got a 24 byte ACPI 3.X response?
;     jbe short .notext
;     test byte [es:di + 20], 1    ; if so: is the "ignore this data" bit clear?
;     je short .skipent
; .notext:
;     mov ecx, [es:di + 8]    ; get lower uint32_t of memory region length
;     or ecx, [es:di + 12]    ; "or" it with upper uint32_t to test for zero
;     jz .skipent        ; if length uint64_t is 0, skip entry
;     inc bp            ; got a good entry: ++count, move to next storage spot
;     add di, 24
; .skipent:
;     test ebx, ebx        ; if ebx resets to 0, list is complete
;     jne short .e820lp
; .e820f:
;     clc            ; there is "jc" on end of list to this point, so the carry must be cleared
;     ret
; .failed:
;     stc
;     ret

vga_text_mode:
    mov ax, 03h
    mov bl, 0
    jmp preload

mode_13h:
    mov ax, 13h
    mov bl, 1
    jmp preload

; sets curdir to the directory passed
; params:
;   si = address of the start of the directory entry
; returns:
;   carry flag set if failed
; clobbers:
;   eax
parse_dir_entry:
    mov ax, [si] ; get len
    mov [curdir_len], ax
    mov eax, [si+2]
    mov [curdir_len], eax
    mov eax, [si+10]
    mov [curdir_dlen], eax
    mov al, [si+32]
    mov [curdir_strlen], al
    ret

not_found:
    mov si, failed_loading_kernel
    call print
    jmp $

; print a string
print:
    pusha
    mov ah, 0x0E ; teletype output
.loop:
    lodsb
    cmp al, 0
    je .finished
    mov bh, 0x0F
    int 10h
    jmp .loop
.finished:
    popa
    ret

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

    jmp KERNEL_OFF

hello: db "Video Modes", 13, 10, \
    " 1) 80x25 Text @ 16  Color (recommended, default)", 13, 10, \
    " 2) 320x200    @ 256 Color", 13, 10, \
    " 3) 640x480    @ 16  Color", 13, 10, 0

unknown:        db "Unknown command", 13, 10, 0
failed_disk:    db "Failed to load the kernel from the disk!", 13, 10, 0
failed_svga:    db "Failed to select an SVGA video mode!", 13, 10, 0
end_list_err:   db "List ended without finding a valid option!", 13, 10, 0
failed_setting: db "Failed setting the video mode!", 13, 10, 0
no_vbe:         db "Bios doesn't support VBE!", 13, 10, 0
failed_getting_vbe: db "Failed getting the VBE info block!", 13, 10, 0
failed_loading_kernel: db "Failed loading SYS/KERNEL.BIN!", 13, 10, 0
yay: db "Success", 13, 10, 0
sys_dir: db "SYS", 0

curdir:
curdir_len: db 0    ; the length in bytes
curdir_addr: dd 0   ; the LBA of the internal data
curdir_dlen: dd 0   ; the length of the internal data
curdir_strlen: db 0 ; the length of curdir_str
curdir_str: times 255 db 0; the directory/file name
ALIGN 4
iso_boot_dapack:
    dw 0x0010
iso_bytes:
    dw 4        ; read one logical block (2048 bytes -- I shouldn't hardcode this but whatever)
iso_addr:
    dd curbuf   ; store it at this address
iso_start:
    dq 64       ; with the starting block

kern_dapack:
    db 0x10
    db 0
blkcnt:
    dw KERNEL_LEN
db_add:
    dw KERNEL_OFF
    dw 0
d_lba:
    dd 5 ; read from (first + second = 5)
    dd 0

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

times 0x800 - ($-$$) db 0