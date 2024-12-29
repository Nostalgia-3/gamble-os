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

#define MEM_BASE   0x00007E00   // There is a max of 480.5 KiB here
#define BLOCK_SIZE 64           // 492032 / 64 = 7688 blocks
#define MAX_BLOCKS 7688         // This is basically `floor(492032/BLOCK_SIZE)`

#define MAX_ALLOCS MAX_BLOCKS   // The max number of specific allocations

void *k_malloc(size_t size);
void k_free(void *mem);

// Get the total amount of memory used (including the extra data at the eend)
int k_get_used();

#endif