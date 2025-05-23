#pragma once

#include <printf.h>
#include <types.h>

#include <memory.h>
#include <fs/vfs.h>
#include <process.h>

#define is_letter(c) ((((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z')) ? true : false)
#define is_printable(c) (((c) >= 32 && (c) <= 126) ? true : false)
#define is_digit(c) (((c) >= '0' && (c) <= '9') ? true : false)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

#define htons(x) (uint16_t)((((x) & 0x00FFU) << 8) | (((x) & 0xFF00U) >> 8))
#define htonl(x) \
    (uint32_t)((((x) & 0x000000FFU) << 24) | \
    (((x) & 0x0000FF00U) << 8)  | \
    (((x) & 0x00FF0000U) >> 8)  | \
    (((x) & 0xFF000000U) >> 24))

#endif

#define kpanic(reason, ...) do { \
    printf_("\x1b[91mKERNEL PANIC\x1b[0m: " reason " (\x1b[96m" __FILE__ "\x1b[0m:\x1b[93m%d\x1b[0m)\n", ##__VA_ARGS__, __LINE__); \
    __asm__ volatile("cli\n"); \
    while(1); } while(0);

void hexdump(uint8_t* addr, size_t count);

void set_fb(uint32_t pitch, uint32_t width, uint32_t height, void* addr);

// A generic structure that is used for every struct the kernel uses
typedef union _chunk {
    uint8_t     size[128]; // CHUNK_SIZE

    inode       inode;
    process     process;
    fs_mount    mount;
} chunk;