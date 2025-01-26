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

void irq_handler(u32 i) {
    k_handle_int((u8)i+0x20);
}

void syscall_handler() {
    kprintf("syscall\n");
}

#include <shell.h>

void _start() {
    initialize_serial();

    puts_dbg("Primary initialization...\n");
    puts_dbg("  Initializing memory...\t\t");
    init_mem();
    // TODO: seriously figure out why this is needed for it not to crash on some hardware
    // k_malloc(sizeof(Device));
    puts_dbg("\x1b[32mOK\x1b[0m\n");

    puts_dbg("  Initializing IDT...\t\t\t");
    idt_init();
    puts_dbg("\x1b[32mOK\x1b[0m\n");

    puts_dbg("  Initializing GDevTable...\t\t");
    int gde = _init_gdevt();
    if(gde || get_gdevt() == NULL) {
        puts_dbg("\x1b[31mFAILED(");
        puts_dbg(itoa(gde, 10));
        puts_dbg(")\x1b[0m\n");
        kpanic();
    } else {
        puts_dbg("\x1b[32mOK\x1b[0m\n");
    }

    puts_dbg("  Initializing PCI...\t\t\t");
    initialize_pci();
    puts_dbg("\x1b[32mOK\x1b[0m\n");

    puts_dbg("  Loading drivers...\t\t\t");
    // Harcode some drivers for testing
    Driver DriverVGA        = { .name = "VGA.DRV",      .DriverEntry = VGA_DriverEntry, .data = (void*)(u32)0 };
    Driver DriverI8042      = { .name = "I8042.DRV",    .DriverEntry = I8042_DriverEntry, .DriverInt = I8042_DriverInt };
    Driver DriverATA_PIO    = { .name = "ATA.DRV",      .DriverEntry = ATA_DriverEntry, .DriverInt = ATA_DriverInt };
    Driver DriverPCI        = { .name = "PCI.DRV",      .DriverEntry = PCI_DriverEntry };

    PCIDriver uhci  = get_uhci_driver();
    PCIDriver ac97  = get_ac97_driver();
    PCIDriver i8254 = get_i8254_driver();

    PCI_ADD(&uhci);
    PCI_ADD(&ac97);
    PCI_ADD(&i8254);

    load_driver(&DriverVGA);
    load_driver(&DriverI8042);
    load_driver(&DriverATA_PIO);
    load_driver(&DriverPCI);
    puts_dbg("\x1b[32mOK\x1b[0m\n");

    puts_dbg("Primary initialization... \x1b[32mOK\x1b[0m\n");

    Device *fb = fb_get(RESOLUTION);
    if(fb == NULL) {
        puts_dbg("No available framebuffers...\n");
    } else if(fb->data == NULL) {
        puts_dbg("Framebuffer data is NULL!\n");
    }

    Device *vt = k_get_device_by_type(DEV_VIRT_TERM);
    if(vt == NULL || vt->data == NULL) {
        puts_dbg("No virtual terminal!\n");
        kpanic();
    }

    if(!((u32)DriverI8042.data & 0b10000000)) {
        puts_dbg("Failed to initialize PS/2 keyboard and mouse :(\n");
        while(1);
    }

    kprintf("GaOS v0.1 (built with gcc v" __VERSION__ ")\n");

    if(shell_main(0))
        kpanic();

    while(1);

    // if(shell_main(0))
    //     kpanic();

    // return;
}