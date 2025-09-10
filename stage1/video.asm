[bits 16]
init_video_graphics:
	mov ah, 0x00
	mov al, 0x13
	int 0x10
	ret

[bits 16]
init_video_text:
	mov ah, 0x00
	mov al, 0x03
	int 0x10
	ret
