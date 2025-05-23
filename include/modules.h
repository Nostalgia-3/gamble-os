#pragma once

#include <module.h>

module get_i8042_module();
module get_ramdisk_module(void* initram, size_t initsize);
module get_tty_module(uint32_t pitch);
module get_tarfs_module();
module get_framebuffer_module(void *fbstart, size_t fbsize);
module get_rtl8139_module();
module get_ata_module();

module get_bcm4312_module();
module get_ar9287_module();