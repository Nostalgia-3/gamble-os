[bits 32]

%macro copy 2
    mov ebx, DWORD %1
    mov DWORD %2, ebx
%endmacro

%macro print 1
    push eax
    push %1
    call printf_
    add esp, 4
    pop eax
%endmacro

section .data

MAX_PROCESSES equ 256

align 8

global second_tick
second_tick: dd 0

global cur_process
cur_process: dd 0

; a list of process pointers
global queue
queue: times MAX_PROCESSES + 1 dd 0

shutting_down:  db "Shutting down.", 10, 0

section .text

extern printf_
extern swap_pagedir

global scheduler_tick
scheduler_tick:
    mov     eax, [second_tick]
    cmp     eax, 1
    je      .tick
    mov     eax, 1
    mov     [second_tick], eax
.copy_registers:
    ; get the queue address
    mov     ecx, DWORD [cur_process]
    shl     ecx, 2
    mov     eax, [queue + ecx]

    ; if the current queue index points to NULL
    cmp      eax, 0
    je      .shut_down

    copy    [eax + 0x00], [esp + 0x00 + 4] ; edi
    copy    [eax + 0x04], [esp + 0x04 + 4] ; esi
    copy    [eax + 0x08], [esp + 0x08 + 4] ; ebp
    copy    [eax + 0x0C], [esp + 0x0C + 4] ; esp
    copy    [eax + 0x10], [esp + 0x10 + 4] ; ebx
    copy    [eax + 0x14], [esp + 0x14 + 4] ; edx
    copy    [eax + 0x18], [esp + 0x18 + 4] ; ecx
    copy    [eax + 0x1C], [esp + 0x1C + 4] ; eax
    copy    [eax + 0x20], [esp + 0x20 + 4] ; eip
    copy    [eax + 0x24], [esp + 0x24 + 4] ; cs
    copy    [eax + 0x28], [esp + 0x28 + 4] ; eflags
    copy    [eax + 0x2C], [esp + 0x2C + 4] ; ss

    push    DWORD [eax + 0x30]
    call    swap_pagedir
    add     esp, 4

    ret
.tick:
    mov     ecx, DWORD [cur_process]
    shl     ecx, 2
    mov     eax, DWORD [queue + ecx]

    cmp     eax, 0
    je      .bump_process_queue
.save_registers:
    copy    [esp + 0x00 + 4], [eax + 0x00] ; edi
    copy    [esp + 0x04 + 4], [eax + 0x04] ; esi
    copy    [esp + 0x08 + 4], [eax + 0x08] ; ebp
    copy    [esp + 0x0C + 4], [eax + 0x0C] ; esp
    copy    [esp + 0x10 + 4], [eax + 0x10] ; ebx
    copy    [esp + 0x14 + 4], [eax + 0x14] ; edx
    copy    [esp + 0x18 + 4], [eax + 0x18] ; ecx
    copy    [esp + 0x1C + 4], [eax + 0x1C] ; eax
    copy    [esp + 0x20 + 4], [eax + 0x20] ; eip
    copy    [esp + 0x24 + 4], [eax + 0x24] ; cs
    copy    [esp + 0x28 + 4], [eax + 0x28] ; eflags
    copy    [esp + 0x2C + 4], [eax + 0x2C] ; ss
.bump_process_queue:
    ; eax = queue[cur_process]
    ; ecx = last_process
    mov     ecx, DWORD [cur_process]
.next_in_queue:
    inc     DWORD [cur_process]

    ; if(cur_process > MAX_PROCESSES) goto .reset_cur_progress;
    cmp     DWORD [cur_process], MAX_PROCESSES
    jg      .reset_cur_process

    mov     edx, DWORD [cur_process]
    shl     edx, 2
    mov     eax, [queue + edx]

    ; if(cur_process == last_process)
    cmp     DWORD [cur_process], ecx
    je      .w2

    jmp     .w3
.reset_cur_process:
    mov     DWORD [cur_process], 0  ; cur_process = 0;
    mov     eax, [queue]            ; get queue[0];
.w3:
    cmp     eax, 0
    je      .next_in_queue
    cmp     DWORD [eax + 0x44], 0   ; if(!can_run)
    je      .next_in_queue          ;   goto .next_in_queue
    jmp     .copy_registers         ; goto .copy_registers
.w2:
    cmp     eax, 0
    je      .shut_down
    cmp     DWORD [eax + 0x44], 0   ; if(!can_run)
    je      .shut_down
    jmp     .copy_registers
.shut_down:
    print   shutting_down
    jmp     $