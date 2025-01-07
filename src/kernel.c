#include "str.h"
#include "types.h"
#include <math.h>

#include <gosh.h>
#include <memory.h>
#include <int.h>

#include <drivers/x86/ata_pio.h>
#include <drivers/x86/i8042.h>
#include <drivers/x86/vga.h>

#include <drivers/pci.h>

#include <drivers/pci/ac97.h>
#include <drivers/pci/uhci.h>

void irq_handler(u32 i) {
    k_handle_int((u8)i+0x20);
}

void syscall_handler() {
    kprintf("syscall\n");
}

#include <shell.h>

typedef struct _KData {
    u8 boot_drive;
    u8 selected_video_mode;
    u16 mem_size;
} KData;

void _start() {
    KData *kd = (KData*)0x500;

    init_mem();

    k_malloc(sizeof(Device));

    _init_gdevt();
    if(get_gdevt() == NULL)
        kpanic();
    idt_init();
    initialize_pci();

    // Harcode some drivers for testing
    Driver DriverVGA        = { .name = "VGA.DRV", .DriverEntry = VGA_DriverEntry, .data = (void*)(u32)kd->selected_video_mode };
    Driver DriverI8042      = { .name = "I8042.DRV", .DriverEntry = I8042_DriverEntry, .DriverInt = I8042_DriverInt };
    Driver DriverATA_PIO    = { .name = "ATA.DRV", .DriverEntry = ATA_DriverEntry, .DriverInt = ATA_DriverInt };
    Driver DriverPCI        = { .name = "PCI.DRV", .DriverEntry = PCI_DriverEntry };

    PCIDriver uhci = get_uhci_driver();
    PCIDriver ac97 = get_ac97_driver();

    PCI_ADD(&uhci);
    PCI_ADD(&ac97);

    load_driver(&DriverVGA);
    load_driver(&DriverI8042);
    load_driver(&DriverATA_PIO);
    load_driver(&DriverPCI); // This automatically loads drivers based on the connected PCI devices

    Device *fb = fb_get(RESOLUTION);
    if(fb == NULL || fb->data == NULL) kpanic();

    Device *vt = k_get_device_by_type(DEV_VIRT_TERM);
    if(vt == NULL || vt->data == NULL) kpanic();

    if(!((u32)DriverI8042.data & 0b10000000)) {
        kprintf("Failed to initialize PS/2 keyboard and mouse :(\n");
        shell_write(vt); // hardcode this for now
        while(1);
    }

    kprintf("Debug Build (built with gcc v" __VERSION__ ")\n");
    if(shell_main(kd->mem_size))
        kpanic();

    return;
}