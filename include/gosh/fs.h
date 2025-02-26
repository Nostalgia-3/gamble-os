#ifndef GOSH_FS_H
#define GOSH_FS_H

#include <types.h>
#include <gosh/common.h>

#define DBGOUT   0x8000
#define STDIN    1

typedef enum _dt {
    // A directory (directory_t*)
    DT_DIR,
    // A regular file (file_t*)
    DT_FILE,
    // A device (device_t*)
    DT_DEV,
    // An unknown file (void*)
    DT_UNKNOWN
} dt_t;

typedef struct _inode_t inode_t;
typedef struct _fs_type fs_type_t;

typedef struct _fs_type {
    const char *name;
    
    inode_t *(*mount)(fs_type_t* fs_type, const char *dev_name);
    void (*unmount)(fs_type_t* fs_type, const char *dev_name);
    ssize_t (*read)(inode_t* inode, void *buf, size_t size, size_t offset);
    ssize_t (*write)(inode_t* path, const void *buf, size_t size, size_t offset);
    
    // Data for use for the filesystem
    void *internal;
} fs_type_t;

typedef struct _inode_t {
    inode_t *parent;
    // The name of the inode
    const char *name;
    // The type of the resource
    dt_t type;
    bool in_use;
    void *resource;
    // Add metadata
} inode_t;

typedef struct _directory_t {
    bool is_mounted;
    inode_t *mount;

    inode_t **children;
    uint32_t children_count;
} directory_t;

typedef struct _file_t {
    fs_type_t *fs;
} file_t;

typedef struct _dirent {
    const char *d_name;
    dt_t d_type;
} dirent;

// Initialize the virtual filesystem
int _init_vfs();

// Close a file based on the file descriptor
void close(fd_t fd);

int mknod(const char* name, const char *path, dt_t type, void *resource);

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

// Read a singular directory entry, returning the total amount
// of directory entries within the directory
int getdents(unsigned int fd, dirent* dirp, unsigned int index);

// Register a filesystem for use when mounting
int register_fs(fs_type_t *fs);

// Mount a drive to dest
int mount(const char *src, const char *dest, const char *fs_type);

#endif//GOSH_FS_H