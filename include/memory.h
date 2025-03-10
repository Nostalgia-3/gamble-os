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
#define MEM_BASE    0x00200000 // 0x60000
#define MEM_END     0x00300000 // 0x7FFFF
#define MEM_MAX     (MEM_END-MEM_BASE)
#define BLOCK_SIZE  32      // 256
#define MAX_BLOCKS  (u32)((MEM_MAX)/BLOCK_SIZE)

#define MAX_ALLOCS  1234          // The max number of specific allocations

void init_mem();

void *k_malloc(size_t size);
void k_free(void *mem);

// Get the total amount of memory used (including the extra data at the eend)
int k_get_used();

void memcpy(void*dest, void*src, size_t num);

extern u32 kernel_page_dir[1024];
extern u32 kernel_page_tables[64][1024];

/// @brief Set a virtual page to a real one
/// @param virt_addr The 32-bit page-aligned logical address
/// @param real_addr The 64-bit page-aligned address (only 32-bits until PAE is supported)
/// @param flags Flags for the page
void set_page(void* virt_addr, u64 real_addr, u32 flags);

// Allocate pagecount pages, returning the virtual address. The location of the
// physical pages is forced to be linear
void *allocate_linear_pages(u32 pagecount);

// Set count pages starting at page-aligned addr to have the same physical
// address as virtual address
void *identity_pages(u32* addr, u32 count);

#endif