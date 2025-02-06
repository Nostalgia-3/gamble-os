#ifndef SHELL_H
#define SHELL_H

#include <multiboot.h>
#include <gosh/gosh.h>
#include <types.h>

int shell_main(multiboot_info_t *mbd);

#endif