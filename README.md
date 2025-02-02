# Gamble OS

GambleOS is the supreme x86_32 kernel.

## Building

GambleOS needs (relatively) modern versions of 32-bit GNU build tools (`gcc`, `ld`), NASM, Deno, `grub-mkrescue`, and optionally `qemu-system-i386`. Using deno, run `./make.ts --help` for further information.

## TODOs

- [ ] A better kernel shell
  - [ ] A functioning cursor (left & right arrow key support)
  - [ ] Command history
- [ ] Real time clock support
- [ ] Functioning memory allocation
- [ ] UEFI bootloader
- [ ] USB Host Controller drivers
  - [ ] XHCI (USB 3.0/2.0/1.0 Host Controller)
  - [ ] EHCI (USB 2.0 Host Controller)