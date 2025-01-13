#ifndef SHELL_H
#define SHELL_H

#include <gosh/gosh.h>
#include <types.h>

void shell_write(Device *vt);
int shell_main(u32 mem);

#endif