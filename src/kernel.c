#include "str.h"
#include "types.h"
#include <math.h>

#include <process.h>

#include <gosh/gosh.h>
#include <memory.h>
#include <int.h>

#include <drivers/x86/ata_pio.h>
#include <drivers/x86/i8042.h>
#include <drivers/x86/vga.h>

#include <drivers/pci/i8254.h>
#include <drivers/pci/ac97.h>
#include <drivers/pci/uhci.h>
#include <drivers/pci/ehci.h>
#include <drivers/pci/nvme.h>
#include <drivers/fs/initrd.h>

#include <multiboot.h>

#include <shell.h>

void irq_handler(u32 i) {
    if(i == 0x00) {
        _next_proc();
    }
    k_handle_int((u8)i+0x20);
}

void syscall_handler() {
    puts_dbg("syscall");
}

void _start(multiboot_info_t *mbd, unsigned int magic) {
    // This is for the framebuffer; passing this is (imo) cheating,
    // and I should find a better way to make this work
    set_vga_mbd(mbd);

    idt_init();
    kprintf("Initialized interrupt table\n");

    init_mem();
    kprintf("Initialized kernel memory allocation\n");

    _setup_device_manager();
    int gde = _setup_module_manager();
    _init_vfs();
    if(gde) {
        kprintf("Failed creating module manager\n");
        kpanic();
    } else {
        kprintf("Module manager created\n");
    }

    multiboot_module_t *m = (void*)mbd->mods_addr;

    module_t vga    = get_vga_module(mbd);
    module_t i8042  = get_i8042_module();
    module_t ata    = get_ata_module();

    module_t uhci   = get_uhci_module();
    module_t ehci   = get_ehci_module();
    module_t ac97   = get_ac97_module();
    module_t i8254  = get_i8254_module();
    module_t nvme   = get_nvme_module();
    module_t initrd = get_initrd_module((void*)m->mod_start, m->mod_end-m->mod_start);

    open_module(&vga);
    open_module(&i8042);
    open_module(&initrd);
    // open_module(&ata);

    // open_module(&uhci);
    // open_module(&ehci);
    // open_module(&ac97);
    // open_module(&i8254);
    // open_module(&nvme);

    // mount("/dev/ramdisk", "/initrd/", "initrd");

    int stdout  = open("/dev/console");
    int stdin   = open("/dev/kbd");

    if(stdout == -1) {
        kprintf("Failed to open \"/dev/console\"!\n");
    }

    if(stdin == -1) {
        kprintf("Failed to open \"/dev/kbd\"!\n");
    }

    multiboot_uint64_t total_mem = 0;

    if(magic != 0x2BADB002) {
        kprintf("Wrong multiboot magic number: (got 0x%08X, expected 0x%08X)\n", magic, 0x2BADB002);
    } else if(!((mbd->flags >> 6) & 0x1)) {
        kprintf("Invalid memory map given by GRUB\n");
    } else {
        for(int i=0;i<mbd->mmap_length;i+=sizeof(multiboot_memory_map_t))  {
            multiboot_memory_map_t* mmmt =  (multiboot_memory_map_t*) (mbd->mmap_addr + i);

            if(mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
                total_mem += mmmt->len;
            }
        }
    }

    if(total_mem > 0xFFFFFFFF) {
        kprintf("(memory larger than 4GiB, so truncating to 4GiB)\n");
        total_mem = 0xFFFFFFFF;
    }

    kprintf("Memory available: %u MB\n", ((u32)(total_mem & 0xFFFFFFFF))/(1024*1024)+1);
    kprintf("GaOS v0.1 (built with gcc v" __VERSION__ ")\n");

    if(shell_main(mbd))
        kpanic();

    while(1);
}