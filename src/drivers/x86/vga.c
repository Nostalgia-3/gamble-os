#include <gosh/gosh.h>
#include <str.h>
#include <memory.h>
#include <port.h>
#include "drivers/x86/vga.h"

// black, red, green, yellow, blue, magenta, cyan, white, reset
static u8   ansi_to_text[]  = { 0x0, 0x4, 0x2, 0x6, 0x1, 0x5, 0x3, 0x7 };
static u8   bansi_to_text[] = { 0x8, 0xC, 0xA, 0xE, 0x9, 0xD, 0xB, 0xF };
static u16  screen_cursor   = 0;
static u8   attributes      = 7;
static bool in_ansi         = FALSE;
static u8   ansi_buf[64]    = {};
static u32  ansi_index      = 0;

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

int VGA_DriverEntry(Device *dev) {
    // Initialize VGA at linear 320x200 @ 256 color mode (this is currently done
    // in BIOS because I'm bad at this)
    // set_mode((u8*)MODE_320_200);

    u32 mode = (u32) ((Driver*)dev->data)->data;

    // Add a framebuffer device
    Device *fb = k_add_dev(dev->id, DEV_FRAMEBUFFER, 0);
    if(fb == NULL)          return DRIVER_FAILED;
    if(fb->data == NULL)    return DRIVER_FAILED;
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

    return DRIVER_SUCCESS;
}

void vga_set_cursor(u16 x, u16 y) {
    u16 pos = y * 80 + x;
    if(pos > 0);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8) (pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8) ((pos >> 8) & 0xFF));
}

void vga_scroll_down() {    
    u32 line_width = (80*2);
    for(u32 i=1;i<25;i++) {
        memcpy((u8*)0xB8000+line_width*(i-1), (u8*)0xB8000+160*i, 160);
    }
    memset((u8*)0xB8000+line_width*(25-1), 0, 160);

    screen_cursor-=80;
}

u16  vga_get_cursor_x() {
    return (screen_cursor % 80);
}

u16  vga_get_cursor_y() {
    return (screen_cursor - (screen_cursor % 80))/80;
}

#include <gosh/pipe.h>
#include <str.h>
void vga_write(char *st, size_t len) {
    for(int i=0;i<len;i++) {
        if(st[i] == '\0') break;
        if(in_ansi) {
            ansi_buf[ansi_index++] = st[i];

            if(is_letter(st[i])) {
                in_ansi = FALSE;

                // I *really* hate doing stuff in C
                if(ansi_buf[0] != '[') {
                    // not good :(
                } else if(ansi_buf[2] == 'm' && is_digit(ansi_buf[1])) {
                    switch(ansi_buf[1]-'0') {
                        case 0: attributes = ansi_to_text[sizeof(ansi_to_text)-1]; break;
                        case 5: attributes |= (1<<7); break;
                    }
                } else if(
                    ansi_buf[3] == 'm' &&
                    is_digit(ansi_buf[1]) &&
                    is_digit(ansi_buf[2])
                ) {
                    u8 com = (ansi_buf[1] - '0');
                    u8 dat = (ansi_buf[2] - '0');
                    u16 n = ((ansi_buf[1] - '0') * 10) + (ansi_buf[2] - '0');
                    if(com == 3)        attributes = (attributes & 0xF0) | ansi_to_text[dat];
                    else if(com == 4)   attributes = (attributes & 0x7F) | (ansi_to_text[dat]<<4);
                    else if(com == 9)   attributes = (attributes & 0xF0) | bansi_to_text[dat];
                } else if(ansi_buf[2] == 'J') {
                    if(ansi_buf[1] == '2') {
                        for(int x=0;x<80*25;x++) {
                            *(u8*)(0xB8000+x*2) = '\0';
                            *(u8*)(0xB8000+x*2+1) = attributes;
                        }
                        screen_cursor = 0;
                    }
                }

                memset(ansi_buf, 0, ansi_index);
                ansi_index = 0;
            }
        } else {
            if(st[i] == '\x1b') {
                in_ansi = TRUE;
            } else if(st[i] == '\n') {
                screen_cursor += 80 - (screen_cursor % 80);
            } else if(st[i] == '\t') {
                screen_cursor += 8 - (screen_cursor % 8);
            } else if(st[i] == '\b') {
                screen_cursor--;
                *(u8*)(0xB8000+screen_cursor*2) = '\0';
                *(u8*)(0xB8000+screen_cursor*2+1) = attributes;
            } else if(st[i] != '\r') {
                *(u8*)(0xB8000+screen_cursor*2) = st[i];
                *(u8*)(0xB8000+screen_cursor*2+1) = attributes;
                if(screen_cursor > 80*25) vga_scroll_down();
                else screen_cursor++;
            }
        }
    }

    vga_set_cursor(vga_get_cursor_x(), vga_get_cursor_y());
}