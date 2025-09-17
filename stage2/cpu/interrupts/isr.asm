[bits 32]
[global _isr0]
[global _isr1]
[global _isr2]
[global _isr3]
[global _isr4]
[global _isr5]
[global _isr6]
[global _isr7]
[global _isr8]
[global _isr9]
[global _isr10]
[global _isr11]
[global _isr12]
[global _isr13]
[global _isr14]
[global isr_common]
[extern isr_handler]

_isr0:
  push 0
  push 0
  jmp isr_common

_isr1:
  push 0
  push 1
  jmp isr_common

_isr2:
  push 0
  push 2
  jmp isr_common

_isr3:
  push 0
  push 3
  jmp isr_common

_isr4:
  push 0
  push 4
  jmp isr_common

_isr5:
  push 0
  push 5
  jmp isr_common

_isr6:
  push 0
  push 6
  jmp isr_common

_isr7:
  push 0
  push 7
  jmp isr_common

_isr8:
  push 8
  jmp isr_common

_isr9:
  push 0
  push 9
  jmp isr_common

_isr10:
  push 10
  jmp isr_common

_isr11:
  push 11
  jmp isr_common

_isr12:
  push 12
  jmp isr_common

_isr13:
  push 13
  jmp isr_common

_isr14:
  push 14
  jmp isr_common

isr_common:
    pusha
    push ds
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call isr_handler
    add esp, 4

    pop ds
    popa
    add esp, 8     ; remove fake error + int_no
    iret

