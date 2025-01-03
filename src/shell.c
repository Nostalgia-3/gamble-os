// Generic shell program for testing

#include <gosh.h>
#include <types.h>
#include <str.h>
#include <memory.h>
#include <math.h>

struct MemoryEntry {
    u32 base1;
    u32 base2;
    u32 len1;
    u32 len2;
    u32 type;
    u32 exattr;
};

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

const char* DEVTYPE_ST[] = {
    "Unknown",
    "Keyboard",
    "Mouse",
    "Network",
    "Speaker",
    "Microphone",
    "Drive",
    "Virtual Terminal",
    "Framebuffer",
    "Driver"
};

static u16 WIDTH = 0;

static u16 curx = 0;
static u16 cury = 0;
static u16 curz = 0;

static u8 cur_color = 15;

static u8 cmdbfr[80];
static u8 ind = 0;

static Device *fb;
static Device *vt;

static u8 running = TRUE;

void clear_screen() {
    fb_clear(fb, 0);
    curx = 0;
    cury = 0;
    curz = 0;
}

void putc(u8 c) {
    kprintfnoup("%c", c);
}

void puts(u8 *ch) {
    for(int i=0;i<strlen(ch);i++) {
        fb_draw_char(fb, curx*8, cury*8, ch[i], (u32)cur_color);
        curx++;
        if(curx >= WIDTH || ch[i] == '\n') {
            curx = 0;
            cury++;
        }
    }
}

#include <math.h>
void hexdump(u8* addr, u32 size) {
    if(size == 0) return;
    for(int i=0;i<size;i++) {
        if((i % 16) == 0) {
            // output the ASCII equivalent of the past 16 bytes
            if(i != 0) {
                kprintfnoup(" |");
                for(int x=0;x<16;x++) {
                    u8 c = *(addr+i-16+x);
                    if(is_printable(c)) putc(c);
                    else putc('.');
                }
                kprintfnoup("|\n");
            }
            kprintfnoup("%08X  ", addr+i);
        } else if((i%8) == 0 && i != 0) putcnoup(vt, ' ');
        kprintfnoup("%02X ", *(addr+i));
    }
    u8 ex = size%16;
    if(ex != 0) {
        putcnoup(vt, ' ');
        for(int i=0;i<16-ex;i++) kprintfnoup("   ");
    }
    kprintfnoup(" |");
    u32 ch_to_write = 16-ex;
    if(size <= 16) ch_to_write = size;
    for(int x=0;x<ch_to_write;x++) {
        u8 c = *(addr+size-(16-ex)+x);
        if(is_printable(c)) kprintfnoup("%c", c);
        else putc('.');
    }
    kprintfnoup("|");

    update(vt);
}

void draw_cursor(u8 set) {
    fb_draw_rect(fb, curx*8, cury*8+6, 8, 2, set ? cur_color : 0);
}

void draw(u8 *text) {
    clear_screen();
    puts(text);
}

static u32 prevind = 0;

// Called whenever any text is written to the virtual terminal
void shell_write(Device *vt) {
    VTDeviceData *dev = vt->data;

    if(dev->textind > prevind) {
        draw_cursor(FALSE);
        puts(dev->text+prevind);
    } else if(dev->textind < prevind) {
        for(int i=0;i<prevind-dev->textind;i++) {
            draw_cursor(FALSE);

            if(cury == 0 && curx == 0) break;
            if(curx == 0) {
                curx = WIDTH-1;
                cury--;
            } else {
                curx--;
            }

            fb_draw_rect(fb, curx*8, cury*8, 8, 8, 0);
        }
    } else {
        draw(dev->text);
    }

    prevind = dev->textind;
    draw_cursor(TRUE);
}

