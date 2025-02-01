#ifndef RTL8139_H
#define RTL8139_H

#include <gosh/gosh.h>
#include <drivers/pci.h>

PCIDriver get_rtl8139_driver();
int RTL8139_DriverEntry(Device *dev);
void RTL8139_DriverInt(Device *dev, u8 intr);

#endif