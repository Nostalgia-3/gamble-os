#pragma once

#include <types.h>

typedef struct _nic_device {
    uint8_t mac[6];
    uint32_t ip;
    uint32_t gateway;

    module* mod;

    // statistics
    uint32_t packets_sent;
    uint32_t packets_recv;
} nic_device;

typedef struct _ethframe {
    uint8_t dest[6];
    uint8_t src[6];
    uint16_t type;
    uint8_t data[];
}__attribute__((packed)) ethframe;

// Allocate a 1518 byte chunk of memory to write a packet to
void*   alloc_packet();

int     free_packet(void* packet);