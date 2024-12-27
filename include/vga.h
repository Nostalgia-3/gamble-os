#ifndef VGA_H
#define VGA_H

#include "types.h"

enum VideoType {
    VIDEO_TYPE_NONE     = 0x00,
    VIDEO_TYPE_COLOR    = 0x20,
    VIDEO_TYPE_MONO     = 0x30
};

enum Color {
    VGA_BLACK,
    VGA_BLUE,
    VGA_GREEN,
    VGA_CYAN,
    VGA_RED,
    VGA_PURPLE,
    VGA_BROWN,
    VGA_GRAY,
    VGA_DGRAY,
    VGA_LBLUE,
    VGA_LGREEN,
    VGA_LCYAN,
    VGA_LRED,
    VGA_LPURPLE,
    VGA_YELLOW,
    VGA_WHITE
};

typedef struct _TextAttribute {
    u8 fg : 4;
    u8 bg : 3;
    u8 h : 1;
} TextAttribute;

u16 get_bios_area_hardware();
enum VideoType get_bios_video_type();

void clear(u8 c, TextAttribute attr);
void drawc(u8 c, TextAttribute attr, u32 addr);
void write(u8 *st, u8 x, u8 y);

#endif