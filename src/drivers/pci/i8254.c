/*

I8254 - Network Card
  The I8254 is the network card found in QEMU by default.
*/

#include <drivers/pci/i8254.h>
#include <drivers/pci.h>
#include <gosh/gosh.h>

#include <port.h>
#include <gosh/net.h>

#include <memory.h>

#define REG_CTRL        0x0000
#define REG_STATUS      0x0008
#define REG_EEPROM      0x0014
#define REG_CTRL_EXT    0x0018
#define REG_IMASK       0x00D0
#define REG_RCTRL       0x0100
#define REG_RXDESCLO    0x2800
#define REG_RXDESCHI    0x2804
#define REG_RXDESCLEN   0x2808
#define REG_RXDESCHEAD  0x2810
#define REG_RXDESCTAIL  0x2818

#define REG_TCTRL       0x0400
#define REG_TXDESCLO    0x3800
#define REG_TXDESCHI    0x3804
#define REG_TXDESCLEN   0x3808
#define REG_TXDESCHEAD  0x3810
#define REG_TXDESCTAIL  0x3818

#define REG_RDTR         0x2820 // RX Delay Timer Register
#define REG_RXDCTL       0x2828 // RX Descriptor Control
#define REG_RADV         0x282C // RX Int. Absolute Delay Timer
#define REG_RSRPD        0x2C00 // RX Small Packet Detect Interrupt

#define REG_TIPG         0x0410      // Transmit Inter Packet Gap
#define ECTRL_SLU        0x40        //set link up

#define RCTL_EN                         (1 << 1)    // Receiver Enable
#define RCTL_SBP                        (1 << 2)    // Store Bad Packets
#define RCTL_UPE                        (1 << 3)    // Unicast Promiscuous Enabled
#define RCTL_MPE                        (1 << 4)    // Multicast Promiscuous Enabled
#define RCTL_LPE                        (1 << 5)    // Long Packet Reception Enable
#define RCTL_LBM_NONE                   (0 << 6)    // No Loopback
#define RCTL_LBM_PHY                    (3 << 6)    // PHY or external SerDesc loopback
#define RTCL_RDMTS_HALF                 (0 << 8)    // Free Buffer Threshold is 1/2 of RDLEN
#define RTCL_RDMTS_QUARTER              (1 << 8)    // Free Buffer Threshold is 1/4 of RDLEN
#define RTCL_RDMTS_EIGHTH               (2 << 8)    // Free Buffer Threshold is 1/8 of RDLEN
#define RCTL_MO_36                      (0 << 12)   // Multicast Offset - bits 47:36
#define RCTL_MO_35                      (1 << 12)   // Multicast Offset - bits 46:35
#define RCTL_MO_34                      (2 << 12)   // Multicast Offset - bits 45:34
#define RCTL_MO_32                      (3 << 12)   // Multicast Offset - bits 43:32
#define RCTL_BAM                        (1 << 15)   // Broadcast Accept Mode
#define RCTL_VFE                        (1 << 18)   // VLAN Filter Enable
#define RCTL_CFIEN                      (1 << 19)   // Canonical Form Indicator Enable
#define RCTL_CFI                        (1 << 20)   // Canonical Form Indicator Bit Value
#define RCTL_DPF                        (1 << 22)   // Discard Pause Frames
#define RCTL_PMCF                       (1 << 23)   // Pass MAC Control Frames
#define RCTL_SECRC                      (1 << 26)   // Strip Ethernet CRC

// Buffer Sizes
#define RCTL_BSIZE_256                  (3 << 16)
#define RCTL_BSIZE_512                  (2 << 16)
#define RCTL_BSIZE_1024                 (1 << 16)
#define RCTL_BSIZE_2048                 (0 << 16)
#define RCTL_BSIZE_4096                 ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192                 ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384                ((1 << 16) | (1 << 25))


// Transmit Command
#define CMD_EOP                         (1 << 0)    // End of Packet
#define CMD_IFCS                        (1 << 1)    // Insert FCS
#define CMD_IC                          (1 << 2)    // Insert Checksum
#define CMD_RS                          (1 << 3)    // Report Status
#define CMD_RPS                         (1 << 4)    // Report Packet Sent
#define CMD_VLE                         (1 << 6)    // VLAN Packet Enable
#define CMD_IDE                         (1 << 7)    // Interrupt Delay Enable


// TCTL Register
#define TCTL_EN                         (1 << 1)    // Transmit Enable
#define TCTL_PSP                        (1 << 3)    // Pad Short Packets
#define TCTL_CT_SHIFT                   4           // Collision Threshold
#define TCTL_COLD_SHIFT                 12          // Collision Distance
#define TCTL_SWXOFF                     (1 << 22)   // Software XOFF Transmission
#define TCTL_RTLC                       (1 << 24)   // Re-transmit on Late Collision
#define TSTA_DD                         (1 << 0)    // Descriptor Done
#define TSTA_EC                         (1 << 1)    // Excess Collisions
#define TSTA_LC                         (1 << 2)    // Late Collision
#define LSTA_TU                         (1 << 3)    // Transmit Underrun

#define E1000_NUM_RX_DESC 32
#define E1000_NUM_TX_DESC 8

typedef struct _rx_desc {
        volatile u64 addr;
        volatile u16 length;
        volatile u16 checksum;
        volatile u8 status;
        volatile u8 errors;
        volatile u16 special;
} __attribute__((packed)) rx_desc;

typedef struct _tx_desc {
        volatile u64 addr;
        volatile u16 length;
        volatile u8 cso;
        volatile u8 cmd;
        volatile u8 status;
        volatile u8 css;
        volatile u16 special;
} __attribute__((packed)) tx_desc;

static Driver driver = (Driver) { .DriverEntry = I8254_DriverEntry, .DriverInt = I8254_DriverInt };

PCIDriver get_i8254_driver() {
    return (PCIDriver) {
        .vendor = 0x8086,
        .device = 0x100E,
        .class  = 0xFF,
        .subclass = 0xFF,
        .interface = 0xFF,
        .driver = &driver
    };
}
static rx_desc *rx_descs[E1000_NUM_RX_DESC];
static tx_desc *tx_descs[E1000_NUM_TX_DESC];
static bool         bar_type;
static volatile u32 mem_base;
static volatile u16 io_base;
static bool         eeprom_exists;
static u8           mac[6];
static u16          rx_cur;
static u16          tx_cur;

void write_comm(u32 reg, u32 val) {
    if(bar_type == 0) {
        *(u32*)(mem_base+reg) = val;
    } else {
        outl(io_base,   reg);
        outl(io_base+4, val);
    }
}

u32 read_comm(u32 reg) {
    if(bar_type == 0) {
        return *(u32*)(mem_base+reg);
    } else {
        outl(io_base, reg);
        io_wait();
        return inl(io_base+4);
    }
}

bool detect_eeprom() {
    u32 val;
    write_comm(REG_EEPROM, 0x1);

    for (int i = 0; i < 1024; i++)
        {
            if ((read_comm(0x14) & 0x10) != 0)
            {
                eeprom_exists = TRUE;
                break;
            }
        }

    return eeprom_exists;
}

u16 read_eeprom(u8 addr) {
    u16 data = 0;
    u16 tmp = 0;
    if(eeprom_exists) {
        write_comm(REG_EEPROM, (1) | ((u32)addr<<8));
        while(!((tmp = read_comm(REG_EEPROM))) & (1<<1));
    } else {
        write_comm( REG_EEPROM, (1) | ((u32)(addr) << 2) );
        while( !((tmp = read_comm(REG_EEPROM)) & (1 << 1)) );
    }
    return (u16)((tmp>>16)&0xFFFF);
}

