#include "str.h"
#include "types.h"
#include <math.h>

#include <gosh/gosh.h>
#include <memory.h>
#include <int.h>

#include <drivers/x86/ata_pio.h>
#include <drivers/x86/i8042.h>
#include <drivers/x86/vga.h>

#include <drivers/pci.h>
#include <drivers/pci/i8254.h>
#include <drivers/pci/ac97.h>
#include <drivers/pci/uhci.h>
#include <drivers/pci/ehci.h>
#include <drivers/pci/nvme.h>

#include <multiboot.h>

#include <shell.h>

void irq_handler(u32 i) {
    k_handle_int((u8)i+0x20);
}

void syscall_handler() {
    puts_dbg("syscall");
}

void _start(multiboot_info_t *mbd, unsigned int magic) {
    idt_init();
    kprintf("[\x1b[92m+\x1b[0m] Initialized IDT\n");

    init_mem();
    kprintf("[\x1b[92m+\x1b[0m] Initialized memory\n");

    int gde = _init_gdevt();
    if(gde || get_gdevt() == NULL) {
        kprintf("[\x1b[91m-\x1b[0m] Failed initializing gdevt with error #%u\n", gde);
        kpanic();
    } else {
        kprintf("[\x1b[92m+\x1b[0m] Initialized gdevt\n");
    }

    initialize_pci();
    kprintf("[\x1b[92m+\x1b[0m] Initialized PCI\n");

    // Harcode some drivers for testing
    Driver DriverVGA        = { .name = "VGA.DRV",      .DriverEntry = VGA_DriverEntry, .data = (void*)(u32)0 };
    Driver DriverI8042      = { .name = "I8042.DRV",    .DriverEntry = I8042_DriverEntry, .DriverInt = I8042_DriverInt };
    Driver DriverATA_PIO    = { .name = "ATA.DRV",      .DriverEntry = ATA_DriverEntry, .DriverInt = ATA_DriverInt };
    Driver DriverPCI        = { .name = "PCI.DRV",      .DriverEntry = PCI_DriverEntry };

    PCIDriver uhci  = get_uhci_driver();
    PCIDriver ehci  = get_ehci_driver();
    PCIDriver ac97  = get_ac97_driver();
    PCIDriver i8254 = get_i8254_driver();
    PCIDriver nvme  = get_nvme_driver();

    PCI_ADD(&uhci);
    PCI_ADD(&ehci);
    PCI_ADD(&ac97);
    PCI_ADD(&i8254);
    PCI_ADD(&nvme);

    load_driver(&DriverVGA);
    load_driver(&DriverI8042);
    load_driver(&DriverATA_PIO);
    load_driver(&DriverPCI); // This is a wrapper that automatically loads registered PCI drivers

    Device *fb = fb_get(RESOLUTION);
    if(fb == NULL) {
        kprintf("No available framebuffers...\n");
    } else if(fb->data == NULL) {
        kprintf("Framebuffer data is NULL!\n");
    }

    if(!((u32)DriverI8042.data & 0b10000000)) {
        kprintf("Failed to initialize PS/2 keyboard and mouse :(\n");
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

    if(total_mem > 0xFFFFFFFF) total_mem = 0xFFFFFFFF;

    kprintf("Memory available: %u MB\n", ((u32)(total_mem & 0xFFFFFFFF))/(1024*1024)+1);
    kprintf("GaOS v0.1 (built with gcc v" __VERSION__ ")\n");

    if(shell_main(0))
        kpanic();

    while(1);
}