#include <drivers/x86/ata_pio.h>
#include <gosh/gosh.h>
#include <port.h>
#include <str.h>
#include <memory.h>

// This driver is going to be REALLY mediocre, but for the bare minimum, idrc
#define IO_BASE1            0x1F0
#define CTRL_BASE1          0x3F6
#define IO_BASE2            0x170
#define CTRL_BASE2          0x376

#define IO_DATA_REG         0 // R/W    (16-bit/16-bit)
#define IO_ERROR_REG        1 // R      (8-bit /16-bit)
#define IO_FEATURE_REG      1 // W      (8-bit /16-bit)
#define IO_SECTOR_COUNT_REG 2 // R/W    (8-bit /16-bit)
#define IO_LBA_LOW_REG      3 // R/W    (8-bit /16-bit)
#define IO_LBA_MID_REG      4 // R/W    (8-bit /16-bit)
#define IO_LBA_HI_REG       5 // R/W    (8-bit /16-bit)
#define IO_DRIVE_SEL_REG    6 // R/W    (8-bit / 8-bit)
#define IO_STATUS_REG       7 // R      (8-bit / 8-bit)
#define IO_COMMAND_REG      7 // W      (8-bit / 8-bit)

#define CONT_ALT_STATUS_REG 0 // R      (8-bit / 8-bit)
#define CONT_DEV_CONT_REG   0 // W      (8-bit / 8-bit)
#define CONT_DRIVE_ADDR_REG 1 // R      (8-bit / 8-bit)

#define IRQ_FIRST_BUS       14
#define IRQ_SECOND_BUS      15

#define ATA_SR_ERR          0b00000001
#define ATA_SR_IDX          0b00000010
#define ATA_SR_CORR         0b00000100
#define ATA_SR_DRQ          0b00001000
#define ATA_SR_SRV          0b00010000
#define ATA_SR_DF           0b00100000
#define ATA_SR_RDY          0b01000000
#define ATA_SR_BSY          0b10000000

#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

#define ATA_CMD_IDENTIFY_PACKET 0xA1
#define ATA_CMD_IDENTIFY        0xEC

// #define PRIMARY_BUS_PORT_BASE       0x1F0
// #define PRIMARY_BUS_ALT_STATUS      0x3F6

#define MASTER_DRIVE 0xA0

#define READ_SECTORS_EXT  0x24

#define ERR  0x1   // Indicates an error occurred. Send new cmd to clear
#define DRQ  0x8   // Set when drive has PIO dat to transfer, or is ready to accept PIO data
#define SRV  0x10  // Overlapped Mode Service Request
#define RDY  0x40  // Bit is clear when drive is spun down or after err. Set otherwise
#define BSY  0x80  // Indicates drive is preparing to send/receive data (wait for it to clear). 

#define INVALID_ATA_STATUS 0xFF

#define LBA_MODE  0x40
#define ATA_ONCE  0x0
#define ATA_WAIT  0x1

typedef enum _ATADEV {
    IDE_ATA,
    IDE_ATAPI
} ATADEV;

typedef struct _ATADevice {
    // TRUE if exists
    bool exists;
    ATADEV type;
    u16 signature;
    u16 capabilities;
    u16 commandSets;
    u32 size;
    u8 model[40];
} ATADevice;

static u8 CURRENT_DRIVE;

static inline void long_wait() {
    io_wait();
    io_wait();
    io_wait();
    io_wait();
}

// Reset the ATA bus connected to ctrl_base
void soft_reset(u16 ctrl_base) {
    // The master drive on the bus is automatically selected.
    // ATAPI drives set values on their LBA_LOW and LBA_HIGH IO ports
    outb(ctrl_base, 0b00000010);    // set bit 2 (=0x02) to disable ints
    long_wait();
    outb(ctrl_base, 0b00000000);    // set bit 2 (=0x02) to disable ints
}

