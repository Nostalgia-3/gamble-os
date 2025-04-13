#pragma once

#include <types.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %w1, %b0"
                   : "=a"(ret)
                   : "Nd"(port)
                   : "memory");
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

void _putchar(char c);

extern uint16_t inw(uint16_t port);
extern void outw(uint16_t port, uint16_t val);

extern uint32_t inl(uint16_t port);
extern void outl(uint16_t port, uint32_t val);

static inline void io_wait(void) {
    // There are no devices connected to 0x80, so this does nothing
    outb(0x80, 0);
}