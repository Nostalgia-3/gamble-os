#include <gosh/gosh.h>
#include <drivers/x86/i8042.h>
#include <memory.h>
#include <port.h>
#include <str.h>

#define PORT_KEYBOARD 0
#define PORT_MOUSE 1

u8  i8042_get_byte();
void i8042_flush();

int i8042_set_config(I8042_Config config);
I8042_Config i8042_get_config();
int i8042_send_command(u8 byte);

I8042_Status i8042_get_status();
int i8042_send_port1(u8 byte);
int i8042_send_port2(u8 byte);
int i8042_ack_port1(u8 com);
int i8042_ack_port2(u8 byte);

#define KBD_RELEASING       0b00000001
#define KBD_IGNORE          0b00000010
#define KBD_SHIFT           0b00000100

#define CON_SYSTEM_RESET    1
#define CON_A20_GATE        2
#define CON_SECOND_PS2_CLK  4
#define CON_SECOND_PS2_DATA 8
#define CON_OUT_BUFF_PS2_1  16
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

void reset_cpu() {
    u16 timeout = 2400;
    while(timeout > 0 && i8042_get_status().input_buff_state == 1) timeout--;
    outb(0x64, COM_RESET_CPU); // reset the cpu
}

// Get the status of the i8042
I8042_Status i8042_get_status() {
    union
    {
        u8 b;
        I8042_Status st;
    } stat = { .b = inb(0x64) };

    return stat.st;
}

// Send a byte to the first port
int i8042_send_port1(u8 byte) {
    u16 timeout = 2400;
    while((timeout--) > 0 && i8042_get_status().input_buff_state == 1) continue;
    if(!timeout) {
        return 1;
    }
    outb(0x60, byte);
    return 0;
}

int i8042_send_port2(u8 byte) {
    i8042_send_command(COM_WRITE_TO_SECOND_PS2);
    u16 timeout = 100;
    while(timeout != 0) {
        if(i8042_get_status().input_buff_state == 0) {
            outb(0x60, byte);
            return 0;
        }
        timeout--;
    }
    if(!timeout) return 1;
}

// Get the latest byte sent; unless in an interrupt, we don't know
// the origin of the byte unless it's an interrupt
u8 i8042_get_byte() {
    u16 timeout = 3500;
    while(timeout > 0 && i8042_get_status().output_buff_state != 1) timeout--;
    if(timeout == 0) {
        return 0;
    }
    u8 d = inb(0x60);
    return d;
}

// Flush the i8042 data FIFO
void i8042_flush() {
    while(i8042_get_status().output_buff_state == 1) {
        i8042_get_byte();
    };
}

// Send a byte and wait for an ACK (or a timeout of ~2400 cycles)
int i8042_ack_port1(u8 com) {
    u8 resp;
    i8042_send_port1(com);
    u16 timeout = 2400;
    do {
        resp = inb(0x60);
        timeout--;
        if(resp == 0xFE) {
            i8042_send_port1(com);
        }
    } while(timeout > 0 && resp != 0xFA);
    if(!timeout) return -1;
}

int i8042_ack_port2(u8 byte) {
    i8042_send_port2(byte);

    u8 resp     = 0x00;
    u16 timeout = 100;
    do {
        resp = inb(0x60);
        if(resp == 0xFE) {
            i8042_send_port2(byte);
        }
        if(resp == 0xFA) break;
        timeout--;
    } while(resp != 0xFA && timeout != 0);

    if(!timeout) return 1;
    i8042_get_byte();
}

// Set the i8042 config
int i8042_set_config(I8042_Config config) {
    union
    {
        u8 b;
        I8042_Config conf;
    } conf = { .conf = config };

    i8042_flush();

    u16 timeout = 2400;
    while(timeout > 0 && i8042_get_status().input_buff_state == 1) timeout--;
    if(!timeout) return 1;
    outb(0x64, COM_WRITE_CONFIG);
    i8042_send_port1(conf.b);
    return 0;
}

// Read the config from the i8042
I8042_Config i8042_get_config() {
    u16 timeout = 2400;
    while(timeout > 0 &&i8042_get_status().input_buff_state == 1) timeout--;
    if(!timeout) {
        I8042_Config conf;
        conf._failed = 1;
        return conf;
    }

    outb(0x64, COM_READ_B0);
    timeout = 2400;
    while(timeout > 0 && i8042_get_status().output_buff_state == 0) timeout--;
    if(!timeout) {
        I8042_Config conf;
        conf._failed = 1;
        return conf;
    }
    u8 config = inb(0x60);

    union
    {
        u8 b;
        I8042_Config conf;
    } conf = { .b = config };

    return conf.conf;
}

