CC = x86_64-elf-gcc
LD = x86_64-elf-ld
CFLAGS = -m32 -ffreestanding -c -Istage2/ -Wall -Wextra -Werror -Os
LDFLAGS = -melf_i386 -o bootloader.bin -Ttext 0x1000 --oformat binary

BUILD = build
IMAGE = image.img

all: stage2 stage1 image

$(BUILD):
	mkdir -p $(BUILD)

stage1: stage1/load_kernel.asm | $(BUILD)
	nasm -f bin stage1/load_kernel.asm -o load_kernel.bin -D USE_GRAPHICS

stage2: stage2/start_loader.asm stage2/loader.c stage2/utils.c stage2/dev/vga.c stage2/io.c stage2/dev/disk.c stage2/dev/serial.c stage2/fs.c stage2/mbr.c stage2/cpu/interrupts/idt.c stage2/cpu/interrupts/isr.asm stage2/cpu/interrupts/isr.c stage2/cpu/interrupts/irq.c stage2/cpu/interrupts/irq.asm stage2/cpu/pic/pic.c stage2/cpu/pit/pit.c stage2/dev/keyboard.c | $(BUILD)
	nasm -f elf stage2/start_loader.asm -o $(BUILD)/start_loader.o
	nasm -f elf stage2/cpu/interrupts/idt.asm -o $(BUILD)/idt_s.o
	nasm -f elf stage2/cpu/interrupts/isr.asm -o $(BUILD)/isr_s.o
	nasm -f elf stage2/cpu/interrupts/irq.asm -o $(BUILD)/irq_s.o
	
	$(CC) $(CFLAGS) stage2/loader.c -o $(BUILD)/loader.o
	$(CC) $(CFLAGS) stage2/utils.c -o $(BUILD)/utils.o
	$(CC) $(CFLAGS) stage2/dev/vga.c -o $(BUILD)/vga.o
	$(CC) $(CFLAGS) stage2/io.c -o $(BUILD)/io.o
	$(CC) $(CFLAGS) stage2/dev/disk.c -o $(BUILD)/disk.o
	$(CC) $(CFLAGS) stage2/dev/serial.c -o $(BUILD)/serial.o
	$(CC) $(CFLAGS) stage2/fs.c -o $(BUILD)/fs.o
	$(CC) $(CFLAGS) stage2/mbr.c -o $(BUILD)/mbr.o
	$(CC) $(CFLAGS) stage2/cpu/interrupts/idt.c -o $(BUILD)/idt.o
	$(CC) $(CFLAGS) stage2/cpu/interrupts/isr.c -o $(BUILD)/isr.o
	$(CC) $(CFLAGS) stage2/cpu/interrupts/irq.c -o $(BUILD)/irq.o
	$(CC) $(CFLAGS) stage2/cpu/pic/pic.c -o $(BUILD)/pic.o
	$(CC) $(CFLAGS) stage2/cpu/pit/pit.c -o $(BUILD)/pit.o
	$(CC) $(CFLAGS) stage2/dev/keyboard.c -o $(BUILD)/keyboard.o

	$(LD) $(LDFLAGS) \
		$(BUILD)/start_loader.o \
		$(BUILD)/loader.o \
		$(BUILD)/utils.o \
		$(BUILD)/vga.o \
		$(BUILD)/serial.o \
		$(BUILD)/io.o \
		$(BUILD)/disk.o \
		$(BUILD)/fs.o \
		$(BUILD)/mbr.o \
		$(BUILD)/idt.o \
		$(BUILD)/idt_s.o \
		$(BUILD)/isr.o \
		$(BUILD)/isr_s.o \
		$(BUILD)/irq.o \
		$(BUILD)/irq_s.o \
		$(BUILD)/pic.o \
		$(BUILD)/pit.o \
		$(BUILD)/keyboard.o

image: stage1 stage2 load_kernel.bin test.wad partition_script
	@if [[ ! -f load_kernel.bin ]]; then \
		echo "Error: load_kernel.bin not found."; \
		exit 1; \
	fi
	@if [[ ! -f test.wad ]]; then \
		echo "Error: test.wad not found."; \
		exit 1; \
	fi
	dd if=/dev/zero of=$(IMAGE) bs=1M count=10
	dd if=load_kernel.bin of=$(IMAGE) conv=notrunc
	sfdisk $(IMAGE) < partition_script
	dd if=load_kernel.bin of=$(IMAGE) bs=446 count=1 conv=notrunc
	LOOP_DEV=$$(sudo losetup --find --show $(IMAGE)); \
	sudo partprobe $$LOOP_DEV; \
	sudo dd if=test.wad of=$${LOOP_DEV}p2; \
	sudo losetup -d $$LOOP_DEV
	@echo "Disk image created! Use xxd $(IMAGE) | less to inspect"

wad_tool: wad_tool.c | $(BUILD)
	gcc -o $(BUILD)/wad_tool wad_tool.c -Istage2/

clean:
	rm -rf $(BUILD)
	rm -f *.bin
	rm -f $(IMAGE)

.PHONY: all clean stage1 stage2 image wad_tool
