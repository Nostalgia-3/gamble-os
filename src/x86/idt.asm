[bits 32]

section .text

extern exception_handler
extern irq_handler

%macro isr_stub 1
isr_stub_%+%1:
    pusha
    push DWORD %1
    call exception_handler
    add esp, 4
    popa
    iret
%endmacro

%macro m_irq_handler 1
irq_handler_%+%1:
    push ax

    mov ah, %1
    mov al, 0x20
    cmp ah, 8
    jl .pic1
.pic2:
    out 0xA0, al
.pic1:
    out 0x20, al
    pop ax
    pushad
    cld
    push DWORD %1
    call irq_handler
    add esp, 4
    popad
    iretd
%endmacro

isr_stub 0
isr_stub 1
isr_stub 2
isr_stub 3
isr_stub 4
isr_stub 5
isr_stub 6
isr_stub 7
isr_stub 8
isr_stub 9
isr_stub 10
isr_stub 11
isr_stub 12
isr_stub 13
isr_stub 14
isr_stub 15
isr_stub 16
isr_stub 17
isr_stub 18
isr_stub 19
isr_stub 20
isr_stub 21
isr_stub 22
isr_stub 23
isr_stub 24
isr_stub 25
isr_stub 26
isr_stub 27
isr_stub 28
isr_stub 29
isr_stub 30
isr_stub 31

; m_irq_handler 0

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

extern syscall_c
extern dump_regs

global syscall_handler_asm
syscall_handler_asm:
    pusha
    call syscall_c
    popa

    iretd

extern scheduler_tick

; this is the timer handler
irq_handler_0:
    push ax
    mov al, 0x20
    out 0x20, al
    pop ax
    
    pusha
    call scheduler_tick
    popa
    iretd