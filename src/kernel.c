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

void irq_handler(u32 i) {
    k_handle_int((u8)i+0x20);
}

void syscall_handler() {
    puts_dbg("syscall");
}

#include <shell.h>

void _start() {
    *(u8*)0xB8000 = '!';

    idt_init();
    puts_dbg("Initialized IDT\n");

    init_mem();
    kprintf("Initialized memory\n");

    int gde = _init_gdevt();
    if(gde || get_gdevt() == NULL) {
        kprintf("Failed initializing gdevt with error #%u\n", gde);
        kpanic();
    } else {
        kprintf("Initialized gdevt\n");
    }

    initialize_pci();
    puts_dbg("Initialized PCI\n");

    // Harcode some drivers for testing
    Driver DriverVGA        = { .name = "VGA.DRV",      .DriverEntry = VGA_DriverEntry, .data = (void*)(u32)0 };
    Driver DriverI8042      = { .name = "I8042.DRV",    .DriverEntry = I8042_DriverEntry, .DriverInt = I8042_DriverInt };
    Driver DriverATA_PIO    = { .name = "ATA.DRV",      .DriverEntry = ATA_DriverEntry, .DriverInt = ATA_DriverInt };
    Driver DriverPCI        = { .name = "PCI.DRV",      .DriverEntry = PCI_DriverEntry };

    PCIDriver uhci  = get_uhci_driver();
    PCIDriver ehci  = get_ehci_driver();
    PCIDriver ac97  = get_ac97_driver();
    PCIDriver i8254 = get_i8254_driver();

    PCI_ADD(&uhci);
    PCI_ADD(&ehci);
    PCI_ADD(&ac97);
    PCI_ADD(&i8254);

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

    divbyzero();
    // end_dbg();

    kprintf("GaOS v0.1 (built with gcc v" __VERSION__ ")\n");

    if(shell_main(0))
        kpanic();

    while(1);
}