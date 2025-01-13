#ifndef GOSH_PCIE_H
#define GOSH_PCIE_H

#include <gosh/common.h>

#define PCI_IO_SPACE        (1 << 0)    // R/W
#define PCI_MEM_SPACE       (1 << 1)    // R/W
#define PCI_BUS_MASTER      (1 << 2)    // R/W
#define PCI_SPEC_CYCLES     (1 << 3)    // RO
#define PCI_MEM_INVAL_EN    (1 << 4)    // RO
#define PCI_VGA_PAL_SNOOP   (1 << 5)    // RO
#define PCI_PARITY_ERR_RESP (1 << 6)    // R/W
#define PCI_SERR_ENABLE     (1 << 8)    // R/W
#define PCI_BTB_ENABLE      (1 << 9)    // RO
#define PCI_INT_DISABLE     (1 << 10)   // R/W

#define PCI_MASS_STORAGE_CONTROLLER 0x01
#define PCI_IDE_CONTROLLER          0x01

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

typedef struct _GenPCIHeader {
    u32 bar[6];
    u32 cis_pointer;
    u16 subsystem_id;
    u16 subsystem_vendor_id;
    u32 expansion_rom_addr;
    u8  capabilities_pointer;
    u8  max_latency;
    u8  min_grant;
    u8  interrupt_pin;
    u8  interrupt_line;
} GenPCIHeader;

// Read a config register, returning a value
PCIHeader pci_get_header(u8 bus, u8 slot);
GenPCIHeader pci_get_gen_header(u8 bus, u8 slot);

u16 pci_read_config(u8 bus, u8 slot, u8 func, u8 offset);

u32 pci_read_config32(u8 bus, u8 slot, u8 func, u8 offset);
void pci_write_config32(u8 bus, u8 slot, u8 func, u8 offset, u32 data);

u16 scan_pci(u16 cls, u16 subclass);

void pci_set_comm(u16 command, u8 bus, u8 slot);
void pci_unset_comm(u16 command, u8 bus, u8 slot);

u16  pci_get_comm(u8 bus, u8 slot);

#endif//GOSH_PCIE_H