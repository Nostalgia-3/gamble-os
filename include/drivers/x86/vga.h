#ifndef VGA_H
#define VGA_H

#include <gosh/gosh.h>
#include "types.h"

#include <multiboot.h>

void set_vga_mbd(multiboot_info_t *mbd);

module_t get_vga_module(multiboot_info_t *mbd);

// int VGA_DriverEntry(Device *dev);

ssize_t vga_write(const void *buf, size_t len, off_t *offset);
void vga_scroll_down();
void vga_set_cursor(u16 x, u16 y);
u16  vga_get_cursor_x();
u16  vga_get_cursor_y();

#endif