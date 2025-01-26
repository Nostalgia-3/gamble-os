#ifndef GOSH_SYSTEM_H
#define GOSH_SYSTEM_H

#include <types.h>
#include <gosh/common.h>

typedef enum TaskType {
    // Task is called repeatedly
    TASK_TYPE_PERIODIC_INTERRUPT,
    // Task is called once after some amount of time
    TASK_TYPE_SINGLE_INTERRUPT
} TaskType_t;

// Waits some amount of milliseconds. This is blocking
void    wait(u32 ms);

// Create a task, with a type and millisecond, returning the ID of the task, or
// -1 if it failed to create a task
int     add_task(void (*handler), TaskType_t type, u32 ms);

// Free the task with the id passed
void    free_task(int id);

// The ID used when the kernel creates devices and such
#define KERNEL_ID 0x8008

// Output a debug character to the serial port
void initialize_serial();
void putc_dbg(u8 c);
void puts_dbg(const char *st);

// Process interrupts for connected devices that care about them
void k_handle_int(u8 int_id);

// Play a sound through the PC speaker
void play_sound(u32 nFrequence);

// Stop playing a sound on the PC speaker
void nosound();

// Reset the CPU
void reset_cpu();

#endif