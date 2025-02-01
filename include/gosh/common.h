#ifndef GOSH_COMMON_H
#define GOSH_COMMON_H

#include <types.h>

enum DeviceType {
    // An unknown device -- this should be used for devices that don't conform
    // to the rest of the generic device types
    DEV_UNKNOWN,
    // A keyboard that can collect FIFO
    DEV_KEYBOARD,
    // A mouse that records absolute and relative position data
    DEV_MOUSE,
    // A network device that can be controlled by writing and reading a buffer
    DEV_NETWORK,
    // An audio device with a buffer that can be written to
    DEV_SPEAKER,
    // An audio device that can be read to
    DEV_MICROPHONE,
    // A storage device that can be read and written to
    DEV_DRIVE,
    
    // A virtual terminal, with stdio/stdout FIFOs
    // DEV_VIRT_TERM,

    // A framebuffer for writing color data
    DEV_FRAMEBUFFER,
    // A driver
    DEV_DRIVER
};

typedef struct _Device {
    /* TRUE is the device is occupied */
    bool is_active;
    /* The generic type of device */
    enum DeviceType type;
    /* The code given to the device */
    u32 code;
    /* The unique ID of the device, given by the kernel */
    u32 id;
    /* The owner ID of the device */
    u32 owner;

    /* The type of this should be inferred based on the device type */
    void* data;
} Device;

/* Cause a kernel panic, halting all operations on the CPU */
void kpanic();

#endif