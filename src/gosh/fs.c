#include <gosh/gosh.h>
#include <drivers/x86/vga.h>
#include <types.h>
#include <memory.h>
#include <inode.h>
#include <str.h>

#define MAX_DESCRIPTORS 128
static inode_t* open_descriptors[MAX_DESCRIPTORS];

static inode_t *root_inode;
static inode_t *inodes;
static u32 inode_count;

int _init_vfs() {
    // The virtual filesystem is just an array of inodes that link
    // to eachother; the first inode (inode #0) is /, and is a directory

    inode_count = 256;
    inodes = k_malloc(sizeof(inode_t)*inode_count);
    if(inodes == NULL) return -1;
    memset(inodes, 0, sizeof(inode_t)*inode_count);

    inode_t* rootdir    = setup_directory(get_free_inode(inodes, inode_count), "/");
    rootdir->parent     = NULL;

    root_inode = rootdir;

    inode_t* devdir     = setup_directory(get_free_inode(inodes, inode_count), "dev");
    inode_t* infodir    = setup_directory(get_free_inode(inodes, inode_count), "info");
    inode_t* initrd     = setup_directory(get_free_inode(inodes, inode_count), "initrd");

    if(add_inode_child(rootdir, devdir) < 0) {
        kprintf("/!\\ Failed creating /dev/\n");
        return -1;
    }
    
    if(add_inode_child(rootdir, infodir) < 0) {
        kprintf("/!\\ Failed creating /info/\n");
        return -1;
    }

    if(add_inode_child(rootdir, initrd) < 0) {
        kprintf("/!\\ Failed creating /initrd/\n");
        return -1;
    }

    return 0;
}


int mknod(const char* name, const char *path, dt_t type, void *resource) {
    inode_t *node = get_inode_from_path(root_inode, path);

    inode_t *child = get_free_inode(inodes, inode_count);
    setup_node(child, name, type, resource);


    if(add_inode_child(node, child) < 0) {
        kprintf("Failed adding node \"%s\" to node %s\n", child->name, path);
    }

    return 0;
}

/// @brief Open a file/device/pipe
/// @param path The path to the file/device/pipe
/// @return returning the file descriptor (or -1 if not found)
fd_t open(const char *path) {
    inode_t *node = get_inode_from_path(root_inode, path);

    if(node == NULL) return -1;

    for(int i=0;i<MAX_DESCRIPTORS;i++) {
        if(open_descriptors[i] == NULL) {
            open_descriptors[i] = node;
            return i;
        }
    }

    return -1;
}

// Close a file based on the file descriptor
void close(fd_t fd) {
    if(fd >= MAX_DESCRIPTORS) return;
    if(open_descriptors[fd] != NULL) {
        open_descriptors[fd] = NULL;
    }
}

// Write a buffer to a system port (similar to a file descriptor in linux)
#include <str.h>
ssize_t write(fd_t fd, void *buf, size_t len) {
    if(fd == DBGOUT) {
        return vga_write(buf, len, 0);
    }

    if(fd > MAX_DESCRIPTORS) return -1;
    if(open_descriptors[fd] == NULL) {
        return -1;
    }

    switch(open_descriptors[fd]->type) {
        case DT_DEV: {
            device_t *d = (device_t*)open_descriptors[fd]->resource;
            if(d->write == NULL) return 0;
            return d->write(buf, len, &d->off);
        break; }
    }
}

// Read data from a port to a buffer of len; returns the number of bytes actually read
ssize_t read(fd_t fd, void *buf, size_t len) {
    if(fd >= MAX_DESCRIPTORS) return -1;
    if(open_descriptors[fd] == NULL) return -1;

    switch(open_descriptors[fd]->type) {
        case DT_DEV: {
            device_t *d = open_descriptors[fd]->resource;
            if(d->read == NULL) return 0;
            return d->read(buf, len, &d->off);
        break; }

        default: {
            // can't read these types of inodes :guh:
            return -1;
        break; }
    }
}

ssize_t pseek(fd_t fd, ssize_t offset, int whence) {
    return 0;
}

/// @brief Read count bytes from fd at offset to buf
ssize_t pread(fd_t fd, void *buf, size_t count, off_t offset) {
    return 0;
}

// Write count bytes to fd at offset from buf
ssize_t pwrite(fd_t fd, void *buf, size_t count, off_t offset) {
    return 0;
}

// Manipulate an open file descriptor
int ioctl(int fd, int op, void *data) {
    if(fd >= MAX_DESCRIPTORS) return -1;
    
    // descriptor doesn't exist
    if(open_descriptors[fd] == NULL) return -1;

    if(open_descriptors[fd]->type == DT_DEV) {
        device_t* dev = (device_t*)open_descriptors[fd]->resource;
        if(dev->ioctl) dev->ioctl(fd, op, data);
    }

    return 0;
}

int getdents(unsigned int fd, dirent* dirp, unsigned int index) {
    if(
        fd >= MAX_DESCRIPTORS ||
        open_descriptors[fd] == NULL ||
        open_descriptors[fd]->type != DT_DIR
    ) return -1;

    directory_t *dir = (directory_t *)open_descriptors[fd]->resource;

    bool set = true;

    unsigned int tc = 0;
    for(int i=0;i<dir->children_count;i++) {
        if(dir->children[i] != NULL && dir->children[i]->in_use) {
            if(set == true && tc == (index)) {
                dirp->d_name = dir->children[i]->name;
                dirp->d_type = dir->children[i]->type;
                set = false;
            }

            tc++;
        }
    }

    return tc;
}