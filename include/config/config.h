#pragma once

#include <config/config_defines.h>

// The CPU is an indicator of what the kernel should compile to, as well
// as what features (e.g. [PAE](https://en.wikipedia.org/wiki/Physical_Address_Extension)) are allowed. The primary CPU that PAE
// is the I686/Pentium class processors.
#define CPU     CPU_I686

// The machine further specifies what should be used; as an example, Intel Macs
// don't have the Intel 8042, while IBM PCs do.
#define MACHINE  MACHINE_IBM_PC

// Uncomment this to allow the kernel to write debug messages
#define KERNEL_DEBUG

// The max number of processes the kernel supports running at once
#define MAX_PROCESSES 256

// Determines the hard limit for the number of open file descriptors a process
// can have open at any moment in time
#define MAX_OPEN_FDS 128

#include <config/config_done.h>