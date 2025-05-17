ENTRY(_start)
OUTPUT_FORMAT(binary)

STARTUP(build/gstd.o)

SECTIONS
{
    . = 0x1000;

    .text :
	{
		*(.text.start)
		*(.text)
	}

	/* Read-only data. */
	.rodata :
	{
		*(.rodata)
	}

	/* Read-write data (initialized) */
	.data :
	{
		*(.data)
	}

	/* Read-write data (uninitialized) and stack */
	.bss :
	{
		*(COMMON)
		*(.bss)
	}
}