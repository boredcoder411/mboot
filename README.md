# kmint
kmint was supposed to only be a kernel for my computer science final, but I'll make it a full os eventually.

## Prequisites
Building kmint depends on:
 - x86_64-elf-gcc
 - x86_64-elf-ld (provided by x86_64-elf-binutils on brew.sh)
 - sfdisk
 - nasm
 - partprobe

These requirements mean you can only fully build the disk image on Linux, I will swap out the missing tools for mac-friendly ones soon.

Running kmint needs:
 - the building tools or a disk image
 - qemu-system-x86_64

## Building
```sh
make
qemu-system-x86_64 -m 4G -drive file=image.img -serial stdio
```

## Note
This is by far not finished, there is a simple demo in ```loader.c``` of a 3d renderer displaying a rotating cube on the vga output

## todo:
- [x] 32 bit protected mode
- [x] cpu exceptions
- [x] individual hardware interrupts
- [x] a ps/2 keyboard
- [x] vga in 320x200x8bpp
- [x] reading ata drives
- [x] rs232 interfaces
- [x] the intel 8259 PIC
- [x] the intel 8253 PIT
- [x] the mbr partitioning scheme
- [x] wad files as the filesystem
- [x] enable x87 fpu
- [ ] better filesystem
- [ ] memory allocator
- [ ] scheduler
- [ ] elf loader
- [ ] syscalls
- [ ] libc