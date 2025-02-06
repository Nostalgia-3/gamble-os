#ifndef PROC_H
#define PROC_H

#include <types.h>

/// @brief A structure that represents a process
typedef struct _process_t {
    /// @brief The unique id for the process
    u32 pid;

    /// @brief Stored registers
    u32 eax, ebx, ecx, edx, esp, ebp, esi, edi, eip;

    /// @brief 1024 pointers that either point to NULL or to a page table
    /// containing 1024 page entries.
    u32 *page_dir;

    char *current_path;

    // TODO: store information about open file descriptors
} process_t;

#endif//PROC_H