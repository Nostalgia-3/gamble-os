#ifndef GOSH_FB_H
#define GOSH_FB_H

#include <types.h>
#include <gosh/common.h>

typedef struct _fb_data_t {
    /* The size of the framebuffer */
    u16 w, h;
    /* Bits per pixel */
    u8 bpp;

    void *buffer;
} fb_data_t;

// Get the best framebuffer based on preference, returning the gdevt index of
// the framebuffer.
// Device* fb_get(enum FBPreference);

// Return the information of the framebuffer
// FBInfo  fb_get_info(Device *dev);

// Draw a single pixel at (x, y) with the best available match for the color
// sent
// void    fb_draw_pixel(Device* fb, u16 x, u16 y, u32 color);
// void    fb_draw_rect(Device* fb, u16 x, u16 y, u16 w, u16 h, u32 color);
// void    fb_draw_char(Device *fb, u16 x, u16 y, u8 c, u32 color);
// void    fb_clear(Device* fb, u32 color);

#endif