// Drive should either be 0 ("master") or 1 ("slave")
u8 select_drive(u16 base, u8 drive) {
    u8 mask = BSY | DRQ | ERR;

    u8 status = inb(base + IO_STATUS_REG);
    if (status & mask) {
        return 2;
    }

    // Already selected, nothing to do
    if (drive == CURRENT_DRIVE) {
        return 1;
    }

    // Select drive, then delay 400ns
    outb(base + IO_DRIVE_SEL_REG, 0xA0 | drive % 2);
    for(int i=0;i<4;i++) {
        inb(base);
    }

    // Read the Status register to clear pending interrupts
    // Ignore value
    inb(base + IO_STATUS_REG);

    // Read Status register one more time, use this to determine status
    status = inb(base + IO_STATUS_REG);

    if(status & mask) {
        kprintf("Error: Drive select failed\n");
        return 0;
    }
    
    CURRENT_DRIVE = drive;
    return 1;
}


#include <str.h>
ATADevice detect_devtype(u16 base, u8 channel) {
    ATADevice dev;
    dev.exists = FALSE;

    select_drive(base, channel);

    outb(base + IO_DRIVE_SEL_REG, 0xA0 | channel << 4);
    io_wait();
    outb(base + IO_LBA_LOW_REG, 0);
    io_wait();
    outb(base + IO_LBA_MID_REG, 0);
    io_wait();
    outb(base + IO_LBA_HI_REG, 0);
    io_wait();
    outb(base + IO_COMMAND_REG, (u8)(ATA_CMD_IDENTIFY));
    if(inb(base + IO_STATUS_REG) == 0) {
        return dev;
    }
    u8 type = IDE_ATA;

    // Polling
    u8 error = 0, status = 0;
    while((status & ATA_SR_BSY) || !(status & ATA_SR_DRQ)) {
        status = inb(base + IO_STATUS_REG);
        if(status & ATA_SR_ERR){ // If Err, Device is not ATA.
            error = inb(base + IO_ERROR_REG);
            break;
        }
    }

    if(error) {
        u8 LBAmid = inb(base + IO_LBA_MID_REG);
        u8 LBAhi = inb(base + IO_LBA_HI_REG);
        if (LBAmid == 0x14 && LBAhi == 0xEB)
            type = IDE_ATAPI;
        else if (LBAmid == 0x69 && LBAhi == 0x96)
            type = IDE_ATAPI;
        else {
            kprintf("Unknown device type (LBAmid=%X, LBAhi=%X)\n", LBAmid, LBAhi);
            soft_reset(channel);
            return dev;
        }
        outb(base + IO_COMMAND_REG, (u8)(ATA_CMD_IDENTIFY_PACKET));
        long_wait();
        error = 0, status = 0;
        while((status & ATA_SR_BSY) || !(status & ATA_SR_DRQ)) {
            status = inb(base + IO_STATUS_REG);
        }
        if(error) {
            // puts("Unexpected error on drive\n");
            soft_reset(channel);
            return dev;
        }
    }
    
    // Reading data
    u8 data_buffer[512];
    p_insw(base + IO_DATA_REG, (u16*)data_buffer, 256);
    dev.exists = TRUE;
    dev.type = type;
    dev.signature = *((u16*)(data_buffer + ATA_IDENT_DEVICETYPE));
    dev.capabilities = *((u16*)(data_buffer + ATA_IDENT_CAPABILITIES));
    dev.commandSets = *((u16*)(data_buffer + ATA_IDENT_COMMANDSETS));

    if(dev.commandSets & (1 << 26)) { // Device uses 48-Bit Addressing
        dev.size = *((u32 *)(data_buffer + ATA_IDENT_MAX_LBA_EXT));
    } else // Device uses CHS or 28-bit Addressing
        dev.size = *((u32 *)(data_buffer + ATA_IDENT_MAX_LBA));

    for(int k=0; k<40; k+=2){
        dev.model[k] = data_buffer[ATA_IDENT_MODEL + k + 1];
        dev.model[k + 1] = data_buffer[ATA_IDENT_MODEL + k];
        dev.model[40] = 0; // Terminate String.
    }

    return dev;
}

u8 ata_wait(u16 bus, u8 flag, u8 mode) {
    u8 mask = flag | ERR;
    u8 status = 0;

    // Continuously poll until response
    if (mode == ATA_WAIT){
        while (!status){
            status = (inb(bus) & mask);
        }
    } else {
        status = (inb(bus) & mask);
    }

    // If we error out, return failure
    if(status & ERR) {
        return 0;
    }

    // Otherwise the desired flag(s) set
    // Return the status since multiple may be set
    return status;
}

