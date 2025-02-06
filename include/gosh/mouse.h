#ifndef GOSH_MOUSE_H
#define GOSH_MOUSE_H

#include <gosh/common.h>

typedef struct _mouse_data_t {
    u8 buttons_down;
    u16 x;
    u16 y;
    u8 id;
} mouse_data_t;

#endif//GOSH_MOUSE_H