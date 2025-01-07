#include <gosh.h>
#include <memory.h>
#include <port.h>
#include "drivers/x86/vga.h"

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

    u32 mode = (u32) ((Driver*)dev->data)->data;

    // Add a framebuffer device
    Device *fb = k_add_dev(dev->id, DEV_FRAMEBUFFER, 0);
    if(fb == NULL) return;
    if(fb->data == NULL) return;
    FBDeviceData* data = fb->data;

    data->buf_update = NULL;

    switch(mode) {
        case 0:
            data->w = 80; data->h = 25;
            data->format = PixelFormat_TEXT_640x480;
            data->buffer = (void*)0xB8000;
        break;

        case 1:
            data->w = 320; data->h = 200;
            data->format = PixelFormat_LINEAR_I8;
            data->buffer = (void*)0xA0000;
        break;

        case 2:
            data->w = 640;
            data->h = 480;
            data->format = PixelFormat_PLANAR_I4;
            data->buffer = (void*)0xA0000;
        break;
    }
}

void vga_set_cursor(u16 x, u16 y) {
    u16 pos = y * 80 + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8) (pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8) ((pos >> 8) & 0xFF));
}