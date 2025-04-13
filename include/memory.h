#pragma once

#include <multiboot.h>
#include <types.h>

#define PAGE_SIZE 4096

#define PAGE_FLAG_COPY (1 << 0)

#define PAGE_SET_ZERO (1 << 0)
#define PAGE_READONLY (1 << 1)

// Align the physical address of malloc
#define MALLOC_ALIGNED_4096 (1 << 0)
#define MALLOC_FILL_ZERO    (1 << 1)

#ifndef NULL
#define NULL 0
#endif

typedef struct {
    uint32_t size;
} malloc_header;

void mem_init(multiboot_info_t *mbd);

void *kmalloc(size_t size, uint32_t flags);

void memset(void *ptr, int val, size_t amount);
void memcpy(void*dest, void*src, size_t num);

// Allocate a page and put it at addr, returning the physical address of the
// page
void* alloc_page(void* addr, uint32_t flags);

// Remove the page table entry at the virtual address, flushing the tlb
void remove_pages(void* addr, size_t count);

// Reallocate pages, optionally copying all data between the previous pages and
// the new pages. Flags are specified as PAGE_FLAG_*
void* realloc_pages(void* start, uint32_t pagecount, uint32_t new_pagecount, uint32_t flags);

void* get_current_pagedir();
void  swap_pagedir(void* pagedir);

// Get the physical address of a virtual address
void* virt_to_phys(void* addr);