#include "gosh.h"
#include <memory.h>
#include <str.h>
#include <vga.h>
#include <math.h>
#include <port.h>

Device* gdevt;
u32 gdevt_len = 0;

volatile u32 d_id = 0;

/************************************ MISC ************************************/

void kpanic() {
    __asm__ volatile ("cli\n" "hlt":);
    while(1);
}

/********************************** KEYBOARD **********************************/

void pushc(u8 c) {
    if(c == 0) return;
    Device* dev = k_get_device_by_owner(KERNEL_ID, DEV_KEYBOARD);
    if(dev == NULL) return;

    KeyboardDeviceData* data = (KeyboardDeviceData*) dev->data;
    if(data == NULL) return;

    if(data->fifo_ind > sizeof(data->fifo)) return;
    data->fifo[data->fifo_ind++] = c;
}

// Halt the process until a character from the keyboard is detected
u8 scanc() {
    u8 d;
    
    do { d = getc(); } while(d == 0);
    return d;
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

void _init_gdevt() {
    if(gdevt != NULL) return;
    gdevt = k_malloc(sizeof(Device)*256);
    gdevt_len = 256;
    memset(gdevt, 0, sizeof(Device)*256);

    // standard keyboard output
    Device *dev = k_add_dev(KERNEL_ID, DEV_KEYBOARD, 0);
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
    if(id == 0) return NULL;

    switch(dev) {
        case DEV_KEYBOARD:
            puts("Registered keyboard ");
            gdevt[ind].data = k_malloc(sizeof(KeyboardDeviceData));
            if(gdevt[ind].data == NULL) return NULL;
            memset(gdevt[ind].data, 0, sizeof(KeyboardDeviceData));
        break;

        case DEV_DRIVER:
            puts("Registered driver ");
            // This is defined in k_load_driver() as the pointer to the driver
            gdevt[ind].data = NULL;
        break;

        default:
            puts("Device #");
            puts(itoa(dev, 10));
            puts(" not handled yet\n");
        break;
    }

    puts("(owner = ");
    puts(itoa(kid, 10));
    puts(", dev_type = ");
    puts(itoa(dev, 10));
    puts(", code = ");
    puts(itoa(code, 10));
    puts(", id = ");
    puts(itoa(id, 10));
    puts(")\n");

    gdevt[ind].id = id;
    gdevt[ind].code = code;
    gdevt[ind].type = dev;
    gdevt[ind].owner = kid;

    return &gdevt[ind];
}

Device* k_get_device_by_owner(u32 owner, enum DeviceType type) {
    for(u32 i=0;i<gdevt_len;i++) {
        if(gdevt[i].owner == owner && gdevt[i].type == type) return &gdevt[i];
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

PCIHeader pci_get_header(u8 bus, u8 slot) {
    PCIHeader header;

    header.vendor           = pci_read_config(bus, slot, 0, 0);
    header.device           = pci_read_config(bus, slot, 0, 2);
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

/******************************** FRAME BUFFER ********************************/

bool fb_is_available() {
    for(u32 i=0;i<gdevt_len;i++) {
        if(gdevt[i].type == DEV_FRAMEBUFFER) return TRUE;
    }

    return FALSE;
}

/** @todo Make FBPreference do something  */
u32 fb_get(enum FBPreference pref) {
    for(u32 i=0;i<gdevt_len;i++) {
        if(gdevt[i].type == DEV_FRAMEBUFFER) return i;
    }

    return 0xFFFFFFFF;
}

FBInfo fb_get_info(u32 gdevt_ind) {

}

// Draw a single pixel at (x, y) with the best available match for the color
// sent
void    fb_draw_pixel(u32 fbid, u16 x, u16 y, u32 color);
void    fb_draw_rect(u32 fbid, u16 x, u16 y, u16 w, u16 h, u32 color);