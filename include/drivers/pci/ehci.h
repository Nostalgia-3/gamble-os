#ifndef EHCI_H
#define EHCI_H

#include <drivers/pci.h>
#include <gosh/gosh.h>

PCIDriver get_ehci_driver();
void ECHIDriverEntry(Device *dev);
void ECHIDriverInt(Device *dev, u8 intr);

#endif