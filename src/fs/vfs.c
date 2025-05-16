#include <fs/fs.h>
#include <fs/vfs.h>

#include <memory.h>
#include <str.h>

static inode* root;

static fs_mount* filesystems[64];

static chunk* inodes = NULL;
static size_t inode_count = 0;

int vfs_init() {
    // Allocate 32 inodes for the time being
    inode_count = 32;
    inodes = (chunk*)alloc_chunks(inode_count, 0);

    if(inodes == NULL) {
        printf("Failed to allocate memory for VFS inodes");
        return -1;
    }

    // Setup the directory structure
    root = (inode*)&inodes[0];

    generate_directory(NULL, (inode*)&inodes[0], "/");
    generate_directory(NULL, (inode*)&inodes[1], "dev");
    generate_directory(NULL, (inode*)&inodes[2], "initrd");

    add_child((inode*)&inodes[0], (inode*)&inodes[1]);
    add_child((inode*)&inodes[0], (inode*)&inodes[2]);

    return 0;
}

inode *node_at(inode* root, const char* path, size_t pathlen) {
    if(strlen((char*)path) == 0) {
        return NULL;
    }

    inode* inode = root;

    char cpath[128] = {0};
    memset(cpath, 0, sizeof(cpath));
    int cpath_ind = 0;

    for(int i=0;i<pathlen;i++) {
        switch(path[i]) {
            case '/':
                if(i == 0) break;
                
                if(inode == NULL) {
                    printf("inode is null");
                    return NULL;
                } else if(inode->type != INODE_DIR) {
                    printf("inode isn't a directory (addr = %08X, resource %08X)", inode->parent, inode->resource);
                    return NULL;
                }

                // inode_dir* dir = (inode_dir*)inode->resource;

                if(inode->resource.dir.children_count == 0) {
                    printf("Directory \"%s\" has no children slots!", inode->name);
                    return NULL;
                }

                bool found = false;
                for(int x=0;x<inode->resource.dir.children_count;x++) {
                    if(inode->resource.dir.children[x] != NULL) {
                        if(strcmp((char*)inode->resource.dir.children[x]->name, cpath) == 0) {
                            found = true;
                            inode = inode->resource.dir.children[x];
                            memset(cpath, 0, cpath_ind);
                            cpath_ind = 0;
                            if(
                                inode != NULL &&
                                inode->type == INODE_DIR &&
                                inode->resource.dir.is_mounted
                            ) {
                                inode = inode->resource.dir.mount;
                            }
                            break;
                        }
                    }
                }

                if(!found) {
                    printf("Failed to find inode with name \"%s\"", cpath);
                    return NULL;
                }
            break;

            default:
                cpath[cpath_ind++] = path[i];
            break;
        }
    }

    if(strlen(cpath)) {
        if(inode == NULL || inode->type != INODE_DIR) {
            printf("inode is either null or not a directory");
            return NULL;
        }

        dir_resource *dir = &inode->resource.dir;

        if(dir->children_count == 0) {
            return NULL;
        }

        for(int x=0;x<dir->children_count;x++) {
            if(dir->children[x] != NULL) {
                // printf("%s, %s", dir->children[x]->name, cpath);
                // printf("cpath = %s, name = %s, cwd = %s", cpath, dir->children[x]->name, inode->name);
                if(strcmp((char*)dir->children[x]->name, cpath) == 0) {
                    return dir->children[x];
                }
            }
        }

        return NULL;
    }

    return inode;
}

inode* get_root() {
    return root;
}

inode* get_free_vfs_inode() {
    for(int i=0;i<inode_count;i++) {
        if(inodes[i].inode.used == 0) return (inode*)&inodes[i];
    }
    return NULL;
}

int register_fs_type(fs_mount* m) {
    for(int i=0;i<(sizeof(filesystems) / sizeof(fs_mount*));i++) {
        if(filesystems[i] != NULL) continue;

        filesystems[i] = m;
        return 0;
    }

    kpanic("Ran out of space for filesystems (failed fs name = %s)?", m->name);
    return -1;
}

void generate_directory(inode *parent, inode *in, const char* name) {
    in->name = name;
    in->parent = parent;
    in->type = INODE_DIR;
    in->used = true;
    
    in->resource.dir = (dir_resource) {
        .is_mounted = 0,
        .mount      = NULL,
        .mount_fs   = NULL,

        .children = (inode**) alloc_chunks(1, 0),
        .children_count = CHUNK_SIZE / sizeof(inode*)
    };
}

int add_child(inode *parent, inode *child) {
    if(parent == NULL || child == NULL) return -1;
    if(parent->type != INODE_DIR) return -1;
    
    dir_resource* dir = &parent->resource.dir;

    // inode_dir *dir = parent->resource;
    if(dir->is_mounted) {
        return add_child(dir->mount, child);
    }

    for(uint32_t i=0;i<=dir->children_count;i++) {
        if(dir->children[i] == NULL) {
            dir->children[i] = child;
            return 0;
        }
    }

    // It's safe to assume the chunks the directory has isn't enough, so we
    // reallocate the chunks, copying over the previous data, before freeing the
    // old array.

    size_t old_chunk_count = (dir->children_count * sizeof(inode*)) / CHUNK_SIZE;
    size_t new_chunk_count = old_chunk_count + 1;

    void* new_children = alloc_chunks(new_chunk_count, 0);

    memcpy(new_children, dir->children, dir->children_count * sizeof(inode*));

    free_chunks((chunk*)dir->children, old_chunk_count);

    dir->children = new_children;
    dir->children_count = (new_chunk_count * CHUNK_SIZE) / sizeof(inode*);

    add_child(parent, child);

    return -1;
}

