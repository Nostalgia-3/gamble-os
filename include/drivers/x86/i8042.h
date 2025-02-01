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

#include <gosh/gosh.h>
#include "types.h"

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
    u8 _failed              : 1;
} I8042_Config;


// I8042_Status    i8042_get_status();

// void            i8042_set_config(I8042_Config config);
// I8042_Config    i8042_get_config();

// void            i8042_send_cont_comm(u8 byte);

// void            i8042_send_byte(u8 byte);
// u8              i8042_get_byte();

int I8042_DriverEntry(Device *dev);
void I8042_DriverInt(Device *dev, u8 int_id);

#endif