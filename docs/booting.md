# Booting

The VBR is defined in `boot/s_vbr.asm`. It is the first bit of code run under
legacy BIOS booting. This loads the `boot/ssbl/**` program at physical address
`0x1000`, where it parses the bootable medium and searches for the kernel binary
file. If SSBL can find the kernel binary, it writes it at `0x00100000`, then
jumps to it.