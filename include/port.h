#ifndef PORT_H
#define PORT_H

#include "types.h"

static inline u8 inb(u16 port);
static inline void outb(u16 port, u8 val);

#endif