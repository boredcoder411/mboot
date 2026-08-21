# mboot
mboot is a minimal os for i386 platforms.

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
# normal os
qemu-system-x86_64 -m 4G -drive file=image.img -serial stdio

# with networking enabled
qemu-system-x86_64 -m 4G -drive file=image.img -serial stdio -device e1000,netdev=n0 -netdev user,id=n0 -object filter-dump,id=f1,netdev=n0,file=netdump.pcap

# with networking enabled and UDP forwarded from host port 10007 to guest port 7
qemu-system-x86_64 -m 4G -drive file=image.img -serial stdio \
  -device e1000,netdev=n0 \
  -netdev user,id=n0,hostfwd=udp::10007-:7 \
  -object filter-dump,id=f1,netdev=n0,file=netdump.pcap
```

## UDP demo
The kernel brings up the network interface at boot (`stage2/net/net.c`): guest IP
`10.0.2.15/24` with gateway `10.0.2.2`. The init system (`user/src/init.c`) assigns each
process a tty at spawn: init stays on tty1, the Lua interpreter runs on tty4, the UDP
echo service on tty2, and `ping` (`user/src/ping.c`, a raw ICMP socket via
`socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)`) on tty3. Processes cannot rebind their stdio
to a different tty — the kernel rejects `dup2` of another tty onto stdin/stdout/stderr.

Run QEMU with host UDP forwarding:
```sh
qemu-system-x86_64 -m 4G -drive file=image.img -serial stdio \
  -device e1000,netdev=n0 \
  -netdev user,id=n0,hostfwd=udp::10007-:7 \
  -object filter-dump,id=f1,netdev=n0,file=netdump.pcap
```

From the host, send a datagram to the forwarded port:
```sh
printf 'hello from host\n' | nc -u -w 1 127.0.0.1 10007
```

If your `nc` supports listen mode, you can also watch the echoed reply with a second terminal:
```sh
nc -u -l 10007
```

On the guest serial log you should see:
- `NET:` and `IPV4:` messages for the kernel-side interface bring-up
- `ping:` messages for the raw-socket ICMP echo round trips
- an `RX a.b.c.d:src -> 10.0.2.15:7` log line for the UDP datagram

## todo:
- [x] 32 bit protected mode
- [x] cpu exceptions
- [x] individual hardware interrupts
- [x] a ps/2 keyboard
- [x] vga driver
- [x] reading ata drives
- [x] rs232 interface
- [x] the intel 8259 PIC
- [x] the intel 8253 PIT
- [x] the mbr partitioning scheme
- [x] fat 16 as the filesystem
- [x] enable x87 fpu
- [x] memory allocator
- [x] elf loader
- [x] paging
- [ ] libc
- [x] implement more demos to show syscalls working

# Features

## Screenshots
Oct 20, 2025:
![mboot running in qemu](screenshots/mboot_cube_time.png)

Aug 6, 2026:
![mboot running in qemu](screenshots/mboot_init.png)