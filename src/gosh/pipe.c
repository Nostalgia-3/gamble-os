#include <gosh/pipe.h>
#include <drivers/x86/vga.h>

// Write a buffer to a system port (similar to a file descriptor in linux)
void write(u32 port, void *buf, size_t len) {
    if(port == STDOUT) {
        vga_write(buf, len);
    } else if(port == STDIN) {
        write(STDOUT, (void*)buf, len);
        // write to a standard input buffer
    }
}

// Read data from a port to a buffer of len; returns the number of bytes actually read
size_t read(u32 port, void *buf, size_t len) {
    return 0;
}