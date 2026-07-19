[bits 32]

extern main
extern exit

section .text
global _start

_start:
    ; Standard i386 ELF entry convention (Linux):
    ;   [esp]   = argc
    ;   [esp+4] = argv[0], argv[1], ..., NULL

    mov eax, [esp]        ; argc
    lea ebx, [esp+4]      ; argv = &argv[0] on stack

    push ebx              ; argv
    push eax              ; argc
    call main
    push eax
    call exit
