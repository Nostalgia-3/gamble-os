/**
 * @file memory.h
 * @author Nostalgia3
 * @brief Have a damn simple memory allocation system
 * so I can make runtime things :)
 * @version 0.1
 * @date 2024-12-26
 * 
 */
#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

#define NULL 0

// This is REALLY bad, and I really should implement proper memory detection,
// but I just want to make a functional shell
// #define MEM_BASE    0x100000
#define MEM_BASE    0x60000
#define MEM_END     0x7FFFF
#define MEM_MAX     (MEM_END-MEM_BASE) // 492032
#define BLOCK_SIZE  16
#define MAX_BLOCKS  (u32)((MEM_MAX)/BLOCK_SIZE)

#define MAX_ALLOCS  1234          // The max number of specific allocations

void init_mem();

void *k_malloc(size_t size);
void k_free(void *mem);

// Get the total amount of memory used (including the extra data at the eend)
int k_get_used();

void memcpy(void*dest, void*src, size_t num);

#endif