#include <gosh/gosh.h>
#include <memory.h>
#include <str.h>

#define MAX_DEVICES 128
static device_t *devs[MAX_DEVICES];
static bool dev_man_setup = false;

void _setup_device_manager() {
    if(dev_man_setup) return;

    if(devs == 0) return;
    memset(devs, 0, sizeof(devs));
    dev_man_setup = true;
}

// Create a device with the name and device, returning -1 if an error occured
int register_device(const char *pathname, device_t *dev) {
    for(int i=0;i<MAX_DEVICES;i++) {
        if(devs[i] == 0) {
            devs[i] = dev;
            devs[i]->name = pathname;

            return mknod(pathname, DT_DEV, devs[i]);
        }
    }

    return -1;
}

#include <drivers/x86/vga.h>
device_t *find_device(const char *name) {
    for(int i=0;i<MAX_DEVICES;i++) {
        if(devs[i] != 0) {
            if(strcmp((char*)devs[i]->name, (char*)name) == 0) return devs[i];
        }
    }
}

device_t **get_devices() {
    return devs;
}

u32 get_total_devices() {
    return MAX_DEVICES;
}

// Delete the entry to the device; you have to free the memory
// for the device yourself
void delete_device(const char *name) {
    // find the device
    for(int i=0;i<MAX_DEVICES;i++) {
        if(devs[i] == 0) continue;

        if(strcmp((char*)devs[i]->name, (char*)name) == 0) {
            devs[i] = 0;
        }
    }
}