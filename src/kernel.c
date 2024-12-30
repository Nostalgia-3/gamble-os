#include "str.h"
#include "types.h"

#include <gosh.h>
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
}

void I8042_DriverInt(Device *dev, u8 int_id) {
    puts("i8042 interrupt\n");
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
    putc('/');
    puts(itoa(MEM_MAX, 10));

    putc(' ');
    puts(itoa(get_used_devices(), 10));
    putc('/');
    puts(itoa(get_gdevt_len(), 10));

    set_cursor(x, y);
}

#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)
#define PIC_EOI		0x20		/* End-of-interrupt command code */
#define ICW1_ICW4	0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */

#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

static inline void io_wait(void) {
    outb(0x80, 0);
}

void PIC_sendEOI(u8 irq) {
    if(irq >= 8)
        outb(PIC2_COMMAND,PIC_EOI);
    
    outb(PIC1_COMMAND,PIC_EOI);
}

void IRQ_set_mask(u8 IRQline) {
    u16 port;

    if(IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    outb(port, inb(port) | (1 << IRQline));
}

void IRQ_clear_mask(u8 IRQline) {
    u16 port;

    if(IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    outb(port, inb(port) & ~(1 << IRQline));
}

void t_init_pic(u8 offset1, u8 offset2) {
    
    u8 a1 = inb(PIC1_DATA);
    u8 a2 = inb(PIC2_DATA);
    
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();
    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();
    
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();
    
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void irq_handler(u32 i) {
    puts(itoa(i, 10));
    putc('\n');
}

void _start() {
    bool running = TRUE;

    TextAttribute norm;

    norm.bg = VGA_BLACK;
    norm.fg = VGA_WHITE;

    clear(' ', norm);
    set_cursor(0, 0);

    u8 text_buf[80];
    u8 index = 0;

    t_init_pic(0x20, 0x28);

    _init_gdevt();
    idt_init();

    Driver I8042_Driver = { .DriverEntry = I8042_DriverEntry, .DriverInt = I8042_DriverInt };
    load_driver(&I8042_Driver);

    draw_status();
    while(1);

    // puts("PCI Devices:\n");
    // PCIHeader header;
    // for(int x=0;x<256;x++) {
    //     for(int i=0;i<32;i++) {
    //         if(pci_get_vendor(x, i) == 0xFFFF) continue;
    //         header = pci_get_header(x, i);
    //         putc(' ');
    //         puts(itoa(header.vendor, 16));
    //         putc(':');
    //         puts(itoa(header.device, 16));
    //         puts(" (");
    //         if(header.class_code <= 0x14) puts((char*)class_codes[header.class_code]);
    //         else puts("Unknown\n");
    //         puts(")\n");
    //     }
    // }

    // puts("> ");
    // draw_status();
    // while(running) {
    //     u8 c = getc();
    //     if(c == '\b') {
    //         if(index != 0) {
    //             move_cursor(-1, 0);
    //             putc(' ');
    //             move_cursor(-1, 0);
    //             index--;
    //             text_buf[index] = '\0';
    //         }
    //     } else if(c == '\n') {
    //         putc('\n');

    //         u8* comm = strtok(text_buf, ' ');

    //         if(strcmp("list", comm) == 0) {
    //             comm = strtok(NULL, ' ');
    //             if(strcmp("pci", comm) == 0) {
                    
    //             } else if(strcmp("", comm) == 0) {
    //                 puts("Subcommands:\n  pci - list all connected pci devices\n");
    //             } else {
    //                 puts("Unkown list subcommand: \"");
    //                 puts(comm);
    //                 puts("\"\n");
    //             }
    //         } else if(strcmp("cls", comm) == 0) {
    //             clear(' ', norm);
    //             set_cursor(0, 0);
    //         } else if(strcmp("malloc", comm) == 0) {
    //             int addr = (int)k_malloc(63);
    //             puts("Memory address: 0x");
    //             puts(itoa((int)addr, 16));
    //             putc('\n');
    //         } else {
    //             puts("Unknown command \"");
    //             puts(comm);
    //             puts("\"\n");
    //         }

    //         for(int x=0;x<index;x++) text_buf[x] = 0;
    //         index = 0;

    //         if(running) puts("> ");
    //     } else {
    //         if(c != 0) {
    //             if(index > sizeof(text_buf)) continue;
    //             text_buf[index++] = c;
    //             putc(c);
    //         }
    //     }

    //     draw_status();
    // }
    // while(1);

    return;
}