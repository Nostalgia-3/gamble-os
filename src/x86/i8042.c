// #include <interrupt.h>
// #include <x86/x86.h>
// #include <memory.h>
// #include <str.h>

// typedef struct _I8042_Status {
//     uint8_t output_buff_state    : 1;
//     uint8_t input_buff_state     : 1;
//     uint8_t system_flag          : 1;
//     uint8_t comm_or_data         : 1;
//     uint8_t keyboard_lock        : 1; // Unused
//     uint8_t recieve_timeout      : 1; // Unused
//     uint8_t timeout_error        : 1;
//     uint8_t parity_error         : 1;
// } I8042_Status;

// typedef struct _I8042_Config {
//     uint8_t first_ps2_int        : 1;
//     uint8_t second_ps2_int       : 1;
//     uint8_t system_flag          : 1;
//     uint8_t                      : 1;
//     uint8_t first_ps2_clock      : 1;
//     uint8_t second_ps2_clock     : 1;
//     uint8_t first_ps2_trans      : 1;
//     uint8_t                      : 1;
// } I8042_Config;


// I8042_Status    i8042_get_status();

// void            i8042_set_config(I8042_Config config);
// I8042_Config    i8042_get_config();

// void            i8042_send_cont_comm(uint8_t byte);

// void            i8042_send_byte(uint8_t byte);
// uint8_t         i8042_get_byte();

// #define DATA    0x60
// #define STATUS  0x64
// #define COMM    0x64

// #define CON_SYSTEM_RESET    1
// #define CON_A20_GATE        2
// #define CON_SECOND_PS2_CLK  4
// #define CON_SECOND_PS2_DATA 8
// // Output buffer full with byte from first PS/2 port (connected to IRQ1)
// #define CON_OUT_BUFF_PS2_1  16
// // 	Output buffer full with byte from second PS/2 port (connected to IRQ12, only if 2 PS/2 ports supported)
// #define CON_OUT_BUFF_PS2_2  32
// #define CON_FIRST_PS2_CLK   64
// #define CON_FIRST_PS2_DATA  128

// #define KBD_RELEASING   0b00000001
// #define KBD_IGNORE      0b00000010
// #define KBD_SHIFT       0b00000100
// #define KBD_LOADED      0b10000000

// // This isn't all of them but I
// // basically just cherry picked
// // the ones I care about
// enum I8042Commands {
//     COM_READ_B0             = 0x20,
//     COM_DISABLE_SECOND_PS2  = 0xA7,
//     COM_ENABLE_SECOND_PS2   = 0xA8,
//     COM_TEST_SECOND_PS2     = 0xA9,
//     COM_TEST_PS2_CONTROLLER = 0xAA,
//     COM_TEST_FIRST_PS2      = 0xAB,
//     COM_DISABLE_FIRST_PS2   = 0xAD,
//     COM_ENABLE_FIRST_PS2    = 0xAE,
//     COM_READ_CONTROLLER_OUT = 0xD0,
//     COM_WRITE_CONTROLLER_OUT= 0xD1,
//     COM_WRITE_TO_SECOND_PS2 = 0xD4,
//     COM_WRITE_CONFIG        = 0x60,
//     COM_RESET_CPU           = 0xFE
// };

// I8042_Status i8042_get_status() {
//     union
//     {
//         uint8_t b;
//         I8042_Status st;
//     } stat = { .b = inb(0x64) };

//     return stat.st;
// }

// void i8042_set_config(I8042_Config config) {
//     union
//     {
//         uint8_t b;
//         I8042_Config conf;
//     } conf = { .conf = config };

//     while(i8042_get_status().input_buff_state == 1) continue;
//     outb(0x64, COM_WRITE_CONFIG);
//     i8042_send_byte(conf.b);
// }

// I8042_Config i8042_get_config() {
//     while(i8042_get_status().input_buff_state == 1) continue;
//     outb(0x64, COM_READ_B0);
//     while(i8042_get_status().output_buff_state == 0) continue;
//     uint8_t config = inb(0x60);

//     union
//     {
//         uint8_t b;
//         I8042_Config conf;
//     } conf = { .b = config };

//     return conf.conf;
// }

// void i8042_send_cont_comm(uint8_t byte) {
//     while(i8042_get_status().input_buff_state == 1) continue;
//     outb(0x64, byte);
// }

// void i8042_send_byte(uint8_t byte) {
//     while(i8042_get_status().input_buff_state == 1) continue;
//     outb(0x60, byte);
// }

// uint8_t i8042_get_byte() {
//     while(i8042_get_status().output_buff_state != 1) continue;
//     io_wait();
//     return inb(0x60);
// }

