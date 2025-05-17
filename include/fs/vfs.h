#pragma once

#include <types.h>
#include <fs/fs.h>

typedef struct _fs_mount fs_mount;

typedef struct _fs_mount {
    const char* name;

    void* internal;
    inode* source;
    inode* dest;

    inode *(*mount)(fs_mount* fs, inode* dev);
    int (*unmount)(fs_mount* fs);
    
    int (*mkdir)(fs_mount* fs, inode* parent, const char* name, int flags);
    int (*create)(fs_mount* fs, inode* parent, const char* name, int flags);
    int (*delete)(fs_mount* fs, inode* in, int flags);
    ssize_t (*read)(fs_mount* fs, inode* in, void* buf, uint32_t count, off_t* offset);
    ssize_t (*write)(fs_mount* fs, inode* in, void* buf, uint32_t count, off_t* offset);
} fs_mount;

typedef struct {
    uint32_t    len;
    uint32_t    type;
    size_t      size;
    char        name[];
} dirent;

typedef struct {
    // The type of the resource
    uint32_t    type;
    // The size of the resource
    size_t      size;
    // The last time the resource was modified in unix-time
    uint64_t    mtime;
    // The unix-time when the file was created
    uint64_t    ctime;
} stat;

// Initialize the virtual filesystem
int vfs_init();

// Get the current root inode
inode* get_root();

// Register a new filesystem type
int register_fs_type(fs_mount* m);

// Get the node at a specified path based on a root inode
inode* node_at(inode* root, const char* path, size_t pathlen);

// Update in to be a directory with a specified parent and name
void generate_directory(inode *parent, inode *in, const char* name);

// Update a parent inode to include a child inode
int add_child(inode *parent, inode *child);

// Create a node at the specified path, with a type and resource
int mknod(const char* path, const char* name, inode_type type, void* resource);

// Write to an inode to a buffer, returning the number of bytes written
ssize_t write(inode* node, void* buf, size_t count, off_t offset);

// Read an inode to a buffer, returning the number of bytes read
ssize_t read(inode* node, void* buf, size_t count, off_t offset);

// Mount a filesystem to a specified directory inode, specifying a source
// inode and a filesystem type
int mount(inode* source, inode* dest, const char *type);

// // Create a directory at the path, with flags
// int mkdir(const char *path, int flags);
// // Create a file at the path, with flags
// int create(const char *path, int flags);

// // Send an io control signal to a device
// int ioctl(const char *path, int op, void *data);

// typedef struct {
//     const char* name;
//     inode_type type;
// } dentry;

// // Get a directory entry, returning the total number of entries (0 means there
// // are no children, and entry was not written to)
// uint32_t getdents(const char* path, dentry* entry, uint32_t index);