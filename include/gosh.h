/**
 * @file gosh.h
 * @author Nostalgia3
 * @brief the standard API for all applications/drivers
 * @version 0.1
 * @date 2024-12-28
 * 
 */
#ifndef GOSH_H
#define GOSH_H

#include "types.h"

/******************************* Documentation *********************************

[Devices]

BRIEF:
    Devices are the way GaOS abstracts actual hardware. There are predefined
functions for interacting with specific types of Devices, meaning no user
program should have to interact directly with the majority of hardware.

USERS:
    Devices, although usable by user programs, isn't recommended for most
purposes. This is because functions like `getc()` are much easier to implement
programs with. However, if you do desire, devices can be obtained by searching
for them in the global device list.

DRIVERS:
    Devices are created by drivers through `set_device()`, and stored in the
global device list, accessible through `get_device()`. Most devices have
functions that are called by both the kernel and users. An example is the
Keyboard's `read_fifo()`, which contains scancodes

[Global Device Table]

BRIEF:
    The gdevt is a list of connected devices, as well as drivers.

DRIVERS:
    The actual implmentation of the gdevt shouldn't matter too much, as
    `k_add_dev()`

[Driver]

BRIEF:
    Drivers, in GaOS, are modules that abstract hardware.

*******************************************************************************/

typedef struct _Driver {
    // The interrupts the driver listens to
    u32 r_active_ints[8];
    // The identifier of the driver (used for creating devices)
    size_t r_id;
    // The ids of the devices owned by the driver
    u32 r_devices[8];
    
    // Called when the driver is created (the system starts, in most cases)
    void (*DriverEntry)(u32 driver_id);
    // Called when a registered interrupt is called
    void (*DriverInt)(u32 driver_id, u8 i);
    // Called when the driver is destroyed (the system closes, in most cases)
    void (*DriverEnd)();
} Driver;

#define LOOPBACK_KEYBOARD_DEVICE 0

enum Key {
    K_None, K_Back, K_Tab, K_Enter, K_Return, K_CapsLock, K_Escape, K_Space,
    K_PageUp, K_PageDown, K_End, K_Home, K_Left, K_Up, K_Right, K_Down, K_Insert,
    K_Delete, K_D0, K_D1, K_D2, K_D3, K_D4, K_D5, K_D6, K_D7, K_D8, K_D9, K_A,
    K_B, K_C, K_D, K_E, K_F, K_G, K_H, K_I, K_J, K_K, K_L, K_M, K_N, K_O, K_P,
    K_Q, K_R, K_S, K_T, K_U, K_V, K_W, K_X, K_Y, K_Z, K_F1, K_F2, K_F3, K_F4,
    K_F5, K_F6, K_F7, K_F8, K_F9, K_F10, K_F11, K_F12, K_LeftShift, K_RightShift,
    K_LeftCtrl, K_RightCtrl, K_LeftAlt, K_RightAlt,
};

enum DeviceType {
    DEV_UNKNOWN,
    DEV_KEYBOARD,
    DEV_MOUSE,
    DEV_NETWORK,
    DEV_AUDIO,
    DEV_TERMINAL
};

typedef struct _Device {
    /* The generic type of device */
    enum DeviceType type;
    /* The code given to the device */
    u32 code;
    /* The unique ID of the device, given by the kernel */
    u32 id;

    void* data;
} Device;

typedef struct _KeyboardDeviceData {
    // Get the held state of a key
    bool (*get_key_status)(enum Key status);
    // Read a scancode from the FIFO
    u8 (*read_fifo)();
} KeyboardDeviceData;

// Load a driver into the system
void load_driver(Driver* driver);

// Unload a driver from the system (NULL is passable)
void unload_driver(Driver *driver);

// Add a device to the global device table, where the id is either
// a process id, or a driver id, then returning a pointer to the device
Device* k_add_dev(u32 id, enum DeviceType type, u32 code);

/* Cause a kernel panic, halting all operations on the CPU */
void kpanic();

#endif