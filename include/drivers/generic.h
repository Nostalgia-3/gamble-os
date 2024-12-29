#ifndef GENERIC_H
#define GENERIC_H

#include "../types.h"
#include "../driver.h"

#include "../gosh.h"

void I8042_DriverEntry() {
    // initialize the PS/2
}

void I8042_DriverInt(u8 i) {

}

const Driver i8042 = { .DriverEntry = I8042_DriverEntry, .DriverInt = I8042_DriverInt };

#endif