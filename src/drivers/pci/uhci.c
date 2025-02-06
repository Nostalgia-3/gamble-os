#include <drivers/pci/uhci.h>
#include <drivers/pci.h>
#include <gosh/gosh.h>

#define CMD_RUN             (1<<0)
#define CMD_HOST_CON_RESET  (1<<1)
#define CMD_GLOBAL_RESET    (1<<2)
#define CMD_GLOBAL_SUSPEND  (1<<3)
#define CMD_GLOBAL_RESUME   (1<<4)
#define CMD_SOFTWARE_DEBUG  (1<<5)
#define CMD_CONFIGURE       (1<<6)
#define CMD_MAX_PACKET      (1<<7)

#define MAX_PACKET32 0
#define MAX_PACKET64 1

#define USBCMD              0x00
#define USBSTS              0x02
#define USBINTR             0x04
#define FRNUM               0x06
#define FRBASEADD           0x08
#define SOFMOD              0x0C
#define PORTSC1             0x10
#define PORTSC2             0x12

typedef struct _UHCI_IO {
    u16 usbcmd;
    u16 usbsts;
    u16 usbintr;
    u16 frame_num;
    u32 frbaseadd;
    u8  sofmod;
    u16 portsc1;
    u16 portsc2;
} UHCI_IO;

int uhci_entry(module_t *mod) {
    u8 bus  = mod->pci_flags.r_bus;
    u8 slot = mod->pci_flags.r_slot;

    kprintf("UCHI detected (pci bus = %u, pci slot = %u)\n", bus, slot);

    return DRIVER_SUCCESS;
}

int uhci_int(module_t *dev, u8 intr) {
    return 0;
}

module_t get_uhci_module() {
    return (module_t) {
        .name = "uhci",
        .module_start = uhci_entry,
        .module_int = uhci_int,
        .pci_flags = {
            .class = 0x0C,
            .subclass = 0x03,
            .interface = 0x00,
            .device = 0xFFFF,
            .vendor = 0xFFFF
        }
    };
}