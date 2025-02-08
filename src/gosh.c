#include <gosh/gosh.h>
#include <stdarg.h>
#include <memory.h>
#include <str.h>
#include <math.h>
#include <port.h>
#include <types.h>

static u32 d_id = 0;

/************************************ MISC ************************************/

#define DEBUG
static u32 cursor = 0;

int initialize_serial() {
    // COM1
    outb(0x3F8 + 1, 0x00);    // Disable all interrupts
    outb(0x3F8 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(0x3F8 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(0x3F8 + 1, 0x00);    //                  (hi byte)
    outb(0x3F8 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(0x3F8 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(0x3F8 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    outb(0x3F8 + 4, 0x1E);    // Set in loopback mode, test the serial chip
    outb(0x3F8 + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    if(inb(0x3F8 + 0) != 0xAE) {
        return -1;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(0x3F8 + 4, 0x0F);
    // puts_dbg("\x1b[2J");
    return 0;
}

#ifdef DEBUG

void putc_dbg(u8 c) {
    while(inb(0x3F8 + 5) & 0x20 == 0);
    if(c == '\n') putc_dbg('\r');
    outb(0x3F8, c);
}

void puts_dbg(const char *st) {
    write(DBGOUT, (void*)st, strlen((char*)st));
    // in_ansi = FALSE; ab = 0; memset(ansi, 0, sizeof(ansi));
    // do {
    //     putc_dbg(*st);
    //     st++;
    // } while(*st != 0);
}

#else
void putc_text(u8 c) {}
void end_dbg_stdout() {}
void puts_dbg(const char *st) {}
#endif

// This is blatantly wrong and mildly disgusting (erego, perfect!)
// (io_wait() takes ~1-4 microseconds; assume it runs at 1 microsecond)
void wait(u32 ms) {
    u32 mis = ms*1000;
    for(u32 i=0;i<mis;i++) {
        io_wait();
        io_wait();
        io_wait();
        io_wait();
    }
}

// Bare minimum function that you probably shouldn't use
void play_sound(u32 nFrequence) {
     // u32 Div;
     // u8 tmp;
 
    // //Set the PIT to the desired frequency
     // Div = 1193180 / nFrequence;
     // outb(0x43, 0xb6);
     // outb(0x42, (u8) (Div) );
     // outb(0x42, (u8) (Div >> 8));
 
    // //And play the sound using the PC speaker
     // tmp = inb(0x61);
      // if (tmp != (tmp | 3)) {
     //     outb(0x61, tmp | 3);
     // }
}
 
//make it shut up
void nosound() {
    u8 tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}

__attribute__((noreturn)) void kpanic() {
    // play_sound(1000);
    // wait(100);
    // nosound();
    puts_dbg("\x1b[1;33mKernel panic!\x1b[0m\n");
    __asm__ volatile ("cli\n" "hlt":);
    while(1);
}

/********************************** KEYBOARD **********************************/
/*********************************** DEVICE ***********************************/
/*********************************** DRIVER ***********************************/

#define MAX_MODULES 128

static module_t *mods[MAX_MODULES];
static bool module_manager_loaded = false;

int _setup_module_manager() {
    if(module_manager_loaded) return -1;
    module_manager_loaded = true;
    memset(mods, 0, sizeof(mods));    
    return 0;
}

int open_module(module_t* module) {
    if(module == NULL) return -1;

    bool found_pci = false;

    if(module->pci_flags.search) {
        for(int i=0;i<256;i++) {
            for(int x=0;x<32;x++) {
                PCIHeader header = pci_get_header(i, x);
                if(header.vendor == 0xFFFF) continue;

                if(module->pci_flags.vendor != 0xFFFF && module->pci_flags.vendor != header.vendor)
                    continue;
                if(module->pci_flags.device != 0xFFFF && module->pci_flags.device != header.device)
                    continue;
                if(module->pci_flags.class != 0xFF && module->pci_flags.class != header.class_code)
                    continue;
                if(module->pci_flags.subclass != 0xFF && module->pci_flags.subclass != header.subclass)
                    continue;
                if(module->pci_flags.interface != 0xFF && module->pci_flags.interface != header.prog_if)
                    continue;

                module->pci_flags.r_slot = x;
                module->pci_flags.r_bus  = i;
                
                found_pci = true;
                break;
            }
            if(found_pci) break;
        }
    }

    if(module->pci_flags.search && found_pci == false) {
        // no pci device for this module is found
        return 0;
       }

    for(int i=0;i<MAX_MODULES;i++) {
        if(mods[i] != NULL) continue;

        mods[i] = module;
        char *name = (char*)module->name;
        if(name == NULL) name = "Unknown";
        module->id = i;
        if(module->module_start) module->module_start(module);
        return 0;
    }

    kprintf("Failed loading module \"%s\"!\n", module->name);
}

/************************************ INTS ************************************/

void k_handle_int(u8 int_id) {
    u16 ind = int_id/8;
    u16 bit = int_id % 8;

    for(u32 i=0;i<MAX_MODULES;i++) {
        if(mods[i] != NULL) {
            module_t* mod = mods[i];

            u8 j = mod->r_active_ints[ind];
            if(j & (1<<bit) && mod->module_int != NULL)
                mod->module_int(mods[i], int_id);
        }
    }
}

int k_register_int(module_t *mod, u8 int_id) {
    if(mod == NULL) return false;
    mod->r_active_ints[(u8)(int_id/8)] |= (1 << (int_id%8));
    return true;
}

int k_unregister_int(module_t *mod, u8 int_id) {
    if(mod == NULL) return false;
    mod->r_active_ints[(u8)(int_id/8)] &= ~(1 << (int_id%8));
    return true;
}

/************************************ PCIE ************************************/

u16 pci_read_config(u8 bus, u8 slot, u8 func, u8 offset) {
    // This is mostly copied from https://wiki.osdev.org/PCI
    u32 address;
    u32 lbus  = (u32)bus;
    u32 lslot = (u32)slot;
    u32 lfunc = (u32)func;
    u16 tmp = 0;
  
    address = (u32)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((u32)0x80000000));
  
    // Write out the address
    outl(0xCF8, address);

    // Read in the data
    tmp = (u16)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
    return tmp;
}

u32 pci_read_config32(u8 bus, u8 slot, u8 func, u8 offset) {
    // This is mostly copied from https://wiki.osdev.org/PCI
    u32 address;
    u32 lbus  = (u32)bus;
    u32 lslot = (u32)slot;
    u32 lfunc = (u32)func;
    u16 tmp = 0;
  
    // Write out the address
    outl(0xCF8, (u32)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | ((u32)0x80000000)));
    return inl(0xCFC);
}

void pci_write_config32(u8 bus, u8 slot, u8 func, u8 offset, u32 data) {
    // This is mostly copied from https://wiki.osdev.org/PCI
    u32 address;
    u32 lbus  = (u32)bus;
    u32 lslot = (u32)slot;
    u32 lfunc = (u32)func;
    u16 tmp = 0;
  
    // Write out the address
    outl(0xCF8, (u32)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | ((u32)0x80000000)));
    outl(0xCFC, data);
}

PCIHeader pci_get_header(u8 bus, u8 slot) {
    PCIHeader header;

    header.vendor           = pci_read_config(bus, slot, 0, 0);
    header.device           = pci_read_config(bus, slot, 0, 2);
    header.command          = pci_read_config(bus, slot, 0, 4);
    header.status           = pci_read_config(bus, slot, 0, 6);
    u16 byte                = pci_read_config(bus, slot, 0, 8);
    header.rev_id           = byte & 0xFF;
    header.prog_if          = (byte >> 8) & 0xFF;
    byte                    = pci_read_config(bus, slot, 0, 10);
    header.subclass         = byte & 0xFF;
    header.class_code       = (byte >> 8) & 0xFF;
    byte                    = pci_read_config(bus, slot, 0, 8);
    header.cache_line_size  = byte & 0xFF;
    header.latency_timer    = (byte >> 8) & 0xFF;
    byte                    = pci_read_config(bus, slot, 0, 10);
    header.header_type      = byte & 0xFF;
    header.BIST             = (byte >> 8) & 0xFF;
    
    return header;
}

GenPCIHeader pci_get_gen_header(u8 bus, u8 slot) {
    GenPCIHeader header;

    if(pci_read_config(bus, slot, 0, 0) == 0xFFFF) return header;

    header.bar[0] = pci_read_config(bus, slot, 0, 0x10)
                | (pci_read_config(bus, slot, 0, 0x12) << 16);
    header.bar[1] = pci_read_config(bus, slot, 0, 0x14)
                | (pci_read_config(bus, slot, 0, 0x16) << 16);
    header.bar[2] = pci_read_config(bus, slot, 0, 0x18)
                | (pci_read_config(bus, slot, 0, 0x1A) << 16);
    header.bar[3] = pci_read_config(bus, slot, 0, 0x1C)
                | (pci_read_config(bus, slot, 0, 0x1E) << 16);
    header.bar[4] = pci_read_config(bus, slot, 0, 0x20)
                | (pci_read_config(bus, slot, 0, 0x22) << 16);
    header.bar[5] = pci_read_config(bus, slot, 0, 0x24)
                | (pci_read_config(bus, slot, 0, 0x26) << 16);
    
    header.cis_pointer          = pci_read_config(bus, slot, 0, 0x28) | (pci_read_config(bus, slot, 0, 0x2A) << 16);
    header.subsystem_vendor_id  = pci_read_config(bus, slot, 0, 0x2C);
    header.subsystem_id         = pci_read_config(bus, slot, 0, 0x2E);
    header.expansion_rom_addr   = pci_read_config(bus, slot, 0, 0x28) | (pci_read_config(bus, slot, 0, 0x30) << 16);

    header.capabilities_pointer = pci_read_config(bus, slot, 0, 0x2C) & 0xFF;

    u16 byte = pci_read_config(bus, slot, 0, 0x3C);
    header.interrupt_line       = byte & 0xFF;
    header.interrupt_pin        = (byte >> 8) & 0xFF;
    byte = pci_read_config(bus, slot, 0, 0x3E);
    header.min_grant            = byte & 0xFF;
    header.max_latency          = (byte >> 8) & 0xFF;
    
    return header;
}

u16 scan_pci(u16 class, u16 subclass) {
    for(u8 i=0;i<255;i++) {
        for(u8 x=0;x<32;x++) {
            PCIHeader header = pci_get_header(i, x);
            if(header.class_code == class && header.subclass) return (i << 8) | x;
        }
    }
    
    return 0xFFFF;
}

void pci_set_comm(u16 command, u8 bus, u8 slot) {
    pci_write_config32(bus, slot, 0, 4, pci_read_config32(bus, slot, 0, 4) | command);
}

void pci_unset_comm(u16 command, u8 bus, u8 slot) {
    pci_write_config32(bus, slot, 0, 4, pci_read_config32(bus, slot, 0, 4) & ~command);
}

u16  pci_get_comm(u8 bus, u8 slot) {
    return pci_read_config(bus, slot, 0, 4);
}

/******************************** FRAME BUFFER ********************************/

// #define draw_index_pixel(buf, x, y, w, col) *(u8*)((buf)+(x)+((y)*w)) = (col)
// #define draw_plane_pixel(buf, x, y, w, col) if((col) == 0) *(u8*)((buf)+((x) + (y)*(w))) |= (col); else *(u8*)((buf)+((x) + (y)*(w))) &= ~(col);

char* __int_str(u32 i, char b[], int base, bool plusSignIfNeeded, bool spaceSignIfNeeded,
                int paddingNo, bool justify, bool zeroPad) {
    
    char digit[32] = {0};
    memset(digit, 0, 32);
    strcpy(digit, "0123456789");
    
    if (base == 16) {
        strcat(digit, "ABCDEF");
    } else if (base == 17) {
        strcat(digit, "abcdef");
        base = 16;
    }
    
    char* p = b;
    if (i < 0) {
        *p++ = '-';
        i *= -1;
    } else if (plusSignIfNeeded) {
        *p++ = '+';
    } else if (!plusSignIfNeeded && spaceSignIfNeeded) {
        *p++ = ' ';
    }
    
    u32 shifter = i;
    do {
        ++p;
        shifter = shifter / base;
    } while (shifter);
    
    *p = '\0';
    do {
        *--p = digit[i % base];
        i = i / base;
    } while (i);
    
    int padding = paddingNo - (int)strlen(b);
    if (padding < 0) padding = 0;
    
    if (justify) {
        while (padding--) {
            if (zeroPad) {
                b[strlen(b)] = '0';
            } else {
                b[strlen(b)] = ' ';
            }
        }
        
    } else {
        char a[256] = {0};
        while (padding--) {
            if (zeroPad) {
                a[strlen(a)] = '0';
            } else {
                a[strlen(a)] = ' ';
            }
        }
        strcat(a, b);
        strcpy(b, a);
    }
    
    return b;
}

void putc(char c) {
    write(DBGOUT, &c, 1);
}

void v_kprintf(const char *format, va_list list) {
    u32 chars = 0;
    char intStrBuffer[256] = {0};

    for(int i=0; format[i]; ++i) {
        char specifier   = '\0';
        char length      = '\0';
        int  lengthSpec  = 0; 
        int  precSpec    = 0;
        bool leftJustify = FALSE;
        bool zeroPad     = FALSE;
        bool spaceNoSign = FALSE;
        bool altForm     = FALSE;
        bool plusSign    = FALSE;
        bool emode       = FALSE;
        int  expo        = 0;

        if(format[i] == '%') {
            ++i;

            bool extBreak = FALSE;
            while(1) {
                switch (format[i]) {
                    case '-':
                        leftJustify = TRUE;
                        ++i;
                        break;
                        
                    case '+':
                        plusSign = TRUE;
                        ++i;
                        break;
                        
                    case '#':
                        altForm = TRUE;
                        ++i;
                        break;
                        
                    case ' ':
                        spaceNoSign = TRUE;
                        ++i;
                        break;
                        
                    case '0':
                        zeroPad = TRUE;
                        ++i;
                        break;
                        
                    default:
                        extBreak = TRUE;
                        break;
                }
                
                if (extBreak) break;
            }

            while(is_digit(format[i])) {
                lengthSpec *= 10;
                lengthSpec += format[i] - '0';
                ++i;
            }

            if(format[i] == '*') {
                lengthSpec = va_arg(list, int);
                ++i;
            }

            if(format[i] == '.') {
                ++i;
                while (is_digit(format[i])) {
                    precSpec *= 10;
                    precSpec += format[i] - 48;
                    ++i;
                }
                
                if (format[i] == '*') {
                    precSpec = va_arg(list, int);
                    ++i;
                }
            }

            if (format[i] == 'h' || format[i] == 'l' || format[i] == 'j' ||
                   format[i] == 'z' || format[i] == 't' || format[i] == 'L') {
                length = format[i];
                ++i;
                if (format[i] == 'h') {
                    length = 'H';
                } else if (format[i] == 'l') {
                    length = 'q';
                    ++i;
                }
            }
            specifier = format[i];
            
            memset(intStrBuffer, 0, 256);
            
            int base = 10;
            if (specifier == 'o') {
                base = 8;
                specifier = 'u';
                if (altForm) {
                    u8 s = '0';
                    write(DBGOUT, &s, 1);
                }
            }
            if (specifier == 'p') {
                base = 16;
                length = 'z';
                specifier = 'u';
            }
            switch (specifier) {
                case 'X':
                    base = 16;
                case 'x':
                    base = base == 10 ? 17 : base;
                    if (altForm) {
                        write(DBGOUT, "0x", 2);
                    }
                    
                case 'u':
                {
                    switch (length) {
                        case 0:
                        {
                            unsigned int integer = va_arg(list, unsigned int);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                            break;
                        }
                        case 'H':
                        {
                            unsigned char integer = (unsigned char) va_arg(list, unsigned int);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                            break;
                        }
                        case 'h':
                        {
                            unsigned short int integer = va_arg(list, unsigned int);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                            break;
                        }
                        case 'l':
                        {
                            unsigned long integer = va_arg(list, unsigned long);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                            break;
                        }
                        case 'q':
                        {
                            unsigned long long integer = va_arg(list, unsigned long long);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                            break;
                        }
                        case 'j':
                        {
                            u32 integer = va_arg(list, u32);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                            break;
                        }
                        case 'z':
                        {
                            size_t integer = va_arg(list, size_t);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                            break;
                        }
                        case 't':
                        {
                            ptrdiff_t integer = va_arg(list, ptrdiff_t);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }
                    
                case 'd':
                case 'i':
                {
                    switch (length) {
                    case 0:
                    {
                        int integer = va_arg(list, int);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                        break;
                    }
                    case 'H':
                    {
                        signed char integer = (signed char) va_arg(list, int);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                        break;
                    }
                    case 'h':
                    {
                        short int integer = va_arg(list, int);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                        break;
                    }
                    case 'l':
                    {
                        long integer = va_arg(list, long);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                        break;
                    }
                    case 'q':
                    {
                        long long integer = va_arg(list, long long);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                        break;
                    }
                    case 'j':
                    {
                        u32 integer = va_arg(list, u32);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                        break;
                    }
                    case 'z':
                    {
                        size_t integer = va_arg(list, size_t);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                        break;
                    }
                    case 't':
                    {
                        ptrdiff_t integer = va_arg(list, ptrdiff_t);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                        break;
                    }
                    default:
                        break;
                    }
                    break;
                }
                    
                case 'c':
                {
                    if (length == 'l') {
                        putc(va_arg(list, wint_t));
                    } else {
                        putc(va_arg(list, int));
                    }
                    
                    break;
                }
                    
                case 's':
                {
                    char *st = va_arg(list, char*);
                    write(DBGOUT, st, strlen(st));
                    break;
                }
                    
                case 'n':
                {
                    switch (length) {
                        case 'H':
                            *(va_arg(list, signed char*)) = chars;
                            break;
                        case 'h':
                            *(va_arg(list, short int*)) = chars;
                            break;
                            
                        case 0: {
                            int* a = va_arg(list, int*);
                            *a = chars;
                            break;
                        }
                            
                        case 'l':
                            *(va_arg(list, long*)) = chars;
                            break;
                        case 'q':
                            *(va_arg(list, long long*)) = chars;
                            break;
                        case 'j':
                            *(va_arg(list, u32*)) = chars;
                            break;
                        case 'z':
                            *(va_arg(list, size_t*)) = chars;
                            break;
                        case 't':
                            *(va_arg(list, ptrdiff_t*)) = chars;
                            break;
                        default:
                            break;
                    }
                    break;
                }
                   
                case 'e':
                case 'E':
                    emode = TRUE;
                    
                case 'f':
                case 'F':
                case 'g':
                case 'G':
                {
                    double floating = va_arg(list, double);
                    
                    while (emode && floating >= 10) {
                        floating /= 10;
                        ++expo;
                    }
                    
                    int form = lengthSpec - precSpec - expo - (precSpec || altForm ? 1 : 0);
                    if (emode) {
                        form -= 4;      // 'e+00'
                    }
                    if (form < 0) {
                        form = 0;
                    }
                    
                    __int_str(floating, intStrBuffer, base, plusSign, spaceNoSign, form, \
                              leftJustify, zeroPad);
                    
                    write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                    
                    floating -= (int) floating;
                    
                    for (int i = 0; i < precSpec; ++i) {
                        floating *= 10;
                    }
                    u32 decPlaces = (u32) (floating + 0.5);
                    
                    if (precSpec) {
                        putc('.');
                        __int_str(decPlaces, intStrBuffer, 10, FALSE, FALSE, 0, FALSE, FALSE);
                        intStrBuffer[precSpec] = 0;
                        write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
                    } else if (altForm) {
                        putc('.');
                    }
                    
                    break;
                }
                    
                    
                case 'a':
                case 'A':
                    //ACK! Hexadecimal floating points...
                    break;
                
                default:
                    break;
            }
            
            if (specifier == 'e') {
                write(DBGOUT, "e+", 2);
            } else if (specifier == 'E') {
                write(DBGOUT, "E+", 2);
            }
            
            if (specifier == 'e' || specifier == 'E') {
                __int_str(expo, intStrBuffer, 10, FALSE, FALSE, 2, FALSE, TRUE);
                write(DBGOUT, intStrBuffer, strlen(intStrBuffer));
            }
            
        } else {
            write(DBGOUT, (char*)format+i, 1);
        }
    }
}

__attribute__ ((format (printf, 1, 2))) void kprintf(const char* format, ...) {
    // Device *vt = k_get_device_by_owner(KERNEL_ID, DEV_VIRT_TERM);
    // if(vt == NULL) return;
    va_list list;
    va_start (list, format);
    v_kprintf(format, list);
    va_end (list);
}

/*********************************** DRIVES ***********************************/
/***************************** VIRTUAL FILESYSTEM *****************************/

typedef struct _BPB {
    u8  jmp[3];             // should be equal to 0xEB, a byte, and 0x90
    u8  oem[8];             // OEM identifier
    u16 bytes_per_sector;   // Bytes per sector
    u8  secs_per_cluster;   // Sectors per cluster
    u16 reserved_sectors;   // Reserved sectors
    u8  fats_count;         // # of FATs on the medium
    u16 rootdir_entries;    // # of root directory entries
    u16 total_secs;         // Total sectors (determines FAT type)
    u8  media_desc_type;    // type of media
    u16 secs_per_fat;       // Sectors per fat
    u16 secs_per_track;     // Sectors per track
    u16 heads_in_media;     // Number of heads
    u32 num_hidden_secs;    // Number of hidden sectors
    u32 large_sec_count;    // Large sector count

    // FAT32
    u32 secsperfat;         // Sectors per FAT
    u16 flags;              // Flags
    u16 fat_version;        // FAT version number
    u32 cluster_root;       // The cluster number of the root directory.
    u16 fsinfo_sec;         // FSInfo sector
    u16 sec_backup_boot;    // Backup boot sector
    u8 resv[12];            // These should all be zero
    u8 drivenum;            // Drive number (0x00 = floppy, 0x80 = harddisk)
    u8 ntflags;             // Reserved (NT flags)
    u8 signature;           // Should be 0x28 or 0x29
    u32 volume_serial_num;  // Serial number
    u8 volume_label[11];    // Volume label, padded with spaces
    u8 sysiden[8];          // Always "FAT32    ", but ignore
    u8 bootcode[420];       // Ignore data here
    u16 bootable_signature; // 0xAA55
} BPB;
