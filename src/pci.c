#include <pci.h>
#include <port.h>

u16 pci_read_config(u8 bus, u8 slot, u8 func, u8 offset) {
    // This is mostly copied from https://wiki.osdev.org/PCI
    u32 address;
    u32 lbus  = (u32)bus;
    u32 lslot = (u32)slot;
    u32 lfunc = (u32)func;
    u16 tmp = 0;
  
    address = (u32)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((u32)0x80000000));
  
    // Write out the address
    outl(0xCF8, address);

    // Read in the data
    tmp = (u16)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
    return tmp;
}

u16 pci_get_vendor(u8 bus, u8 slot) {
    return pci_read_config(bus, slot, 0, 0);
}

u16 pci_get_device(u8 bus, u8 slot) {
    u16 vendor, device;

    if((vendor = pci_read_config(bus, slot, 0, 0)) != 0xFFFF) {
        device = pci_read_config(bus, slot, 0, 2);
        return device;
    }

    return 0xFFFF;
}

PCIHeader pci_get_header(u8 bus, u8 slot) {
    PCIHeader header;

    if((u16)pci_read_config(bus, slot, 0, 0) == 0xFFFF) return header;

    header.vendor           = pci_read_config(bus, slot, 0, 0);
    header.device           = pci_read_config(bus, slot, 0, 2);
    header.status           = pci_read_config(bus, slot, 0, 6);
    u16 byte                = pci_read_config(bus, slot, 0, 8);
    header.rev_id           = byte & 0xFF;
    header.prog_if          = (byte >> 8) & 0xFF;
    byte                    = pci_read_config(bus, slot, 0, 10);
    header.subclass         = byte & 0xFF;
    header.class_code       = (byte >> 8) & 0xFF;
    byte                    = pci_read_config(bus, slot, 0, 8);
    header.cache_line_size  = byte & 0xFF;
    header.latency_timer    = (byte >> 8) & 0xFF;
    byte                    = pci_read_config(bus, slot, 0, 10);
    header.header_type      = byte & 0xFF;
    header.BIST             = (byte >> 8) & 0xFF;
    

    return header;
}