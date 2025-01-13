#pragma once

#include <types.h>
#include <gosh/common.h>

typedef enum _ConnType {
    CONN_PCI,
    CONN_USB
} ConnType;

typedef struct _Connection {
    ConnType type; // This is zero for PCI and one for USB
} Connection;

typedef struct _Driver {
    // The irqs the driver listens to
    u8 r_active_ints[32];
    // The identifier of the driver (used for creating devices)
    size_t r_id;
    // The null-terminated string for the name of a driver
    u8* name;
    // Generic data
    void *data;
    
    // Called when the driver is created (the system starts, in most cases).
    // The device passed is the driver psuedo-device
    void (*DriverEntry)(Device *dev);
    // Called when a registered interrupt is called
    void (*DriverInt)(Device *dev, u8 int_id);
    // Called when a PCI/USB device is connected
    void (*DriverConnection)(Device *driver, Connection conn);
    // Called when the driver is destroyed (the system closes, in most cases)
    void (*DriverEnd)();
} Driver;

// Load a driver into the system. Returns the ID
u32 load_driver(Driver* driver);

// Unload a driver from the system
void unload_driver(u32 id);

// Connect an interrupt to a driver
bool k_register_int(Driver *driver, u8 int_id);