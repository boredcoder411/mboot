[org 0x7c00]

LOADER_OFFSET equ 0x1000

mov [BOOT_DISK], dl

mov bp, 0x7c00
mov sp, bp

%ifdef USE_GRAPHICS
call init_video_graphics
%else
call init_video_text
%endif

call load_kernel
jmp bootsector_memory

detect_memory:
    xor ebx, ebx            ; Start with EBX = 0
    mov edi, E820_MAP_ADDR  ; Buffer to store entries
    mov word [E820_COUNT_ADDR], 0 ; Initialize entry count to 0

.loop:
    mov eax, 0xE820         ; E820 function
    mov edx, 0x534D4150     ; 'SMAP' signature
    mov ecx, 24             ; Request 24 bytes for the entry
    mov dword [edi + 20], 1 ; Force ACPI 3.0 extended entry type
    int 0x15                ; Call BIOS
    jc .error               ; If carry is set, there was an error

    cmp eax, 0x534D4150     ; Check for 'SMAP' signature on return
    jne .error

    test ebx, ebx           ; If EBX is 0, we are done
    jz .done

    inc word [E820_COUNT_ADDR] ; Increment entry count
    add edi, 24             ; Move to the next 24-byte entry slot
    jmp .loop

.done:
    ret

.error:
	mov ah, 0x0e
	mov al, 'E'
	int 0x10
    jmp $

%include "stage1/utils.inc"
%include "stage1/gdt.asm"
%include "stage1/video.asm"

[bits 16]
load_kernel:
	mov bx, LOADER_OFFSET
	mov dh, NUM_STAGE2_SECTORS
	mov dl, [BOOT_DISK]
	call disk_load
	ret

[bits 16]
switch_to_pm:
	cli
	enable_a20
	lgdt [gdt_descriptor]

	mov eax, cr0
	or eax, 0x1
	mov cr0, eax

	jmp CODE_SEG:init_pm

[bits 16]
disk_load:
	pusha

	push dx

	mov ah, 0x02
	mov al, dh
	mov ch, 0x00
	mov dh, 0x00
	mov cl, 0x02

	int 0x13
	
	pop dx

	popa

	ret

[bits 16]
bootsector_memory:
    ; Address to store the E820 map and its entry count
    E820_MAP_ADDR equ 0x9000
    E820_COUNT_ADDR equ 0x8E00

    call detect_memory
	call switch_to_pm
	jmp $

[bits 32]
init_pm:
	mov ax, DATA_SEG

	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	mov ebp, 0x90000
	mov esp, ebp

	call BEGIN_PM

[bits 32]
BEGIN_PM:
	call LOADER_OFFSET

	jmp $

BOOT_DISK db 0x00

times 446-($-$$) db 0
db 0x80
db 0x00, 0x01, 0x00
db 0x17
db 0x00, 0x02, 0x00
db 0x00, 0x00, 0x00, 0x00
db 0x02, 0x00, 0x00, 0x00

times 510-($-$$) db 0
dw 0xaa55

NUM_STAGE2_SECTORS equ (stage2_end-stage2_start+511) / 512

stage2_start:
	incbin "bootloader.bin"

stage2_end:
