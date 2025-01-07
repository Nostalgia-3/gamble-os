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

typedef struct _PCIDriver {
    Driver* driver;
    u16 vendor;     // Set to 0xFFFF to ignore
    u16 device;     // Set to 0xFFFF to ignore
    u8 class;       // Set to 0xFF to ignore
    u8 subclass;    // Set to 0xFF to ignore
} PCIDriver;

// Initialize the PCI bus. Do this before adding any PCI drivers
void initialize_pci();

// Add a driver to the PCI list, with options
void PCI_ADD(PCIDriver*);

void PCI_DriverEntry(Device *dev);

#endif