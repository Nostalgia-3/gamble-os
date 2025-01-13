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
void putcnoup(Device *vt, u8 key);
// Write a string to the virtual terminal's stdout, without updating the virtual
// terminal
void putsnoup(Device *vt, u8 *st);

// Send an update reqeuest to the virtual terminal
void update(Device *vt);

// Write a formatted string to the first virtual terminal without update the
// virtual terminal
__attribute__ ((format (printf, 1, 2))) void kprintfnoup(const char* format, ...);

// Write a formatted string to the first virtual terminal
__attribute__ ((format (printf, 1, 2))) void kprintf(const char* format, ...);

// Write a string to the virtual terminal's stdout
void    puts_vt(Device *vt, u8* str);
// Write a character to the virtual terminal's stdout
void    putc_vt(Device *vt, u8 key);
// Write a character to the virtual terminal's stdin
void    input_vt(Device *vt, u8 key);
// Read a character from the virtual terminal's stdin FIFO
u8      readc_vt(Device *vt);
// Flush the virtual terminal, clearing all stored text data
void    flush_vt(Device *vt);
// Remove count characters from the bottom of the virtual terminal
void    vt_shift(Device *vt, u32 count);

#endif//GOSH_VT_H