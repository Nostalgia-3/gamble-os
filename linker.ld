ENTRY(_load)
OUTPUT_FORMAT(elf32-i386)
/* OUTPUT_FORMAT(binary) */

STARTUP(build/a_kernel-entry.asm.o)

SECTIONS
{
	. = 0x01000000;

	.entry.text BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
		*(.entry.text)
	}

	.entry.data BLOCK(4K) : ALIGN(4K)
	{
		*(.entry.data)
	}

	.entry.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.entry.rodata)
	}

	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.text)
	}

	/* Read-only data. */
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	/* Read-write data (initialized) */
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}

	/* Read-write data (uninitialized) and stack */
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}
}
