// Generic shell program for testing
#include <shell.h>
#include <gosh/gosh.h>
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
    "Unknown         ",
    "Keyboard        ",
    "Mouse           ",
    "Network         ",
    "Speaker         ",
    "Microphone      ",
    "Drive           ",
    "Framebuffer     ",
    "Driver          "
};

static u16 WIDTH    = 0;
static u16 HEIGHT   = 0;

static u16 curx     = 0;
static u16 cury     = 0;

static u8 cur_color = 15;

static u8 cmdbfr[80];
static u8 ind = 0;

static Device *fb;
static Device *vt;
static FBInfo info;

static u8 running = TRUE;

static u32 prevind = 0;

static bool in_color = FALSE;

void clear_screen() {
    fb_clear(fb, 0);
    curx = 0;
    cury = 0;
}

void putc(u8 c) {
    kprintf("%c", c);
}

#include <math.h>
void hexdump(u8* addr, u32 size) {
    if(size == 0) return;
    for(int i=0;i<size;i++) {
        if((i % 16) == 0) {
            // output the ASCII equivalent of the past 16 bytes
            if(i != 0) {
                kprintf(" |");
                for(int x=0;x<16;x++) {
                    u8 c = *(addr+i-16+x);
                    if(is_printable(c)) putc(c);
                    else putc('.');
                }
                kprintf("|\n");
            }
            kprintf("%08X  ", addr+i);
        } else if((i%8) == 0 && i != 0) putc(' ');
        kprintf("%02X ", *(addr+i));
    }
    u8 ex = size%16;
    if(ex != 0) {
        // putc(' ');
        for(int i=0;i<16-ex;i++) kprintf("   ");
    }
    kprintf(" |");
    ex = size%17;
    u32 ch_to_write = 16-ex;
    if(size <= 16) ch_to_write = size;
    for(int x=0;x<ex;x++) {
        u8 c = *(addr+size-(16-ex)+x);
        if(is_printable(c)) kprintf("%c", c);
        else putc('.');
    }
    kprintf("|");
}


