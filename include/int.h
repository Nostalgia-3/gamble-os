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

#define IDT_MAX_DESCRIPTORS 256

typedef struct {
	u16 isr_low;      // The lower 16 bits of the ISR's address
	u16 kernel_cs;    // The GDT segment selector that the CPU will load into CS before calling the ISR
	u8  reserved;     // Set to zero
	u8  attributes;   // Type and attributes; see the IDT page
	u16 isr_high;     // The higher 16 bits of the ISR's address
} __attribute__((packed)) idt_entry_t;

typedef struct {
	u16	limit;
	u32	base;
} __attribute__((packed)) idtr_t;

__attribute__((noreturn)) void exception_handler(u8 d);

void idt_init(void);
void idt_set_descriptor(u8 vector, void* isr, u8 flags);

#endif