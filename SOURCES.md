# Sources

References consulted for `mboot`. Grouped by subsystem; each maps to code in this repo. Only sources actually read during development are listed.

## 1. CPU, Memory, and Privilege

| # | Source | Used for | File |
|---|--------|----------|------|
| 1 | Intel Corporation. *Intel 64 and IA-32 Architectures Software Developer's Manual, Volume 3A: System Programming Guide, Part 1.* Ch. 3 (Protected Mode), Ch. 4 (Paging), Ch. 5-6 (Interrupts/Exceptions), Ch. 7 (Task Management / TSS). https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html | GDT/TSS, IDT, PSE paging (`CR0`/`CR4`/`CR3`), `lgdt`/`ltr`, `int 0x80` | `stage1/gdt.asm`, `stage2/cpu/gdt.c:19`, `stage2/paging.c:40`, `stage2/cpu/interrupts/idt.c:158` |
| 2 | OSDev Wiki — *Bare Bones*, *GDT*, *IDT*, *Paging*, *Detecting Memory (E820)*. https://wiki.osdev.org/Bare_Bones https://wiki.osdev.org/GDT https://wiki.osdev.org/IDT https://wiki.osdev.org/Paging https://wiki.osdev.org/Detecting_Memory_(x86) | Boot skeleton, E820 map at `0x71000`, flat segments, 4 MiB identity map | `stage1/boot.asm:1`, `stage1/boot.asm:19`, `stage2/mem.c:73` |

## 2. Interrupts, Timers, and Peripherals

| # | Source | Used for | File |
|---|--------|----------|------|
| 3 | Intel Corporation. *8259A Programmable Interrupt Controller Datasheet.* https://pdos.csail.mit.edu/6.828/2005/readings/i8259A.pdf + OSDev *PIC* https://wiki.osdev.org/PIC | PIC remap to `0x20`/`0x28`, ICW1-4, EOI, per-IRQ masking | `stage2/cpu/pic/pic.c:31` |
| 4 | OSDev Wiki — *PIT* https://wiki.osdev.org/PIT + Intel 8254 datasheet | PIT channel 0, mode 2/3, divisor `1193` | `stage2/cpu/pit/pit.c:4` |
| 5 | OSDev Wiki — *PS/2 Keyboard*, *VGA Hardware*, *Text Mode Cursor*, *Serial Ports* https://wiki.osdev.org/PS2_Keyboard https://wiki.osdev.org/VGA_Hardware https://wiki.osdev.org/Text_Mode_Cursor https://wiki.osdev.org/Serial_Ports + Ralf Brown's Interrupt List (`int 0x10`, `int 0x13`) | Scancode set 1 (`0x60`), VGA `0xB8000` + cursor `0x3D4/0x3D5`, 16550 UART | `stage2/dev/keyboard.c:45`, `stage2/dev/vga.c:8`, `stage2/dev/serial.c` |
| 6 | Tisc, M. *La Bible PC — Programmation système.* https://archive.org/details/labiblepcprogram0000tisc | French-language reference for BIOS services, VGA and PIC/PIT register details (cross-checked with Intel/OSDev) | `stage1/boot.asm:84` (`int 0x13 AH=0x02`), `stage2/dev/vga.c` |

## 3. Storage and Filesystems

| # | Source | Used for | File |
|---|--------|----------|------|
| 7 | OSDev Wiki — *ATA PIO Mode* https://wiki.osdev.org/ATA_PIO_Mode + ATA/ATAPI-6 §5.10 | LBA28 PIO via `0x1F0/0x3F6`, status `0x80 BSY / 0x08 DRQ`, `insw` | `stage2/dev/disk.c:40` |
| 8 | Microsoft Corporation. *Microsoft Extensible Firmware Initiative FAT32 File System Specification.* v1.03, 2005. https://academy.cba.mit.edu/classes/networking_communications/SD/FAT.pdf + OSDev *FAT* https://wiki.osdev.org/FAT | BPB, FAT chain, `0xFFF8` EOC, LFN, root/data cluster layout | `stage2/fat16.c:196` |
| 9 | rweichler. *FAT16 generic driver.* https://github.com/rweichler/FAT16 | Reference for FAT16 cluster-chain walk and `get_next_cluster` logic; adapted to MBR-partitioned image and `mtools`/`mkfs.fat` flow | `stage2/fat16.c:39`, `image.sh:78` |
| 10 | Tool Interface Standard (TIS). *Executable and Linkable Format (ELF) Specification, Version 1.2.* https://refspecs.linuxfoundation.org/elf/elf.pdf + OSDev *ELF* https://wiki.osdev.org/ELF | `Elf32_Ehdr`/`Phdr`, `PT_LOAD`, `0x7F 'E' 'L' 'F'` magic | `stage2/elf.c:12`, `inc/elf.h` |

## 4. PCI and Networking

| # | Source | Used for | File |
|---|--------|----------|------|
| 11 | *PCI Local Bus Specification, Revision 3.0.* PCI-SIG. https://pcisig.com/specifications + OSDev *PCI* https://wiki.osdev.org/PCI | Config space `0xCF8/0xCFC`, BAR decode (I/O vs 32/64-bit MMIO), bus mastering | `stage2/dev/pci.c`, `stage2/dev/e1k.c:404` |
| 12 | Intel Corporation. *PCI/PCI-X Family of Gigabit Ethernet Controllers Software Developer's Manual — 82540EM / 82574L.* https://www.intel.com/content/dam/www/public/us/en/documents/manuals/gigabit-ethernet-controller-datasheet.pdf | e1000 TX/RX descriptor rings, `CTRL/RCTL/TCTL/RDBAL/RDT`, MMIO vs I/O BAR | `stage2/dev/e1k.c:103`, `stage2/dev/e1k.c:369` |
| 13 | Postel, J. *RFC 791: Internet Protocol.* IETF, 1981. https://datatracker.ietf.org/doc/html/rfc791 | IPv4 header, addressing `10.0.2.15/24` | `stage2/net/ipv4.c` |
| 14 | Plummer, D. *RFC 826: An Ethernet Address Resolution Protocol.* IETF, 1982. https://datatracker.ietf.org/doc/html/rfc826 | ARP request/reply | `stage2/net/arp.c` |
| 15 | Postel, J. *RFC 768: User Datagram Protocol.* IETF, 1980. https://datatracker.ietf.org/doc/html/rfc768 | UDP header + checksum, socket demux | `stage2/net/udp.c` |
| 16 | Postel, J. *RFC 792: Internet Control Message Protocol.* IETF, 1981. https://datatracker.ietf.org/doc/html/rfc792 | ICMP echo request/reply, raw sockets | `stage2/net/icmp.c`, `stage2/net/socket.c:88` |

## 5. Userspace, Toolchain, and Emulation

| # | Source | Used for | File |
|---|--------|----------|------|
| 17 | System V Application Binary Interface, *Intel386 Architecture Processor Supplement.* + `man 2 socket,bind,sendto,recvfrom,fork,execve,waitpid,dup2` | `int 0x80` calling convention, socket API, `fork`/`exec` semantics | `user/src/libc.c:18`, `stage2/cpu/interrupts/idt.c:14`, `stage2/scheduler.c:471` |
| 18 | Ierusalimschy, R. et al. *Lua 5.5 Reference Manual.* PUC-Rio, 2024. https://www.lua.org/manual/5.5/ + OSDev *Lua* https://wiki.osdev.org/Lua | Porting Lua to freestanding `i386-elf` with custom `libc` | `user/lua/lua.h`, `user/src/libc.c` |
| 19 | *NASM — The Netwide Assembler Manual.* https://www.nasm.us/doc/ + LLVM Project — *Clang Cross-compilation*, *lld* https://clang.llvm.org/docs/CrossCompilation.html https://lld.llvm.org/ | `nasm -f bin/elf`, `clang -target i386-elf -ffreestanding`, `ld.lld -m elf_i386`, `link.ld:6` | `Makefile:1`, `stage1/boot.asm:1`, `link.ld:1` |
| 20 | *QEMU System Emulation Documentation — x86, e1000, user-mode networking.* https://www.qemu.org/docs/master/system/index.html https://www.qemu.org/docs/master/system/devices/net.html | `-device e1000,netdev=n0 -netdev user,hostfwd=udp::...`, `-object filter-dump` pcap | `README.md` Running, `stage2/dev/e1k.c` |

> Note: Intel SDM is the normative reference; OSDev Wiki pages were used as practical guides and were cross-checked against the SDM/datasheets. RFCs are cited directly for wire formats.
