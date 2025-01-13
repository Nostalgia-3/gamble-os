#ifndef GOSH_MOUSE_H
#define GOSH_MOUSE_H

#include <gosh/common.h>

typedef struct _MouseDeviceData {
    u8 buttons_down;
    u16 x;
    u16 y;

    void (*update_handler)(Device *dev);
} MouseDeviceData;

#endif//GOSH_MOUSE_H