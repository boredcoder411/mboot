CC = clang
LD = ld.lld
OBJCOPY = llvm-objcopy

CFLAGS = -target i386-elf -ffreestanding -fno-pic -fno-pie -mno-red-zone \
         -Wall -Wextra -Werror -g -c -Istage2/ -Os
LDFLAGS = -m elf_i386 -T link.ld -nostdlib -static -o kernel.elf

BUILD = build
IMAGE = image.img

all: stage2 stage1 image

$(BUILD):
	mkdir -p $(BUILD)

stage1: stage1/boot.asm | $(BUILD)
	nasm -f bin stage1/boot.asm -o boot.bin

stage2: stage2/start_loader.asm stage2/loader.c stage2/utils.c stage2/dev/vga.c stage2/io.c stage2/dev/disk.c stage2/dev/serial.c stage2/fs.c stage2/mbr.c stage2/cpu/interrupts/idt.c stage2/cpu/interrupts/isr.asm stage2/cpu/interrupts/isr.c stage2/cpu/interrupts/irq.asm stage2/cpu/interrupts/irq.c stage2/cpu/pic/pic.c stage2/cpu/pit/pit.c stage2/dev/keyboard.c stage2/mem.c | $(BUILD)
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
	$(CC) $(CFLAGS) stage2/mem.c -o $(BUILD)/mem.o

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
		$(BUILD)/keyboard.o \
		$(BUILD)/mem.o
	
	$(OBJCOPY) --only-keep-debug kernel.elf kernel.sym
	$(OBJCOPY) -O binary kernel.elf kernel.bin

image: stage1 stage2 boot.bin assets.wad mkpart wpart
	@if [[ ! -f boot.bin ]]; then \
		echo "Error: boot.bin not found."; \
		exit 1; \
	fi
	@if [[ ! -f assets.wad ]]; then \
		echo "Error: assets.wad not found."; \
		exit 1; \
	fi
	dd if=/dev/zero of=$(IMAGE) bs=1M count=10
	dd if=boot.bin of=$(IMAGE) conv=notrunc
	$(BUILD)/mkpart $(IMAGE)
	$(BUILD)/wpart $(IMAGE) 2 assets.wad
	@echo "Disk image created! Use xxd $(IMAGE) | less to inspect"

wad_tool: tools/wad_tool.c | $(BUILD)
	gcc -o $(BUILD)/wad_tool tools/wad_tool.c -Istage2/

mkpart: tools/mkpart.c | $(BUILD)
	gcc -o $(BUILD)/mkpart tools/mkpart.c -Istage2/

psf: tools/psf.c | $(BUILD)
	gcc -o $(BUILD)/psf tools/psf.c -Istage2/ $$(pkg-config --cflags --libs libpng)

wpart: tools/wpart.c | $(BUILD)
	gcc -o $(BUILD)/wpart tools/wpart.c -Istage2/

imf: tools/imf.c | $(BUILD)
	gcc -o $(BUILD)/imf tools/imf.c -Istage2/ $$(pkg-config --cflags --libs libpng)

assets.wad: psf wad_tool imf | $(BUILD)
	$(BUILD)/psf test_files/font.png $(BUILD)/font.psf
	$(BUILD)/imf test_files/icon.png $(BUILD)/icon.imf
	$(BUILD)/wad_tool pack assets.wad IWAD $(BUILD)/font.psf $(BUILD)/icon.imf test_files/test.txt

clean:
	rm -rf $(BUILD)
	rm -f *.bin
	rm -f $(IMAGE)
	rm -f kernel.sym
	rm -f kernel.elf
	rm -f assets.wad

.PHONY: all clean stage1 stage2 image wad_tool mkpart psf wpart imf
