#include <drivers/pci/nvme.h>
#include <gosh/gosh.h>
#include <memory.h>
#include <types.h>
#include <port.h>

// static Driver driver = { .name = "NVME.DRV", .DriverEntry=NVME_DriverEntry, .DriverInt=NVME_DriverInt };

int nvme_entry(module_t *mod) {
    return DRIVER_SUCCESS;
}

void nvme_int(module_t *mod, u8 intr) {
    return;
}

module_t get_nvme_module() {
    return (module_t) {
        .name           = "nvme",
        .module_start   = nvme_entry,
        .module_int     = nvme_int,
        .pci_flags = {
            .vendor = 0xFFFF,
            .device = 0xFFFF,
            .class  = 0x01,
            .subclass = 0x08,
            .interface = 0xFF
        }
    };
}