#ifndef GOSH_DEVICE_H
#define GOSH_DEVICE_H

#include <types.h>
#include <gosh/common.h>

typedef struct _device_t {
    /* the name of the device; setting this does nothing */
    const char *name;
    u32 owner;
    off_t off;

    // Called when the device is opened
    void    (*open)();
    // Called when the device is closed
    void    (*close)();
    // Called when the device is written to
    ssize_t (*write)(const void *buf, size_t len, off_t *offset);
    // Called when the device is read to
    ssize_t (*read)(void *buf, size_t len, off_t *offset);
    // Called when the device is `ioctl()`-ed to
    int     (*ioctl)(int fd, int op, void *data);
    
    /* The type of this should be inferred based on the device type */
    void* data;
} device_t;

void _setup_device_manager();

// Create a device with the name and device, returning -1 if an error occured
int register_device(const char *pathname, device_t *dev);
device_t *find_device(const char *pathnam);

// Return a list of devices
device_t **get_devices();
u32 get_total_devices();

void delete_device(const char *name);

#endif//GOSH_DEVICE_H