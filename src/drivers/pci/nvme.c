#include <drivers/pci/nvme.h>
#include <gosh/gosh.h>
#include <memory.h>
#include <types.h>
#include <port.h>

static Driver driver = { .name = "NVME.DRV", .DriverEntry=NVME_DriverEntry, .DriverInt=NVME_DriverInt };

PCIDriver get_nvme_driver() {
    return (PCIDriver) {
        .vendor = 0xFFFF,
        .device = 0xFFFF,
        .class  = 0x01,
        .subclass = 0x08,
        .interface = 0xFF,
        .driver = &driver
    };
}

int NVME_DriverEntry(Device *dev) {
    return DRIVER_SUCCESS;
}

void NVME_DriverInt(Device *dev, u8 intr) {

}