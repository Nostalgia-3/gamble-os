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

#define MEM_BASE    0x00007E00
#define MEM_MAX     492032
#define BLOCK_SIZE  16
#define MAX_BLOCKS  (u32)(MEM_MAX/BLOCK_SIZE)

#define MAX_ALLOCS  1234          // The max number of specific allocations

void *k_malloc(size_t size);
void k_free(void *mem);

extern void get_memory_tree(u16 addr);

// Get the total amount of memory used (including the extra data at the eend)
int k_get_used();

#endif