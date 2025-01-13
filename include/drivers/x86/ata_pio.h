#ifndef ATA_PIO_H
#define ATA_PIO_H

#include <gosh/gosh.h>

void ATA_DriverEntry(Device *dev);
void ATA_DriverInt(Device *dev, u8 int_id);

#endif