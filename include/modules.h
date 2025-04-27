#pragma once

#include <module.h>

module get_i8042_module();
module get_initrd_module(void* initram, size_t initsize);
module get_tty_module();