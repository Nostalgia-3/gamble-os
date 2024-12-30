#ifndef GENERIC_H
#define GENERIC_H

#include "../types.h"

#include <stdio.h>

#include "../gosh.h"
#include "../i8042.h"
#include "../port.h"

void I8042_send_ack(u8 com) {
    u8 resp;
    i8042_send_byte(com);
    do {
        resp = inb(0x60);
        if(resp == 0xFE) {
            i8042_send_byte(com);
        }
    } while(resp != 0xFA);
}

void I8042_DriverEntry(u32 id) {
    // Disable PS/2 ports
    i8042_send_controller_byte(COM_DISABLE_FIRST_PS2);
    i8042_send_controller_byte(COM_DISABLE_SECOND_PS2);

    // Flush output buffer
    while(i8042_get_status().output_buff_state != 0) {
        i8042_get_byte();
    };

    // Enable interrupt and clock, disabling translation
    I8042_Config conf = i8042_get_config();
    conf.first_ps2_int  = 1;
    conf.first_ps2_trans= 0;
    conf.first_ps2_clock= 1;
    i8042_set_config(conf);

    i8042_send_controller_byte(COM_TEST_PS2_CONTROLLER);
    u8 resp = i8042_get_byte();
    if(resp != 0x55) {
        // this really shouldn't cause a kernel panic, but
        // for testing purposes it shouldn't matter
        printf("I8042 self-test failed (expected 0x55, got 0x%02X)\n", resp);
        kpanic();
    }

    i8042_send_controller_byte(COM_TEST_FIRST_PS2);
    resp = i8042_get_byte();
    if(resp != 0x00) {
        // this really shouldn't cause a kernel panic, but
        // for testing purposes it shouldn't matter
        printf("I8042 PS/2 port #1 failed (expected 0x55, got 0x%02X)\n", resp);
        kpanic();
    }

    i8042_send_controller_byte(COM_ENABLE_FIRST_PS2); // enable device
    i8042_send_byte(0xFF); // reset device

    I8042_send_ack(0xF5); // Disable scanning

    I8042_send_ack(0xF2); // Identify the device
    u8 device = i8042_get_byte();

    Device* dev = k_add_dev(id, DeviceType::DEV_KEYBOARD, device);

    // Failed to add a device
    if(dev == NULL) {
        printf("Failed to add the PS/2 keyboard to the gdevt\n");
        kpanic();
    }
}

void I8042_DriverInt(u32 th, u8 i) {

}

const Driver i8042 = { .DriverEntry = I8042_DriverEntry, .DriverInt = I8042_DriverInt };

#endif