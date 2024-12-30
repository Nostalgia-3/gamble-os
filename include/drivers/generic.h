#ifndef GENERIC_H
#define GENERIC_H

#include <stdio.h>

#include "types.h"
#include "gosh.h"
#include "i8042.h"
#include "port.h"

#endif

// i8042_send_controller_byte(COM_DISABLE_FIRST_PS2); // disable PS/2 port one
// i8042_send_controller_byte(COM_DISABLE_SECOND_PS2); // disable PS/2 port two
// // flush data FIFO
// while(i8042_get_status().output_buff_state != 0) {
//     i8042_get_byte();
// };
// I8042_Config conf = i8042_get_config();
// conf.first_ps2_int  = 0;
// conf.first_ps2_trans= 0;
// conf.first_ps2_clock= 1;
// i8042_set_config(conf);
// i8042_send_controller_byte(COM_TEST_PS2_CONTROLLER);
// u8 resp = i8042_get_byte();
// if(resp != 0x55) {
//     puts("I8042 self-test failed (expected 0x55, got 0x");
//     puts(itoa(resp, 16));
//     puts(")\n");
//     while(1);
// }
// // check for ps/2 device #1 (not doing allat because no idc about mouse rn)
// i8042_send_controller_byte(COM_TEST_FIRST_PS2);
// resp = i8042_get_byte();
// if(resp != 0x00) {
//     puts("I8042 PS/2 port #1 failed (expected 0x55, got 0x");
//     puts(itoa(resp, 16));
//     puts(")\n");
//     while(1);
// }
// i8042_send_controller_byte(COM_ENABLE_FIRST_PS2); // enable device
// i8042_send_byte(0xFF); // reset device
// ack_send(0xF5); // Disable Scanning
// ack_send(0xF2); // Identify
// u16 device = i8042_get_byte() << 8;
// if(device == 0xAB00) {
//     device |= inb(0x60);
// }
// ack_send(0xF4); // Enable Scanning
// puts("[%] Enabled PS/2 device #0 (device id = 0x");
// puts(itoa(device, 16));
// puts(")\n");