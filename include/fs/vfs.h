#pragma once

#include <types.h>
#include <fs/fs.h>

typedef struct _fs_mount fs_mount;

typedef struct _fs_mount {
    const char *name;

    void *internal;

    inode *(*mount)(fs_mount* fs, inode* dev);
    int (*unmount)(fs_mount* fs);
    
    int (*mkdir)(fs_mount *fs, inode* parent, const char* name, int flags);
    int (*create)(fs_mount *fs, inode* parent, const char* name, int flags);
    int (*delete)(fs_mount *fs, inode* in, int flags);
    ssize_t (*read)(fs_mount *fs, inode* in, void* buf, uint32_t count, off_t* offset);
    ssize_t (*write)(fs_mount *fs, inode* in, void* buf, uint32_t count, off_t* offset);
} fs_mount;

// Initialize the virtual filesystem
int vfs_init();

int register_fs_type(fs_mount* m);

inode* node_at(inode* root, const char* path, int pathlen);
void generate_directory(inode *parent, inode *in, const char* name, uint32_t child_count);
int add_child(inode *parent, inode *child);

inode* get_root();

// Mount a source device (or NULL, depending on the filesystem type) to
// destination
int mount(inode* source, inode* dest, const char *type);

// Create a directory at the path, with flags
int mkdir(const char *path, int flags);
// Create a file at the path, with flags
int create(const char *path, int flags);

// Read a file at the path specified
ssize_t read(inode* node, void* buf, uint32_t count, off_t offset);
// ssize_t read(const char* path, void* buf, uint32_t count, off_t offset);

// Write a file at the path specified
ssize_t write(inode* node, void* buf, uint32_t count, off_t offset);
// ssize_t write(const char *path, void* buf, uint32_t count, off_t offset);

// Send an io control signal to a device
int ioctl(const char *path, int op, void *data);

// Create a node at the specified path
int mknod(const char* path, inode_type type, void *resource);

typedef struct {
    const char* name;
    inode_type type;
} dentry;

// Get a directory entry, returning the total number of entries (0 means there
// are no children, and entry was not written to)
uint32_t getdents(const char* path, dentry* entry, uint32_t index);