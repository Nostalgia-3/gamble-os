#ifndef GOSH_DEVICE_H
#define GOSH_DEVICE_H

#include <types.h>
#include <gosh/common.h>

enum GDEVT_Errors {
    GDEVT_FAILED_KBD    = 1,
    GDEVT_FAILED_VT     = 2,
    GDEVT_FAILED_VT_DATA= 3,
    GDEVT_FAILED_ALLOC  = 4
};

// Initialize the global device table. Don't call this unless you know what
// you're doing. Returns 0 if successful, otherwise an error code
int _init_gdevt();

// Get the global device table.
Device* get_gdevt();

// Get the total number of devices (used and unused) in the global device table.
u32 get_gdevt_len();

// Get the number of used devices in the global device table.
u32 get_used_devices();

// Add a device to the global device table, where the id is either
// a process id, or a driver id, then returning a pointer to the device
Device* k_add_dev(u32 owner, enum DeviceType dev, u32 code);

// Get a device by querying the owner and type
Device* k_get_device_by_owner(u32 owner, enum DeviceType type);

// Get a device by the ID and type
Device* k_get_device_by_id(u32 id, enum DeviceType type);

Device* k_get_device_by_type(enum DeviceType type);

#endif//GOSH_DEVICE_H