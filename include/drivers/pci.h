/**
 * @file pci.h
 * @author Nostalgia3
 * @brief This loads other drivers depending on PCI
 * devices connected.
 * @version 0.1
 * @date 2025-1-5
 * 
 */
#ifndef PCI_H
#define PCI_H

#include <gosh.h>

// When set to 0xFF/0xFFFF, values are ignored
typedef struct _PCIDriver {
    Driver* driver;
    u16 vendor;
    u16 device;
    u8 class;
    u8 subclass;
    u8 interface;
} PCIDriver;

// Initialize the PCI bus. Do this before adding any PCI drivers
void initialize_pci();

// Add a driver to the PCI list, with options
void PCI_ADD(PCIDriver* driver);

void PCI_DriverEntry(Device *dev);

#endif