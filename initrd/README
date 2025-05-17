# GambleOS

GambleOS is an operating system built for Pentium III-era hardware, meant for learning more about kernel development and low-level programming in C.

## Building

Building is done via a Deno script (`make.ts`) and requires the following programs:

- Deno (2.2.12)
- i686-elf build tools (works with i686-elf-gcc 13.2.0)
- nasm (2.16.03)
- qemu-system-i386 (9.2.3, only needed for the emulate task)
- grub-mkrescue (2.06, for making the iso)

A list of subcommands can be found below:

- `compile`: compile the operating system into a grub ISO image for i386
- `initrd`: generate an initrd archive using the initrd/**
- `run`: run the process