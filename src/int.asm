[bits 32]

section .text

extern exception_handler
extern irq_handler

%macro isr_err_stub 1
isr_stub_%+%1:
    pusha
    push DWORD %1
    call exception_handler
    pop eax
    popa
    iret 
%endmacro

%macro isr_no_err_stub 1
isr_stub_%+%1:
    pusha
    push DWORD %1
    call exception_handler
    add esp, 4
    iret
%endmacro

%macro m_irq_handler 1
irq_handler_%+%1:
    pushad
    cld
    push DWORD %1
    call irq_handler
    add esp, 4
    mov ah, %1
    mov al, 0x20
    cmp ah, 8
    jl .pic1
.pic2:
    out 0xA0, al
.pic1:
    out 0x20, al
    popad
    iretd
%endmacro

isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_no_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub    30
isr_no_err_stub 31

m_irq_handler 0
m_irq_handler 1
m_irq_handler 2
m_irq_handler 3
m_irq_handler 4
m_irq_handler 5
m_irq_handler 6
m_irq_handler 7
m_irq_handler 8
m_irq_handler 9
m_irq_handler 10
m_irq_handler 11
m_irq_handler 12
m_irq_handler 13
m_irq_handler 14
m_irq_handler 15

extern syscall_handler
global sysint_handler_asm
sysint_handler_asm:
    pushad
    call syscall_handler
    popad
    iretd

global irq_handle_table
irq_handle_table:
%assign i 0
%rep    16
    dd irq_handler_%+i
%assign i i+1
%endrep

global isr_stub_table
isr_stub_table:
%assign i 0
%rep    32
    dd isr_stub_%+i
%assign i i+1
%endrep