// Send a command to the i8042
int i8042_send_command(u8 byte) {
    u16 timeout = 2400;
    while((timeout-- > 0) && i8042_get_status().input_buff_state == 1) continue;
    if(!timeout) {
        return 1;
    }
    outb(0x64, byte);
    return 0;
}

unsigned char enlower_scan1[128] = {
    0,   0,   '1', '2',  '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=', '\b','\0', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', '\n', 0,   'a', 's', 'd', 'f', 'g', 'h',
    'j', 'k', 'l', ';', '\'', '`', 0,   '\\','z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0,    '*', 0, ' ', 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6',
    '+', '1', '2', '3', '0', '.'
};

unsigned char enshift_scan1[128] = {
    0,   0,   '!', '@',  '#', '$', '%', '^', '&', '*', '(', ')',
    '_', '+', '\b','\0', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{', '}', '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
    'J', 'K', 'L', ':', '"', '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0,    '*', 0, ' ', 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6',
    '+', '1', '2', '3', '0', '.'
};

static int port1_type;
static int port2_type;
static device_t kbd;
static device_t mouse;
static u8   fifo[256];
static u32  fifo_ind;
static bool ready_to_push_line;

ssize_t i8042_kbd_read(void* buf, size_t len, off_t *offset) {
    if(len == 0) return 0;
    if(fifo_ind == 0) return 0;

    if(len > fifo_ind) {
        size_t d = fifo_ind;
        memcpy(buf, fifo, fifo_ind);
        memset(fifo, 0, fifo_ind);
        fifo_ind = 0;
        return d;
    } else {
        memcpy(buf, fifo, len);
        fifo_ind -= len;
        return len;
    }
}

// Write a character to FIFO
void i8042_pushc(char c) {
    if(fifo_ind > sizeof(fifo)) return;
    if(c == '\0') return;
    fifo[fifo_ind++] = c;
    if(c == '\n') ready_to_push_line = true;
}

int setup_port(module_t *dev, u8 type) {
    switch(type) {
        case 0xAB: {
            if(register_device("/dev/kbd", &kbd) < 0 && register_device("/dev/kbd1", &kbd) < 0) {
                kprintf("Failed to create keyboard device! (step=3)\n");
                return -1;
            }

            if(!k_register_int(dev, 0x21)) {
                kprintf("Failed to register interrupt 0x21 (step=3)\n");
                return -1;
            }

            i8042_ack_port1(0xF4);

            return PORT_KEYBOARD;
        break; }

        case 0x00: {
            if(register_device("/dev/mouse", &mouse) < 0 && register_device("/dev/mouse1", &kbd) < 0) {
                kprintf("Failed to create mouse device! (step=3, port=0)\n");
                return -1;
            }

            if(!k_register_int(dev, 0x21)) {
                kprintf("Failed to register interrupt 0x21 (step=3)\n");
                return -1;
            }

            i8042_ack_port1(0xF4);

            return PORT_MOUSE;
        break; }

        default:
            kprintf("Unknown device (step=3, 0x%X)\n", type);
        break;
    }
}

u8 handle_keyboard(u8 scan, u8 x) {
    if(x & KBD_IGNORE) {
        if(scan == 0x53) { // Delete key
            reset_cpu();
        } else {
            // There should be some ioctl to enable/disable this
            i8042_pushc(0xE0);
            i8042_pushc(scan);
        }

        x &= ~(KBD_IGNORE);
    } else if(scan == 0xE0) {
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
                i8042_pushc(enshift_scan1[scan]);
            else
                i8042_pushc(enlower_scan1[scan]);
        }
    }

    return x;
}

u8 handle_mouse(u8 byte, u8 x) {
    return x;
}

