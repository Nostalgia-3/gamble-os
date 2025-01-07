#ifndef UHCI_H
#define UHCI_H

#include <drivers/pci.h>
#include <gosh.h>

PCIDriver get_uhci_driver();
void UCHIDriverEntry(Device *dev);
void UCHIDriverInt(Device *dev, u8 intr);

#endif