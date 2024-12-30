#include "gosh.h"
#include <memory.h>
#include <str.h>
#include <vga.h>

Device* gdevt;
u32 gdevt_len = 0;

volatile u32 d_id = 0;

void _init_gdevt() {
    if(gdevt != NULL) return;
    gdevt = k_malloc(sizeof(Device)*256);
    gdevt_len = 256;
    memset(gdevt, 0, sizeof(Device)*256);
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

void load_driver(Driver *driver) {
    if(driver == NULL) return;

    Device *dev = k_add_dev(KERNEL_ID, DEV_DRIVER, 0);
    if(dev == NULL) return;
    driver->r_id = dev->id;
    dev->data = driver;
    if(driver->DriverEntry != NULL)
        driver->DriverEntry(dev);
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

void kpanic() {
     __asm__ volatile ("cli; hlt":);
    while(1);
}