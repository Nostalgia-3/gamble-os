/**
 * @file int.h
 * @author Nostalgia3
 * @brief The interrupt manager (this is pretty important)
 * @version 0.1
 * @date 2024-12-28
 * 
 */
#ifndef INT_H
#define INT_H

#include "types.h"

#define IDT_MAX_DESCRIPTORS 255

typedef struct _idt_entry {
    u16 offset1;    // The bottom 16 bits of the address
    u16 segment;    // The GDT segment of the handler
    u8 reserved;    // No valid data here
    u8 flags;       // The flags
    u16 offset2;    // The top 16 bits of the address
} __attribute__((packed)) idt_entry;

typedef struct _idtr_t {
    u16 limit;
    u32 base;
} __attribute__((packed)) idtr_t;

__attribute__((noreturn)) void exception_handler(void);

void idt_init(void);
void idt_set_descriptor(u8 vector, void* isr, u8 flags);

#endif