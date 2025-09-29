; check https://wiki.osdev.org/Interrupts_Tutorial for source
[bits 32]

%assign l 0
%rep 32
[global _isr%+l]
%assign l l+1
%endrep

[global isr_common]
[global isr_stub_table]
[extern isr_handler]

%macro isr_err_stub 1
_isr%+%1:
    push %1
    jmp isr_common
%endmacro

%macro isr_no_err_stub 1
_isr%+%1:
    push 0
    push %1
    jmp isr_common
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

isr_common:
    pusha_c
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call isr_handler
    add esp, 4

    popa_c
    add esp, 8
    iret

isr_stub_table:
%assign i 0
%rep 32
  dd _isr%+i
%assign i i+1
%endrep
