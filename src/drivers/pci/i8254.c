#include <drivers/pci/i8254.h>
#include <drivers/pci.h>
#include <gosh.h>

static Driver driver = (Driver) { .DriverEntry = I8254_DriverEntry, .DriverInt = I8254_DriverInt };

PCIDriver get_i8254_driver() {
    return (PCIDriver) {
        .vendor = 0x8086,
        .device = 0x100E,
        .class  = 0xFF,
        .subclass = 0xFF,
        .driver = &driver
    };
}

void I8254_DriverEntry(Device *dev) {
    Driver* driver = (Driver*)dev->data;
    u32 d = (u32)driver->data;
    u8 bus = d & 0xFF;
    u8 slot = (d >> 8) & 0xFF;

    GenPCIHeader header = pci_get_gen_header(bus, slot);
    pci_set_comm(PCI_IO_SPACE | PCI_MEM_SPACE | PCI_BUS_MASTER, bus, slot);

}

void I8254_DriverInt(Device *dev, u8 intr) {

}