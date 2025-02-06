#ifndef GOSH_FS_H
#define GOSH_FS_H

#include <types.h>
#include <gosh/common.h>

// linked to the 80x25 vga text driver
#define DBGOUT   0x8000
// linked to the i8042 keyboard driver
#define STDIN    1

typedef enum _dt {
    // A directory
    DT_DIR,
    // A regular file
    DT_REG,
    // A device
    DT_DEV,
    // An unknown file
    DT_UNKNOWN
} dt_t;

typedef struct _file_t {
    // TODO: Fill in information about the file
} file_t;

typedef struct _inode_t {
    // The name of the inode
    const char *name;
    // The type of the resource
    dt_t type;
    bool in_use;
    // The resource can be a directory_t, device_t, or a file_t
    void *resource;
    // Add metadata
} inode_t;

typedef struct _directory_t {
    inode_t **children;
    uint32_t children_count;
    // Add metadata
} directory_t;

typedef struct _dirent {
    const char *d_name;
    char d_type;
} dirent;

// Initialize the virtual filesystem
int _init_vfs();

// Close a file based on the file descriptor
void close(fd_t fd);

int mknod(const char *pathname, dt_t type, void *resource);

/// Open an inode returning the file descriptor, or -1 if not found
int open(const char *pathname);

// Write a buffer to a system port (similar to a file descriptor in linux)
ssize_t write(fd_t fd, void *buf, size_t len);

// Read data from a port to a buffer of len; returns the number of bytes actually read
ssize_t read(fd_t port, void *buf, size_t len);

ssize_t pseek(fd_t fd, ssize_t offset, int whence);

/// @brief Read count bytes from fd at offset to buf
ssize_t pread(fd_t fd, void *buf, size_t count, off_t offset);

// Write count bytes to fd at offset from buf
ssize_t pwrite(fd_t fd, void *buf, size_t count, off_t offset);

// Manipulate an open file descriptor
int ioctl(int fd, int op, void *data);

// Read a number of directory entries from the directory fd passed; works
// similarly to getdents(2) on unix-based operating systems. Count is the number
// of of dirents are within the passed directory
int getdents(unsigned int fd, dirent* dirp, unsigned int count);

// Mount a drive to dest
int mount(const char *src, const char *dest);

#endif//GOSH_FS_H