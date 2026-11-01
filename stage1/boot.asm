; BIOS boot sector: collect E820 memory entries, read the kernel and enter protected mode. The disk image layout and link address must agree with this loader.
[org 0x7c00]

LOADER_OFFSET equ 0x10000
LOADER_SEG equ 0x1000
LOADER_OFF equ 0x0000

; BIOS passes the boot drive in DL; retain it for the sector read.
mov [BOOT_DISK], dl

mov bp, 0x7c00
mov sp, bp

mov ah, 0x00
mov al, 0x03
int 0x10

call load_kernel
jmp bootsector_memory

; BIOS writes 24-byte E820 entries at 0x71000 and the count at 0x70ff0.
; EBX is the continuation token returned by BIOS.
detect_memory:
    E820_SEG equ 0x7000
    E820_MAP_OFF equ 0x1000
    E820_COUNT_OFF equ 0x0FF0

    mov ax, E820_SEG
    mov es, ax
    xor ebx, ebx            ; Start with EBX = 0
    mov di, E820_MAP_OFF    ; Buffer to store entries
    mov word [es:E820_COUNT_OFF], 0 ; Initialize entry count to 0

.loop:
    mov eax, 0xE820         ; E820 function
    mov edx, 0x534D4150     ; 'SMAP' signature
    mov ecx, 24             ; Request 24 bytes for the entry
    mov dword [edi + 20], 1 ; Force ACPI 3.0 extended entry type
    int 0x15                ; Call BIOS
    jc .error               ; If carry is set, there was an error

    cmp eax, 0x534D4150     ; Check for 'SMAP' signature on return
    jne .error

    ; This implementation checks the continuation token before counting the
    ; final returned entry, so the stored count can omit that last entry.
    test ebx, ebx           ; If EBX is 0, we are done
    jz .done

    inc word [es:E820_COUNT_OFF] ; Increment entry count
    add di, 24              ; Move to the next 24-byte entry slot
    jmp .loop

.done:
    ret

.error:
	mov ah, 0x0e
	mov al, 'E'
	int 0x10
    jmp $

%include "stage1/gdt.asm"

[bits 16]
load_kernel:
	mov ax, LOADER_SEG
	mov es, ax
	mov bx, LOADER_OFF
	mov dh, NUM_STAGE2_SECTORS
	mov dl, [BOOT_DISK]
	call disk_load
	ret

[bits 16]
; Enable A20, load the flat GDT, set CR0.PE and use a far jump to reload CS.
switch_to_pm:
	cli
	in al, 0x92
	or al, 2
	out 0x92, al
	lgdt [gdt_descriptor]

	mov eax, cr0
	or eax, 0x1
	mov cr0, eax

	jmp CODE_SEG:init_pm

[bits 16]
; BIOS CHS read begins at sector 2, immediately after the boot sector.
; This simple path does not check the BIOS carry flag after the read.
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
    call detect_memory
	call switch_to_pm
	jmp $

[bits 32]
; Load the data selector into each segment register and establish a stack
; before calling the kernel at its linked address, 0x10000.
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
	incbin "kernel.bin"

stage2_end:
