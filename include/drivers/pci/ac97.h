#ifndef AC97_H
#define AC97_H

#include <drivers/pci.h>
#include <gosh.h>

PCIDriver get_ac97_driver();
void AC97_DriverEntry(Device *dev);
void AC97_DriverInt(Device *dev, u8 intr);

#endif