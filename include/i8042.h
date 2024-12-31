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

#include <gosh.h>
#include "types.h"

#define CON_SYSTEM_RESET    1
#define CON_A20_GATE        2
#define CON_SECOND_PS2_CLK  4
#define CON_SECOND_PS2_DATA 8
// Output buffer full with byte from first PS/2 port (connected to IRQ1)
#define CON_OUT_BUFF_PS2_1  16
// 	Output buffer full with byte from second PS/2 port (connected to IRQ12, only if 2 PS/2 ports supported)
#define CON_OUT_BUFF_PS2_2  32
#define CON_FIRST_PS2_CLK   64
#define CON_FIRST_PS2_DATA  128

// This isn't all of them but I
// basically just cherry picked
// the ones I care about
enum I8042Commands {
    COM_READ_B0             = 0x20,
    COM_DISABLE_SECOND_PS2  = 0xA7,
    COM_ENABLE_SECOND_PS2   = 0xA8,
    COM_TEST_SECOND_PS2     = 0xA9,
    COM_TEST_PS2_CONTROLLER = 0xAA,
    COM_TEST_FIRST_PS2      = 0xAB,
    COM_DISABLE_FIRST_PS2   = 0xAD,
    COM_ENABLE_FIRST_PS2    = 0xAE,
    COM_READ_CONTROLLER_OUT = 0xD0,
    COM_WRITE_CONTROLLER_OUT= 0xD1,
    COM_WRITE_TO_SECOND_PS2 = 0xD4,
    COM_WRITE_CONFIG        = 0x60,
    COM_RESET_CPU           = 0xFE
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


I8042_Status    i8042_get_status();

void            i8042_set_config(I8042_Config config);
I8042_Config    i8042_get_config();

void            i8042_send_cont_comm(u8 byte);

void            i8042_send_byte(u8 byte);
u8              i8042_get_byte();

void I8042_DriverEntry(Device *dev);
void I8042_DriverInt(Device *dev, u8 int_id);

#endif