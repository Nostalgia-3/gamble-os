#pragma once

// Limits capabilities to Pentium CPUs
#define CPU_I686 0

// Most i686/Pentium PCs; this option adds the following modules:
// - I8042 PS/2 controller (`MODULE_I8042`)
// - (P/S)ATA controller (`MODULE_ATA_PIO`)
#define MACHINE_IBM_PC 0