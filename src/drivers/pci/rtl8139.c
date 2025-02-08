#include "drivers/pci/rtl8139.h"

int rtl8139_entry(module_t *dev) {
    kprintf("RTL8139!\n");
    return DRIVER_SUCCESS;
}

int rtl8139_int(module_t *dev, u8 intr) {
    return 0;
}

module_t get_rtl8139_driver() {
    return (module_t) {
        .name = "RTL8139",
        .module_start   = rtl8139_entry,
        .module_int     = rtl8139_int,
        .pci_flags = {
            .vendor     = 0x8086,
            .device     = 0x100E,
            .class      = 0xFF,
            .subclass   = 0xFF,
            .interface  = 0xFF,
            .search = true
        }
    };
}