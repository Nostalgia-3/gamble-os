#pragma once

#include <types.h>

typedef enum {
    INODE_FILE,
    INODE_DEV,
    INODE_DIR,
    INODE_LINK
} inode_type;

// // A symbolic link to an inode; this number is unique only to specific filesystems
// typedef int32_t inode_t;

typedef struct _fs_mount fs_mount;
typedef struct _inode inode;

typedef struct _inode {
    const char *name;
    bool used;

    inode *parent;
    inode_type type;
    void *resource;
} inode;

typedef struct _inode_dir {
    bool        is_mounted;
    inode*      mount;
    fs_mount*   mount_fs;

    inode **children;
    uint32_t children_count;
} inode_dir;

typedef struct _inode_file {
    fs_mount *fs;
    size_t filesize;
    uint64_t creation;
    uint64_t last_modified;
    void* _internal;
} inode_file;