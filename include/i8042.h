/**
 * @file i8042.h
 * @author Nostalgia3
 * @brief The PS/2 controller, among other things
 * @version 0.1
 * @date 2024-12-26
 * 
 */
#ifndef I8042_H
#define I8042_H

#include "types.h"

#define I8042_DATA_PORT 0x60
#define I8042_STAT_REG  0x64
#define I8042_COMM_REG  0x64

// This isn't all of them but I
// basically just cherry picked
// the ones I care about
enum I8042Commands {
    READ_B0             = 0x20,
    DISABLE_SECOND_PS2  = 0xA7,
    ENABLE_SECOND_PS2   = 0xA8,
    TEST_SECOND_PS2     = 0xA9,
    TEST_PS2_CONTROLLER = 0xAA,
    TEST_FIRST_PS2      = 0xAB,
    DISABLE_FIRST_PS2   = 0xAD,
    ENABLE_FIRST_PS2    = 0xAE,
    READ_CONTROLLER_OUT = 0xD0,
    WRITE_CONTROLLER_OUT= 0xD1,
    WRITE_TO_SECOND_PS2 = 0xD4,
    RESET_CPU           = 0xFE
};

typedef struct _I8042_Status {
    u8 output_buff_state    : 1;
    u8 input_buff_state     : 1;
    u8 system_flag          : 1;
    u8 comm_or_data         : 1;
    u8 keyboard_lock        : 1; // Unused
    u8 recieve_timeout      : 1; // Unused
    u8 timeout_error        : 1;
    u8 parity_error         : 1;
} I8042_Status;

typedef struct _I8042_Config {
    u8 first_ps2_int        : 1;
    u8 second_ps2_int       : 1;
    u8 system_flag          : 1;
    u8                      : 1;
    u8 first_ps2_clock      : 1;
    u8 second_ps2_clock     : 1;
    u8 first_ps2_trans      : 1;
    u8                      : 1;
} I8042_Config;


I8042_Status    I8042_get_status();

void            I8042_set_config(I8042_Config config);
I8042_Config    I8042_get_config();

void            I8042_send_controller_byte(u8 byte);
u8              I8042_get_controller_byte();

#endif