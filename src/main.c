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

static uint32_t step = 0;
#define STEP(x) (step++);

extern void exception_handler(const struct regs* d) {
    if(get_current_process() == NULL) {
        printf_(
            "\x1b[91mException #%u\x1b[0m (\x1b[92m%s\x1b[0m):\n"
            " EAX="EREG" EBX="EREG" ECX="EREG" EDX="EREG"\n"
            " ESI="EREG" EDI="EREG" EBP="EREG" ESP="EREG"\n"
            " EIP="EREG" EFL="EREG"  CS="EREG"  SS="EREG"\n"
            " USP="EREG" CR2="EREG" STP=\x1b[92m%u\x1b[0m\n",
            d->err, exception_messages[d->err],
            d->eax, d->ebx, d->ecx, d->edx,
            d->esi, d->edi, d->ebp, d->esp,
            d->eip, d->eflags, d->cs, d->ss,
            d->useresp, read_cr2(), step
        );
        while(1);
    } else {
        printf("\nA process caused an exception! (process id = %u)", get_current_process()->pid);
        printf_(
            "\x1b[91mException #%u\x1b[0m (\x1b[92m%s\x1b[0m):\n"
            " EAX="EREG" EBX="EREG" ECX="EREG" EDX="EREG"\n"
            " ESI="EREG" EDI="EREG" EBP="EREG" ESP="EREG"\n"
            " EIP="EREG" EFL="EREG"  CS="EREG"  SS="EREG"\n"
            " USP="EREG" CR2="EREG" STP=\x1b[92m%u\x1b[0m\n",
            d->err, exception_messages[d->err],
            d->eax, d->ebx, d->ecx, d->edx,
            d->esi, d->edi, d->ebp, d->esp,
            d->eip, d->eflags, d->cs, d->ss,
            d->useresp, read_cr2(), step
        );
        while(1);
    }
}

void irq_handler(const uint32_t i) {
    module_int(i + 0x20);
}

void _start(multiboot_info_t *r_mbd, unsigned int magic) {
    multiboot_info_t* mbd = ((void*)r_mbd + 0xC0000000);
    
    step = 0;

    idt_init(); STEP(1);
    mem_init(mbd); STEP(2);
    gdt_init(); STEP(3);

    if(vfs_init() < 0) kpanic("Failed to initialize virtual filesystem"); STEP(4);
    if(module_init() < 0) kpanic("Failed to initialize module system"); STEP(5);
    if(device_init() < 0) kpanic("Failed to initialize device manager"); STEP(6);

    multiboot_module_t* m = (void*)(mbd->mods_addr + 0xC0000000);

    module mods[] = {
        get_i8042_module(),
        get_tty_module(mbd->framebuffer_pitch),
        get_tarfs_module(),
        get_ramdisk_module((void*)(m->mod_start + 0xC0000000), m->mod_end - m->mod_start),
        get_framebuffer_module((void*)0xE0001000, mbd->framebuffer_height*mbd->framebuffer_pitch),
        
        // blk
        get_ata_module(),

        // net
        get_rtl8139_module(),
        get_bcm4312_module(),
        get_ar9287_module()
    };
    
    for(int i=0;i<(sizeof(mods)/sizeof(module));i++) {
        module_load(&mods[i]);
    }

    if(mount(node_at(get_root(), "/dev/ramdisk", sizeof("/dev/ramdisk")), node_at(get_root(), "/initrd", sizeof("/initrd")), "tarfs") < 0) {
        kpanic("Failed to mount initrd!");
    }

    STEP(15);

    inode* init = node_at(get_root(), "/initrd/init", sizeof("/initrd/init"));

    if(init == NULL) {
        kpanic("/initrd/init not found!");
    }

    STEP(16);
    process* p = create_process(init);
    STEP(17);
    add_to_process_queue(p);

    STEP(18);
    pqueue_start();
    STEP(19);
    while(1);
}