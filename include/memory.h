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

// there's a potential this doesn't exist
#define MEM_BASE   0x00EFFFFF
#define BLOCK_SIZE 4096

void *malloc(size_t size);
void free(void *mem);

#endif