bool read_mac_addr() {
    if(eeprom_exists) {
        u32 temp;
        temp    = read_eeprom(0);
        mac[0]  = temp & 0xff;
        mac[1]  = temp >> 8;
        temp    = read_eeprom(1);
        mac[2]  = temp & 0xff;
        mac[3]  = temp >> 8;
        temp    = read_eeprom(2);
        mac[4]  = temp & 0xff;
        mac[5]  = temp >> 8;
    } else {
        u32 mac0 = *(u32*)(mem_base+0x5400);
        u32 mac1 = *(u32*)(mem_base+0x5404);
        if(mac1 == 0) return FALSE;

        mac[0] = mac0 & 0xFF;
        mac[1] = (mac0 >> 8) & 0xFF;
        mac[2] = (mac0 >> 16) & 0xFF;
        mac[3] = (mac0 >> 24) & 0xFF;
        mac[4] = (mac1) & 0xFF;
        mac[5] = (mac1 >> 8) & 0xFF;
        
    }
    return TRUE;
}

void enable_ints() {
    write_comm(REG_IMASK, 0x1F6DC);
    write_comm(REG_IMASK, 0xFF & (~4));
    read_comm(0xC0);
}

void rxinit() {
    u8* ptr;
    rx_desc *descs;

    // Allocate buffer for receive descriptors. For simplicity, in my case khmalloc returns a virtual address that is identical to it physical mapped address.
    // In your case you should handle virtual and physical addresses as the addresses passed to the NIC should be physical ones
 
    ptr = (u8*)(k_malloc(sizeof(rx_desc)*E1000_NUM_RX_DESC + 16));

    descs = (rx_desc*)ptr;
    for(int i = 0; i < E1000_NUM_RX_DESC; i++)
    {
        rx_descs[i] = (rx_desc*)((u8*)descs + i*16);
        rx_descs[i]->addr = (u32)(u8 *)(k_malloc(8192 + 16));
        rx_descs[i]->status = 0;
    }

    write_comm(REG_TXDESCLO, 0);
    write_comm(REG_TXDESCHI, (u32)((u32)ptr & 0xFFFFFFFF));

    write_comm(REG_RXDESCLO, ptr);
    write_comm(REG_RXDESCHI, 0);

    write_comm(REG_RXDESCLEN, E1000_NUM_RX_DESC * 16);

    write_comm(REG_RXDESCHEAD, 0);
    write_comm(REG_RXDESCTAIL, E1000_NUM_RX_DESC-1);
    rx_cur = 0;
    write_comm(REG_RCTRL, RCTL_EN| RCTL_SBP| RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC  | RCTL_BSIZE_8192);
    
}

void I8254_DriverEntry(Device *dev) {
    Driver* driver = (Driver*)dev->data;
    u32 d = (u32)driver->data;
    u8 bus = d & 0xFF;
    u8 slot = (d >> 8) & 0xFF;

    GenPCIHeader header = pci_get_gen_header(bus, slot);

    pci_set_comm(PCI_IO_SPACE | PCI_MEM_SPACE | PCI_BUS_MASTER, bus, slot);

    // bar_type = header.bar[0] & 1;
    bar_type = 1;
    mem_base = (header.bar[0] & (~3));
    for(int i=0;i<5;i++) {
        if(header.bar[i+1] & 1) {
            io_base = (u16)(header.bar[i+1]) & 0xFFFFFFFC;
            break;
        }
    }

    kprintf("mem = %X, io = %X\n", mem_base, io_base);

    detect_eeprom();
    if(!read_mac_addr()) {
        kprintf("Fatal(i8254): Unable to read mac address\n");
        return;
    }

    enable_ints();

    rxinit();

    for(int i=0;i<0x80;i++)
        write_comm(0x5200+i*4, 0);
    
    kprintf("%02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void I8254_DriverInt(Device *dev, u8 intr) {

}