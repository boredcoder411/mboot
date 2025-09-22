[bits 32]
[extern loader_start]
[global div0_fault]
[global enable_fpu]

jmp loader_start
jmp $

enable_fpu:
  fninit
  mov eax, cr0
  and eax, 0xFFFB
  or  eax, 0x2
  mov cr0, eax

  mov eax, cr4
  or  eax, (3 << 9)
  mov cr4, eax
  ret

div0_fault:
  xor eax, eax
  mov ebx, 0
  div ebx
  ret
