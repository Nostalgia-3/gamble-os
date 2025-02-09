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
static u8 cur_color = 15;
static u8 cmdbfr[80];
static u8 ind = 0;
static u8 running = TRUE;
static u32 prevind = 0;
static bool in_color = FALSE;

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

void write_char(char c) {
    cmdbfr[ind++] = c;
}

void run_command(multiboot_info_t *mbd) {
    u8* comm = strtok(cmdbfr, ' ');

    if(strcmp("", comm) == 0) {
    } else if(strcmp("help", comm) == 0) {
        kprintf("Commands:\n");
        kprintf("  \x1b[93mlist\x1b[90m     <subcommand>        \x1b[0mlist system data\n");
        kprintf("  \x1b[93mcls\x1b[90m                          \x1b[0mclears the terminal\n");
        kprintf("  \x1b[93mhexdump\x1b[90m  <addr> <count>      \x1b[0mhexdump count bytes at addr\n");
        kprintf("  \x1b[93mread\x1b[90m     <did> <sec> <addr>  \x1b[0mRead a sector from a drive\n");
        kprintf("  \x1b[93mmount\x1b[90m    <drive> <st:path>   \x1b[0mMount a drive\n");
        kprintf("  \x1b[93mmalloc\x1b[90m   <size>              \x1b[0mAllocate memory and output the address\n");
        kprintf("  \x1b[93mhreset\x1b[90m                       \x1b[0mHard restart\n");
        kprintf("  \x1b[93min[b/l]\x1b[90m  <port>              \x1b[0mRead data from a system port\n");
        kprintf("  \x1b[93mout[b/l]\x1b[90m <port> <data>       \x1b[0mWrite data to a system port\n");
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
            for(int i=0;i<mbd->mmap_length;i+=sizeof(multiboot_memory_map_t))  {
                multiboot_memory_map_t* mmmt =  (multiboot_memory_map_t*) (mbd->mmap_addr + i);

                kprintf(
                    "0x%08X%08X | 0x%08X%08X | %u\n",
                    (u32)(mmmt->addr >> 32), (u32)(mmmt->addr & 0xFFFFFFFF),
                    (u32)(mmmt->len >> 32), (u32)(mmmt->len & 0xFFFFFFFF),
                    mmmt->type
                );
            }
        } else if(strcmp("stats", comm) == 0) {
            kprintf("Memory usage: %uKiB / %uKiB\n", k_get_used()/1024, MEM_MAX/1024);
        } else if(strcmp("", comm) == 0) {
            kprintf("Subcommands:\n  pci - list all connected pci devices\n  mem - list all memory segments\n  stats - list OS stats\n  devs - list OS devices\n");
        } else if(strcmp("devs", comm) == 0) {
            device_t** devs = get_devices();
            u16 col = 0;
            for(int i=0;i<get_total_devices();i++) {
                if(devs[i] != NULL) {
                    if((col + strlen((char*)devs[i]->name) + 1) > 80) {
                        kprintf("\n");
                        col = 0;
                    }
                    kprintf("%s ", devs[i]->name);
                    col += strlen((char*)devs[i]->name) + 1;
                }
            }
            kprintf("\n");
        } else {
            kprintf("Unkown list subcommand: \"%s\"\n", comm);
        }
    } else if(strcmp("cls", comm) == 0 || strcmp("clear", comm) == 0) {
        kprintf("\x1b[2J");
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
        // comm = strtok(NULL, ' ');
        // u32 did = atoi(comm);
        // comm = strtok(NULL, ' ');
        // u32 sector = hatoi(comm);
        // comm = strtok(NULL, ' ');
        // u32 addr = hatoi(comm);
        // Device *dev = k_get_device_by_id(did, DEV_UNKNOWN);
        // if(dev == NULL || addr == NULL) {
        //     kprintf("Format: read <int:drive id> <hex:sector> <hex:addr>\n");
        // }

        // DriveDeviceData* d = (DriveDeviceData*)dev->data;
        // if(d->read_sector) {
        //     d->read_sector(dev, sector, (u8*)addr);
        //     kprintf("Read one sector.\n");
        // } else {
        //     kprintf("Drive has no read_sector() function!\n");
        // }
    } else if(strcmp("mount", comm) == 0) {
        // comm = strtok(NULL, ' ');
        // u32 did = atoi(comm);
        // if(strcmp(comm, "") == 0) {
        //     kprintf("Format: mount <int: id> <string: path>\n");
        //     return;
        // }

        // comm = strtok(NULL, ' ');
        // if(strcmp(comm, "") == 0) {
        //     kprintf("Format: mount <int: id> <string: path>\n");
        //     return;
        // }

        // if(!mount_drive(k_get_device_by_id(did, DEV_UNKNOWN), comm)) {
        //     kprintf("Mounting failed.\n");
        // }
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
    } else if(strcmp("ls", comm) == 0) {
        comm = strtok(NULL, '\0');
        if(strlen(comm) == 0) {
            // TODO: replace this with a cwd
            comm = "/";
        }
        
        int dir = open(comm);
        if(dir == -1) {
            kprintf("Failed to open \"%s\": no such directory\n", comm);
            return;
        }
        dirent ent = {0};
        int c = getdents(dir, &ent, 0);

        if(c == -1) {
            kprintf("Failed to read directory \"%s\"\n", comm);
            close(dir);
            return;
        }

        for(int i=0;i<c;i++) {
            getdents(dir, &ent, i);
            switch(ent.d_type) {
                case DT_DIR:
                    kprintf("\x1b[94m");
                break;

                case DT_DEV:
                    kprintf("\x1b[32m");
                break;
            }
            kprintf("%s\x1b[0m ", ent.d_name);
        }

        kprintf("\n");
        close(dir);
    } else {
        kprintf("Unknown command \"%s\"\n", comm);
    }
}

int shell_main(multiboot_info_t *mbd) {
    const char* prompt = "\x1b[94mShell\x1b[0m> ";

    kprintf("-= \x1b[93mGaOs \x1b[94mShell\x1b[0m v1.0 =-\n");
    kprintf("%s\x1b[0m", prompt);

    bool extra = FALSE;
    u8 sc = '\0';

    while(running) {
        ssize_t b = read(STDIN, &sc, 1);
        if(b == 0 || sc == '\0') continue;

        if(extra) {
            kprintf("%X\n", sc);

            switch(sc) {
                case 0x4B: if(ind > 0) ind--; break; // left arrow
                case 0x4D: if(ind < sizeof(cmdbfr)) ind++; break; // right arrow
                case 0x48: break; // Up Arrow
                case 0x5C: break; // Down Arrow
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
                run_command(mbd);
                memset(cmdbfr, 0, sizeof(cmdbfr));
                ind = 0;
            } else {
                if(ind > sizeof(cmdbfr)) continue;
                write_char(sc);
            }

            if(running) {
                // Fancy command highlighting
                char *comr = strtok(cmdbfr, ' ');
                char command[80];
                memset(command, 0, 80);
                for(int i=0;i<strlen(comr);i++) command[i] = comr[i];
                kprintf("\x1b[2K\r%s\x1b[93m%s\x1b[0m%s", prompt, command, cmdbfr+strlen(comr));
            }
        }
    }

    return 0;
}