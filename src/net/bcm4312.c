#include <module.h>
#include <printf.h>
#include <memory.h>

int bcm4312_start(module* mod) {
    uint8_t bus = mod->requirements.pci.bus, slot = mod->requirements.pci.slot;

    printf_("bcm4312 bars:\n");
    for(int i=0;i<6;i++) {
        printf_("bar%u: %08x\n", i+1, pci_config_read_long(bus, slot, 0, 0x10 + i*4));
    }

    return 0;
}

int bcm4312_int(module* mod, uint32_t in) {
    return 0;
}

module get_bcm4312_module() {
    return (module) {
        .name = "bcm4312",

        .module_start = bcm4312_start,
        .module_int   = bcm4312_int,

        .priority = 0,
        .type = MODULE_PCI,
        .requirements = {
            .pci = {
                .vendor = 0x14e4,
                .device = 0x4315,
                .class  = 0xFF,
                .subclass = 0xFF,
                .interface = 0xFF
            }
        }
    };
}