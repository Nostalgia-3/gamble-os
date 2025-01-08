# Gamble OS

GaOS is the supreme 32-bit operating system (technically kernel).

## Building

GaOS needs at least GCC v12.2.0, LD v2.40, NASM v2.16.01, Deno v2.1.3, and optionally qemu-system-i386 v9.2.0 (although earlier versions of these programs will probably work). Once you have those installed, use the build tool (`make.ts`) to build it.

## Small To-Do List

Here are a few things that are possible

- [ ] A better system shell
  - [ ] A functioning cursor (left & right arrow key support)
  - [ ] Command history
- [ ] Real time clock support
- [ ] Functioning memory allocation
- [ ] UEFI bootloader
- [ ] USB Host Controller drivers
  - [ ] XHCI (USB 3.0/2.0/1.0 Host Controller)
  - [ ] EHCI (USB 2.0 Host Controller)
  - [ ] UHCI/OHCI (USB 1.0 Host Controllers)