// void i8042_send_ack(uint8_t com) {
//     uint8_t resp;
//     i8042_send_byte(com);
//     do {
//         resp = inb(0x60);
//         if(resp == 0xFE) {
//             i8042_send_byte(com);
//         }
//     } while(resp != 0xFA);
// }

#include <module.h>
#include <interrupt.h>
#include <x86/x86.h>
#include <utils.h>
#include <printf.h>
#include <device.h>

#define DATA    0x60
#define STATUS  0x64
#define COMM    0x64

#define buf_len 128

const uint8_t enlower_scan1[128] = {
    0,   0,   '1', '2',  '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=', '\b','\0', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', '\n', 0,   'a', 's', 'd', 'f', 'g', 'h',
    'j', 'k', 'l', ';', '\'', '`', 0,   '\\','z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0,    '*', 0, ' ', 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6',
    '+', '1', '2', '3', '0', '.'
};

const uint8_t enshift_scan1[128] = {
    0,   0,   '!', '@',  '#', '$', '%', '^', '&', '*', '(', ')',
    '_', '+', '\b','\0', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{', '}', '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
    'J', 'K', 'L', ':', '"', '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0,    '*', 0, ' ', 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6',
    '+', '1', '2', '3', '0', '.'
};

static device kbd           = {0};
static char fifo[buf_len]   = {0};
static uint32_t fifo_ind    = 0;
static bool shift           = false;

ssize_t kbd_read(void *buf, size_t len, off_t *offset) {
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

ssize_t kbd_write(const void* buf, size_t len, off_t* offset) {
    if(buf == NULL) return -1;

    uint8_t* c = (uint8_t*)buf;

    for(int i=0;i<len;i++) {
        if(fifo_ind > sizeof(fifo)) return i;
        if(c[i] == '\0') continue;
        fifo[fifo_ind++] = c[i];
    }

    return len;
}

void flush_buf() { while(inb(STATUS) & (1 << 0)) inb(DATA); }

int i8042_entry(module *dev) {
    kbd.read = kbd_read;
    kbd.write = kbd_write;

    if(mknod("/dev/", "kbd", INODE_DEV, &kbd) < 0) {
        kpanic("Failed to create keyboard device");
    }

    // Dummy reads
    for(int i=0;i<16;i++) inb(DATA);

    // Get the current controller configuration byte
    outb(COMM, 0x20);
    uint8_t config = inb(DATA);
    
    inb(DATA); // Dummy read

    // Update config (disable IRQs, enable clock, disable translation)
    // printf("starting config: %02X\n", config);
    config = 0x44;
    outb(COMM, 0x60);
    outb(DATA, config);

    inb(DATA); // Dummy read

    // Check for device B
    outb(COMM, 0xA7); inb(DATA);
    
    outb(COMM, 0x20);
    uint8_t device_b = inb(DATA);
    inb(DATA);

    if(device_b & (0x20)) {
        // printf("There is a second port\n");

        // Disable scanning on device B
        outb(DATA, 0xF5);
    }

    // Disable scanning on device A
    outb(DATA, 0xF5);
    inb(DATA);

    // Check device A
    for(int i=0;i<16;i++) inb(DATA);
    outb(COMM, 0xAB);
    uint8_t response = inb(DATA);
    if(response != 0x00 && response != 0xFA) {
        printf("Device A failed test (expected 0x00, got 0x%02X)\n", response);
    }

    for(int i=0;i<16;i++) inb(DATA);

    outb(DATA, 0xFF);
    if(inb(DATA) != 0xFA || inb(DATA) != 0xAA) {
        printf("Device A failed to reset properly!\n");
    }

    for(int i=0;i<16;i++) inb(DATA);

    // Enable device A interrupt
    config |= 1;
    outb(COMM, 0x60);
    outb(DATA, config);

    if(hook_interrupt(dev, 0x21) < 0) {
        kpanic("Failed to hook to interrupt 0x21");
    }

    return 0;
}

int i8042_int(module *dev, uint32_t irq) {
    if(irq == 0x21) {
        uint8_t data = inb(DATA);
        for(int i=0;i<4;i++) inb(DATA);
        
        if(data == 0x2A) {
            shift = true;
        } else if(data == 0xAA) {
            shift = false;
        }

        if(data > 0x80) return 0;
        if(shift)   data = enshift_scan1[data];
        else        data = enlower_scan1[data];
        kbd_write(&data, 1, NULL);
    }

    return 0;
}

module get_i8042_module() {
    return (module) {
        .name = "i8042",
        .module_start = i8042_entry,
        .module_int = i8042_int,
        
        .type = 0
    };
}