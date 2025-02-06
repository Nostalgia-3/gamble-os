#ifndef SYSTEM_H
#define SYSTEM_H

typedef unsigned int size_t;
typedef signed int ssize_t;

// Exit the current process with a specified code
void exit(int code);

// Open a pipe or file, returning the path file descriptor, or -1 if failed
int open(const char *path);

// Close a pipe or file
void close(int fd);

// Write to a file or pipe, returning the number of bytes actually written
ssize_t write(int fd, const void *buf, size_t count);

// Read from a file or pipe, returning the number of bytes actually read
ssize_t read(int fd, void *buf, size_t count);

// Set the position of a file or pipe, returning -1 if it failed for whatever
// reason
int seek(int fd, int offset, int whence);

#endif//SYSTEM_H