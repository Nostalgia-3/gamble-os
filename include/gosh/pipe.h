#ifndef PIPE_H
#define PIPE_H

#include <gosh/common.h>

// linked to 80x25 text mode; handles ansi, scrolling, and more
#define STDOUT   0
// The default behavior of debug input is to pass dbgin to dbgout, as well as
// give it to the program
#define STDIN    1

// Write a buffer to a system port (similar to a file descriptor in linux)
void write(u32 port, void *buf, size_t len);

// Read data from a port to a buffer of len; returns the number of bytes actually read
size_t read(u32 port, void *buf, size_t len);

#endif