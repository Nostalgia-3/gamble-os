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

void pci_config_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t val) {
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

    return outl(0xCFC + (offset & 0x2), val);
}

uint32_t pci_config_read_long(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
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

    return inl(0xCFC);
}

void pci_config_write_long(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t l) {
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

    outl(
        0xCFC,
        l
    );
}

uint16_t pci_get_vendor(uint8_t bus, uint8_t slot) {
    return pci_config_read_word(bus, slot, 0, 0);
}

uint16_t pci_get_device(uint8_t bus, uint8_t slot) {
    return pci_config_read_word(bus, slot, 0, 2);
}

uint32_t pci_get_bar_size(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar) {
    uint32_t size = 0;
    uint32_t initial = pci_config_read_long(bus, slot, func, 0x10 + bar*4);

    pci_config_write_long(bus, slot, 0, 0x10 + bar*4, ~((uint32_t)0));
    size = ~pci_config_read_long(bus, slot, 0, 0x10 + bar*4) + 1;
    pci_config_write_long(bus, slot, 0, 0x10, initial);

    return size;
}