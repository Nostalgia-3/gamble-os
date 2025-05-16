#pragma once

#include <module.h>

module get_i8042_module();
module get_ramdisk_module(void* initram, size_t initsize);
module get_tty_module(uint32_t pitch);
module get_tarfs_module();