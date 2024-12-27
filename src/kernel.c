#include <vga.h>

void _start() {
	TextAttribute norm;

	norm.bg = VGA_BLACK;
	norm.fg = VGA_WHITE;

	u8* st = "Hello, world!\0";

	clear(' ', norm);
	write(st, 0, 0);

	return;
}
