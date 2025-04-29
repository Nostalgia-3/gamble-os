#include <multiboot.h>
#include <memory.h>
#include <module.h>
#include <fs/vfs.h>
#include <device.h>
#include <x86/idt.h>
#include <modules.h>
#include <printf.h>
#include <utils.h>
#include <str.h>
#include <process.h>
#include <x86/gdt.h>
#include <scheduler.h>

extern uint32_t read_cr2();

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

#define EREG "\x1b[93m%08X\x1b[0m"

extern void exception_handler(struct regs d) {
    printf(
        "\x1b[91mException #%u\x1b[0m (\x1b[92m%s\x1b[0m):\n"
        "  EAX="EREG" EBX="EREG" ECX="EREG" EDX="EREG"\n"
        "  ESI="EREG" EDI="EREG" EBP="EREG" ESP="EREG"\n"
        "  EIP="EREG" EFL="EREG"  CS="EREG"  SS="EREG"\n"
        "  USP="EREG" CR2="EREG"",
        d.err, exception_messages[d.err],
        d.eax, d.ebx, d.ecx, d.edx,
        d.esi, d.edi, d.ebp, d.esp,
        d.eip, d.eflags, d.cs, d.ss,
        d.useresp, read_cr2()
    );

    __asm__ volatile("cli\n");
    while(1);
}

void irq_handler(uint32_t i) {
    module_int(i + 0x20);
}

typedef void (*func_ptr)(void);

void _start(multiboot_info_t *r_mbd, unsigned int magic) {
    multiboot_info_t* mbd = ((void*)r_mbd + 0xC0000000);

    idt_init();
    mem_init(mbd);
    gdt_init();

    pqueue_init();

    if(vfs_init() < 0) kpanic("Failed to initialize virtual filesystem");
    if(module_init() < 0) kpanic("Failed to initialize module system");
    if(device_init() < 0) kpanic("Failed to initialize device manager");

    multiboot_module_t* m = (void*)(mbd->mods_addr + 0xC0000000);

    module i8042    = get_i8042_module();
    module initrd   = get_initrd_module((void*)(m->mod_start + 0xC0000000), m->mod_end - m->mod_start);
    module tty      = get_tty_module();

    module_load(&tty);
    module_load(&i8042);
    module_load(&initrd);

    if(mount(NULL, node_at(get_root(), "/initrd", sizeof("/initrd")), "initrd") < 0) {
        kpanic("Failed to mount initrd!");
    }

    inode* init = node_at(get_root(), "/initrd/init", sizeof("/initrd/init"));

    if(init == NULL) {
        kpanic("/initrd/init not found!");
    }

    process* p = create_process(init);
    add_to_process_queue(p);

    pqueue_start();
    while(1);
}