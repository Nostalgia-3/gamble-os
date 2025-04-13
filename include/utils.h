#pragma once

#include <printf.h>
#include <types.h>

#define is_letter(c) ((((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z')) ? true : false)
#define is_printable(c) (((c) >= 32 && (c) <= 126) ? true : false)
#define is_digit(c) (((c) >= '0' && (c) <= '9') ? true : false)

#define kpanic(reason, ...) do { \
    printf_("\x1b[91mKERNEL PANIC\x1b[0m: " reason " (\x1b[96m" __FILE__ "\x1b[0m:\x1b[93m%d\x1b[0m)\n", ##__VA_ARGS__, __LINE__); \
    __asm__ volatile("cli\n"); \
    while(1); } while(0);

void hexdump(uint8_t* addr, size_t count);