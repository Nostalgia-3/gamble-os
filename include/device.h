#pragma once

#include <types.h>
#include <module.h>

typedef struct _device {
    module *owner;

    // Called when the device is opened
    void    (*open)();
    // Called when the device is closed
    void    (*close)();
    // Called when the device is written to
    ssize_t (*write)(const void *buf, size_t len, off_t *offset);
    // Called when the device is read to
    ssize_t (*read)(void *buf, size_t len, off_t *offset);
    // Called when the device is `ioctl()`-ed to
    int     (*ioctl)(int op, void *data);
} device;

int device_init();