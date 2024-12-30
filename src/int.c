// This is basically all from https://wiki.osdev.org/Interrupts_Tutorial
// because I'm too stupid to do this without it
#include "int.h"
#include "types.h"

__attribute__((aligned(0x10))) static idt_entry idt[256];
static idtr_t idtr;
static bool vectors[IDT_MAX_DESCRIPTORS];

extern void* isr_stub_table[];

void exception_handler(void) {
    __asm__ volatile (
        "cli; hlt":
    ); // Completely hangs the computer
}

void idt_init(void) {
    idtr.base = (size_t)&idt[0];
    idtr.limit = (u16)sizeof(idt_entry) * IDT_MAX_DESCRIPTORS - 1;

    for (u8 vector = 0; vector < 32; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = TRUE;
    }

    __asm__ volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
    __asm__ volatile ("sti"); // set the interrupt flag
}

void idt_set_descriptor(u8 vector, void* isr, u8 flags) {
    idt_entry* descriptor = &idt[vector];

    descriptor->offset1         = (u32)isr & 0xFFFF;
    descriptor->segment         = 0x08; // this value can be whatever offset your kernel code selector is in your GDT
    descriptor->flags           = flags;
    descriptor->offset2         = (u32)isr >> 16;
    descriptor->reserved        = 0;
}