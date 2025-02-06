#ifndef GOSH_COMMON_H
#define GOSH_COMMON_H

#include <types.h>

// enum DeviceType {
//     // An unknown device -- this should be used for devices that don't conform
//     // to the rest of the generic device types
//     DEV_UNKNOWN,
//     // A keyboard that can collect FIFO
//     DEV_KEYBOARD,
//     // A mouse that records absolute and relative position data
//     DEV_MOUSE,
//     // A network device that can be controlled by writing and reading a buffer
//     DEV_NETWORK,
//     // An audio device with a buffer that can be written to
//     DEV_SPEAKER,
//     // An audio device that can be read to
//     DEV_MICROPHONE,
//     // A storage device that can be read and written to
//     DEV_DRIVE,
//     // A framebuffer for writing color data
//     DEV_FRAMEBUFFER
// };

/* Write a formatted string to kernel/stdout */
__attribute__ ((format (printf, 1, 2))) void kprintf(const char* format, ...);

void putc(char c);

/* Cause a kernel panic, halting all operations on the CPU */
void kpanic();

#endif