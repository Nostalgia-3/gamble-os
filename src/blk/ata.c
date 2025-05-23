#include <module.h>
#include <printf.h>
#include <fs/vfs.h>
#include <utils.h>

#define PRIMARY_BUS     0x1F0
#define SECONDARY_BUS   0x170

#define PRIMARY_ALT     0x3F6
#define SECONDARY_ALT   0x376

#define ALT_STATUS      0 // read
#define DEV_CTRL        0 // write
#define DRIVE_ADDR      1 // read

#define DATA            0 // read/write
#define ERROR           1 // read
#define FEATURES        1 // write
#define SECTOR_COUNT    2 // read/write
#define LBA_LOW         3 // read/write
#define LBA_MID         4 // read/write
#define LBA_HI          5 // read/write
#define DRIVE_SEL       6 // read/write
#define STATUS          7 // read
#define COMMAND         7 // write

#define ERR  0x1   // Indicates an error occurred. Send new cmd to clear
#define DRQ  0x8   // Set when drive has PIO dat to transfer, or is ready to accept PIO data
#define SRV  0x10  // Overlapped Mode Service Request
#define RDY  0x40  // Bit is clear when drive is spun down or after err. Set otherwise
#define BSY  0x80  // Indicates drive is preparing to send/receive data (wait for it to clear). 

// bit 0 = master/slave, bit 1 = primary/secondary bus
static uint8_t current_drive = 0xFF;

static inline volatile void long_wait() {
    io_wait(); io_wait(); io_wait(); io_wait();
}

void write_u8(uint16_t bus, uint8_t offset, uint8_t val) {
    return outb(bus + offset, val);
}

uint8_t read_u8(uint16_t bus, uint8_t offset) {
    return inb(bus + offset);
}

void soft_reset(uint16_t bus) {
    write_u8(bus, DEV_CTRL, 0b10);
    long_wait();
    write_u8(bus, DEV_CTRL, 0b00);
}

int select_drive(uint16_t bus, uint8_t drive) {
    uint8_t status = read_u8(bus, STATUS);

    if(status & (BSY | DRQ | ERR)) return -1;

    if(current_drive != 0xFF && drive == (current_drive & 1) && (current_drive >> 1) == !((bus >> 7) & 1)) {
        // drive already selected
        return 0;
    }

    outb(bus + DRIVE_SEL, 0xA0 | drive);
    long_wait();

    status = inb(bus + STATUS);

    if(status & (BSY | DRQ | ERR)) {
        printf("Failed to select drive %u (bus = %u)", drive, bus);
    }

    current_drive = drive | (!((bus >> 7) & 1) << 1);

    return 0;
}

int ata_start(module* mod) {
    if(read_u8(PRIMARY_BUS, STATUS) == 0xFF && read_u8(SECONDARY_BUS, STATUS) == 0xFF) {
        printf("ata primary + secondary bus is probably empty; exiting ata module\n");
        return 0;
    }

    soft_reset(PRIMARY_BUS);
    select_drive(PRIMARY_BUS, 0);

    write_u8(PRIMARY_BUS, LBA_LOW, 0);
    write_u8(PRIMARY_BUS, LBA_MID, 0);
    write_u8(PRIMARY_BUS, LBA_HI, 0);
    long_wait();
    write_u8(PRIMARY_BUS, COMMAND, 0xEC); // ATA_IDENTIFY
    long_wait();

    uint8_t status = inb(PRIMARY_BUS + STATUS);
    while((status & 0b10000000) || !(status & 0b1000)) {
        status = inb(PRIMARY_BUS + STATUS);
        if(status & ERR) {
            printf_("%02x\n", inb(PRIMARY_BUS + ERROR));
            break;
        }
    }

    // printf_("%02x, %02x\n", read_u8(PRIMARY_BUS, LBA_MID), read_u8(PRIMARY_BUS, LBA_HI));
    // volatile uint8_t cl = read_u8(PRIMARY_BUS, LBA_MID);
    // volatile uint8_t ch = read_u8(PRIMARY_BUS, LBA_HI);
    // printf_("cl: %02x, ch: %02x\n", cl, ch);

    return 0;
}

int ata_int(module* mod, uint32_t i) {
    return 0;
}

module get_ata_module() {
    return (module) {
        .name = "ata",

        .module_start = ata_start,
        .module_int   = ata_int,

        .priority = 0,
        .type = MODULE_NONE
    };
}