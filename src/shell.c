// Generic shell program for testing
#include <shell.h>
#include <gosh.h>
#include <types.h>
#include <str.h>
#include <memory.h>
#include <math.h>

#include <port.h>

#include <drivers/x86/vga.h>

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

static u32 prevind = 0;

static bool in_color = FALSE;

void clear_screen() {
    fb_clear(fb, 0);
    curx = 0;
    cury = 0;
    curz = 0;
}

void putc(u8 c) {
    if(in_color) {
        cur_color = c;
        in_color = FALSE;
    } else if(c == '\x1b') {
        in_color = TRUE;
    } else {
        kprintfnoup("%c", c);
    }
}

void puts(u8 *ch) {
    for(int i=0;i<strlen(ch);i++) {
        if(in_color) {
            cur_color = ch[i];
            in_color = FALSE;
        } else if(ch[i] == '\x1b') {
            in_color = TRUE;
        } else {
            fb_draw_char(fb, curx*8, cury*8, ch[i], (u32)cur_color);
            if(is_printable(ch[i])) curx++;
            if(curx >= WIDTH || ch[i] == '\n') {
                curx = 0;
                cury++;
            }
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
    if(((FBInfo*)fb->data)->format == PixelFormat_TEXT_640x480)
        vga_set_cursor(curx, cury);
    else
        fb_draw_rect(fb, curx*8, cury*8+6, 8, 2, set ? cur_color : 0);
}

void draw(u8 *text) {
    clear_screen();
    puts(text);
}

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
            fb_draw_char(fb, curx*8, cury*8, ' ', 0x0F);
        }
    } else {
        draw(dev->text);
    }

    prevind = dev->textind;
    draw_cursor(TRUE);
}

void run_command(u32 mem) {
    u8* comm = strtok(cmdbfr, ' ');

    if(strcmp("", comm) == 0) {
    } else if(strcmp("help", comm) == 0) {
        kprintfnoup("Commands:\n");
        kprintfnoup("  list    <pci | mem | stats>  list tables of data\n");
        kprintfnoup("  cls                          clears the terminal\n");
        kprintfnoup("  color   <color>              set the foreground text color\n");
        kprintfnoup("  hexdump <addr> <count>       hexdump count bytes at addr\n");
        kprintfnoup("  lsdevs                       list all registered devices\n");
        kprintfnoup("  read    <did> <sec> <addr>   Read a sector from a drive\n");
        kprintfnoup("  mount   <drive> <st:path>    Mount a drive\n");
        kprintfnoup("  malloc  <size>               Allocate memory and output the address\n");
        kprintfnoup("  hreset                       Hard restart\n");
        update(stdvt());
    } else if(strcmp("list", comm) == 0) {
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
                struct MemoryEntry* e = (struct MemoryEntry*)(0x1000);
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
    } else if(strcmp("read", comm) == 0) {
        comm = strtok(NULL, ' ');
        u32 did = atoi(comm);
        comm = strtok(NULL, ' ');
        u32 sector = hatoi(comm);
        comm = strtok(NULL, ' ');
        u32 addr = hatoi(comm);
        Device *dev = k_get_device_by_id(did, DEV_UNKNOWN);
        if(dev == NULL || addr == NULL) {
            kprintf("Format: read <int:drive id> <hex:sector> <hex:addr>\n");
        }

        DriveDeviceData* d = (DriveDeviceData*)dev->data;
        if(d->read_sector) {
            d->read_sector(dev, sector, (u8*)addr);
            kprintf("Read one sector.\n");
        } else {
            kprintf("Drive has no read_sector() function!\n");
        }
    } else if(strcmp("mount", comm) == 0) {
        comm = strtok(NULL, ' ');
        u32 did = atoi(comm);
        if(strcmp(comm, "") == 0) {
            kprintf("Format: mount <int: id> <string: path>\n");
            return;
        }

        comm = strtok(NULL, ' ');
        if(strcmp(comm, "") == 0) {
            kprintf("Format: mount <int: id> <string: path>\n");
            return;
        }

        if(!mount_drive(k_get_device_by_id(did, DEV_UNKNOWN), comm)) {
            kprintf("Mounting failed.\n");
        }
    } else if(strcmp("malloc", comm) == 0) {
        comm = strtok(NULL, ' ');
        u32 did = hatoi(comm);
        if(did == 0) {
            kprintf("Format: malloc <hex:size>\n");
        } else {
            kprintf("address: %X\n", k_malloc(did));
        }
    } else if(strcmp("hreset", comm) == 0) {
        reset_cpu();
    } else if(strcmp("sb16detect", comm) == 0) {
        // reset DSP
        outb(0x226, 1);
        io_wait();
        outb(0x226, 0);
        u16 l = 0xFFFF; // this is for slow SBs
        while(l != 0) {
            if(inb(0x22E) >> 7) {
                kprintf("output = 0x%X (expected 0xAA)\n", inb(0x22A));
                break;
            }
            l--;
        }
    } else {
        kprintf("Unknown command \"%s\"\n", comm);
    }
}

int shell_main(u32 mem) {
    fb = fb_get(RESOLUTION);
    if(fb == NULL) return 1;

    if(((FBInfo*)fb->data)->format == PixelFormat_TEXT_640x480) {
        WIDTH = ((FBDeviceData*)fb->data)->w;
    } else {
        WIDTH = ((FBDeviceData*)fb->data)->w/8;
    }

    vt = stdvt();
    if(vt == NULL) return 1;

    VTDeviceData *vtd = vt->data;
    vtd->write_handler = shell_write;

    const char* prompt = "\x1b\x0EShell\x1b\x0F>";

    clear_screen();

    kprintf("-= \x1b\x03GaOs \x1b\x0EShell\x1b\x0F v1.0 =-\n");
    kprintf("%s\x1b%c ", prompt, cur_color);

    while(running) {
        u8 sc = scanc();
        if(sc == '\b') {
            if(ind != 0) {
                ind--;
                putc_vt(vt, sc);
                cmdbfr[ind] = '\0';
            }
        } else if(sc == '\n') {
            putc_vt(vt, sc);
            run_command(mem);
            for(int x=0;x<ind;x++) cmdbfr[x] = 0;
            ind = 0;
            if(running) kprintf("%s\x1b%c ", prompt, cur_color);
        } else {
            if(ind > sizeof(cmdbfr)) continue;
            putc_vt(vt, sc);
            cmdbfr[ind++] = sc;
        }
    }

    return 0;
}