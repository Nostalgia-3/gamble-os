global read_cr2
global read_cr3
global get_kernel_size
global swap_page_dir

extern _kernel_end
extern _kernel_start

read_cr2:
    mov eax, cr2
    ret

read_cr3:
    mov eax, cr3
    ret

get_kernel_size:
    mov eax, _kernel_end - 0xC0200000
    ret

swap_page_dir:
    push    ebp
    mov     ebp, esp
    mov     eax, DWORD [ebp+8]
    mov     cr3, eax
    pop     ebp
    ret

global flush_tss
flush_tss:
    mov ax, (5 * 8) | 0
    ltr ax
    ret