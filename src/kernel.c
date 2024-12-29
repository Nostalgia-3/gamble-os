#include "str.h"
#include "types.h"

#include <memory.h>

#include <vga.h>
#include <pci.h>
#include <i8042.h>
#include <int.h>

const char* class_codes[0x14] = {
    "Unknown",
    "Mass Storage Controller",
    "Network Controller",
    "Display Controller",
    "Multimedia Device",
    "Memory Controller",
    "Bridge Device",
    "Simple Communication Controller",
    "Base System Peripheral",
    "Input Device",
    "Docking Station",
    "Processor",
    "Serial Bus Controller",
    "Wireless Controller",
    "Intelligent I/O Controller",
    "Satellite Communication Controller",
    "Encryption/Decryption Controller",
    "Data Acquisition and Signal Processing Controller",
    "Processing Accelerators",
    "Non-Essential Instrumentation"
};

unsigned char kbdmix[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '+', /*'´' */0, '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '<',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '-',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,  '<',
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

void ack_send(u8 com) {
    u8 resp;
    i8042_send_byte(com);
    do {
        resp = inb(0x60);
        if(resp == 0xFE) {
            i8042_send_byte(com);
        }
    } while(resp != 0xFA);
}

u8 getc() {
    u8 scancode = i8042_get_byte();
    if(scancode & 0x80) return 0; // release
    return kbdmix[scancode];
}

static inline bool are_interrupts_enabled()
{
    unsigned long flags;
    asm volatile ( "pushf\n\t"
                   "pop %0"
                   : "=g"(flags) );
    return flags & (1 << 9);
}

void draw_status() {
    set_attr(0, VGA_HEIGHT-1, VGA_WIDTH, (TextAttribute) { .bg = VGA_GRAY, .fg = VGA_BLACK });

    u16 x = get_cursor_x();
    u16 y = get_cursor_y();

    set_cursor(0, VGA_HEIGHT-1);

    if(are_interrupts_enabled()) putc('T');
    else putc('F');

    putc(' ');

    puts(itoa(k_get_used(), 10));

    set_cursor(x, y);
}

void _start() {
    u8 text_buf[80];
    u8 index = 0;
    TextAttribute norm;

    norm.bg = VGA_BLACK;
    norm.fg = VGA_WHITE;

    clear(' ', norm);
    set_cursor(0, 0);

    // hardcode initializing PS/2 keyboard port
    // Step 1: check the ACPI for a PS/2 controller (not doing allat)
    i8042_send_controller_byte(COM_DISABLE_FIRST_PS2); // disable PS/2 port one
    i8042_send_controller_byte(COM_DISABLE_SECOND_PS2); // disable PS/2 port two
    // flush data FIFO
    while(i8042_get_status().output_buff_state != 0) {
        i8042_get_byte();
    };
    I8042_Config conf = i8042_get_config();
    conf.first_ps2_int  = 1;
    conf.first_ps2_trans= 0;
    conf.first_ps2_clock= 1;
    i8042_set_config(conf);

    i8042_send_controller_byte(COM_TEST_PS2_CONTROLLER);
    u8 resp = i8042_get_byte();
    if(resp != 0x55) {
        puts("I8042 self-test failed (expected 0x55, got 0x");
        puts(itoa(resp, 16));
        puts(")\n");
        while(1);
    }
    // check for ps/2 device #1 (not doing allat because no idc about mouse rn)
    i8042_send_controller_byte(COM_TEST_FIRST_PS2);
    resp = i8042_get_byte();
    if(resp != 0x00) {
        puts("I8042 PS/2 port #1 failed (expected 0x55, got 0x");
        puts(itoa(resp, 16));
        puts(")\n");
        while(1);
    }

    i8042_send_controller_byte(COM_ENABLE_FIRST_PS2); // enable device
    i8042_send_byte(0xFF); // reset device

    ack_send(0xF5); // Disable Scanning
    
    ack_send(0xF2); // Identify
    u16 device = i8042_get_byte() << 8;
    if(device == 0xAB00) {
        device |= inb(0x60);
    }

    ack_send(0xF4); // Enable Scanning

    puts("[%] Enabled PS/2 device #0 (device id = 0x");
    puts(itoa(device, 16));
    puts(")\n");

    bool running = TRUE;

    puts("> ");
    draw_status();
    while(running) {
        u8 c = getc();
        if(c == '\b') {
            if(index != 0) {
                move_cursor(-1, 0);
                putc(' ');
                move_cursor(-1, 0);
                index--;
                text_buf[index] = '\0';
            }
        } else if(c == '\n') {
            putc('\n');

            u8* comm = strtok(text_buf, ' ');

            if(strcmp("list", comm) == 0) {
                comm = strtok(NULL, ' ');
                if(strcmp("pci", comm) == 0) {
                    puts("PCI Devices:\n");
                    PCIHeader header;
                    for(int x=0;x<256;x++) {
                        for(int i=0;i<32;i++) {
                            if(pci_get_vendor(x, i) == 0xFFFF) continue;
                            header = pci_get_header(x, i);
                            putc(' ');
                            puts(itoa(header.vendor, 16));
                            putc(':');
                            puts(itoa(header.device, 16));
                            puts(" (");
                            if(header.class_code <= 0x14) puts((char*)class_codes[header.class_code]);
                            else puts("Unknown\n");
                            puts(")\n");
                        }
                    }
                } else if(strcmp("", comm) == 0) {
                    puts("Subcommands:\n  pci - list all connected pci devices\n");
                } else {
                    puts("Unkown list subcommand: \"");
                    puts(comm);
                    puts("\"\n");
                }
            } else if(strcmp("cls", comm) == 0) {
                clear(' ', norm);
                set_cursor(0, 0);
            } else if(strcmp("malloc", comm) == 0) {
                int addr = (int)k_malloc(63);
                puts("Memory address: 0x");
                puts(itoa((int)addr, 16));
                putc('\n');
            } else {
                puts("Unknown command \"");
                puts(comm);
                puts("\"\n");
            }

            for(int x=0;x<index;x++) text_buf[x] = 0;
            index = 0;

            if(running) puts("> ");
        } else {
            if(c != 0) {
                if(index > sizeof(text_buf)) continue;
                text_buf[index++] = c;
                putc(c);
            }
        }

        draw_status();
    }

    while(1);

    return;
}