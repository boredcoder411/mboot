CC = clang
LD = ld.lld
OBJCOPY = llvm-objcopy

CFLAGS = -target i386-elf -ffreestanding -fno-pic -fno-pie -mno-red-zone \
         -Wall -Wextra -Werror -g -c -Iinc/ -Idemos/ -Os \
         -ffunction-sections -fdata-sections -flto
LDFLAGS = -m elf_i386 -T link.ld -nostdlib -static --gc-sections -o kernel.elf

BUILD = build

all: stage2 stage1 file_transforms image

CFLAGS += -DE1K_DEMO -DALLOC_DBG

$(BUILD):
	mkdir -p $(BUILD)

stage1: stage1/boot.asm | $(BUILD)
	nasm -f bin stage1/boot.asm -o boot.bin

stage2: stage2/start_loader.asm stage2/loader.c stage2/utils.c stage2/dev/vga.c stage2/io.c stage2/dev/disk.c stage2/dev/serial.c stage2/fat16.c stage2/cpu/interrupts/idt.c stage2/cpu/interrupts/isr.asm stage2/cpu/interrupts/isr.c stage2/cpu/interrupts/irq.asm stage2/cpu/interrupts/irq.c stage2/cpu/pic/pic.c stage2/cpu/pit/pit.c stage2/dev/keyboard.c stage2/mem.c stage2/dev/rtc.c stage2/dev/pci.c stage2/dev/pci_devices.c stage2/dev/e1k.c stage2/vfs.c stage2/elf.c | $(BUILD)
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
	$(CC) $(CFLAGS) stage2/fat16.c -o $(BUILD)/fat16.o
	$(CC) $(CFLAGS) stage2/cpu/interrupts/idt.c -o $(BUILD)/idt.o
	$(CC) $(CFLAGS) stage2/cpu/interrupts/isr.c -o $(BUILD)/isr.o
	$(CC) $(CFLAGS) stage2/cpu/interrupts/irq.c -o $(BUILD)/irq.o
	$(CC) $(CFLAGS) stage2/cpu/pic/pic.c -o $(BUILD)/pic.o
	$(CC) $(CFLAGS) stage2/cpu/pit/pit.c -o $(BUILD)/pit.o
	$(CC) $(CFLAGS) stage2/dev/keyboard.c -o $(BUILD)/keyboard.o
	$(CC) $(CFLAGS) stage2/mem.c -o $(BUILD)/mem.o
	$(CC) $(CFLAGS) stage2/dev/rtc.c -o $(BUILD)/rtc.o
	$(CC) $(CFLAGS) stage2/dev/pci.c -o $(BUILD)/pci.o
	$(CC) $(CFLAGS) stage2/dev/pci_devices.c -o $(BUILD)/pci_devices.o
	$(CC) $(CFLAGS) stage2/dev/e1k.c -o $(BUILD)/e1k.o
	$(CC) $(CFLAGS) stage2/vfs.c -o $(BUILD)/vfs.o
	$(CC) $(CFLAGS) stage2/elf.c -o $(BUILD)/elf.o

	$(LD) $(LDFLAGS) \
		$(BUILD)/start_loader.o \
		$(BUILD)/loader.o \
		$(BUILD)/utils.o \
		$(BUILD)/vga.o \
		$(BUILD)/serial.o \
		$(BUILD)/io.o \
		$(BUILD)/disk.o \
		$(BUILD)/fat16.o \
		$(BUILD)/idt.o \
		$(BUILD)/idt_s.o \
		$(BUILD)/isr.o \
		$(BUILD)/isr_s.o \
		$(BUILD)/irq.o \
		$(BUILD)/irq_s.o \
		$(BUILD)/pic.o \
		$(BUILD)/pit.o \
		$(BUILD)/keyboard.o \
		$(BUILD)/mem.o \
		$(BUILD)/rtc.o \
		$(BUILD)/pci.o \
		$(BUILD)/pci_devices.o \
		$(BUILD)/e1k.o \
		$(BUILD)/vfs.o \
		$(BUILD)/elf.o
	
	$(OBJCOPY) --only-keep-debug kernel.elf kernel.sym
	$(OBJCOPY) -O binary kernel.elf kernel.bin

image:
	@./image.sh test_files/hi.txt build/test.elf

psf: tools/psf.c | $(BUILD)
	gcc -o $(BUILD)/psf tools/psf.c -Iinc/ $$(pkg-config --cflags --libs libpng)

imf: tools/imf.c | $(BUILD)
	gcc -o $(BUILD)/imf tools/imf.c -Iinc/ $$(pkg-config --cflags --libs libpng)

file_transforms: psf imf | $(BUILD)
	$(BUILD)/psf test_files/font.png $(BUILD)/font.psf
	$(BUILD)/imf test_files/icon.png $(BUILD)/icon.imf --rle
	$(CC) -target i386-elf -m32 -ffreestanding -nostdlib -o $(BUILD)/test.o -c test_files/test.c
	$(LD) $(BUILD)/test.o -m elf_i386 -static -o $(BUILD)/test.elf

format:
	@find . -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +

clean:
	rm -rf $(BUILD)
	rm -f *.bin
	rm -f kernel.sym
	rm -f kernel.elf
	rm -f image.img

.PHONY: all clean stage1 stage2 image psf imf file_transforms format
