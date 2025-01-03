#include <gosh.h>
#include <memory.h>
#include <port.h>
#include "vga.h"

static u16 cursor_x = 0;
static u16 cursor_y = 0;

#define MMIO_BASE 0xA0000

const u8 MODE_320_200[29] = {
    0x41, 0x00, 0x0F, 0x00, 0x00, 0x63, 0x01, 0x00, 0x0E, 0x40, 0x05, 0x5F,
    0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 0x00, 0x41, 0x9C, 0x8E, 0x8F,
    0x28, 0x40, 0x96, 0xB9, 0xA3
};

void out_first(u8 index, u8 data) {
    outb(0x3C0, index);
    outb(0x3C0, data);
}

void out_indexed_port(u16 port, u8 index, u8 data) {
    outb(port, index);
    io_wait();
    outb(port+1, data);
}

// This is broken, and I'm not sure why :/
void set_mode(u8 bytes[29]) {
    // go to index state
    inb(0x3C0);

    out_first(0x10, bytes[0]);
    out_first(0x11, bytes[1]);
    out_first(0x12, bytes[2]);
    out_first(0x13, bytes[3]);
    out_first(0x14, bytes[4]);
    outb(0x3C2, bytes[5]);
    out_indexed_port(0x3C4, 0x01, bytes[6]);
    out_indexed_port(0x3C4, 0x03, bytes[7]);
    out_indexed_port(0x3C4, 0x04, bytes[8]);
    out_indexed_port(0x3CE, 0x05, bytes[9]);
    out_indexed_port(0x3CE, 0x06, bytes[10]);
    out_indexed_port(0x3D4, 0x01, bytes[12]);
    out_indexed_port(0x3D4, 0x02, bytes[13]);
    out_indexed_port(0x3D4, 0x03, bytes[14]);
    out_indexed_port(0x3D4, 0x04, bytes[15]);
    out_indexed_port(0x3D4, 0x05, bytes[16]);
    out_indexed_port(0x3D4, 0x06, bytes[17]);
    out_indexed_port(0x3D4, 0x07, bytes[18]);
    out_indexed_port(0x3D4, 0x08, bytes[19]);
    out_indexed_port(0x3D4, 0x09, bytes[20]);
    out_indexed_port(0x3D4, 0x10, bytes[21]);
    out_indexed_port(0x3D4, 0x11, bytes[22]);
    out_indexed_port(0x3D4, 0x12, bytes[23]);
    out_indexed_port(0x3D4, 0x13, bytes[24]);
    out_indexed_port(0x3D4, 0x14, bytes[25]);
    out_indexed_port(0x3D4, 0x15, bytes[26]);
    out_indexed_port(0x3D4, 0x16, bytes[27]);
    out_indexed_port(0x3D4, 0x17, bytes[28]);
}

void VGA_DriverEntry(Device *dev) {
    // Initialize VGA at linear 320x200 @ 256 color mode (this is currently done
    // in BIOS because I'm bad at this)
    // set_mode((u8*)MODE_320_200);

    // Add a framebuffer device
    Device *fb = k_add_dev(dev->id, DEV_FRAMEBUFFER, 0);
    if(fb == NULL) return; // <-- this should give some notice that it failed
    if(fb->data == NULL) return;
    FBDeviceData* data = fb->data;

    // data->w = 640;
    // data->h = 480;
    // data->format = PixelFormat_RGB_I4;
    data->w = 320; data->h = 200;
    data->format = PixelFormat_LINEAR_I8;
    data->buffer = (void*)0xA0000;
    data->buf_update = NULL;
}

// u16 get_bios_area_hardware() {
//     return *(const u16*)0x410;
// }

// // Get the BIOS video type
// enum VideoType get_bios_video_type() {
//     return (enum VideoType) (get_bios_area_hardware() & 0x30);
// }

// void clear(u8 c, TextAttribute attr) {
//     for(u32 i=0;i<80*25*2;i+=2) {
//         *(u8*)(i+0xB8000) = c;
//         *(TextAttribute*)(i+0xB8000+1) = attr;
//     }
// }

// u16 write(u8* st, u8 x, u8 y) {
//     u8 c = *st;
//     u32 i = 0;

//     while(c != '\0') {
//         *(u8*)(0xB8000+(i+x+y*VGA_WIDTH)*2) = c;
//         i++;
//         c = *(st+i);
//     }

//     return i;
// }

// void set_cursor(u16 x, u16 y) {
//     u16 pos = y * VGA_WIDTH + x;
//     outb(0x3D4, 0x0F);
//     outb(0x3D5, (u8) (pos & 0xFF));
//     outb(0x3D4, 0x0E);
//     outb(0x3D5, (u8) ((pos >> 8) & 0xFF));
//     cursor_x = x;
//     cursor_y = y;
// }

// void move_cursor(i16 x, i16 y) {
//     set_cursor(cursor_x+x, cursor_y+y);
// }

// void putc_raw(u8 c) {
//     if(c == '\n') {
//         cursor_x = 0;
//         cursor_y++;
//         if(cursor_y > VGA_HEIGHT) {
//             scroll_down((TextAttribute) { .fg = VGA_WHITE, .bg = VGA_BLACK });
//             cursor_y--;
//         }
//     } else {
//         *(u8*)(0xB8000+(cursor_x+cursor_y*VGA_WIDTH)*2) = c;
//         cursor_x++;
//         if(cursor_x > VGA_WIDTH) {
//             cursor_x = 0;
//             cursor_y++;
//         }
//     }
// }

// void puts(u8* st) {
//     u8 c = *st;
//     u32 i = 0;

//     while(c != '\0') {
//         putc_raw(c);
//         i++;
//         c = *(st+i);
//     }

//     set_cursor(cursor_x, cursor_y);
// }

// void putc(u8 c) {
//     putc_raw(c);
//     set_cursor(cursor_x, cursor_y);
// }

// #include <str.h>
// void scroll_down() {
//     u32 line_width = VGA_WIDTH*2;

//     for(u32 i=1;i<25;i++) {
//         // i = 1, dest = 0, src = 1
//         // i = 2, dest = 1, src = 2
//         memcpy((u8*)0xB8000+line_width*(i-1), (u8*)0xB8000+line_width*i, line_width);
//     }
//     memset((u8*)0xB8000+line_width*(VGA_HEIGHT-1), 0, line_width);
// }

// void set_attr(u8 x, u8 y, u8 w, TextAttribute attr) {
//     for(int i=0;i<w*2;i+=2) {
//         *(volatile u8*)(0xB8000+(x+i+1)+(VGA_WIDTH*y*2)) = (attr.h << 7) | (attr.bg << 4) | attr.fg;
//     }
// }

// u16 get_cursor_x() {
//     return cursor_x;
// }

// u16 get_cursor_y() {
//     return cursor_y;
// }