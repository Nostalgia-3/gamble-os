#include "str.h"
#include "types.h"

#include <gosh.h>
#include <memory.h>
#include <int.h>

#include <i8042.h>

#include <vga.h>
#include <pci.h>

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

void irq_handler(u32 i) {
    k_handle_int((u8)i+0x20);
}

// PS/2 controller
const Driver DriverI8042 = { .DriverEntry = I8042_DriverEntry, .DriverInt = I8042_DriverInt };
// Generic Drive Manager
Driver DriverATA;

struct MemoryEntry {
    u32 base1;
    u32 base2;
    u32 len1;
    u32 len2;
    u32 type;
    u32 exattr;
};

void _start(u8 boot, u16 mem) {
    TextAttribute norm;
    norm.bg = VGA_BLACK;
    norm.fg = VGA_WHITE;
    clear(' ', norm);
    set_cursor(0, 0);

    puts(itoa(mem, 10));

    _init_gdevt();
    idt_init();
    load_driver((Driver*)&DriverI8042);


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

    // u8 text_buf[80];
    // u8 index = 0;
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