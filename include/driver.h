#ifndef DRIVER_H
#define DRIVER_H

#include "types.h"

typedef struct _Driver {
    // The interrupts the driver listens to
    u32 r_active_ints[8];
    // The identifier of the driver
    size_t r_id;
    
    // Called when the driver is created (the system starts, in most cases)
    void (*DriverEntry)();
    // Called when a registered interrupt is called
    void (*DriverInt)(u8);
    // Called when the driver is destroyed (the system closes, in most cases)
    void (*DriverEnd)();
} Driver;

// Load a driver into the system
void load_driver(Driver* driver);

// Unload a driver from the system (NULL is passable)
void unload_driver(Driver *driver);

#endif