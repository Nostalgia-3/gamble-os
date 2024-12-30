// This is basically all from https://wiki.osdev.org/Interrupts_Tutorial
// because I'm too stupid to do this without it
#include "int.h"
#include "types.h"

__attribute__((aligned(0x10))) static idt_entry_t idt[IDT_MAX_DESCRIPTORS];
static idtr_t idtr;
static bool vectors[IDT_MAX_DESCRIPTORS];

extern void* isr_stub_table[];

#include <str.h>
#include <vga.h>
void exception_handler(u8 d) {
    puts("Exception #");
    puts(itoa(d, 10));
    puts(" called\n");
    if(d == 13) {

    }
    __asm__ volatile (
        "cli\n hlt":
    ); // Completely hangs the computer
    while(1); // <-- this is to make the compiler shut up
}

static bool vectors[IDT_MAX_DESCRIPTORS];

extern void* isr_stub_table[];
extern void* irq_handle_table[16];

void idt_init() {
    idtr.base = (u32)&idt[0];
    idtr.limit = (u16)sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

    for (u8 vector = 0; vector < 32; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = TRUE;
    }

    for(u8 vector = 0; vector < 16; vector++) {
        idt_set_descriptor(vector+0x20, irq_handle_table[vector], 0x8E);
        vectors[vector + 0x20] = TRUE;
    }

    __asm__ volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
    __asm__ volatile ("sti"); // set the interrupt flag
}

void idt_set_descriptor(u8 vector, void* isr, u8 flags) {
    idt_entry_t* descriptor = &idt[vector];

    descriptor->isr_low        = (u32)isr & 0xFFFF;
    descriptor->kernel_cs      = 0x08; // this value can be whatever offset your kernel code selector is in your GDT
    descriptor->attributes     = flags;
    descriptor->isr_high       = (u32)isr >> 16;
    descriptor->reserved       = 0;
}