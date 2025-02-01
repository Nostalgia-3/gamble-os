#ifndef GOSH_VT_H
#define GOSH_VT_H

#include <gosh/common.h>

typedef struct _VTDeviceData {
    // Contains all in text
    u8 infifo[32];
    u32 infifo_ind;
    size_t infifo_len;
    
    // Contains all unflushed out text
    u8* text;
    size_t textlen;
    size_t textind;

    // called when the infifo is written to
    void (*write_handler)(Device *vt);
} VTDeviceData;

Device *stdvt();

// Write a character to the virtual terminal's stdout, without updating the
// virtual terminal
void putcnoup(u8 key);
// Write a string to the virtual terminal's stdout, without updating the virtual
// terminal
void putsnoup(u8 *st);

// Write a formatted string to the first virtual terminal without update the
// virtual terminal
__attribute__ ((format (printf, 1, 2))) void kprintfnoup(const char* format, ...);

// Write a formatted string to the first virtual terminal
__attribute__ ((format (printf, 1, 2))) void kprintf(const char* format, ...);

#endif//GOSH_VT_H