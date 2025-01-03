; thank GOD for godbolt

global inw
inw:
    push    ebp
    mov     ebp, esp
    sub     esp, 4
    mov     eax, DWORD [ebp+8]
    mov     WORD [ebp-4], ax
    movzx   edx, WORD [ebp-4]
    in      ax,  dx
    leave
    ret

global outw
outw:
    push    ebp
    mov     ebp, esp
    sub     esp, 8
    mov     edx, DWORD [ebp+8]
    mov     eax, DWORD [ebp+12]
    mov     WORD [ebp-4], dx
    mov     WORD [ebp-8], ax
    movzx   edx, WORD [ebp-4]
    movzx   eax, WORD [ebp-8]
    out     dx, ax
    leave
    ret

global inl
inl:
    push    ebp
    mov     ebp, esp
    sub     esp, 4
    mov     eax, DWORD [ebp+8]
    mov     WORD [ebp-4], ax
    mov dx, WORD [ebp-4]
    in      eax, dx
    leave
    ret

global outl
outl:
    push    ebp
    mov     ebp, esp
    sub     esp, 4
    mov     eax, DWORD [ebp+8]
    mov     WORD [ebp-4], ax
    movzx   eax, WORD [ebp-4]
    mov     edx, eax
    mov     eax, DWORD [ebp+12]
    out     dx, eax
    leave
    ret

; extern void insw(u16 port, u16* buf, u16 size);
global p_insw
p_insw:
    push    ebp
    mov     ebp, esp
    push    edi
    sub     esp, 8
    mov     edx, DWORD [ebp+8]
    mov     eax, DWORD [ebp+16]
    mov     WORD [ebp-8], dx
    mov     WORD [ebp-12], ax
    movzx   edx, WORD  [ebp-8]
    mov     eax, DWORD [ebp+12]
    mov     edi, eax
    movzx   ecx, WORD [ebp-12]
    rep insw
    nop
    mov     edi, DWORD [ebp-4]
    leave
    ret