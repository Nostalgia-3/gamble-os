#ifndef INITRD_H
#define INITRD_H

#include <gosh/gosh.h>

module_t get_initrd_module(void *initrd, u32 size);

#endif//INITRD_H