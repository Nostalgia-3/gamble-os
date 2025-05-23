#include <module.h>
#include <printf.h>
#include <memory.h>

uint32_t r32(void* mmio, uint32_t offset) {
    return *(uint32_t*)(mmio + offset);
}

void w32(void* mmio, uint32_t offset, uint32_t val) {
    *(uint32_t*)(mmio + offset) = val;
}

int ar9287_start(module* mod) {
    uint8_t bus = mod->requirements.pci.bus, slot = mod->requirements.pci.slot;

    // enable bus master, memory space, i/o space
    printf_("%04x\n", pci_config_read_word(bus, slot, 0, 4));

    pci_config_write_word(
        bus, slot, 0, 4,
        pci_config_read_word(bus, slot, 0, 4) | 0b111
    );

    uint32_t* mmio = NULL;

    uint32_t addr = pci_config_read_long(bus, slot, 0, 0x10);
    uint32_t size = pci_get_bar_size(bus, slot, 0, 0);

    mmio = map_io((void*)(addr & (~0b111)), (size + (PAGE_SIZE - 1)) / PAGE_SIZE, 0);

    printf_("addr: %08x, size: %08x\n", addr & ~(0b111), size);
    printf_("io addr: %08X\n", mmio);

    return 0;
}

int ar9287_int(module* mod, uint32_t in) {
    return 0;
}

module get_ar9287_module() {
    return (module) {
        .name = "ar9287",

        .module_start = ar9287_start,
        .module_int   = ar9287_int,
        
        .priority = 0,
        .type = MODULE_PCI,
        .requirements = {
            .pci = {
                .vendor = 0x168c,
                .device = 0x002e,
                .class  = 0xFF,
                .subclass = 0xFF,
                .interface = 0xFF
            }
        }
    };
}