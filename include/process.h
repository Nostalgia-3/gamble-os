#pragma once

#include <types.h>
#include <fs/fs.h>

// The stack size in pages
#define STACK_SIZE 8

typedef struct {
    uint32_t entries[1024];
} page_table;

typedef struct {
    /* 0x00 */ uint32_t edi;
    /* 0x04 */ uint32_t esi;
    /* 0x08 */ uint32_t ebp;
    /* 0x0C */ uint32_t esp;
    /* 0x10 */ uint32_t ebx;
    /* 0x14 */ uint32_t edx;
    /* 0x18 */ uint32_t ecx;
    /* 0x1C */ uint32_t eax;
    /* 0x20 */ uint32_t eip;
    /* 0x24 */ uint32_t cs;
    /* 0x28 */ uint32_t eflags;
    /* 0x2C */ uint32_t ss;

    /* 0x30 */ size_t** page_dir;
    /* 0x34 */ uint32_t pid;
    /* 0x38 */ uint32_t data_size;
    /* 0x3C */ uint32_t text_size;
    /* 0x40 */ uint32_t stack_size;
    /* 0x44 */ uint32_t can_run;

    /* 0x48 */ inode** open_fds;
    /* 0x4C */ uint32_t data_start;
    
    /* 0x50 */ inode* source;
} process;

process*    create_process(inode* file);

// Clone a process, similar to how the fork() syscall on unix works
process*    clone_process(process* parent);

int         delete_process(process* p);

// id is equal to the requested id, and is set to the actual id. Returns true on success
// and false on failure.
int         open_inode(process* process, inode* in, int* id);
int         close_inode(process* process, int id);