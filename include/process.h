#ifndef PROC_H
#define PROC_H

#define MAX_FDS 64

#include <types.h>

/// @brief A structure that represents a process
typedef struct _process_t {
    /// @brief The unique id for the process
    u32 pid;

    /// @brief Stored registers
    u32 eax, ebx, ecx, edx, esp, ebp, esi, edi, eip;

    /// @brief 1024 pointers that either point to NULL or to a page table
    /// containing 1024 page entries (each of which correspond to 4KiB).
    u32 *page_dir;

    const char *path;
    char *current_path;

    fd_t fds[MAX_FDS];
} process_t;

// Initialize the process switcher
void _init_procs();

// Switch the process to the next process
void _next_proc();

#endif//PROC_H