#pragma once

#include <types.h>
#include <x86/x86.h>

typedef struct _pci_reqs {
    uint16_t vendor;
    uint16_t device;
    uint8_t class;
    uint8_t subclass;
    uint8_t interface;

    // Set during module initialization
    uint8_t bus;
    // Set during module initialization
    uint8_t slot;
} pci_requirements;

#include <module.h>

typedef struct _pci_device {
    uint16_t vendor;
    uint16_t device;
    uint8_t class;
    uint8_t subclass;
    uint8_t interface;

    uint8_t bus;
    uint8_t slot;

    module* owner;
} pci_device;

uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_config_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t val);

uint32_t pci_config_read_long(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_config_write_long(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t l);

uint16_t pci_get_vendor(uint8_t bus, uint8_t slot);
uint16_t pci_get_device(uint8_t bus, uint8_t slot);

uint32_t pci_get_bar_size(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar);