#include <pci.h>

uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    outl(
        0xCF8,
        (uint32_t)(
            ((uint32_t)bus << 16) |
            ((uint32_t)slot << 11) |
            ((uint32_t)func << 8) |
            (offset & 0xFC) |
            ((uint32_t)0x80000000)
        )
    );

    return (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
}

uint16_t pci_get_vendor(uint8_t bus, uint8_t slot) {
    return pci_config_read_word(bus, slot, 0, 0);
}

uint16_t pci_get_device(uint8_t bus, uint8_t slot) {
    return pci_config_read_word(bus, slot, 0, 2);
}