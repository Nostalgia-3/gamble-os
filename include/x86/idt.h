#pragma once

#include <types.h>

typedef struct {
    uint16_t isr_low;       // The lower 16 bits of the ISR's address
    uint16_t kernel_cs;     // The GDT segment selector that the CPU will load into CS before calling the ISR
    uint8_t  reserved;      // Set to zero
    uint8_t  attributes;    // Type and attributes; see the IDT page
    uint16_t isr_high;      // The higher 16 bits of the ISR's address
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idtr_t;

struct regs {
    /* 0x00 */ uint32_t err;
    /* 0x04 */ uint32_t edi;
    /* 0x08 */ uint32_t esi;
    /* 0x0C */ uint32_t ebp;
    /* 0x10 */ uint32_t esp;
    /* 0x14 */ uint32_t ebx;
    /* 0x18 */ uint32_t edx;
    /* 0x1C */ uint32_t ecx;
    /* 0x20 */ uint32_t eax;

    /* 0x24 */ uint32_t eip;
    /* 0x28 */ uint32_t cs;
    /* 0x2C */ uint32_t eflags;
    /* 0x30 */ uint32_t useresp;
    /* 0x34 */ uint32_t ss;

    // uint32_t eax;
    // uint32_t ecx;
    // uint32_t edx;
    // uint32_t ebx;
    // uint32_t esp;
    // uint32_t ebp;
    // uint32_t esi;
    // uint32_t edi;
    // uint32_t ss;
    // uint32_t useresp;
    // uint32_t eflags;
    // uint32_t cs;
    // uint32_t eip;

    // unsigned int err;
    // unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    // unsigned int eip, cs, eflags, useresp, ss;
} __attribute__((packed));

void idt_init();
void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags);