#include <drivers/pci/ac97.h>
#include <gosh/gosh.h>
#include <memory.h>
#include <types.h>
#include <port.h>

#define RESET           0x00
#define CAPABILITIES    0x00
#define MASTER_VOL      0x02
#define AUX_OUT_VOL     0x04
#define MONO_VOL        0x06
#define MASTER_TONE     0x08
#define PC_BEEP         0x0A
#define PHONE_VOL       0x0C
#define MIC_VOL         0x0E
#define LINEIN_VOL      0x10
#define CD_VOL          0x12
#define VIDEO_VOL       0x14
#define AIXIN_VOL       0x16
#define PCM_OUT_VOL     0x18
#define RECORD_SEL      0x1A
#define RECORD_GAIN     0x1C
#define RECORD_GAIN_MIC 0x1E
#define GENERAL_PURPOSE 0x20
#define CONTROL_3D      0x22
#define POWERDOWN_CTRL  0x26
#define EX_AUDIO        0x28
#define EX_AUDIO_CTRL   0x2A
#define PCM_FRONT_FAC   0x2C
#define PCM_SURR_DAC    0x2E
#define PCM_LFE_DACRATE 0x30
#define PCM_LR_ADCRATE  0x32
#define MIC_ADC_RATE    0x34
#define CH6VOL_LFE      0x36
#define CH6VOL_LRSURR   0x38
#define SPDIF           0x3A

// Uses the Intel AC'97 Mnemonics
#define PI_BDBAR    0x00
#define PI_CIV      0x04
#define PI_LVI      0x05
#define PI_SR       0x06
#define PI_PICB     0x08
#define PI_PIV      0x0A
#define PI_CR       0x0B
#define PO_BDBAR    0x10
#define PO_CIV      0x14
#define PO_LVI      0x15
#define PO_SR       0x16
#define PO_PICB     0x17
#define PO_PIV      0x1A
#define PO_CR       0x1B
#define MC_BDBAR    0x20
#define MC_CIV      0x24
#define MC_LVI      0x25
#define MC_SR       0x26
#define MC_PICB     0x28
#define MC_PIV      0x2A
#define MC_CR       0x2B
#define GLOB_CNT    0x2C
#define GLOB_STA    0x30
#define CAS         0x34
#define MC2_BDBAR   0x40
#define MC2_CIV     0x44
#define MC2_LVI     0x45
#define MC2_SR      0x46
#define MC2_PICB    0x48
#define MC2_PIV     0x4A
#define MC2_CR      0x4B
#define PI2_BDBAR   0x50
#define PI2_CIV     0x54
#define PI2_LVI     0x55
#define PI2_SR      0x56
#define PI2_PICB    0x58
#define PI2_PIV     0x5A
#define PI2_CR      0x5B
#define SPBAR       0x60
#define SPCIV       0x64
#define SPLVI       0x05
#define SPSR        0x66
#define SPPICB      0x68
#define SPPIV       0x6A
#define SPCR        0x6B
#define SDM         0x80

static Driver driver = { .name = "AC97.DRV", .DriverEntry=AC97_DriverEntry, .DriverInt=AC97_DriverInt };
static bool headphones_connected;

typedef struct _AC97BufferEntry {
 u32 sample_memory;
 u16 number_of_samples;
 u16 reserved: 14;
 u8  last_buffer_entry: 1;
 u8  interrupt_on_completion: 1;
}__attribute__((packed)) AC97BufferEntry;

AC97BufferEntry *buf_mem;

PCIDriver get_ac97_driver() {
    return (PCIDriver) {
        .vendor = 0xFFFF,
        .device = 0xFFFF,
        .class  = 0x04,
        .subclass = 0x01,
        .interface = 0xFF,
        .driver = &driver
    };
}

void AC97_DriverEntry(Device *dev) {
    Driver* driver = (Driver*)dev->data;
    u32 d = (u32)driver->data;
    u8 bus = d & 0xFF;
    u8 slot = (d >> 8) & 0xFF;

    GenPCIHeader header = pci_get_gen_header(bus, slot);
    pci_set_comm(PCI_IO_SPACE | PCI_BUS_MASTER, bus, slot);

    u16 nam     = (u16) (header.bar[0] & 0xFFFFFFFC);
    u16 nabm    = (u16) (header.bar[1] & 0xFFFFFFFC);

    // 16-bit audio, no interrupt, 2 channels
    outw(nabm + GLOB_CNT, (0b00 << 22) | (0b00 << 20) | (0<<2) | (1<<1));
    wait(20);

    // reset control registers
    outb(nabm + PI_CR, 0b10);
    outb(nabm + PO_CR, 0b10);
    outb(nabm + MC_CR, 0b10);
    wait(20);

    // reset NAM registers
    outw(nam + RESET, 0xFF);

    // @TODO make this
    headphones_connected = FALSE;
    if((inw(nam + CAPABILITIES) & 0x10) == 0x10 && inw(nam + AUX_OUT_VOL) == 0x8000) {
        headphones_connected = TRUE;
    }

    // Disable PCM volume
    outw(nam + PCM_OUT_VOL, 0);

    buf_mem = k_malloc(sizeof(AC97BufferEntry)*32);

    u16 ex_capabilities = inw(nam + EX_AUDIO);
    if((ex_capabilities & 0x01) == 0x01) {
        // Enable variable sample rate
        outw(nam + EX_AUDIO_CTRL, 0x01);
    }

    outl(nabm + PO_BDBAR, (u32)buf_mem);

    // kprintf("int pin = %u, int line = %u\n", header.interrupt_pin, header.interrupt_line);
    // kprintf("PO_SR = %04X\n", inw((u16)(nabm) + PO_SR));
    // kprintf("AC97 (bus=%u, slot=%u, bar0=%X, bar1=%X)\n", bus, slot, header.bar0 & 0xFFFFFFFC, header.bar1 & 0xFFFFFFFC);
}

void AC97_DriverInt(Device *dev, u8 intr) {

}