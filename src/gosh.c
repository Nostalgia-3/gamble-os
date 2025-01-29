#include <gosh/gosh.h>
#include <stdarg.h>
#include <memory.h>
#include <str.h>
#include <math.h>
#include <port.h>
#include <types.h>

Device* gdevt;
u32 gdevt_len = 0;

static u32 d_id = 0;

/************************************ MISC ************************************/

#define DEBUG
static u32 cursor = 0;

void initialize_serial() {
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
        return;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(0x3F8 + 4, 0x0F);
    // puts_dbg("\x1b[2J");
    return;
}

#ifdef DEBUG

// ansi:        black, red, green, yellow, blue, magenta, cyan, white, reset
u8 ansi_to_text[] = { 0, 4, 2, 14, 1, 5, 3, 15, 7 };

bool debug_write_to_stdout = TRUE;

u8 ansi[64];
u8 ab;
bool in_ansi;
u8 color = 7;

void setcolor_dbg(u8 c) {
    color = c;
}

void putc_dbg(u8 c) {
    if(in_ansi) {
        if(c == 'm') {
            ansi[ab+1] = '\0';
            if(ansi[1] == '3' && is_digit(ansi[2])) {
                setcolor_dbg(ansi_to_text[ansi[2]-'0']);
            } else if(ansi[1] == '0') {
                setcolor_dbg(ansi_to_text[sizeof(ansi_to_text)-1]);
            }
            in_ansi = FALSE;
            ab = 0;
        } else {
            ansi[ab++] = c;
        }
    } else {
        if(c == '\x1b') {
            in_ansi = TRUE;
            ab = 0;
        } else {
            if(c == '\n') {
                cursor += 80 - (cursor % 80);
            } else if(c == '\t') {
                cursor += 8 - (cursor % 8);
            } else if(c != '\r' && debug_write_to_stdout) {
                *(char*)(0xB8000+cursor*2) = c;
                *(char*)(0xB8000+cursor*2+1) = color;
                cursor++;
            }
        }
    }

    // while(inb(0x3F8 + 5) & 0x20 == 0);
    // if(c == '\n') putc_dbg('\r');
    // outb(0x3F8, c);
}

void puts_dbg(const char *st) {
    in_ansi = FALSE; ab = 0; memset(ansi, 0, sizeof(ansi));
    do {
        putc_dbg(*st);
        st++;
    } while(*st != 0);
}

