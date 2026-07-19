; Build the standard i386 ELF entry stack layout, then jump to entry point.
; This matches what Linux does at _start so the same binary works on both.
;
; C signature:
;   void run_elf_with_args_asm(void *entry, int argc, char **argv);
;
; Stack layout built (low to high address):
;   [esp]     = argc
;   [esp+4]   = argv[0] (char*)
;   [esp+8]   = argv[1]
;   ...
;   [esp+4*argc] = NULL (argv terminator)
;   [esp+4*argc+4] = NULL (envp terminator)

[BITS 32]

section .text
global run_elf_with_args_asm

run_elf_with_args_asm:
    mov ecx, [esp+4]      ; entry point
    mov edx, [esp+8]      ; argc
    mov eax, [esp+12]     ; argv (char**)

    push dword 0          ; envp[0] = NULL

    ; Push argv entries in reverse order (last to first)
    mov esi, edx          ; esi = argc
    shl esi, 2            ; esi = argc * 4
    add esi, eax          ; esi = &argv[argc] (one past the end)

    mov ebx, edx          ; ebx = argc (loop counter)
    test ebx, ebx
    jz .done

.loop:
    sub esi, 4            ; esi = &argv[i-1]
    push dword [esi]      ; push argv[i-1]
    dec ebx
    jnz .loop

.done:
    push dword 0          ; argv terminator = NULL
    push edx              ; argc

    jmp ecx               ; jump to entry (stack now matches Linux convention)
