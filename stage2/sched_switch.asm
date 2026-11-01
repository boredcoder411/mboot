; Voluntary task switch. Construct the same saved frame expected by the interrupt return path.
[bits 32]

global scheduler_switch
extern scheduler_perform_switch

scheduler_switch:
    ; Synthesize the interrupt number/error slots and iret frame so this
    ; voluntary switch can share the scheduler's interrupt-frame layout.
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

    ; The scheduler returns the selected task's saved frame, possibly ours.
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
