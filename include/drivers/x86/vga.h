#ifndef VGA_H
#define VGA_H

#include <gosh/gosh.h>
#include "types.h"

int VGA_DriverEntry(Device *dev);

void vga_write(char *st, size_t len);
void vga_scroll_down();
void vga_set_cursor(u16 x, u16 y);
u16  vga_get_cursor_x();
u16  vga_get_cursor_y();

#endif