void run_command(u32 mem) {
    u8* comm = strtok(cmdbfr, ' ');

    if(strcmp("list", comm) == 0) {
        comm = strtok(NULL, ' ');
        if(strcmp("pci", comm) == 0) {
            puts("PCI Devices:\n");
            PCIHeader header;
            for(int x=0;x<256;x++) {
                for(int i=0;i<32;i++) {
                    header = pci_get_header(x, i);
                    if(header.vendor == 0xFFFF) continue;
                    kprintf("%04X:%04X (%s)\n",
                        header.vendor, header.device,
                        (char*)class_codes[
                            (header.class_code <= 0x14) ? header.class_code : 0
                        ]
                    );
                }
            }
        } else if(strcmp("mem", comm) == 0) {
            for(u16 i=0;i<mem;i++) {
                struct MemoryEntry* e = (struct MemoryEntry*)(0x7E00);
                kprintfnoup("0x%08X%08X | 0x%08X%08X | %u\n", e[i].base2, e[i].base1, e[i].len2, e[i].len1, e[i].type);
            }
        } else if(strcmp("stats", comm) == 0) {
            kprintf("Memory usage: %u/%u\n", k_get_used(), MEM_MAX);
        } else if(strcmp("", comm) == 0) {
            kprintf("Subcommands:\n  pci - list all connected pci devices\n  mem - list all memory segments\n  stats - list OS stats\n");
        } else {
            kprintf("Unkown list subcommand: \"");
            kprintf(comm);
            kprintf("\"\n");
        }
    } else if(strcmp("cls", comm) == 0) {
        flush_vt(vt);
        clear_screen();
        kprintf("> ");
    } else if(strcmp("help", comm) == 0) {
        kprintf("Commands:\n  list <pci | mem | stats> - list tables of data\n  cls - clears the terminal\n  hexdump <addr> <size> - output int <size> bytes at hex <addr>\n");
    } else if(strcmp("color", comm) == 0) {
        comm = strtok(NULL, ' ');
        if(strlen(comm) == 0) {
            kprintf("Format: color <col>\n");
        } else {
            u32 s = atoi(comm);
            cur_color = (u8)s;
        }
    } else if(strcmp("hexdump", comm) == 0) {
        bool successful = TRUE;
        comm = strtok(NULL, ' ');
        u32 addr = hatoi(comm);
        if(strcmp("", comm) == 0) successful = FALSE;
        comm = strtok(NULL, ' ');
        u32 size = atoi(comm);
        if(strcmp("", comm) == 0) successful = FALSE;

        if(successful) {
            hexdump((u8*)addr, size);
            if(size) putc('\n');
        } else {
            kprintf("Format: hexdump <addr> <size>\n");
        }
    } else if(strcmp("lsdevs", comm) == 0) {
        Device* devs    = get_gdevt();
        u32 devsize     = get_gdevt_len();
        for(int i=0;i<devsize;i++) {
            if(!devs[i].is_active) continue;
            kprintf("Device(%d) = %s\n", i, DEVTYPE_ST[devs[i].type]);
        }
    } else if(strcmp("fyou", comm) == 0) {
        for(u32 i=0;i<0x10000*4;i++)
            *(u8*)(0xA0000+i) = (u32)fibonacci_lfsr();
    } else if(strcmp("", comm) == 0) {

    } else {
        kprintf("Unknown command \"%s\"\n", comm);
    }

    for(int x=0;x<ind;x++) cmdbfr[x] = 0;
    ind = 0;
}

int shell_main(u32 mem) {
    fb = fb_get(RESOLUTION);
    if(fb == NULL) return 1;

    WIDTH = ((FBDeviceData*)fb->data)->w/8;

    vt = k_get_device_by_type(DEV_VIRT_TERM);
    if(vt == NULL) return 1;

    // Assume that the colors are the 320x200 @ 256 color palette
    clear_screen();
    kprintf("> ");

    while(running) {
        u8 sc = scanc();
        if(sc == '\b') {
            if(ind != 0) {
                // curx--;
                ind--;
                putc_vt(vt, sc);
                cmdbfr[ind] = '\0';
            }
        } else if(sc == '\n') {
            putc_vt(vt, sc);
            run_command(mem);
            ind = 0;
            if(running) kprintf("> ");
        } else {
            if(ind > sizeof(cmdbfr)) continue;
            putc_vt(vt, sc);
            cmdbfr[ind++] = sc;
        }
    }

    return 0;
}