void run_command(u32 mem) {
    u8* comm = strtok(cmdbfr, ' ');

    if(strcmp("", comm) == 0) {
    } else if(strcmp("help", comm) == 0) {
        kprintf("\x1b[97m");
        kprintf("Commands:\n");
        kprintf("  list     <subcommand>        list system data\n");
        kprintf("  cls                          clears the terminal\n");
        kprintf("  color    <color>             set the foreground text color\n");
        kprintf("  hexdump  <addr> <count>      hexdump count bytes at addr\n");
        kprintf("  read     <did> <sec> <addr>  Read a sector from a drive\n");
        kprintf("  mount    <drive> <st:path>   Mount a drive\n");
        kprintf("  malloc   <size>              Allocate memory and output the address\n");
        kprintf("  hreset                       Hard restart\n");
        kprintf("  in[b/l]  <port>              Read data from a system port\n");
        kprintf("  out[b/l] <port> <data>       Write data to a system port\n");
        kprintf("\x1b[0m", cur_color);
    } else if(strcmp("list", comm) == 0) {
        comm = strtok(NULL, ' ');
        if(strcmp("pci", comm) == 0) {
            kprintf("PCI Devices:\n");
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
                kprintf("0x%08X%08X | 0x%08X%08X | %u\n", e[i].base2, e[i].base1, e[i].len2, e[i].len1, e[i].type);
            }
        } else if(strcmp("stats", comm) == 0) {
            kprintf("Memory usage: %uKiB / %uKiB\n", k_get_used()/1024, MEM_MAX/1024);
        } else if(strcmp("devs", comm) == 0) {
            Device* devs    = get_gdevt();
            u32 devsize     = get_gdevt_len();
            for(int i=0;i<devsize;i++) {
                if(!devs[i].is_active) continue;
                kprintf("%03u: %s (id=%03u, owner = 0x%X)\n", i, DEVTYPE_ST[devs[i].type], devs[i].id, devs[i].owner);
            }
        } else if(strcmp("", comm) == 0) {
            kprintf("Subcommands:\n  pci - list all connected pci devices\n  mem - list all memory segments\n  stats - list OS stats\n  devs - list OS devices\n");
        } else {
            kprintf("Unkown list subcommand: \"");
            kprintf(comm);
            kprintf("\"\n");
        }
    } else if(strcmp("cls", comm) == 0) {
        kprintf("\x1b[2J");
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
    } else if(strcmp("inb", comm) == 0) {
        comm = strtok(NULL, ' ');
        u32 port = hatoi(comm);
        if(strcmp(comm, "") == 0) {
            kprintf("Format: inb <port: hex>\n");
            return;
        }

        kprintf("0x%02X (port = 0x%04X)\n", inb(port & 0xFFFF), port & 0xFFFF);
    } else if(strcmp("outb", comm) == 0) {
        comm = strtok(NULL, ' ');
        u32 port = hatoi(comm);
        if(strcmp(comm, "") == 0) {
            kprintf("Format: outb <port: hex> <val: hex>\n");
            return;
        }

        comm = strtok(NULL, ' ');
        u32 val = hatoi(comm);
        if(strcmp(comm, "") == 0) {
            kprintf("Format: outb <port: hex> <val: hex>\n");
            return;
        }

        outb(port & 0xFFFF, val & 0xFF);
    } else if(strcmp("inl", comm) == 0) {
        comm = strtok(NULL, ' ');
        u32 port = hatoi(comm);
        if(strcmp(comm, "") == 0) {
            kprintf("Format: inl <port: hex>\n");
            return;
        }

        kprintf("0x%08X (port = 0x%04X)\n", inl(port & 0xFFFF), port & 0xFFFF);
    } else if(strcmp("outl", comm) == 0) {
        comm = strtok(NULL, ' ');
        u32 port = hatoi(comm);
        if(strcmp(comm, "") == 0) {
            kprintf("Format: outl <port: hex> <val: hex>\n");
            return;
        }

        comm = strtok(NULL, ' ');
        u32 val = hatoi(comm);
        if(strcmp(comm, "") == 0) {
            kprintf("Format: outl <port: hex> <val: hex>\n");
            return;
        }

        outl(port & 0xFFFF, val);
    } else if(strcmp("curat", comm) == 0) {
        kprintf("%u, %u\n", curx, cury);
    } else {
        kprintf("Unknown command \"%s\"\n", comm);
    }
}

int shell_main(u32 mem) {
    const char* prompt = "\x1b[94mShell\x1b[97m>";

    clear_screen();

    kprintf("-= \x1b[93mGaOs \x1b[94mShell\x1b[0m v1.0 =-\n");
    kprintf("%s\x1b[97m ", prompt);

    bool extra = FALSE;

    while(running) {
        u8 sc = scanc();

        if(extra) {
            if (sc == 0x4B) {
                // Left Arrow
            } else if (sc == 0x4D) {
                // Right Arrow
            } else if (sc == 0x48) {
                // Up Arrow
            } else if (sc == 0x5C) {
                // Down Arrow
            }

            extra = FALSE;
        } else if(sc == 0xE0) {
            extra = TRUE;
        } else {
            if(sc == '\b') {
                if(ind != 0) {
                    ind--;
                    putc(sc);
                    cmdbfr[ind] = '\0';
                }
            } else if(sc == '\n') {
                putc(sc);
                run_command(mem);
                for(int x=0;x<ind;x++) cmdbfr[x] = 0;
                ind = 0;

                if(running) kprintf("%s ", prompt);
            } else {
                if(ind > sizeof(cmdbfr)) continue;
                putc(sc);
                cmdbfr[ind++] = sc;
            }
        }

    }

    return 0;
}