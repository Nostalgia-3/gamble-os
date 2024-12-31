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

    outb(0x64, conf.b);
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

void i8042_send_controller_byte(u8 byte) {
    while(i8042_get_status().input_buff_state == 1) continue;
    outb(0x64, byte);
}

void i8042_send_byte(u8 byte) {
    while(i8042_get_status().input_buff_state == 1) continue;
    outb(0x60, byte);
}

u8 i8042_get_byte() {
    while(i8042_get_status().output_buff_state != 1) continue;
    return inb(0x60);
}

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

unsigned char kbdmix[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',    /* 9 */
  '9', '0', '+', /*'´' */0, '\b',    /* Backspace */
  '\t',            /* Tab */
  'q', 'w', 'e', 'r',    /* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',    /* Enter key */
    0,            /* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',    /* 39 */
 '\'', '<',   0,        /* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',            /* 49 */
  'm', ',', '.', '-',   0,                /* Right shift */
  '*',
    0,    /* Alt */
  ' ',    /* Space bar */
    0,    /* Caps lock */
    0,    /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,    /* < ... F10 */
    0,    /* 69 - Num lock*/
    0,    /* Scroll Lock */
    0,    /* Home key */
    0,    /* Up Arrow */
    0,    /* Page Up */
  '-',
    0,    /* Left Arrow */
    0,
    0,    /* Right Arrow */
  '+',
    0,    /* 79 - End key*/
    0,    /* Down Arrow */
    0,    /* Page Down */
    0,    /* Insert Key */
    0,    /* Delete Key */
    0,   0,  '<',
    0,    /* F11 Key */
    0,    /* F12 Key */
    0,    /* All other keys are undefined */
};


void I8042_DriverEntry(Device *dev) {
    // Disable PS/2 ports
    i8042_send_controller_byte(COM_DISABLE_FIRST_PS2);
    i8042_send_controller_byte(COM_DISABLE_SECOND_PS2);

    // Flush output buffer
    while(i8042_get_status().output_buff_state != 0) {
        i8042_get_byte();
    };

    // Enable interrupt and clock, disabling translation
    I8042_Config conf = i8042_get_config();
    conf.first_ps2_int  = 0;
    conf.first_ps2_trans= 0;
    conf.first_ps2_clock= 1;
    conf.second_ps2_int = 0;
    i8042_set_config(conf);

    i8042_send_controller_byte(COM_TEST_PS2_CONTROLLER);
    u8 resp = i8042_get_byte();
    if(resp != 0x55) {
        // this really shouldn't cause a kernel panic, but
        // for testing purposes it shouldn't matter
        // printf("I8042 self-test failed (expected 0x55, got 0x%02X)\n", resp);
        puts("I8042 self-test failed (expected 0x55, got 0x");
        puts(itoa(resp, 16));
        puts(")\n");
        return;
    }

    i8042_send_controller_byte(COM_TEST_FIRST_PS2);
    resp = i8042_get_byte();
    if(resp != 0x00) {
        // this really shouldn't cause a kernel panic, but
        // for testing purposes it shouldn't matter
        // printf("I8042 PS/2 port #1 failed (expected 0x55, got 0x%02X)\n", resp);
        puts("I8042 PS/2 port #1 failed (expected 0x55, got 0x");
        puts(itoa(resp, 16));
        puts(")\n");
        return;
    }

    i8042_send_controller_byte(COM_ENABLE_FIRST_PS2); // enable device
    i8042_send_byte(0xFF); // reset device

    I8042_send_ack(0xF5); // Disable scanning

    I8042_send_ack(0xF2); // Identify the device
    u8 device = i8042_get_byte();

    Device* kbd = k_add_dev(dev->id, DEV_KEYBOARD, device);

    // Failed to add a device
    if(kbd == NULL) {
        puts("Failed to add the PS/2 keyboard to the gdevt\n");
        kpanic();
    }

    // register the keyboard int
    k_register_int((Driver*)dev->data, 0x21);
    I8042_send_ack(0xF4);
}
void I8042_DriverInt(Device *dev, u8 int_id) {
    puts("data = ");
    puts(itoa(inb(0x60), 16));
    putc('\n');
}