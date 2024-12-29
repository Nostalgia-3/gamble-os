/**
 * @file pci.h
 * @author Nostalgia3
 * @brief A relatively simple PCI manager, for testing
 * @version 0.1
 * @date 2024-12-27
 * 
 */
#ifndef PCI_H
#define PCI_H

#include "types.h"

typedef struct _PCIHeader {
    u16 vendor;
    u16 device;
    u16 command; // I believe this is write-only
    u16 status;
    u8 rev_id;
    u8 prog_if;
    u8 subclass;
    u8 class_code;
    u8 cache_line_size;
    u8 latency_timer;
    u8 header_type;
    u8 BIST;
} PCIHeader;

// Read a config register, returning a value
u16 pci_read_config(u8 bus, u8 slot, u8 func, u8 offset);

// Get the vendor of a PCI device. If the vendor is
// set to 0xFFFF, then there is no device in that slot
u16 pci_get_vendor(u8 bus, u8 slot);

// Get the device ID of a PCI device. If the device is
// set to 0xFFFF, then there is no device in that slot
u16 pci_get_device(u8 bus, u8 slot);

PCIHeader pci_get_header(u8 bus, u8 slot);

#endif