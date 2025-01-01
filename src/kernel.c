#include "str.h"
#include "types.h"

#include <gosh.h>
#include <memory.h>
#include <int.h>

#include <i8042.h>

#include <vga.h>

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

void draw_status() {
    set_attr(0, VGA_HEIGHT-1, VGA_WIDTH, (TextAttribute) { .bg = VGA_CYAN, .fg = VGA_BLACK });

    u16 x = get_cursor_x();
    u16 y = get_cursor_y();

    set_cursor(0, VGA_HEIGHT-1);

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

void padputs(char*st, u8 length, u8 c) {
    if(strlen(st) > length) {
        puts(st);
    } else {
        for(int i=0;i<length-strlen(st);i++)
            putc(c);
        puts(st);
    }
}

// the 0 is a macro that gets filled in by `make.ts`
#define VERSION "0.0.0b1"

void _start(u8 boot, u32 mem) {
    TextAttribute norm;
    norm.bg = VGA_BLACK;
    norm.fg = VGA_WHITE;
    clear(' ', norm);
    set_cursor(0, 0);

    k_malloc(mem*sizeof(struct MemoryEntry));

    Driver DriverI8042 = (Driver) { .DriverEntry = I8042_DriverEntry, .DriverInt = I8042_DriverInt };

    puts("Debug Build v" VERSION " (built with gcc v" __VERSION__ ")\n");
    _init_gdevt();
    idt_init();
    load_driver(&DriverI8042);

    if(!((u32)DriverI8042.data & 0b10000000)) { // !((u32)DriverI8042.data & 0b10000000)
        clear(' ', norm);
        set_cursor(0, 0);
        puts("(failed to initialize I8042 keyboard, so here's debug info)\n");
        PCIHeader header;
        for(int x=0;x<256;x++) {
            for(int i=0;i<32;i++) {
                header = pci_get_header(x, i);
                if(header.vendor == 0xFFFF) continue;
                putc(' ');
                puts(itoa(header.vendor, 16));
                putc(':');
                puts(itoa(header.device, 16));
                puts(" (");
                if(header.class_code < 0x14) puts((char*)class_codes[header.class_code]);
                else puts("Unknown");
                puts(")\n");
            }
        }

        for(u16 i=0;i<mem;i++) {
            struct MemoryEntry* e = (struct MemoryEntry*)(0x7E00);
            puts("");
            puts("0x");
            padputs(itoa(e[i].base2, 16), 8, '0');
            padputs(itoa(e[i].base1, 16), 8, '0');
            puts(" | ");
            puts("0x");
            padputs(itoa(e[i].len2, 16), 8, '0');
            padputs(itoa(e[i].len1, 16), 8, '0');
            puts(" | ");
            puts(itoa(e[i].type, 10));
            putc('\n');
        }

        while(1);
    }

    bool running = TRUE;
    u8 text_buf[80];
    u8 index = 0;

    puts("> ");
    while(running) {
        u8 c = scanc();
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
                            header = pci_get_header(x, i);
                            if(header.vendor == 0xFFFF) continue;
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
                } else if(strcmp("mem", comm) == 0) {
                    for(u16 i=0;i<mem;i++) {
                        struct MemoryEntry* e = (struct MemoryEntry*)(0x7E00);
                        puts("0x");
                        padputs(itoa(e[i].base2, 16), 8, '0');
                        padputs(itoa(e[i].base1, 16), 8, '0');
                        puts(" | ");
                        puts("0x");
                        padputs(itoa(e[i].len2, 16), 8, '0');
                        padputs(itoa(e[i].len1, 16), 8, '0');
                        puts(" | ");
                        puts(itoa(e[i].type, 10));
                        putc('\n');
                    }
                } else if(strcmp("", comm) == 0) {
                    puts("Subcommands:\n  pci - list all connected pci devices\n  mem - list all memory segments\n");
                } else {
                    puts("Unkown list subcommand: \"");
                    puts(comm);
                    puts("\"\n");
                }
            } else if(strcmp("cls", comm) == 0) {
                clear(' ', norm);
                set_cursor(0, 0);
            } else if(strcmp("help", comm) == 0) {
                puts("Commands:\n  list <pci | mem> - list tables of data\n  cls - clears the terminal\n");
            } else {
                puts("Unknown command \"");
                puts(comm);
                puts("\" (try running help)\n");
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

        // draw_status();
    }
    while(1);

    return;
}
