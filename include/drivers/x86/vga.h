#ifndef VGA_H
#define VGA_H

#include <gosh/gosh.h>
#include "types.h"

void VGA_DriverEntry(Device *dev);

void vga_set_cursor(u16 x, u16 y);

#endif