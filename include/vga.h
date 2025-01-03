#ifndef VGA_H
#define VGA_H

#include <gosh.h>
#include "types.h"

void VGA_DriverEntry(Device *dev);

// #define VGA_WIDTH   80
// #define VGA_HEIGHT  25

// enum VideoType {
//     VIDEO_TYPE_NONE     = 0x00,
//     VIDEO_TYPE_COLOR    = 0x20,
//     VIDEO_TYPE_MONO     = 0x30
// };

// enum Color {
//     VGA_BLACK,
//     VGA_BLUE,
//     VGA_GREEN,
//     VGA_CYAN,
//     VGA_RED,
//     VGA_PURPLE,
//     VGA_BROWN,
//     VGA_GRAY,
//     VGA_DGRAY,
//     VGA_LBLUE,
//     VGA_LGREEN,
//     VGA_LCYAN,
//     VGA_LRED,
//     VGA_LPURPLE,
//     VGA_YELLOW,
//     VGA_WHITE
// };

// typedef struct _TextAttribute {
//     u8 fg : 4;
//     u8 bg : 3;
//     u8 h : 1;
// } TextAttribute;

// u16 get_bios_area_hardware();
// enum VideoType get_bios_video_type();

// // Move the cursor relative to the current location
// void move_cursor(i16 x, i16 y);

// u16 get_cursor_x();
// u16 get_cursor_y();

// void set_cursor(u16 x, u16 y);

// void clear(u8 c, TextAttribute attr);
// u16  write(u8 *st, u8 x, u8 y);

// // Set a horizontal line starting from (x, y) and going right
// void set_attr(u8 x, u8 y, u8 w, TextAttribute attr);

// // Put a single character on the screen, incrementing the text cursor
// void putc(u8 c);
// // Put a string on the screen, incrementing the text cursor
// void puts(u8* st);

// // Scroll down one line
// void scroll_down();

#endif