void check_PIO_status(u16 alt_bus) {
    u8 status = inb(alt_bus);
    u8 mask   = ERR | DRQ | ERR;

    if (!(status & mask)){
        kprintf("No flags set\n");
        return;
    }

    if(status & ERR) {
        kprintf("PIO error\n");
    }
    if(status & DRQ) {
        kprintf("DRQ set\n");
    }
    if(status & ERR) {
        kprintf("PIO BSY\n");
    }
}

void ATAPIO_read_sector(device_t *dev, u32 block, u8 *addr) {    
    // u16 base = IO_BASE1;
    // if(dev->code > 1) base = IO_BASE2;

    // outb(base + IO_DRIVE_SEL_REG, 0xE0 | (dev->code%2)<<4);

    // // Sectorcount high byte
    // outb(base + IO_SECTOR_COUNT_REG, (1 >> 8) & 0xFF);
    // io_wait();
    // io_wait();
    // io_wait();
    // io_wait();

    // // LBA bytes 4, 5, 6
    // outb(base + IO_LBA_LOW_REG, (block >> 24) & 0xFF);
    // outb(base + IO_LBA_MID_REG, (0) & 0xFF);
    // outb(base + IO_LBA_HI_REG, (0 >> 8) & 0xFF);

    // // Sectorcount low byte
    // outb(base + IO_SECTOR_COUNT_REG, (1) & 0xFF);

    // // LBA bytes 1, 2, 3
    // outb(base + IO_LBA_LOW_REG, (block) & 0xFF);
    // outb(base + IO_LBA_MID_REG, (block >> 8) & 0xFF);
    // outb(base + IO_LBA_HI_REG, (block >> 16) & 0xFF);

    // // Read sectors command
    // outb(base + IO_COMMAND_REG, READ_SECTORS_EXT);

    // if(ata_wait(base+0x206, DRQ, ATA_WAIT)) {
    //     for(int i=0;i<256;i++) {
    //         ((u16*)addr)[i] = inw(base + IO_DATA_REG);
    //     }
    // }
}

void ATAPIO_write_sector(device_t *dev, u32 sector, u8 *addr) {
    return;
}

static device_t sda;
static device_t sdb;
static device_t sdc;
static device_t sdd;

int ata_entry(module_t *mod) {
    return DRIVER_SUCCESS;
    // data->read_sector = ATAPIO_read_sector;
    // data->write_sector = ATAPIO_write_sector;
    // data->sectors = d1.size/512;
    // data->sector_size = 512;
    if(inb(IO_BASE1 + IO_STATUS_REG) == 0xFF) {
        kprintf("Warning: IO bus #1 has no drives\n");
    } else {
        ATADevice d1 = detect_devtype(IO_BASE1, 0);
        ATADevice d2 = detect_devtype(IO_BASE1, 1);
        if(d1.exists && d1.size > 0) {
            // create a drive device
            sda = (device_t) { 0 };
            register_device("/dev/sda", &sda);
        }

        if(d2.exists && d2.size > 0) {
            // create a drive device
            sdb = (device_t) { 0 };
            register_device("/dev/sdb", &sdb);
        }

        k_register_int(mod, IRQ_FIRST_BUS);
    }

    if(inb(IO_BASE2 + IO_STATUS_REG) != 0xFF) {
        ATADevice d3 = detect_devtype(IO_BASE2, 0);
        ATADevice d4 = detect_devtype(IO_BASE2, 1);

        if(d3.exists && d3.size > 0) {
            // create a drive device
            sdc = (device_t) { 0 };
            register_device("/dev/sdc", &sdd);
        }

        if(d4.exists && d4.size > 0) {
            // create a drive device
            sdd = (device_t) { 0 };
            register_device("/dev/sdd", &sdd);
        }

        k_register_int(mod, IRQ_SECOND_BUS);
    }

    return DRIVER_SUCCESS;
}

int ata_int(module_t *dev, u8 int_id) {
    return 0;
}

module_t get_ata_module() {
    return (module_t) {
        .name = "ata_pio",
        .module_start = ata_entry,
        .module_int = ata_int
    };
}