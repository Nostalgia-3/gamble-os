#include <drivers/pci/ehci.h>
#include <drivers/pci.h>
#include <gosh/gosh.h>

int ehci_entry(module_t *mod) {
    u8 bus  = mod->pci_flags.r_bus;
    u8 slot = mod->pci_flags.r_slot;

    kprintf("ECHI detected (pci bus = %u, pci slot = %u)\n", bus, slot);
    return DRIVER_SUCCESS;
}

void ehci_exit(module_t *mod) {

}

module_t get_ehci_module() {
    return (module_t) {
        .module_start   = ehci_entry,
        .module_int     = ehci_exit,
        .name           = "ehci",
        .pci_flags = {
            .class = 0x0C,
            .subclass = 0x03,
            .interface = 0x20,
            .device = 0xFFFF,
            .vendor = 0xFFFF
        }
    };
}

// void ECHIDriverInt(Device *dev, u8 intr) {
    
// }