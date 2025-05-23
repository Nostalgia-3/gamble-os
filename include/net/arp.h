#pragma once

#include <net/net.h>

typedef struct _arp {
    uint16_t htype; // Hardware type
    uint16_t ptype; // Protocol type
    uint8_t  hlen; // Hardware address length (Ethernet = 6)
    uint8_t  plen; // Protocol address length (IPv4 = 4)
    uint16_t opcode; // ARP Operation Code
    uint8_t  dstsrc[]; // Variable size (hlen*2 + plen*2)
}__attribute__((packed)) arp;