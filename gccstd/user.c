#include <gosh.h>

int TestProgram() {
    if(!fb_is_available())
        return 1;
    u32 fb = fb_get(RESOLUTION); // get the best framebuffer resolution-wise
    FBInfo info = fb_get_info(fb);
    if(info.format == PixelFormat_RGB_I8) {
        fb_draw_pixel(fb, 0, 0, 1);
    }
}