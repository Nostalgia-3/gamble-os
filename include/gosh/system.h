#ifndef GOSH_SYSTEM_H
#define GOSH_SYSTEM_H

#include <types.h>
#include <gosh/common.h>

// Waits some amount of milliseconds. This is blocking
void    wait(u32 ms);

// The ID used when the kernel creates devices and such
#define KERNEL_ID 0x8008

// Output a debug character to the serial port
int initialize_serial();
void putc_dbg(u8 c);
void puts_dbg(const char *st);
// Stop writing debugging to stdout
void end_dbg_stdout();

// Process interrupts for connected devices that care about them
void k_handle_int(u8 int_id);

// Play a sound through the PC speaker
void play_sound(u32 nFrequence);

// Stop playing a sound on the PC speaker
void nosound();

// Reset the CPU
void reset_cpu();

#endif