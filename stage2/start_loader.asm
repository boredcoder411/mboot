[bits 32]
[extern loader_start]
global div0_fault

jmp loader_start
jmp $

div0_fault:
  xor eax, eax
  mov ebx, 0
  div ebx
  ret
