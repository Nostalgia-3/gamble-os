#include <x86/idt.h>
#include <x86/pic.h>
#include <interrupt.h>
#include <stdbool.h>
#include <memory.h>

__attribute__((aligned(16))) static idt_entry_t idt[256];
static idtr_t idtr;

extern void* isr_stub_table[32];
extern void* irq_handle_table[16];

extern void syscall_handler_asm(void);

#include <printf.h>

void idt_init() {
    idtr.base = (uint32_t) &idt[0];
    idtr.limit = (uint16_t) sizeof(idt_entry_t) * 256 - 1;

    // Setup PIC
    pic_move_ints(0x20, 0x28);
    for(int i=0;i<16;i++) pic_disable_irq(i);
    pic_enable_irq(2);

    for(int i=0;i<32;i++) {
        // 0x8E = interrupt gate
        idt_set_descriptor(i, isr_stub_table[i], 0x8E);
    }

    for(int i=0;i<16;i++) {
        // 0x8E = interrupt gate
        idt_set_descriptor(i+0x20, irq_handle_table[i], 0x8E);
    }

    // present, can be called by ring three, 32-bit interrupt gate
    // 0b1 11 0 1110
    idt_set_descriptor(0x80, syscall_handler_asm, 0x8E);

    __asm__ volatile ("lidt %0" : : "m"(idtr));
    __asm__ volatile ("sti");
}

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags) {
    idt_entry_t* descriptor = &idt[vector];

    descriptor->isr_low        = ((uint32_t)isr) & 0xFFFF;
    descriptor->kernel_cs      = 0x08; // bytes into the GDT (this is the code)
    descriptor->attributes     = flags;
    descriptor->isr_high       = ((uint32_t)isr) >> 16;
    descriptor->reserved       = 0;
}

int hook_interrupt(module *mod, uint8_t id) {
    if(mod == NULL) return -1;

    if(id >= 0x20 && id <= 0x30) {
        pic_enable_irq(id-0x20);
    }

    mod->_hooked_ints[id / 32] |= 1 << (id % 32);

    return 0;
}