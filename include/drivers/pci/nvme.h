#ifndef NVME_H
#define NVME_H

#include <drivers/pci.h>
#include <gosh/gosh.h>

PCIDriver get_nvme_driver();
int NVME_DriverEntry(Device *dev);
void NVME_DriverInt(Device *dev, u8 intr);

#endif