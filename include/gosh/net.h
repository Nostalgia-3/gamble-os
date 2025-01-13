#ifndef GOSH_NET_H
#define GOSH_NET_H

#include <types.h>

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