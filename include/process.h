#pragma once

#include <types.h>
#include <fs/fs.h>

// The stack size in pages
#define STACK_SIZE 8

typedef struct {
    uint32_t entries[1024];
} page_table;

typedef struct {
    uint32_t eax, ebx, ecx, edx, esp, ebp, esi, edi, eip;
    uint32_t cs, eflags, ss;

    // 1024 page tables which, if bit 1 of the entry is set, contain 1024
    // pointers to pages (~1/4th of these point to the kernel_page_table)
    size_t** page_dir;

    // Process ID (a unique id)
    uint32_t pid;

    // The size of the data segment in bytes
    uint32_t data_size;
    
    // The size of the text segment in pages
    uint32_t text_size;

    // The size of the stack in pages
    uint32_t stack_size;

    // Whether the process can run currently (atm used as an excuse to not deal with page deallocation)
    bool    can_run;
} process;

process*    create_process(inode* file);