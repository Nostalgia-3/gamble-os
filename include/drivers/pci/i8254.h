#ifndef I8254_H
#define I8254_H

#include <gosh/gosh.h>
#include <drivers/pci.h>

PCIDriver get_i8254_driver();
int I8254_DriverEntry(Device *dev);
void I8254_DriverInt(Device *dev, u8 intr);

#endif