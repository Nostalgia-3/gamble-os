// This is basically all from https://wiki.osdev.org/Interrupts_Tutorial
// because I'm too stupid to do this without it
#include "int.h"
#include "types.h"

#include <gosh/gosh.h>
#include <port.h>

#define PIC1            0x20        /* IO base address for master PIC */
#define PIC2            0xA0        /* IO base address for slave PIC */
#define PIC1_COMMAND    PIC1
#define PIC1_DATA       (PIC1+1)
#define PIC2_COMMAND    PIC2
#define PIC2_DATA       (PIC2+1)
#define PIC_EOI         0x20        /* End-of-interrupt command code */
#define ICW1_ICW4       0x01        /* Indicates that ICW4 will be present */
#define ICW1_SINGLE     0x02        /* Single (cascade) mode */
#define ICW1_INTERVAL4  0x04        /* Call address interval 4 (8) */
#define ICW1_LEVEL      0x08        /* Level triggered (edge) mode */
#define ICW1_INIT       0x10        /* Initialization - required! */

#define ICW4_8086       0x01        /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02        /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08        /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C        /* Buffered mode/master */
#define ICW4_SFNM       0x10        /* Special fully nested (not) */

__attribute__((aligned(16))) static idt_entry_t idt[IDT_MAX_DESCRIPTORS];
static bool vectors[IDT_MAX_DESCRIPTORS];

static idtr_t idtr;

const char* exception_messages[] = 
{
	"Division By Zero",
	"Debug",
	"Non Maskable Interrupt",
	"Breakpoint",
	"Into Detected Overflow",
	"Out of Bounds",
	"Invalid Opcode",
	"No Coprocessor",
	"Double Fault",
	"Coprocessor Segment Overrun",
	"Bad TSS",
	"Segment Not Present",
	"Stack Fault",
	"General Protection Fault",
	"Page Fault",
	"Unknown Interrupt",
	"Coprocessor Fault",
	"Alignment Check",
	"Machine Check",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved"
};

#include <str.h>
void exception_handler(struct regs d) { // struct regs d
    // int err;
    // unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    // unsigned int eip, cs, eflags, useresp, ss;
    kprintf(
        "Exception \"%s\"\nEAX=%08X EBX=%08X ECX=%08X EDX=%08X\nESI=%08X EDI=%08X EBP=%08X ESP=%08X\nEIP=%08X EFL=%08X  CS=%08X  SS=%08X\nUSP=%08X\n",
        exception_messages[d.err],
        d.eax, d.ebx, d.ecx, d.edx,
        d.esi, d.edi, d.ebp, d.esp,
        d.eip, d.eflags, d.cs, d.ss,
        d.useresp
    );
    
    __asm__ volatile (
        "cli":
    ); // Completely hangs the computer
    while(1) __asm__ volatile("hlt");
}

static bool vectors[IDT_MAX_DESCRIPTORS];

extern void* isr_stub_table[];
extern void* irq_handle_table[16];
extern void sysint_handler_asm();
extern void _idt_load();

void disable_IRQ(u8 IRQline) {
    u16 port;
    u8 value;

    if(IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = inb(port) | (1 << IRQline);
    outb(port, value);
}

void enable_IRQ(u8 IRQline) {
    u16 port;
    u8 value;

    if(IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = inb(port) & ~(1 << IRQline);
    outb(port, value);
}

void t_move_pic(u8 off1, u8 off2) {
    u8 a1 = inb(PIC1_DATA);
    u8 a2 = inb(PIC2_DATA);
    
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC1_DATA, off1);
    io_wait();
    outb(PIC2_DATA, off2);
    io_wait();
    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();
    
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();
    
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void idt_init() {
    idtr.base = (u32)&idt[0];
    idtr.limit = (u16)sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

    t_move_pic(0x20, 0x28);
    enable_IRQ(0x0); enable_IRQ(0x1); enable_IRQ(0x2); enable_IRQ(0x3);
    enable_IRQ(0x4); enable_IRQ(0x5); enable_IRQ(0x6); enable_IRQ(0x7);
    enable_IRQ(0x9); enable_IRQ(0x9); enable_IRQ(0xA); enable_IRQ(0xB);
    enable_IRQ(0xC); enable_IRQ(0xD); enable_IRQ(0xE); enable_IRQ(0xF);

    for (u8 vector = 0; vector < 32; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = TRUE;
    }

    for(u8 vector = 0; vector < 16; vector++) {
        idt_set_descriptor(vector+0x20, irq_handle_table[vector], 0x8E);
        vectors[vector + 0x20] = TRUE;
    }

    // 32-bit interrupt gate with a DPL equal to #3
    idt_set_descriptor(80, sysint_handler_asm, 0b11101110);

    __asm__ volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
    __asm__ volatile ("sti");                   // set the interrupt flag
}

void idt_set_descriptor(u8 vector, void* isr, u8 flags) {
    idt_entry_t* descriptor = &idt[vector];

    descriptor->isr_low        = (u32)isr & 0xFFFF;
    descriptor->kernel_cs      = 0x08; // bytes into the GDT (this is the code)
    descriptor->attributes     = flags;
    descriptor->isr_high       = (u32)isr >> 16;
    descriptor->reserved       = 0;
}