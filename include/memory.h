#pragma once

#include <multiboot.h>
#include <types.h>
#include <utils.h>

#define PAGE_SIZE 4096
#define CHUNK_SIZE 128
#define NFHEAP_SIZE 4096 * 64 // 256KiB

#define CONTINUOUS (1 << 1) // Allocate pages in a continuous chunk of memory

#define ALIGN_PAGE (1 << 0) // Align the address to 4096-bytes

typedef union _chunk chunk;

// Initialize the memory manager
void mem_init(multiboot_info_t *mbd);

// Returns a free physical address
void* consume_free_page();

// Free a page, invalidating it and marking it as free in the page list
void free_page(void* addr);

// Set a block of memory to a specified value
void* memset(void* ptr, char val, size_t amount);

// Copy a section of memory to another
void memcpy(void* dest, void* src, size_t amount);

// Returns 1 if equal, 0 if not equal, or -1 if an error occurred
int memcmp(void* a1, void* a2, int len);

// Allocate a page and put it at addr, returning the
// physical address of the page
void* map_pages(void* addr, size_t count, uint32_t flags);

// Map a single physical page to a virtual page
void* map_address(void* virt, void* physical, uint32_t flags);

// Map a physical address to I/O space
void* map_io(void* addr, size_t pagecount, uint32_t flags);

// Copy a page from a physical source address to a virtual destination address
int copy_page(uint32_t** pdir, void* virt_source, void* virt_dest);

// Free a number of pages starting at address
void free_pages(void* addr, size_t count);

// Allocate a generic structure
chunk* alloc_chunks(size_t count, int flags);

// Free several chunks. Returns zero on success
int free_chunks(chunk* addr, size_t count);

// Allocate a section of memory from the no-free heap; only use for small
// elements such as strings. This will most likely be replaced in the future
void* allocate_nfheap(size_t count);

// Returns the physical address of the current page directory
void* get_current_pagedir();

// Swap the current page directory with another; page directory is a
// virtual address in the current page directory
void  swap_pagedir(void* pagedir);

// Get the physical address of a virtual address
void* virt_to_phys(void* addr);