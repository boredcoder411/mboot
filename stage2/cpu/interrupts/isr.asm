; check https://wiki.osdev.org/Interrupts_Tutorial for source
[bits 32]

; ========= ISR =========
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

; ========= IRQ =========

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
    pusha
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_dispatcher
    add esp, 4

    popa
    add esp, 8
    iret

irq_stub_table:
%assign i 0
%rep 16
  dd _irq%+i
%assign i i+1
%endrep

isr_common:
    pusha
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call isr_handler
    add esp, 4

    popa
    add esp, 8
    iret

isr_stub_table:
%assign i 0
%rep 32
  dd _isr%+i
%assign i i+1
%endrep
