#include <module.h>
#include <device.h>
#include <printf.h>
#include <memory.h>
#include <pci.h>

const char* pci_classes[] = {
    "Unclassified",
    "Mass Storage Controller",
    "Network Controller",
    "Display Controller",
    "Multimedia Controller",
    "Memory Controller",
    "Bridge",
    "Simple Communication Controller",
    "Base System Peripheral",
    "Input Device Controller",
    "Docking Station",
    "Processor",
    "Serial Bus Controller",
    "Wireless Controller",
    "Intelligent Controller",
    "Satellite Communication Controller",
    "Encryption Controller",
    "Signal Processing Controller",
    "Processing Accelerator",
    "Non-Essential Instrumentation",
    "0x3F (Reserved)",
    "Co-Processor",
    "0xFE (Reserved)"
};

#define DEVICE_ENTRIES 128

static dev_entry* entries = NULL;

int register_device(dev_entry dev) {
    if(entries == NULL) return -1;

    // pci_device d = dev.pci;
    // printf_("pci device(%u, %u): %04x:%04x (%s, %u, %u)\n", d.bus, d.slot, d.vendor, d.device, pci_classes[d.class], d.subclass, d.interface);

    for(int i=0;i<DEVICE_ENTRIES;i++) {
        if(entries[i].type != DEVICE_UNUSED) continue;

        entries[i] = dev;
        return 0;
    }
    
    return -1;
}

int find_pci_device(pci_requirements* req) {
    for(int i=0;i<DEVICE_ENTRIES;i++) {
        if(entries[i].type != DEVICE_PCI) continue;

        if(req->vendor != 0xFFFF && req->vendor != entries[i].pci.vendor) continue;
        if(req->device != 0xFFFF && req->device != entries[i].pci.device) continue;
        if(req->interface != 0xFF && req->interface != entries[i].pci.interface) continue;
        if(req->class != 0xFF && req->class != entries[i].pci.class) continue;
        if(req->subclass != 0xFF && req->subclass != entries[i].pci.subclass) continue;

        req->bus = entries[i].pci.bus;
        req->slot = entries[i].pci.slot;

        return 0;
    }

    return -1;
}

int device_init() {
    entries = (dev_entry*)alloc_chunks((DEVICE_ENTRIES * sizeof(dev_entry))/sizeof(chunk), 0);
    
    for(int bus=0;bus<256;bus++) {
        for(int device=0;device<32;device++) {
            uint16_t vendor = pci_get_vendor(bus, device);
            if(vendor != 0xFFFF) {
                register_device((dev_entry) {
                    .type = DEVICE_PCI,
                    .pci = {
                        .vendor = pci_get_vendor(bus, device),
                        .device = pci_get_device(bus, device),
                        .class  = (pci_config_read_word(bus, device, 0, 10) >> 8) & 0xFF,
                        .subclass = (pci_config_read_word(bus, device, 0, 10)) & 0xFF,
                        .interface = (pci_config_read_word(bus, device, 0, 10) >> 8) & 0xFF,
                        .bus = bus,
                        .slot = device
                    }
                });
            }
        }
    }

    return 0;
}