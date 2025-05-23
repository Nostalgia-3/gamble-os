#include <module.h>
#include <printf.h>
#include <memory.h>
#include <interrupt.h>
#include <x86/idt.h>
#include <x86/pic.h>

// There are four of these, offset by four for each one (0x20, 0x24, 0x28, 0x2C)
#define TSTART  0x20

// There are four of these, offset by four for each one (0x20, 0x24, 0x28, 0x2C)
#define TSTATUS 0x10

static void*     recv   = NULL;
static uint16_t  recvoff = 0;
static uint32_t* mmio   = NULL;
static uint16_t  io     = 0;
// round robin status
static uint32_t  rrs    = 0;

#define CRC32_POLY 0x04C11DB7 // 0xEDB88320

uint32_t ethernet_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ CRC32_POLY;
            else
                crc >>= 1;
        }
    }

    return ~crc;  // Final inversion
}

// Convert an MSB byte to a LSB byte (for ethernet crc32 checksum)
unsigned char reverse(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

int rtl8139_start(module* mod) {
    uint8_t bus = mod->requirements.pci.bus, slot = mod->requirements.pci.slot;
    // void* mmio = 0;
    // uint16_t io = 0;
    
    pci_config_write_word(
        bus, slot, 0, 4,
        (pci_config_read_word(bus, slot, 0, 4) & ~(1 << 10)) | 0b111
    );

    for(int i=0;i<6;i++) {
        uint32_t addr = pci_config_read_long(bus, slot, 0, 0x10 + i*4);

        if(addr == 0 || addr == ~0) continue;

        if(addr & 1) {
            io = (uint16_t)(pci_config_read_long(bus, slot, 0, 0x10 + i*4) & ~(0b11));
        } else {
            mmio = (void*)(pci_config_read_long(bus, slot, 0, 0x10 + i*4) & ~(0b111));
        }
    }

    recv = (void*)alloc_chunks(8192 / CHUNK_SIZE + 1, 0);

    outb(io + 0x52, 0);     // "power on" the device
    outb(io + 0x37, 0x10);  // software reset
    rrs = 0;

    while((inb(io + 0x37) & 0x10) != 0);

    // Setup receiving buffer
    outl(io + 0x30, (uint32_t)virt_to_phys(recv));

    // Enable receiving broadcast, multicast, physical match, and "all" packets
    outl(io + 0x44, 0xF);

    // enable transmitter and receiver bits in the command register
    outb(io + 0x37, (1 << 3) | (1 << 2));

    // enable Transmit OK and Receive OK interrupts
    outw(io + 0x3C, (1 << 2) | (1 << 0));

    hook_interrupt(mod, 0x20 + (pci_config_read_word(bus, slot, 0, 0x3C) & 0xFF));

    register_device((dev_entry) {
        .type = DEVICE_NIC,

        .nic = {
            .mac = { inb(io), inb(io+1), inb(io+2), inb(io+3), inb(io+4), inb(io+5) },
            .mod = mod,
            .ip = 0x0a00020f,
            .gateway = 0x0a000202,
            .packets_recv = 0,
            .packets_sent = 0
        }
    });

    // outl(io + TSTART + rrs*4, (uint32_t)virt_to_phys(packet));
    // outl(io + TSTATUS, 64 | (0b10 << 16));
    // rrs++;

    return 0;
}

int rtl8139_int(module* mod, uint32_t in) {
    uint16_t status = inw(io + 0x3e);
    outw(io + 0x3E, 0x0005);

    if(status & (1 << 2)) {
        // transmit
        printf_("sent packet\n");
    }

    if(status & (1 << 0)) {
        // receive
        // printf_("received packet (%u)\n", recvoff);
        uint16_t* packet = (uint16_t*)(recv + recvoff);

        uint16_t header = packet[0];
        uint16_t packet_len = packet[1];

        if((header & (1 << 0)) == 0) {
            // printf_("invalid packet\n");
            return -1;
        }

        if(packet_len >= 4096 || packet_len == 0) {
            // printf_("bogus packet size (%u)\n", packet_len);
            return -1;
        }

        // printf_("header: %04X, length: %04X\n", header, packet_len);

        recvoff = (recvoff + packet_len + 4 + 3) & ~3;
    }

    return 0;
}

module get_rtl8139_module() {
    return (module) {
        .name = "rtl8139",

        .module_start = rtl8139_start,
        .module_int   = rtl8139_int,

        .priority = 0,
        .type = MODULE_PCI,
        .requirements = {
            .pci = {
                .vendor = 0x10ec,
                .device = 0x8139,
                .class  = 0xFF,
                .subclass = 0xFF,
                .interface = 0xFF
            }
        }
    };
}