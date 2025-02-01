#include "drivers/pci/rtl8139.h"

static Driver driver = (Driver) { .DriverEntry = RTL8139_DriverEntry, .DriverInt = RTL8139_DriverInt };

PCIDriver get_rtl8139_driver() {
    return (PCIDriver) {
        .vendor = 0x8086,
        .device = 0x100E,
        .class  = 0xFF,
        .subclass = 0xFF,
        .interface = 0xFF,
        .driver = &driver
    };
}

int RTL8139_DriverEntry(Device *dev) {
    kprintf("RTL8139!\n");
    return DRIVER_SUCCESS;
}

void RTL8139_DriverInt(Device *dev, u8 intr) {

}