ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)
STARTUP(build/a_kernel-entry.o)

SECTIONS
{
    . = 0x1000;
  .text : ALIGN(CONSTANT(MAXPAGESIZE)) {
_TEXT_START_ = .;
    *(.text)
_TEXT_END_ = .;
  }

  .data : ALIGN(CONSTANT(MAXPAGESIZE)) {
_DATA_START_ = .;
    *(.data)
_DATA_END_ = .;
  }

  .rodata : ALIGN(CONSTANT(MAXPAGESIZE)) {
_RODATA_START_ = .;
    *(.rodata)
_RODATA_END_ = .;
  }

  .bss : ALIGN(CONSTANT(MAXPAGESIZE)) {
_BSS_START_ = .;
    *(.bss)
_BSS_END_ = .;
  }
}