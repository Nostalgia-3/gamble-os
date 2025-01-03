#include "str.h"
#include "types.h"
#include <math.h>

#include <gosh.h>
#include <memory.h>
#include <int.h>

#include <ata_pio.h>
#include <i8042.h>
#include <vga.h>


void irq_handler(u32 i) {
    k_handle_int((u8)i+0x20);
}

void syscall_handler() {
    // putc('S');
}

#include <shell.h>

// the 0 is a macro that gets filled in by `make.ts`
#define VERSION "0.0.0b1"

void _start(u8 boot, u32 mem) {
    Driver DriverATA_PIO    = { .name = "ATA.DRV", .DriverEntry = ATA_DriverEntry, .DriverInt = ATA_DriverInt };
    Driver DriverI8042      = { .name = "I8042.DRV", .DriverEntry = I8042_DriverEntry, .DriverInt = I8042_DriverInt };
    Driver DriverVGA        = { .name = "VGA.DRV", .DriverEntry = VGA_DriverEntry };

    // Test if the kernel is in C
    play_sound(80);
    while(1);

    play_sound(440);
    kprintf("Debug Build v" VERSION " (built with gcc v" __VERSION__ ")\n");
    play_sound(220);
    _init_gdevt();
    play_sound(440);
    idt_init();
    play_sound(220);
    load_driver(&DriverVGA);
    play_sound(440);
    load_driver(&DriverI8042);
    play_sound(220);
    // load_driver(&DriverATA_PIO);

    Device *fb = fb_get(RESOLUTION);
    fb_draw_rect(fb, 0, 0, 32, 32, 7);
    if(fb == NULL || fb->data == NULL) kpanic();
    fb_draw_rect(fb, 0, 0, 32, 32, 15);

    Device *vt = k_get_device_by_type(DEV_VIRT_TERM);
    if(vt == NULL || vt->data == NULL) kpanic();
    VTDeviceData *vtd = vt->data;
    vtd->write_handler = shell_write;

    if(!((u32)DriverI8042.data & 0b10000000)) {
        kprintf("Failed to initialize PS/2 keyboard and mouse :(\n");
        shell_write(vt); // hardcode this for now
        while(1);
    }

    nosound();
    if(shell_main(mem))
        kpanic();
    while(1);

    bool running = TRUE;
    u8 text_buf[80];
    u8 index = 0;

    return;
}
