#include <device.h>
#include <printf.h>
#include <pci.h>

int device_init() {
    for(int bus=0;bus<256;bus++) {
        for(int device=0;device<32;device++) {
            uint16_t vendor = pci_get_vendor(bus, device);
            if(vendor != 0xFFFF) {
                printf("pci device(%u, %u): %04x:%04x\n", bus, device, pci_get_vendor(bus, device), pci_get_device(bus, device));
            }
        }
    }

    return 0;
}