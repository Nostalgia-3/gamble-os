#include <gosh/gosh.h>
#include <drivers/x86/vga.h>
#include <types.h>
#include <memory.h>
#include <str.h>

#define MAX_DESCRIPTORS 128
static inode_t* open_descriptors[MAX_DESCRIPTORS];

static inode_t *inodes;
static u32 inode_count;

directory_t *create_directory() {
    directory_t *d = k_malloc(sizeof(directory_t));
    memset(d, 0, sizeof(directory_t));
    d->children = k_malloc(sizeof(inode_t*)*256);
    memset(d->children, 0, sizeof(inode_t*)*256);
    d->children_count = 256;
    return d;
}

int _init_vfs() {
    // The virtual filesystem is just an array of inodes that link
    // to eachother; the first inode (inode #0) is /, and is a directory

    inode_count = 256;
    inodes = k_malloc(sizeof(inode_t)*inode_count);
    if(inodes == NULL) return -1;
    memset(inodes, 0, sizeof(inode_t)*inode_count);

    directory_t* rootdir    = create_directory();
    directory_t* devdir     = create_directory();

    inodes[0].name      = "/";
    inodes[0].type      = DT_DIR;
    inodes[0].resource  = rootdir;
    inodes[0].in_use    = true;
    inodes[1].name      = "dev";
    inodes[1].type      = DT_DIR;
    inodes[1].resource  = devdir;
    inodes[1].in_use    = true;

    rootdir->children[0] = &inodes[1];

    return 0;
}

inode_t* get_free_inode() {
    for(int i=0;i<inode_count;i++) {
        if(!inodes[i].in_use) return &inodes[i];
    }
}

int mknod(const char *pathname, dt_t type, void *resource) {
    if(strlen((char*)pathname) == 0) {
        kprintf("");
        return -1;
    }

    inode_t *inode;

    if(pathname[0] == '/') {
        // kprintf("absolute path\n");
        inode = &inodes[0];
    } else {
        // kprintf("relative path\n");
        // TODO: use the processes curdir once processes are setup
        inode = &inodes[0];
    }

    char cpath[128] = {0};
    memset(cpath, 0, sizeof(cpath));
    int cpath_ind = 0;

    for(int i=0;i<strlen((char*)pathname);i++) {
        switch(pathname[i]) {
            case '/':
                if(i != 0) {
                    if(inode == NULL || inode->type != DT_DIR) {
                        // kprintf("inode is either null or not a directory\n");
                        return -1;
                    }

                    directory_t *dir = ((directory_t*)inode->resource);
                    if(dir->children_count == 0) {
                        // kprintf("Directory \"%s\" has no children slots!\n", inode->name);
                        return -1;
                    }

                    bool found = false;
                    for(int x=0;x<dir->children_count;x++) {
                        if(dir->children[x] != NULL) {
                            if(strcmp((char*)dir->children[x]->name, cpath) == 0) {
                                found = true;
                                inode = dir->children[x];
                                memset(cpath, 0, cpath_ind);
                                cpath_ind = 0;
                                break;
                            }
                        }
                    }

                    if(!found) {
                        // kprintf("Failed to find inode with name \"%s\"\n", cpath);
                        return -1;
                    }
                }
            break;

            default:
                cpath[cpath_ind++] = pathname[i];
            break;
        }
    }

    if(strlen(cpath) == 0) {
        // was given a directory instead of a file
        // kprintf("cannot create a directory node \"%s\"\n", pathname);
        return -1;
    }

    if(inode == NULL || inode->type != DT_DIR) {
        // kprintf("inode is either null or not a directory\n");
        return -1;
    }

    directory_t *dir = ((directory_t*)inode->resource);
    if(dir->children_count == 0) {
        // kprintf("Directory \"%s\" doesn't have any free children!\n", inode->name);
        return -1;
    }

    for(int x=0;x<dir->children_count;x++) {
        if(dir->children[x] != NULL) {
            if(strcmp((char*)dir->children[x]->name, cpath) == 0) {
                // kprintf("Node \"%s\" already exists!", cpath);
                return -1;
            }
        }
    }

    
    for(int x=0;x<dir->children_count;x++) {
        if(dir->children[x] == NULL) {
            inode_t *node = get_free_inode();
            node->in_use = true;
            node->name = get_last_del(pathname, '/');
            node->type = type;
            node->resource = resource;
            dir->children[x] = node;
            return 0;
        }
    }

    return -1;
}

/// @brief Open a file/device/pipe
/// @param path The path to the file/device/pipe
/// @return returning the file descriptor (or -1 if not found)
fd_t open(const char *path) {
    // invalid path
    if(strlen((char*)path) == 0) return -1;

    inode_t *inode;

    if(path[0] == '/') {
        inode = &inodes[0];
    } else {
        // TODO: use the processes curdir once processes are setup
        inode = &inodes[0];
    }

    char cpath[128] = {0};
    memset(cpath, 0, sizeof(cpath));
    int cpath_ind = 0;

    for(int i=0;i<strlen((char*)path);i++) {
        switch(path[i]) {
            case '/':
                if(i != 0) {
                    if(inode == NULL || inode->type != DT_DIR) {
                        kprintf("inode is either null or not a directory\n");
                        return -1;
                    }

                    directory_t *dir = ((directory_t*)inode->resource);
                    if(dir->children_count == 0) {
                        return -1;
                    }

                    bool found = false;
                    for(int x=0;x<dir->children_count;x++) {
                        if(dir->children[x] != NULL) {
                            if(strcmp((char*)dir->children[x]->name, cpath) == 0) {
                                found = true;
                                inode = dir->children[x];
                                memset(cpath, 0, cpath_ind);
                                cpath_ind = 0;
                                break;
                            }
                        }
                    }

                    if(!found) {
                        kprintf("Failed to find inode with name \"%s\"\n", cpath);
                        return -1;
                    }
                }
            break;

            default:
                cpath[cpath_ind++] = path[i];
            break;
        }
    }

    if(strlen(cpath)) {
        if(inode == NULL || inode->type != DT_DIR) {
            kprintf("inode is either null or not a directory\n");
            return -1;
        }

        directory_t *dir = ((directory_t*)inode->resource);
        if(dir->children_count == 0) {
            return -1;
        }

        bool found = false;
        for(int x=0;x<dir->children_count;x++) {
            if(dir->children[x] != NULL) {
                if(strcmp((char*)dir->children[x]->name, cpath) == 0) {
                    found = true;
                    inode = dir->children[x];
                    memset(cpath, 0, cpath_ind);
                    cpath_ind = 0;

                    for(int i=0;i<MAX_DESCRIPTORS;i++) {
                        if(open_descriptors[i] != NULL) continue;
                        open_descriptors[i] = inode;
                        return i;
                    }
                }
            }
        }

        return -1;
    } else {
        for(int i=0;i<MAX_DESCRIPTORS;i++) {
            if(open_descriptors[i] != NULL) continue;
            open_descriptors[i] = inode;
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