// @TODO handle device B
int i8042_entry(module_t *dev) {
    dev->data = 0;
    memset(fifo, 0, sizeof(fifo));
    fifo_ind = 0;

    kbd = (device_t) {
        .owner = dev->id,
        .read = i8042_kbd_read
    };

    i8042_flush();
    I8042_Config conf = i8042_get_config();
    kprintf("%X\n", conf);

    if(conf._failed) {
        kprintf("Failed to get i8042 config!\n");
        return DRIVER_FAILED;
    }

    conf.first_ps2_clock    = 1;
    conf.first_ps2_trans    = 1;
    conf.second_ps2_clock   = 1;
    conf.system_flag        = 0;

    if(i8042_set_config(conf) < 0) {
        kprintf("Failed to set i8042 config!\n");
        return DRIVER_SUCCESS;
    }

    i8042_ack_port1(0xF4);
    kprintf("%X ", i8042_get_byte());
    i8042_ack_port2(0xF4);
    kprintf("%X\n", i8042_get_byte());

    conf = i8042_get_config();

    conf.first_ps2_int      = 1;
    conf.second_ps2_int     = 1;

    if(i8042_set_config(conf) < 0) {
        kprintf("Failed to set i8042 config (enabling interrupts)!\n");
    }

    if(register_device("/dev/kbd", &kbd) < 0) {
        kprintf("Failed creating keyboard!\n");
        return DRIVER_FAILED;
    }

    if(!k_register_int(dev, 0x21)) {
        kprintf("Failed to register interrupt\n");
        return DRIVER_FAILED;
    }

    return DRIVER_SUCCESS;

    // // Setup the first PS/2 slot
    // conf.first_ps2_int      = 0;
    // conf.first_ps2_trans    = 1;
    // conf.first_ps2_clock    = 1;

    // // Setup the second PS/2 slot
    // conf.second_ps2_int     = 0;
    // conf.second_ps2_clock   = 1;

    // if(i8042_set_config(conf)) {
    //     kprintf("Failed to set i8042 config! (step=0)\n");
    //     return DRIVER_FAILED;
    // }

    // // TODO: Test for second PS/2
    
    // // Disable scanning on both ports
    // i8042_ack_port1(0xF5);
    // i8042_ack_port2(0xF5);

    // i8042_send_command(COM_TEST_FIRST_PS2);
    // u8 resp = i8042_get_byte();
    // if(resp != 0x00) {
    //     kprintf("I8042 port #1 test failed (step=1, expected 0x00 but got 0x%02X)\n", resp);
    //     slot_a[0] = false;
    //     return DRIVER_FAILED;
    // }

    // i8042_flush();

    // conf = i8042_get_config();
    // if(conf._failed) {
    //     kprintf("Failed to get i8042 config! (step=3)\n");
    //     return DRIVER_FAILED;
    // }

    // u8 info     = 0;
    // u16 timeout = 2400;

    // i8042_ack_port1(0xF2); // Identify
    // while(info == 0x00 && timeout != 0) {
    //     info = i8042_get_byte();
    //     timeout--;
    // }

    // port1_type = setup_port(dev, info);

    // if(port1_type == DRIVER_FAILED) {
    //     kprintf("Failed to setup PS/2 port #1\n");
    //     // return DRIVER_FAILED;
    // }

    // i8042_flush();
    // i8042_ack_port2(0xF2); // Identify
    // info = 0; timeout = 0;
    // while(info == 0x00 && timeout != 0) {
    //     info = i8042_get_byte();
    //     timeout--;
    // }

    // port2_type = setup_port(dev, info);

    // if(port2_type == -1) {
    //     kprintf("Failed to setup PS/2 port #2\n");
    // }

    // conf.first_ps2_int = 1;
    // conf.second_ps2_int = 1;

    // if(i8042_set_config(conf)) {
    //     kprintf("Failed to set i8042 config! (step=3)\n");
    //     return DRIVER_FAILED;
    // }


    // return DRIVER_SUCCESS;
}

// port1_type

int i8042_int(module_t *dev, u8 irq) {
    u32 x = (u32)dev->data; // bitfield

    if(irq == 0x21) { // port #1
        u8 byte = inb(0x60);
        if(port1_type == PORT_MOUSE) {
            x = handle_mouse(byte, x);
        } else if(port1_type == PORT_KEYBOARD) {
            x = handle_keyboard(byte, x);
        }

        dev->data = (void*)x;
    } else if(irq == 0x2C) { // port #2
        u8 byte = inb(0x60);
        if(port2_type == PORT_MOUSE) {
            x = handle_mouse(byte, x);
        } else if(port2_type == PORT_KEYBOARD) {
            x = handle_keyboard(byte, x);
        }

        dev->data = (void*)x;
    }

    return 0;
}

module_t get_i8042_module() {
    return (module_t) {
        .name = "i8042",
        .module_start = i8042_entry,
        .module_int = i8042_int
    };
}