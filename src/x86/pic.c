#include <x86/pic.h>
#include <x86/x86.h>

void pic_move_ints(uint8_t off1, uint8_t off2) {
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

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

void pic_disable_irq(uint8_t line) {
    uint16_t port = PIC1_DATA;

    if(line >= 8) { port = PIC2_DATA; line -= 8; }
    
    outb(port, inb(port) | (1 << line));
}

#include <printf.h>
void pic_enable_irq(uint8_t line) {
    uint16_t port = PIC1_DATA;

    if(line >= 8) {
        port = PIC2_DATA;
        line -= 8;
    }

    outb(port, inb(port) & ~(1 << line));
}