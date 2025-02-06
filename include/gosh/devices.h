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

// enum GDEVT_Errors {
//     GDEVT_FAILED_KBD    = 1,
//     GDEVT_FAILED_VT     = 2,
//     GDEVT_FAILED_VT_DATA= 3,
//     GDEVT_FAILED_ALLOC  = 4
// };

// Initialize the global device table. Don't call this unless you know what
// you're doing. Returns 0 if successful, otherwise an error code
// int _init_gdevt();

// // Get the global device table.
// Device* get_gdevt();

// // Get the total number of devices (used and unused) in the global device table.
// u32 get_gdevt_len();

// // Get the number of used devices in the global device table.
// u32 get_used_devices();

// // Add a device to the global device table, where the id is either
// // a process id, or a driver id, then returning a pointer to the device
// Device* k_add_dev(u32 owner, enum DeviceType dev, u32 code);

// // Get a device by querying the owner and type
// Device* k_get_device_by_owner(u32 owner, enum DeviceType type);

// // Get a device by the ID and type
// Device* k_get_device_by_id(u32 id, enum DeviceType type);

// Device* k_get_device_by_type(enum DeviceType type);

#endif//GOSH_DEVICE_H