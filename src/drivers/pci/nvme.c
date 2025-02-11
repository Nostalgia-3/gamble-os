#include <drivers/pci/nvme.h>
#include <gosh/gosh.h>
#include <memory.h>
#include <types.h>
#include <port.h>

// static Driver driver = { .name = "NVME.DRV", .DriverEntry=NVME_DriverEntry, .DriverInt=NVME_DriverInt };

static device_t nvme0;

ssize_t nvme_write(const void *buf, size_t len, off_t *offset) {
    kprintf("writing block %u\n", (*offset)/512);
    return 0;
}

ssize_t nvme_read(void *buf, size_t len, off_t *offset) {
    kprintf("reading block %u\n", (*offset)/512);

    return 0;
}

int nvme_entry(module_t *mod) {
    nvme0 = (device_t) {
        .read = nvme_read,
        .write = nvme_write,
    };

    if(register_device("nvme0", &nvme0) < 0) {
        kprintf("Failed to create nvme device\n");
    }

    return DRIVER_SUCCESS;
}

int nvme_int(module_t *mod, u8 intr) {
    return 0;
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
            .interface = 0xFF,
            .search = true
        }
    };
}