#ifndef PORT_H
#define PORT_H

#include "types.h"

static inline u8 inb(u16 port) {
    u8 ret;
    __asm__ volatile ( "inb %w1, %b0"
                   : "=a"(ret)
                   : "Nd"(port)
                   : "memory");
    return ret;
}

static inline void outb(u16 port, u8 val) {
    __asm__ volatile ( "outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

extern u16 inw(u16 port);
extern void outw(u16 port, u16 val);

extern u32 inl(u16 port);
extern void outl(u16 port, u32 val);

extern void p_insw(u16 port, u16* buf, u16 size);

#endif