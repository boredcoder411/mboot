[bits 32]
[global idt_load]
[global syscall_handler]
[extern syscall_dispatch]

%macro pusha_c 0
    push eax
    push ebx
    push ecx
    push edx
    push ebp
    push esi
    push edi
%endmacro

%macro popa_c 0
    pop edi
    pop esi
    pop ebp
    pop edx
    pop ecx
    pop ebx
    pop eax
%endmacro

idt_load:
  mov eax, [esp + 4]
  lidt [eax]
  ret

syscall_handler:
    push 0
    push 0
    pusha_c
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call syscall_dispatch
    add esp, 4

    popa_c
    add esp, 8
    iret