void end_dbg_stdout() {
    debug_write_to_stdout = FALSE;
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
 	// 	outb(0x61, tmp | 3);
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

void pushc(u8 c) {
    if(c == 0) return;
    Device* dev = k_get_device_by_owner(KERNEL_ID, DEV_KEYBOARD);
    if(dev == NULL) kpanic();

    KeyboardDeviceData* data = (KeyboardDeviceData*) dev->data;
    if(data == NULL) kpanic();

    if(data->fifo_ind > sizeof(data->fifo)) kpanic();
    data->fifo[data->fifo_ind++] = c;
}

// Halt the process until a character from the keyboard is detected
u8 scanc() {
    u8 k;
    do { k = getc(); } while(k == 0);
    return k;
}

// Get the latest ascii key pressed, or 0 if there was no key
u8 getc() {
    Device* dev = k_get_device_by_owner(KERNEL_ID, DEV_KEYBOARD);
    if(dev == NULL) return 0;

    KeyboardDeviceData* data = (KeyboardDeviceData*) dev->data;
    u8 k = data->fifo[0];
    if(k == 0) return 0;

    memcpy(data->fifo, data->fifo+1, sizeof(data->fifo)-1);
    if(data->fifo_ind != 0) data->fifo_ind--;
    return k;
}

/*********************************** DEVICE ***********************************/

int _init_gdevt() {
    if(gdevt != NULL) return 0;
    gdevt_len = 128;
    gdevt = k_malloc(sizeof(Device)*gdevt_len);
    if(gdevt == NULL) return GDEVT_FAILED_ALLOC;
    memset((void*)gdevt, 0, sizeof(Device)*gdevt_len);
    for(int i=0;i<gdevt_len;i++)
        gdevt[i].is_active = FALSE;

    // standard keyboard output
    Device *kbd = k_add_dev(KERNEL_ID, DEV_KEYBOARD, 0);
    if(kbd == NULL) {
        return GDEVT_FAILED_KBD;
    }

    Device *stdvt = k_add_dev(KERNEL_ID, DEV_VIRT_TERM, 0);
    if(stdvt != NULL) {
        VTDeviceData* vtdata = stdvt->data;
        if(vtdata != NULL) {
            vtdata->text = k_malloc(6000);
            if(vtdata->text != NULL) {
                memset(vtdata->text, 0, 6000);
                vtdata->textlen = 6000;
            }
        } else {
            // puts_dbg("VTDeviceData wasn't created!\n");
            return GDEVT_FAILED_VT_DATA;
        }
    } else {
        // puts_dbg("Failed to create stdout virtual terminal!\n");
        return GDEVT_FAILED_VT;
    }

    return 0;
}

Device* get_gdevt() {
    return gdevt;
}

u32 get_gdevt_len() {
    return gdevt_len;
}

u32 get_used_devices() {
    u32 total = 0;
    for(int i=0;i<gdevt_len;i++) {
        if(gdevt[i].id != 0) total++;
    }
    return total;
}

Device* k_add_dev(u32 kid, enum DeviceType dev, u32 code) {
    u32 id = 0;
    u32 ind = 0;

    for(size_t i=0;i<gdevt_len;i++) {
        if(gdevt[i].id == 0) {
            id = ++d_id;
            ind = i;
            break;
        }
    }

    // no free devices
    if(id == 0) {
        puts_dbg("No free devices left!\n");
        return NULL;
    }

    memset(&gdevt[ind], 0, sizeof(Device));

    switch(dev) {
        case DEV_KEYBOARD:
            gdevt[ind].data = k_malloc(sizeof(KeyboardDeviceData));
            if(gdevt[ind].data == NULL) return NULL;
            memset(gdevt[ind].data, 0, sizeof(KeyboardDeviceData));
        break;

        case DEV_DRIVER:
            // This is defined in k_load_driver() as the pointer to the driver
            gdevt[ind].data = NULL;
        break;

        case DEV_DRIVE:
            gdevt[ind].data = k_malloc(sizeof(DriveDeviceData));
            if(gdevt[ind].data == NULL) return NULL;
            memset(gdevt[ind].data, 0, sizeof(DriveDeviceData));
        break;

        case DEV_FRAMEBUFFER:
            gdevt[ind].data = k_malloc(sizeof(FBDeviceData));
            if(gdevt[ind].data == NULL) return NULL;
            memset(gdevt[ind].data, 0, sizeof(FBDeviceData));
        break;

        case DEV_VIRT_TERM:
            gdevt[ind].data = k_malloc(sizeof(VTDeviceData));
            if(gdevt[ind].data == NULL) return NULL;
            memset(gdevt[ind].data, 0, sizeof(VTDeviceData));
        break;

        case DEV_MOUSE:
            gdevt[ind].data = k_malloc(sizeof(MouseDeviceData));
            if(gdevt[ind].data == NULL) return NULL;
            memset(gdevt[ind].data, 0, sizeof(MouseDeviceData));
        break;

        default:
            kprintf("Device type #%u not handled yet\n", dev);
            return NULL;
        break;
    }

    gdevt[ind].id = id;
    gdevt[ind].code = code;
    gdevt[ind].type = dev;
    gdevt[ind].owner = kid;
    gdevt[ind].is_active = TRUE;

    return &gdevt[ind];
}

Device* k_get_device_by_owner(u32 owner, enum DeviceType type) {
    for(u32 i=0;i<gdevt_len;i++) {
        if(gdevt[i].owner == owner && (type == DEV_UNKNOWN || gdevt[i].type == type)) return &gdevt[i];
    }

    return NULL;
}

Device* k_get_device_by_id(u32 id, enum DeviceType type) {
    for(u32 i=0;i<gdevt_len;i++) {
        if(i == id && (type == DEV_UNKNOWN || gdevt[i].type == type)) return &gdevt[i];
    }

    return NULL;
}

Device* k_get_device_by_type(enum DeviceType type) {
    for(u32 i=0;i<gdevt_len;i++) {
        if(gdevt[i].type == type) return &gdevt[i];
    }

    return NULL;
}

/*********************************** DRIVER ***********************************/

u32 load_driver(Driver *driver) {
    if(driver == NULL) return 0;

    Device *dev = k_add_dev(KERNEL_ID, DEV_DRIVER, 0);
    if(dev == NULL) return 0;
    driver->r_id = dev->id;
    dev->data = driver;
    if(driver->DriverEntry != NULL)
        driver->DriverEntry(dev);
    return driver->r_id;
}

/************************************ INTS ************************************/

void k_handle_int(u8 int_id) {
    u16 ind = int_id/8;
    u16 bit = int_id % 8;

    for(u32 i=0;i<gdevt_len;i++) {
        if(gdevt[i].type == DEV_DRIVER) {
            Driver* driver = (Driver*)gdevt[i].data;
            
            if(driver == NULL) {
                // this shouldn't happen, but it does
                continue;
            }

            u8 j = driver->r_active_ints[ind];
            if(j & (1<<bit) && driver->DriverInt != NULL)
                driver->DriverInt(&gdevt[i], int_id);
        }
    }
}

bool k_register_int(Driver *driver, u8 int_id) {
    if(driver == NULL) return FALSE;

    driver->r_active_ints[(u8)(int_id/8)] |= (1 << (int_id%8));

    return TRUE;
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

char font8x8_basic[128][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0000 (nul)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0001
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0002
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0003
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0004
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0005
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0006
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0007
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0008
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0009
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0010
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0011
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0012
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0013
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0014
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0015
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0016
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0017
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0018
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0019
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0020 (space)
    { 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00},   // U+0021 (!)
    { 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0022 (")
    { 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00},   // U+0023 (#)
    { 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00},   // U+0024 ($)
    { 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00},   // U+0025 (%)
    { 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00},   // U+0026 (&)
    { 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0027 (')
    { 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00},   // U+0028 (()
    { 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00},   // U+0029 ())
    { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00},   // U+002A (*)
    { 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00},   // U+002B (+)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // U+002C (,)
    { 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00},   // U+002D (-)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // U+002E (.)
    { 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00},   // U+002F (/)
    { 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00},   // U+0030 (0)
    { 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00},   // U+0031 (1)
    { 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00},   // U+0032 (2)
    { 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00},   // U+0033 (3)
    { 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00},   // U+0034 (4)
    { 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00},   // U+0035 (5)
    { 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00},   // U+0036 (6)
    { 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00},   // U+0037 (7)
    { 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00},   // U+0038 (8)
    { 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00},   // U+0039 (9)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // U+003A (:)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // U+003B (;)
    { 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00},   // U+003C (<)
    { 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00},   // U+003D (=)
    { 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00},   // U+003E (>)
    { 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00},   // U+003F (?)
    { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00},   // U+0040 (@)
    { 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00},   // U+0041 (A)
    { 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00},   // U+0042 (B)
    { 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00},   // U+0043 (C)
    { 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00},   // U+0044 (D)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00},   // U+0045 (E)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00},   // U+0046 (F)
    { 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00},   // U+0047 (G)
    { 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00},   // U+0048 (H)
    { 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0049 (I)
    { 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00},   // U+004A (J)
    { 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00},   // U+004B (K)
    { 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00},   // U+004C (L)
    { 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00},   // U+004D (M)
    { 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00},   // U+004E (N)
    { 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00},   // U+004F (O)
    { 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00},   // U+0050 (P)
    { 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00},   // U+0051 (Q)
    { 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00},   // U+0052 (R)
    { 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00},   // U+0053 (S)
    { 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0054 (T)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00},   // U+0055 (U)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // U+0056 (V)
    { 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00},   // U+0057 (W)
    { 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00},   // U+0058 (X)
    { 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00},   // U+0059 (Y)
    { 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00},   // U+005A (Z)
    { 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00},   // U+005B ([)
    { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00},   // U+005C (\)
    { 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00},   // U+005D (])
    { 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00},   // U+005E (^)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF},   // U+005F (_)
    { 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0060 (`)
    { 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00},   // U+0061 (a)
    { 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00},   // U+0062 (b)
    { 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00},   // U+0063 (c)
    { 0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00},   // U+0064 (d)
    { 0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00},   // U+0065 (e)
    { 0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00},   // U+0066 (f)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // U+0067 (g)
    { 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00},   // U+0068 (h)
    { 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0069 (i)
    { 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E},   // U+006A (j)
    { 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00},   // U+006B (k)
    { 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+006C (l)
    { 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00},   // U+006D (m)
    { 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00},   // U+006E (n)
    { 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00},   // U+006F (o)
    { 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F},   // U+0070 (p)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78},   // U+0071 (q)
    { 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00},   // U+0072 (r)
    { 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00},   // U+0073 (s)
    { 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00},   // U+0074 (t)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00},   // U+0075 (u)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // U+0076 (v)
    { 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00},   // U+0077 (w)
    { 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00},   // U+0078 (x)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // U+0079 (y)
    { 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00},   // U+007A (z)
    { 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00},   // U+007B ({)
    { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00},   // U+007C (|)
    { 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00},   // U+007D (})
    { 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+007E (~)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}    // U+007F
};

#define draw_index_pixel(buf, x, y, w, col) *(u8*)((buf)+(x)+((y)*w)) = (col)
#define draw_plane_pixel(buf, x, y, w, col) if((col) == 0) *(u8*)((buf)+((x) + (y)*(w))) |= (col); else *(u8*)((buf)+((x) + (y)*(w))) &= ~(col);

bool fb_is_available() {
    for(u32 i=0;i<gdevt_len;i++) {
        if(gdevt[i].type == DEV_FRAMEBUFFER) return TRUE;
    }

    return FALSE;
}

/** @todo Make FBPreference do something  */
Device* fb_get(enum FBPreference pref) {
    for(u32 i=0;i<gdevt_len;i++) {
        if(gdevt[i].type == DEV_FRAMEBUFFER) return &gdevt[i];
    }

    return NULL;
}

FBInfo fb_get_info(Device *fb) {
    return *(FBInfo*)fb->data;
}

// Draw a single pixel at (x, y) with the best available match for the color
// sent
void fb_draw_pixel(Device *fb, u16 x, u16 y, u32 color) {
    if(fb == NULL) return; // invalid fb

    FBInfo* info = (FBInfo*)fb->data;
    if(info == NULL) return;

    if(x > info->w || y > info->h) return;

    // no pixels on text :(
    if(info->format == PixelFormat_TEXT_640x480) return;

    if(info->format == PixelFormat_LINEAR_I8) {
        *(u8*)(info->buffer + x+y*info->w) = (u8)color;
    } else if(info->format == PixelFormat_PLANAR_I4) {
        u16 bit = x%8;
        u16 byte = ((x-bit)+y*640)/8;

        outb(0x3C4, 2); // mask register
        if(color == 0)
            outb(0x3C5, 0b1111);
        else
            outb(0x3C5, (color) | 1);

        *(u8*)(info->buffer + ((x)+y*info->w)/8) |= (1) << (8-(x%8+1));
    }

    if(info->buf_update) info->buf_update();
}

void fb_draw_rect(Device* fb, u16 x, u16 y, u16 w, u16 h, u32 color) {
    if(fb == NULL) return; // invalid fb

    FBInfo* info = (FBInfo*)fb->data;
    if(info == NULL) return;

    // no pixels on text :(
    if(info->format == PixelFormat_TEXT_640x480) return;

    if(info->format == PixelFormat_LINEAR_I8) {
        for(u16 iy=0;iy<h;iy++) {
            for(u16 ix=0;ix<w;ix++) {
                *(u8*)((info->buffer)+(ix+x)+((iy+y)*info->w)) = (color);
            }
        }
    } else if(info->format == PixelFormat_PLANAR_I4) {
        outb(0x3C4, 2); // mask register
        if(color == 0)
            outb(0x3C5, 0b1111);
        else
            outb(0x3C5, color); // + 0x0FFFF

        for(u16 iy=0;iy<h;iy++) {
            for(u16 ix=0;ix<w;ix++) {
                u8 bit = (1) << (8-((x+ix)%8+1));
                u32 byte = ((x+ix)+(y+iy)*info->w)/8;

                if(color)
                    *(u8*)(info->buffer + (byte)) |= bit;
                else
                    *(u8*)(info->buffer + (byte)) &= ~bit;
            }
        }
    }

    if(info->buf_update) info->buf_update();
}

void fb_draw_char(Device *fb, u16 x, u16 y, u8 c, u32 color) {
    if(fb == NULL) return;
    
    FBInfo* info = (FBInfo*)fb->data;
    if(info == NULL) return;

    if(info->format == PixelFormat_TEXT_640x480) {
        // this is harcoded to be 8x8 glyphs
        // *(u8*)(0xB8000) = '?';
        if(c != '\n') {
            *(u8*)(info->buffer+((x+y*info->w)/8)*2) = c;
            *(u8*)(info->buffer+((x+y*info->w)/8)*2+1) = (u8)color;
        }
    } else if(info->format == PixelFormat_LINEAR_I8) {
        for(u16 iy=0;iy<8;iy++) {
            for(u16 ix=0;ix<8;ix++) {
                if(font8x8_basic[c][iy] & (1 << ix))
                    *(u8*)((info->buffer)+(ix+x)+((iy+y)*info->w)) = (color);
            }
        }
    } else if(info->format == PixelFormat_PLANAR_I4) {
        outb(0x3C4, 2); // mask register
        if(color == 0)
            outb(0x3C5, 0b1111);
        else
            outb(0x3C5, color | 1);

        for(u16 iy=0;iy<8;iy++) {
            for(u16 ix=0;ix<8;ix++) {
                if(font8x8_basic[c][iy] & (1 << ix))
                    *(u8*)(info->buffer + ((x+ix)+(y+iy)*info->w)/8) |= (1) << (8-((x+ix)%8+1));
                else
                    *(u8*)(info->buffer + ((x+ix)+(y+iy)*info->w)/8) &= ~(1 << (8-((x+ix)%8+1)));
            }
        }
    }
}

void fb_clear(Device *fb, u32 color) {
    if(fb == NULL) return; // invalid fb

    FBInfo* info = (FBInfo*)fb->data;
    if(info == NULL) return;

    if(info->format == PixelFormat_LINEAR_I8) {
        for(int i=0;i<info->w*info->h;i++) {
            *(u8*)(info->buffer+i) = (u8)color;
        }
    } else if(info->format == PixelFormat_PLANAR_I4) {
        outb(0x3C4, 2); // mask register
        if(color == 0)
            outb(0x3C5, 0b1111);
        else
            outb(0x3C5, color | 1);

        for(u16 iy=0;iy<info->h;iy++) {
            for(u16 ix=0;ix<info->w;ix++) {
                if(color)
                    *(u8*)(info->buffer + (((ix)+(iy)*info->w)/8) + 0x00000) = 0xFF;
                else
                    *(u8*)(info->buffer + (((ix)+(iy)*info->w)/8) + 0x00000) = 0x00;
            }
        }
    } else if(info->format == PixelFormat_TEXT_640x480) {
        for(u16 i=0;i<info->w*info->h;i++) {
            *(u8*)(info->buffer + i*2) = 0x00;
            *(u8*)(info->buffer + i*2+1) = 0x0F;
        }
    }

    if(info->buf_update) info->buf_update();
}

/****************************** VIRTUAL TERMINAL ******************************/

Device *stdvt() {
    return k_get_device_by_owner(KERNEL_ID, DEV_VIRT_TERM);
}

void putcnoup(Device *vt, u8 key) {
    VTDeviceData* data = (VTDeviceData*) vt->data;
    if(data->textind+1 > data->textlen) return;
    if(key == '\b') {
        data->textind--;
        memset(data->text+data->textind, 0, data->textlen-data->textind);
    } else {
        data->text[data->textind++] = key;
    }
}

void putsnoup(Device *vt, u8 *st) {
    for(int i=0;i<strlen(st);i++) {
        putcnoup(vt, st[i]);
    }
}

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

void v_kprintf(const char *format, va_list list) {
    Device *vt = k_get_device_by_owner(KERNEL_ID, DEV_VIRT_TERM);
    if(vt == NULL) return; // no virtual terminal?

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
                    putcnoup(vt, '0');
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
                        putsnoup(vt, "0x");
                    }
                    
                case 'u':
                {
                    switch (length) {
                        case 0:
                        {
                            unsigned int integer = va_arg(list, unsigned int);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            putsnoup(vt, intStrBuffer);
                            break;
                        }
                        case 'H':
                        {
                            unsigned char integer = (unsigned char) va_arg(list, unsigned int);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            putsnoup(vt, intStrBuffer);
                            break;
                        }
                        case 'h':
                        {
                            unsigned short int integer = va_arg(list, unsigned int);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            putsnoup(vt, intStrBuffer);
                            break;
                        }
                        case 'l':
                        {
                            unsigned long integer = va_arg(list, unsigned long);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            putsnoup(vt, intStrBuffer);
                            break;
                        }
                        case 'q':
                        {
                            unsigned long long integer = va_arg(list, unsigned long long);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            putsnoup(vt, intStrBuffer);
                            break;
                        }
                        case 'j':
                        {
                            u32 integer = va_arg(list, u32);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            putsnoup(vt, intStrBuffer);
                            break;
                        }
                        case 'z':
                        {
                            size_t integer = va_arg(list, size_t);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            putsnoup(vt, intStrBuffer);
                            break;
                        }
                        case 't':
                        {
                            ptrdiff_t integer = va_arg(list, ptrdiff_t);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            putsnoup(vt, intStrBuffer);
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
                        putsnoup(vt, intStrBuffer);
                        break;
                    }
                    case 'H':
                    {
                        signed char integer = (signed char) va_arg(list, int);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        putsnoup(vt, intStrBuffer);
                        break;
                    }
                    case 'h':
                    {
                        short int integer = va_arg(list, int);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        putsnoup(vt, intStrBuffer);
                        break;
                    }
                    case 'l':
                    {
                        long integer = va_arg(list, long);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        putsnoup(vt, intStrBuffer);
                        break;
                    }
                    case 'q':
                    {
                        long long integer = va_arg(list, long long);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        putsnoup(vt, intStrBuffer);
                        break;
                    }
                    case 'j':
                    {
                        u32 integer = va_arg(list, u32);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        putsnoup(vt, intStrBuffer);
                        break;
                    }
                    case 'z':
                    {
                        size_t integer = va_arg(list, size_t);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        putsnoup(vt, intStrBuffer);
                        break;
                    }
                    case 't':
                    {
                        ptrdiff_t integer = va_arg(list, ptrdiff_t);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        putsnoup(vt, intStrBuffer);
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
                        putcnoup(vt, va_arg(list, wint_t));
                    } else {
                        putcnoup(vt, va_arg(list, int));
                    }
                    
                    break;
                }
                    
                case 's':
                {
                    putsnoup(vt, va_arg(list, char*));
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
                    
                    putsnoup(vt, intStrBuffer);
                    
                    floating -= (int) floating;
                    
                    for (int i = 0; i < precSpec; ++i) {
                        floating *= 10;
                    }
                    u32 decPlaces = (u32) (floating + 0.5);
                    
                    if (precSpec) {
                        putcnoup(vt, '.');
                        __int_str(decPlaces, intStrBuffer, 10, FALSE, FALSE, 0, FALSE, FALSE);
                        intStrBuffer[precSpec] = 0;
                        putsnoup(vt, intStrBuffer);
                    } else if (altForm) {
                        putcnoup(vt, '.');
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
                putsnoup(vt, "e+");
            } else if (specifier == 'E') {
                putsnoup(vt, "E+");
            }
            
            if (specifier == 'e' || specifier == 'E') {
                __int_str(expo, intStrBuffer, 10, FALSE, FALSE, 2, FALSE, TRUE);
                putsnoup(vt, intStrBuffer);
            }
            
        } else {
            putcnoup(vt, format[i]);
        }
    }
}

void update(Device *vt) {
    if(vt == NULL) return;
    if(((VTDeviceData*)vt->data)->write_handler)
        ((VTDeviceData*)vt->data)->write_handler(vt);
}

__attribute__ ((format (printf, 1, 2))) void kprintf(const char* format, ...) {
    Device *vt = k_get_device_by_owner(KERNEL_ID, DEV_VIRT_TERM);
    if(vt == NULL) return;
    va_list list;
    va_start (list, format);
    v_kprintf(format, list);
    va_end (list);

    if(((VTDeviceData*)vt->data)->write_handler)
        ((VTDeviceData*)vt->data)->write_handler(vt);
}

__attribute__ ((format (printf, 1, 2))) void kprintfnoup(const char* format, ...) {
    Device *vt = k_get_device_by_owner(KERNEL_ID, DEV_VIRT_TERM);
    if(vt == NULL) return;
    va_list list;
    va_start (list, format);
    v_kprintf(format, list);
    va_end (list);
}

void puts_vt(Device *vt, u8* str) {
    if(vt == NULL || vt->type != DEV_VIRT_TERM || vt->data == NULL) return;

    for(size_t i=0;i<strlen(str);i++) {
        putcnoup(vt, str[i]);
    }

    if(((VTDeviceData*)vt->data)->write_handler)
        ((VTDeviceData*)vt->data)->write_handler(vt);
}

void putc_vt(Device *vt, u8 key) {
    if(vt == NULL || vt->type != DEV_VIRT_TERM || vt->data == NULL) return;

    putcnoup(vt, key);

    if(((VTDeviceData*)vt->data)->write_handler)
        ((VTDeviceData*)vt->data)->write_handler(vt);
}

void input_vt(Device *vt, u8 key) {
    if(vt == NULL || vt->type != DEV_VIRT_TERM || vt->data == NULL) return;

    VTDeviceData* vtd = (VTDeviceData*)vt->data;
    
    if(vtd->infifo_ind > vtd->infifo_len) return;
    vtd->infifo[vtd->infifo_ind++] = key;
}

u8 readc_vt(Device *vt) {
    if(vt == NULL || vt->type != DEV_VIRT_TERM || vt->data == NULL) return 0;

    VTDeviceData* data = (VTDeviceData*) vt->data;
    u8 k = data->infifo[0];
    if(k == 0) return 0;

    memcpy(data->infifo, data->infifo+1, sizeof(data->infifo)-1);
    if(data->infifo_ind != 0) data->infifo_ind--;

    return k;
}

void flush_vt(Device *vt) {
    if(vt == NULL || vt->type != DEV_VIRT_TERM) {
        return;
    }

    VTDeviceData* data = (VTDeviceData*) vt->data;
    memset(data->text, 0, data->textlen);
    data->textind = 0;
}

void vt_shift(Device *vt, u32 count) {
    if(vt == NULL || vt->type != DEV_VIRT_TERM) {
        return;
    }

    VTDeviceData* data = (VTDeviceData*) vt->data;
    memcpy(data->text, data->text+count, data->textlen - count);
    // Make sure the count bytes at the end are zero
    memset(data->text+data->textind, 0, count);
    data->textind-=count;
}

/*********************************** DRIVES ***********************************/

// Read a sector from a drive drive, and write it to data
bool read_sector(Device *drive, u32 sector, u8* data) {
    if(drive != NULL || !drive->is_active || drive->type != DEV_DRIVE) return FALSE;
    
    DriveDeviceData* d = (DriveDeviceData*)drive->data;

    if(d->read_sector)
        d->read_sector(drive, sector, data);
}

bool write_sector(Device *drive, u32 sector, u8* data) {
    if(drive != NULL || !drive->is_active || drive->type != DEV_DRIVE) return FALSE;
    
    DriveDeviceData* d = (DriveDeviceData*)drive->data;

    if(d->write_sector)
        d->write_sector(drive, sector, data);
}

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

typedef i32 fd_t;
typedef i64 off_t;

fd_t create_st() {

}

bool mount_drive(Device *drive, char* path) {
    if(drive == NULL || !drive->is_active || drive->type != DEV_DRIVE) return FALSE;

    DriveDeviceData* d = (DriveDeviceData*)drive->data;
    if(d == NULL || !d->read_sector) return FALSE;

    kprintf("sizeof(BPB) = %u\n", sizeof(BPB));

    // I'm hardcoding FAT32 at the only partition on the drive
    BPB * bpb = (BPB *)k_malloc(d->sector_size);
    if(bpb == NULL) return FALSE;
    d->read_sector(drive, 0, (u8*)bpb);

    if(bpb->jmp[0] != 0xEB || bpb->jmp[2] != 0x90) goto fail;
    if(bpb->total_secs != 0) goto fail;

    for(int i=0;i<8;i++) {
        putc_vt(stdvt(), bpb->oem[i]);
    }
    putc_vt(stdvt(), '\n');

    // kprintf("sector count: %u\n", bpb->large_sec_count);
    for(int i=0;i<11;i++) {
        putc_vt(stdvt(), bpb->volume_label[i]);
    }
    putc_vt(stdvt(), '\n');

    k_free(bpb);
    return TRUE;
fail:
    k_free(bpb);
    return FALSE;
}

// Open a file at a specified path, returning the file descriptor, or an error
// if it failed to open
fd_t open(char* path) {

}

// Read count bytes from fd at offset to buf
int pread(fd_t fd, void *buf, size_t count, off_t offset) {

}

// Write count bytes to fd at offset from buf
int pwrite(fd_t fd, void *buf, size_t count, off_t offset) {

}