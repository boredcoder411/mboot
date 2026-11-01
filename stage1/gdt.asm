; Initial flat protected-mode segment descriptors used during the BIOS-to-kernel transition.
gdt_start:
gdt_null:
  dq 0x0

; Flat 4 GiB code segment: executable/readable, present, ring 0.
gdt_code:
  dw 0xFFFF
  dw 0x0000
  db 0x00
  db 0x9A
  db 0xCF
  db 0x00

; Flat 4 GiB data segment: writable, present, ring 0.
gdt_data:
  dw 0xFFFF
  dw 0x0000
  db 0x00
  db 0b10010010
  db 0b11001111
  db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start