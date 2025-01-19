#ifndef GOSH_NET_H
#define GOSH_NET_H

#include <gosh/common.h>

typedef struct _nic_packet {
    u16 len;
    u8* buf;
} nic_packet;

typedef void(*packet_handler_t)(nic_packet* packet);

typedef struct _NICDeviceData {
    // Send a packet (max length being 64KiB)
    bool (*send_packet)(void *p_data, u16 p_len);
    // Poll for a packet, returning a packet or NULL if there are none
    nic_packet* (*get_packet)();

    // Add a packet handler, returning zero if unsuccessful
    bool (*add_handler)(packet_handler_t handle);
} NICDeviceData;

typedef enum _socket_t {
    socket_t_raw // This lets the program deal with the header stuff
} socket_t;

typedef struct _socket {
    Device *net_dev;
    socket_t type;
} socket;

// Create a socket, returning the pointer to the created socket.
socket* create_socket(Device *net_dev, socket_t type);

// Send a packet to a socket
bool    socket_send(socket *sock, void *data, u32 datalen);

inline long htonl(long v) {
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    // reverse bytes
    return
        (v & 0xFF000000) >> 24|
        (v & 0x00FF0000) >> 8 |
        (v & 0x0000FF00) << 8 |
        (v & 0x000000FF) << 24;
    #else
    return v;
    #endif
}

inline long long htonll(long long v) {
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    // reverse bytes
    #else
    return v;
    #endif
}

inline short htons(short v) {
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return ((v & 0xFF) << 8) | (v >> 8);
    #else
    return v;
    #endif
}

inline long ntohl(long v) {
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return
        (v & 0xFF000000) >> 24|
        (v & 0x00FF0000) >> 8 |
        (v & 0x0000FF00) << 8 |
        (v & 0x000000FF) << 24;
    #else
    return v;
    #endif
}

inline long long ntohll(long long v) {
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    // reverse bytes
    #else
    return v;
    #endif
}

inline short ntohs(short v) {
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return ((v & 0xFF) << 8) | (v >> 8);
    #else
    return v;
    #endif
}

typedef struct _NetDeviceData {
    u8 mac[6];
    bool connected;
    u32 ip;
} NetDeviceData;

#endif//GOSH_NET_H
