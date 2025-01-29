#include <drivers/pci/ehci.h>
#include <drivers/pci.h>
#include <gosh/gosh.h>

static Driver driver = { .name = "EHCI.DRV", .DriverEntry = ECHIDriverEntry, .DriverInt = ECHIDriverInt };

PCIDriver get_ehci_driver() {
    return (PCIDriver) {
        .class = 0x0C,
        .subclass = 0x03,
        .interface = 0x20,
        .device = 0xFFFF,
        .vendor = 0xFFFF,
        .driver = &driver
    };
}

void ECHIDriverEntry(Device *dev) {
    Driver* driver = (Driver*)dev->data;
    u32 d = (u32)driver->data;

    u8 bus = d & 0xFF;
    u8 slot = (d >> 8) & 0xFF;

    kprintf("ECHI detected (pci bus = %u, pci slot = %u)\n", bus, slot);
}

void ECHIDriverInt(Device *dev, u8 intr) {
    
}