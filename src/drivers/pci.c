#include <drivers/pci.h>
#include <gosh.h>
#include <memory.h>
#include <str.h>

#define MAX_PCI_DRIVERS 64

// A list of pointers to PCI drivers
static PCIDriver** drivers;
static int next = 0;
static bool initialized = FALSE;

void initialize_pci() {
    drivers = (PCIDriver**) k_malloc(sizeof(MAX_PCI_DRIVERS*sizeof(PCIDriver*)));
    if(drivers != NULL) {
        initialized = TRUE;
        memset(drivers, 0, sizeof(MAX_PCI_DRIVERS*sizeof(PCIDriver*)));
    } else {
        kprintf("Failed to initialize PCI!\n");
    }
}

void PCI_ADD(PCIDriver* opt) {
    if(!initialized || drivers == NULL) return;
    drivers[next++] = opt;
}

void PCI_DriverEntry(Device *dev) {
    if(!initialized) return;
    if(drivers == NULL) return;

    for(int i=0;i<256;i++) {
        for(int x=0;x<32;x++) {
            PCIHeader header = pci_get_header(i, x);
            if(header.vendor == 0xFFFF) continue;
            for(int z=0;z<MAX_PCI_DRIVERS;z++) {
                if(drivers[z] == NULL) break;
                if(drivers[z]->vendor != 0xFFFF && drivers[z]->vendor != header.vendor)
                    continue;
                if(drivers[z]->device != 0xFFFF && drivers[z]->device != header.device)
                    continue;
                if(drivers[z]->class != 0xFF && drivers[z]->class != header.class_code)
                    continue;
                if(drivers[z]->subclass != 0xFF && drivers[z]->subclass != header.subclass)
                    continue;

                if(drivers[z]->driver != NULL) {
                    drivers[z]->driver->data = (void*)((u32)((u8)x << 8) | ((u8)i));
                    load_driver(drivers[z]->driver);
                } else
                    kprintf("VGA Driver #%u is NULL!\n", drivers[z]->vendor);
            }
        }
    }
}