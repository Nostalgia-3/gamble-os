#ifndef GOSH_FB_H
#define GOSH_FB_H

#include <types.h>
#include <gosh/common.h>

enum FBPreference {
    RESOLUTION,
    COLORS
};

enum FB_PixelFormat {
    /* A 32-bit RGB (8 bits per color, 1 extra byte) */
    PixelFormat_RGBX_32,
    /* An 8-bit index to a palette (ex. linear 320x200) */
    PixelFormat_LINEAR_I8,
    /* A 4-bit index to a palette (ex. planar 640x480) */
    PixelFormat_PLANAR_I4,
    /* Technically not a pixel format; used for compatibility reasons */
    PixelFormat_TEXT_640x480
};

typedef struct _FBDeviceData {
    /* The width of the framebuffer */
    u16 w;
    /* The height of the framebuffer */
    u16 h;
    /* Format of pixels (16-color or 256-color, in most cases) */
    enum FB_PixelFormat format;

    void *buffer;

    void (*buf_update)();
} FBDeviceData;

typedef FBDeviceData FBInfo;

// Framebuffers are the lowest possible abstraction (directly in front of
// drivers) for video graphics. In most cases, programs should use the future
// "desktop" library for rendering.

// Returns TRUE if a framebuffer is available, otherwise FALSE. This should be
// used as a check before calling other fb_* functions.
bool    fb_is_available();

// Get the best framebuffer based on preference, returning the gdevt index of
// the framebuffer.
Device* fb_get(enum FBPreference);

// Return the information of the framebuffer
FBInfo  fb_get_info(Device *dev);

// Draw a single pixel at (x, y) with the best available match for the color
// sent
void    fb_draw_pixel(Device* fb, u16 x, u16 y, u32 color);
void    fb_draw_rect(Device* fb, u16 x, u16 y, u16 w, u16 h, u32 color);
void    fb_draw_char(Device *fb, u16 x, u16 y, u8 c, u32 color);
void    fb_clear(Device* fb, u32 color);

#endif