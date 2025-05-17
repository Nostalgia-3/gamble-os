#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdnoreturn.h>

#define STDIN  0
#define STDOUT 1

#define TTY_CHANGE_PALETTE 0
#define TTY_GET_SIZE 1

typedef int32_t ssize_t;

typedef enum {
    INODE_FILE,
    INODE_DEV,
    INODE_DIR,
    INODE_LINK
} inode_type;

typedef struct {
    uint32_t    len;
    uint32_t    type;
    size_t      size;
    char        name[];
} dirent;

struct stat {
    // The type of the resource
    uint32_t    type;
    // The size of the resource
    size_t      size;
    // The last time the resource was modified in unix-time
    uint64_t    mtime;
    // The unix-time when the file was created
    uint64_t    ctime;
};

// Exit the current process, cleaning up all open system resources.
void noreturn exit(int code);

// Write to an open file descriptor, specifying a buffer with a size in bytes.
extern ssize_t write(uint32_t fd, const void* buf, size_t size);

// Read from an open file descriptor, specifying a buffer with a size in bytes.
extern ssize_t read(uint32_t fd, void* buf, size_t size);

// Open a file/directory specified by a zero-terminated (c-style) path string.
extern int32_t open(const char* path);

// Close an open file descriptor. Returns zero on success, and one on error.
extern int32_t close(uint32_t fd);

extern int32_t getdents(uint32_t fd, void* buf, size_t size);

// Creates a copy of the current process. Returns zero on the new process, and
// the pid of the new process on the original. 
extern int32_t fork();

// Creates a new process with the executable passed by the (null-terminated)
// path string. Returns zero on success and one on failure.
extern int32_t exec(const char* path);

// Returns the process id of the current process.
extern int32_t getpid();

extern int32_t stat(uint32_t fd, struct stat* statbuf);

extern int32_t brk(size_t size);

extern int32_t ioctl(uint32_t fd, int op, void* data);