#include <gosh.h>
#include <i8042.h>
#include <memory.h>
#include <port.h>
#include <vga.h>
#include <str.h>

I8042_Status i8042_get_status() {
    union
    {
        u8 b;
        I8042_Status st;
    } stat = { .b = inb(0x64) };

    return stat.st;
}

void i8042_set_config(I8042_Config config) {
    union
    {
        u8 b;
        I8042_Config conf;
    } conf = { .conf = config };

    while(i8042_get_status().input_buff_state == 1) continue;
    outb(0x64, COM_WRITE_CONFIG);
    i8042_send_byte(conf.b);
}

I8042_Config i8042_get_config() {
    while(i8042_get_status().input_buff_state == 1) continue;
    outb(0x64, COM_READ_B0);
    while(i8042_get_status().output_buff_state == 0) continue;
    u8 config = inb(0x60);

    union
    {
        u8 b;
        I8042_Config conf;
    } conf = { .b = config };

    return conf.conf;
}

void i8042_send_cont_comm(u8 byte) {
    while(i8042_get_status().input_buff_state == 1) continue;
    outb(0x64, byte);
}

void i8042_send_byte(u8 byte) {
    while(i8042_get_status().input_buff_state == 1) continue;
    outb(0x60, byte);
}

u8 i8042_get_byte() {
    while(i8042_get_status().output_buff_state != 1) continue;
    io_wait();
    return inb(0x60);
}

void i8042_send_ack(u8 com) {
    u8 resp;
    i8042_send_byte(com);
    do {
        resp = inb(0x60);
        if(resp == 0xFE) {
            i8042_send_byte(com);
        }
    } while(resp != 0xFA);
}

unsigned char kbdlower_scan1[128] = {
    0,   0,   '1', '2',  '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=', '\b','\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', '\n', 0,   'a', 's', 'd', 'f', 'g', 'h',
    'j', 'k', 'l', ';', '\'', '`', 0,   '\\','z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0,    '*', 0, ' ', 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6',
    '+', '1', '2', '3', '0', '.'
};

unsigned char kbdshift_scan1[128] = {
    0,   0,   '!', '@',  '#', '$', '%', '^', '&', '*', '(', ')',
    '_', '+', '\b','\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{', '}', '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
    'J', 'K', 'L', ':', '"', '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0,    '*', 0, ' ', 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6',
    '+', '1', '2', '3', '0', '.'
};

#define KBD_RELEASING   0b00000001
#define KBD_IGNORE      0b00000010
#define KBD_SHIFT       0b00000100
#define KBD_LOADED      0b10000000

// @TODO handle device B
void I8042_DriverEntry(Device *dev) {
    Driver* driver = (Driver*)dev->data;
    driver->data = 0b00;

    bool dual = FALSE;
    bool device_a = TRUE;

    // Flush output buffer
    while(i8042_get_status().output_buff_state != 0) {
        i8042_get_byte();
    };
    I8042_Config conf = i8042_get_config(); // get controller config byte
    conf.first_ps2_int  = 0;
    conf.first_ps2_trans= 1;
    conf.first_ps2_clock= 1;
    conf.second_ps2_int = 0;
    conf.second_ps2_clock=1;
    inb(0x60);
    i8042_set_config(conf);
    inb(0x60);
    i8042_send_cont_comm(COM_DISABLE_SECOND_PS2);
    conf = i8042_get_config();
    if(conf.second_ps2_clock) {
        dual = TRUE;
    }
    conf.second_ps2_clock=1;
    i8042_set_config(conf);

    i8042_send_ack(0xF5); // disable scanning
    // @TODO disable device B (if possible)
    i8042_send_cont_comm(0xAB); // device #1 testing
    u8 resp = i8042_get_byte();
    if(resp != 0x00) {
        puts("! I8042 dev #1 test failed\n");
        device_a = FALSE;
    }
    // @TODO test device B (if possible)
    i8042_send_ack(0xFF);
    resp = i8042_get_byte(); // device #1 self-test
    if(resp != 0xAA) {
        puts("! I8042 dev #1 BAT test failed\n");
        device_a = FALSE;
    }
    if(device_a) {
        i8042_send_ack(0xF5);
        i8042_send_ack(0xF2); // Identify the device
        u16 device = i8042_get_byte();
        io_wait();
        io_wait();
        device |= inb(0x60) << 8;

        if((device&0xFF) == 0xAB) {
            Device* kbd = k_add_dev(dev->id, DEV_KEYBOARD, device);

            // Failed to add a device
            if(kbd == NULL) {
                puts("Failed to add the PS/2 keyboard to the gdevt\n");
                kpanic();
            }

            k_register_int((Driver*)dev->data, 0x21);
            if(device_a) driver->data = (u32*)((u32)driver->data | KBD_LOADED);
            
            i8042_send_ack(0xF4); // enable scanning
            
            conf = i8042_get_config();
            conf.first_ps2_int = 1; // enable interrupts
            i8042_set_config(conf);
        } else {
            puts("unkown ps/2 device: 0x");
            puts(itoa(device, 16));
            putc('\n');
        }
    }
}

void I8042_DriverInt(Device *dev, u8 int_id) {
    Driver* driver = (Driver*)dev->data;
    u32 x = (u32)driver->data; // bitfield

    if(int_id == 0x21) { // KBD
        u8 scan = inb(0x60);
        if(x & KBD_IGNORE) return;
        if(scan == 0xE0) {
            x |= KBD_IGNORE;
        } else {
            if(scan > 0x80) {
                if(scan == 0xAA || scan == 0xB6) {
                    x &= ~(KBD_SHIFT);
                }
            } else if(scan == 0x2A || scan == 0x36) {
                x |= KBD_SHIFT;
            } else {
                if(x & KBD_SHIFT)
                    pushc(kbdshift_scan1[scan]);
                else
                    pushc(kbdlower_scan1[scan]);
            }
        }

        driver->data = (void*)x;
    }
}