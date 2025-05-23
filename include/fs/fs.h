#pragma once

#include <types.h>
#include <device.h>

typedef enum {
    INODE_FILE,
    INODE_DEV,
    INODE_DIR,
    INODE_LINK
} inode_type;

typedef struct _fs_mount fs_mount;
typedef struct _inode inode;

#include <fs/vfs.h>

typedef struct {
    uint32_t    is_mounted;
    inode*      mount;
    fs_mount*   mount_fs;
    inode**     children;
    uint32_t    children_count;
} dir_resource;

typedef struct {
    fs_mount *fs;
    size_t filesize;
    uint64_t creation;
    uint64_t last_modified;
    void* _internal;
} file_resource;

typedef struct _inode {
    const char *name;
    bool used;

    inode *parent;
    inode_type type;

    // A value that determines the number of times this file is open
    // TODO: change this to a dynamic array pointing to resources that have this
    // inode open
    size_t open_count;

    union {
        file_resource file;
        dir_resource dir;
        device* dev;
    } resource;
} inode;