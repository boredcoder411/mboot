; same as isr.asm but with irqs and no error codes
[bits 32]

%assign l 0
%rep 16
[global _irq%+l]
%assign l l+1
%endrep

[global irq_common]
[global irq_stub_table]
[extern irq_dispatcher]

%macro irq_stub 1
_irq%+%1:
    push 0
    push %1+32
    jmp irq_common
%endmacro

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

irq_stub 0
irq_stub 1
irq_stub 2
irq_stub 3
irq_stub 4
irq_stub 5
irq_stub 6
irq_stub 7
irq_stub 8
irq_stub 9
irq_stub 10
irq_stub 11
irq_stub 12
irq_stub 13
irq_stub 14
irq_stub 15

irq_common:
    pusha_c
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_dispatcher
    add esp, 4

    popa_c
    add esp, 8
    iret

irq_stub_table:
%assign i 0
%rep 16
  dd _irq%+i
%assign i i+1
%endrep
