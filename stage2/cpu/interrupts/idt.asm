global idt_load
global isr_common

extern isr_handler

idt_load:
  mov eax, [esp + 4]
  lidt [eax]
  ret

isr_common:
  pusha
  push ds
  push es
  push fs
  push gs

  mov ax, 0x10
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  cld

  push esp
  call isr_handler
  add esp, 4

  pop gs
  pop fs
  pop es
  pop ds

  popa

  add esp, 8
  iret
