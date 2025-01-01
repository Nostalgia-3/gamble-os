#include "vga.h"
#include <memory.h>

static u16 cursor_x = 0;
static u16 cursor_y = 0;

u16 get_bios_area_hardware() {
    return *(const u16*)0x410;
}

// Get the BIOS video type
enum VideoType get_bios_video_type() {
    return (enum VideoType) (get_bios_area_hardware() & 0x30);
}

void clear(u8 c, TextAttribute attr) {
    for(u32 i=0;i<80*25*2;i+=2) {
        *(u8*)(i+0xB8000) = c;
        *(TextAttribute*)(i+0xB8000+1) = attr;
    }
}

u16 write(u8* st, u8 x, u8 y) {
    u8 c = *st;
    u32 i = 0;

    while(c != '\0') {
        *(u8*)(0xB8000+(i+x+y*VGA_WIDTH)*2) = c;
        i++;
        c = *(st+i);
    }

    return i;
}

void set_cursor(u16 x, u16 y) {
    u16 pos = y * VGA_WIDTH + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8) (pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8) ((pos >> 8) & 0xFF));
    cursor_x = x;
    cursor_y = y;
}

void move_cursor(i16 x, i16 y) {
    set_cursor(cursor_x+x, cursor_y+y);
}

void putc_raw(u8 c) {
    if(c == '\n') {
        cursor_x = 0;
        cursor_y++;
        if(cursor_y > VGA_HEIGHT) {
            scroll_down((TextAttribute) { .fg = VGA_WHITE, .bg = VGA_BLACK });
            cursor_y--;
        }
    } else {
        *(u8*)(0xB8000+(cursor_x+cursor_y*VGA_WIDTH)*2) = c;
        cursor_x++;
        if(cursor_x > VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }
}

void puts(u8* st) {
    u8 c = *st;
    u32 i = 0;

    while(c != '\0') {
        putc_raw(c);
        i++;
        c = *(st+i);
    }

    set_cursor(cursor_x, cursor_y);
}

void putc(u8 c) {
    putc_raw(c);
    set_cursor(cursor_x, cursor_y);
}

#include <str.h>
void scroll_down() {
    u32 line_width = VGA_WIDTH*2;

    for(u32 i=1;i<25;i++) {
        // i = 1, dest = 0, src = 1
        // i = 2, dest = 1, src = 2
        memcpy((u8*)0xB8000+line_width*(i-1), (u8*)0xB8000+line_width*i, line_width);
    }
    memset((u8*)0xB8000+line_width*(VGA_HEIGHT-1), 0, line_width);
}

void set_attr(u8 x, u8 y, u8 w, TextAttribute attr) {
    for(int i=0;i<w*2;i+=2) {
        *(volatile u8*)(0xB8000+(x+i+1)+(VGA_WIDTH*y*2)) = (attr.h << 7) | (attr.bg << 4) | attr.fg;
    }
}

u16 get_cursor_x() {
    return cursor_x;
}

u16 get_cursor_y() {
    return cursor_y;
}