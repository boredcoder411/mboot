[bits 32]

global scheduler_switch
extern scheduler_perform_switch

scheduler_switch:
    pop ecx
    pushf
    or dword [esp], 0x200
    push dword 0x08
    push ecx
    push dword 0
    push dword 0
    push eax
    push ebx
    push ecx
    push edx
    push ebp
    push esi
    push edi

    mov eax, esp
    push eax
    call scheduler_perform_switch

    mov esp, eax

    pop edi
    pop esi
    pop ebp
    pop edx
    pop ecx
    pop ebx
    pop eax
    add esp, 8
    iret