int mknod(const char* path, const char* name, inode_type type, void *resource) {
    int s = strlen((char*)path);

    inode* node = node_at(root, path, s);
    if(node == NULL || node->type != INODE_DIR) return -1;

    inode* child = get_free_vfs_inode();
    if(child == NULL) return -1;

    child->used     = true;
    child->name     = name;
    child->parent   = node;
    child->type     = type;

    switch(type) {
        case INODE_DIR: child->resource.dir = *(dir_resource*)resource; break;
        case INODE_DEV: child->resource.dev = (device*)resource; break;
        case INODE_FILE: child->resource.file = *(file_resource*)resource; break;
        case INODE_LINK: kpanic("link inodes aren't implemented yet"); break;
    }

    return add_child(node, child);
}

ssize_t write(inode* node, void* buf, size_t count, off_t offset) {
    if(node == NULL || node->type == INODE_DIR) return -1;

    if(node->type == INODE_DEV) {
        device* dev = node->resource.dev;
        if(dev->write == NULL) return -1;
        return dev->write(buf, count, &offset);
    } else if(node->type == INODE_FILE) {
        if(node->resource.file.fs == NULL) kpanic("file with name '%s' doesn't have an associated mount?", node->name);
        if(node->resource.file.fs->write == NULL) return -1;
        return node->resource.file.fs->write(node->resource.file.fs, node, buf, count, &offset);
    }

    return -1;
}

ssize_t read(inode* node, void* buf, size_t count, off_t offset) {
    if(node == NULL || node->type == INODE_DIR) return -1;

    if(node->type == INODE_DEV) {
        device* dev = node->resource.dev;
        if(dev->read == NULL) return -1;
        return dev->read(buf, count, &offset);
    } else if(node->type == INODE_FILE) {
        if(node->resource.file.fs == NULL) kpanic("file with name '%s' doesn't have an associated mount?", node->name);
        if(node->resource.file.fs->read == NULL) return -1;
        return node->resource.file.fs->read(node->resource.file.fs, node, buf, count, &offset);
    }

    return -1;
}

int mount(inode* source, inode* dest, const char *type) {
    if(dest == NULL || dest->type != INODE_DIR) return -1;
    
    for(int i=0;i<(sizeof(filesystems) / sizeof(inode*));i++) {
        if(filesystems[i] != NULL && strcmp((char*)type, (char*)filesystems[i]->name) == 0) {
            if(filesystems[i]->mount == NULL) return -1;
            
            fs_mount* mount = (fs_mount*)alloc_chunks(1, 0);
            memcpy(mount, filesystems[i], sizeof(fs_mount));

            mount->source = source;
            mount->dest = dest;

            dest->resource.dir.mount      = mount->mount(mount, source);
            dest->resource.dir.mount_fs   = mount;
            dest->resource.dir.is_mounted = true;

            if(dest->resource.dir.mount == NULL) return -1;
            return 0;
        }
    }
    return -1;
}

// int mkdir(const char* path, int flags) {
//     int s = strlen((char*)path);
//     int i = 0;

//     for(i=s;i>0;i--) {
//         if(path[i] == '/') { i++; break; }
//     }

//     inode* node = node_at(root, path, i);
//     if(node == NULL || node->type != INODE_DIR) return -1;

//     if(node->resource.dir.is_mounted) {
//         if(node->resource.dir.mount_fs == NULL) return -1;
//         if(node->resource.dir.mount_fs->mkdir == NULL) return -1;
//         return node->resource.dir.mount_fs->mkdir(node->resource.dir.mount_fs, node, path+i, flags);
//     } else {
//         inode* child = get_free_vfs_inode();
//         if(child == NULL) return -1;

//         generate_directory(node, child, path+i);

//         return add_child(node, child);
//     }

// }

// int create(const char* path, int flags) {
//     int s = strlen((char*)path);
//     int i = 0;

//     for(i=s;i>0;i--) {
//         if(path[i] == '/') { i++; break; }
//     }

//     inode* node = node_at(root, path, i);
//     if(node == NULL || node->type != INODE_DIR) return -1;

//     if(node->resource.dir.is_mounted) {
//         if(node->resource.dir.mount_fs == NULL) return -1;
//         if(node->resource.dir.mount_fs->mkdir == NULL) return -1;
//         return node->resource.dir.mount_fs->create(node->resource.dir.mount_fs, node, path+i, flags);
//     } else {
//         // The VFS can't contain files
//         return -1;
//     }
// }

// int ioctl(const char *path, int op, void *data) {
//     inode* node = node_at(root, path, strlen((char*)path));
//     if(node == NULL || node->type == INODE_DIR) return -1;

//     if(node->type == INODE_DEV) {
//         device* dev = node->resource.dev;
//         if(dev->ioctl == NULL) return -1;
//         return dev->ioctl(op, data);
//     } else if(node->type == INODE_FILE) {
//         return -1;
//     }

//     return -1;
// }

// uint32_t getdents(const char* path, dentry* entry, uint32_t index) {
//     int s = strlen((char*)path);
//     int i = 0;

//     for(i=s;i>0;i--) {
//         if(path[i] == '/') { i++; break; }
//     }

//     inode* node = node_at(root, path, i);

//     if(node == NULL || node->type != INODE_DIR) return 0;

//     dir_resource* dir = &node->resource.dir;

//     uint32_t ac_children = 0;

//     for(int i=0;i<dir->children_count;i++) {
//         if(dir->children[i] != NULL) {
//             ac_children++;
//             if(ac_children == index) {
//                 entry->name = dir->children[i]->name;
//                 entry->type = dir->children[i]->type;
//             }
//         }
//     }

//     return ac_children;
// }