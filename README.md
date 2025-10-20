# mboot
mboot is a bootloader for i386 platforms.

## Prequisites
Building mboot depends on:
 - llvm tools (clang, ld.lld)
 - nasm

Running mboot needs:
 - the building tools or a disk image
 - qemu-system-x86_64

## Building
```sh
make
qemu-system-x86_64 -m 4G -drive file=image.img -serial stdio
```

## Note
As filesystems and elf files aren't implemented yet, programs you want to launch from mboot need to be linked into it and called from `loader.c`

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
- [ ] memory allocator
- [ ] elf loader
- [ ] paging
- [ ] libc

## Screenshots
![mboot running in qemu](screenshots/mboot_sc.png)
