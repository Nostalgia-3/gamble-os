#include <fs/vfs.h>
#include <fs/fs.h>

#include <memory.h>
#include <device.h>
#include <str.h>

#include <printf.h>

#define MAX_FS_TYPES 64

static inode *root;

static inode *inodes;
static uint32_t inode_count;

static fs_mount** registered_fs_types;

void generate_directory(inode *parent, inode *in, const char* name, uint32_t child_count) {
    inode_dir* dir = kmalloc(sizeof(inode_dir), 0);
    memset(dir, 0, sizeof(dir));

    dir->children_count = child_count;
    dir->children = kmalloc(sizeof(inode*)*dir->children_count, 0);
    memset(dir->children, 0, sizeof(inode*)*dir->children_count);

    in->name = name;
    in->parent = parent;
    in->resource = dir;
    in->type = INODE_DIR;
    in->used = true;
}

int add_child(inode *parent, inode *child) {
    if(parent == NULL || child == NULL) return -1;
    if(parent->type != INODE_DIR) return -1;
    
    inode_dir *dir = parent->resource;
    if(dir->is_mounted) {
        return add_child(dir->mount, child);
    }

    for(uint32_t i=0;i<=dir->children_count;i++) {
        if(dir->children[i] == NULL) {
            dir->children[i] = child;
            return 0;
        }
    }

    return -1;
}

int vfs_init() {
    inode_count = 128;
    inodes = kmalloc(sizeof(inode) * inode_count, 0);
    if(inodes == NULL) return -1;

    registered_fs_types = kmalloc(sizeof(fs_mount) * MAX_FS_TYPES, 0);
    if(registered_fs_types == NULL) return -1;

    // make a very temporary place for /dev/ to do
    root = &inodes[0];
    generate_directory(NULL, &inodes[0], "/", 32);
    generate_directory(&inodes[0], &inodes[1], "dev", 64);
    generate_directory(&inodes[0], &inodes[2], "initrd", 4);

    add_child(root, &inodes[1]);
    add_child(root, &inodes[2]);

    return 0;
}

int register_fs_type(fs_mount* m) {
    if(m == NULL) return -1;

    for(int i=0;i<MAX_FS_TYPES;i++) {
        if(registered_fs_types[i] == NULL) {
            registered_fs_types[i] = m;
            return 0;
        }
    }

    return -1;
}

int mount(inode* source, inode* dest, const char *type) {
    if(dest == NULL || dest->type != INODE_DIR) return -1;
    
    inode_dir* dir = dest->resource;

    for(int i=0;i<MAX_FS_TYPES;i++) {
        if(registered_fs_types[i] != NULL && strcmp((char*)type, (char*)registered_fs_types[i]->name) == 0) {
            if(registered_fs_types[i]->mount == NULL) return -1;
            fs_mount* mount = kmalloc(sizeof(fs_mount), 0);
            memcpy(mount, registered_fs_types[i], sizeof(fs_mount));

            dir->mount      = mount->mount(mount, source);
            dir->mount_fs   = mount;
            dir->is_mounted = true;

            if(dir->mount == NULL) return -1;
            return 0;
        }
    }
    return -1;
}

inode* get_free_vfs_inode() {
    for(int i=0;i<inode_count;i++) {
        if(inodes[i].used == 0) return &inodes[i];
    }
    return NULL;
}

inode* get_root() {
    return root;
}

inode *node_at(inode* root, const char* path, int pathlen) {
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
                if(i != 0) {
                    if(inode == NULL || inode->type != INODE_DIR) {
                        printf("inode is either null or not a directory");
                        return NULL;
                    }

                    inode_dir* dir = (inode_dir*)inode->resource;

                    if(dir->children_count == 0) {
                        printf("Directory \"%s\" has no children slots!", inode->name);
                        return NULL;
                    }

                    bool found = false;
                    for(int x=0;x<dir->children_count;x++) {
                        if(dir->children[x] != NULL) {
                            if(strcmp((char*)dir->children[x]->name, cpath) == 0) {
                                found = true;
                                inode = dir->children[x];
                                memset(cpath, 0, cpath_ind);
                                cpath_ind = 0;
                                if(
                                    inode != NULL &&
                                    inode->type == INODE_DIR &&
                                    ((inode_dir*)inode->resource)->is_mounted
                                ) {
                                    inode = ((inode_dir*)inode->resource)->mount;
                                }
                                break;
                            }
                        }
                    }

                    if(!found) {
                        printf("Failed to find inode with name \"%s\"", cpath);
                        return NULL;
                    }
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

        inode_dir *dir = ((inode_dir*)inode->resource);

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

int mkdir(const char* path, int flags) {
    int s = strlen((char*)path);
    int i = 0;

    for(i=s;i>0;i--) {
        if(path[i] == '/') { i++; break; }
    }

    inode* node = node_at(root, path, i);
    if(node == NULL || node->type != INODE_DIR) return -1;

    inode_dir* dir = node->resource;

    if(dir->is_mounted) {
        if(dir->mount_fs == NULL) return -1;
        if(dir->mount_fs->mkdir == NULL) return -1;
        return dir->mount_fs->mkdir(dir->mount_fs, node, path+i, flags);
    } else {
        inode* child = get_free_vfs_inode();
        if(child == NULL) return -1;

        generate_directory(node, child, path+i, 64);

        return add_child(node, child);
    }

}

int create(const char* path, int flags) {
    int s = strlen((char*)path);
    int i = 0;

    for(i=s;i>0;i--) {
        if(path[i] == '/') { i++; break; }
    }

    inode* node = node_at(root, path, i);
    if(node == NULL || node->type != INODE_DIR) return -1;

    inode_dir* dir = node->resource;

    if(dir->is_mounted) {
        if(dir->mount_fs == NULL) return -1;
        if(dir->mount_fs->mkdir == NULL) return -1;
        return dir->mount_fs->create(dir->mount_fs, node, path+i, flags);
    } else {
        // The VFS can't contain files
        return -1;
    }
}

ssize_t read(const char* path, void* buf, uint32_t count, off_t offset) {
    inode* node = node_at(root, path, strlen((char*)path));
    if(node == NULL || node->type == INODE_DIR) return -1;

    if(node->type == INODE_DEV) {
        device* dev = node->resource;
        if(dev->read == NULL) return -1;
        return dev->read(buf, count, &offset);
    } else if(node->type == INODE_FILE) {
        inode_file* file = node->resource;
        if(file->fs == NULL) return -1;
        if(file->fs->read == NULL) return -1;
        return file->fs->read(file->fs, node, buf, count, &offset);
    }

    return -1;
}

ssize_t write(const char *path, void* buf, uint32_t count, off_t offset) {
    inode* node = node_at(root, path, strlen((char*)path));
    if(node == NULL || node->type == INODE_DIR) return -1;

    if(node->type == INODE_DEV) {
        device* dev = node->resource;
        if(dev->write == NULL) return -1;
        return dev->write(buf, count, &offset);
    } else if(node->type == INODE_FILE) {
        inode_file* file = node->resource;
        if(file->fs == NULL) return -1;
        if(file->fs->write == NULL) return -1;
        return file->fs->write(file->fs, node, buf, count, &offset);
    }

    return -1;
}

int ioctl(const char *path, int op, void *data) {
    inode* node = node_at(root, path, strlen((char*)path));
    if(node == NULL || node->type == INODE_DIR) return -1;

    if(node->type == INODE_DEV) {
        device* dev = node->resource;
        if(dev->ioctl == NULL) return -1;
        return dev->ioctl(op, data);
    } else if(node->type == INODE_FILE) {
        return -1;
    }

    return -1;
}

int mknod(const char* path, inode_type type, void *resource) {
    int s = strlen((char*)path);
    int i = 0;

    for(i=s;i>0;i--) {
        if(path[i] == '/') { i++; break; }
    }

    inode* node = node_at(root, path, i);
    if(node == NULL || node->type != INODE_DIR) return -1;

    inode* child = get_free_vfs_inode();
    if(child == NULL) return -1;

    child->used     = true;
    child->name     = path+i;
    child->parent   = node;
    child->type     = type;
    child->resource = resource;

    return add_child(node, child);
}

uint32_t getdents(const char* path, dentry* entry, uint32_t index) {
    int s = strlen((char*)path);
    int i = 0;

    for(i=s;i>0;i--) {
        if(path[i] == '/') { i++; break; }
    }

    inode* node = node_at(root, path, i);

    if(node == NULL || node->type != INODE_DIR) return 0;

    inode_dir* dir = node->resource;

    uint32_t ac_children = 0;

    for(int i=0;i<dir->children_count;i++) {
        if(dir->children[i] != NULL) {
            ac_children++;
            if(ac_children == index) {
                entry->name = dir->children[i]->name;
                entry->type = dir->children[i]->type;
            }
        }
    }

    return ac_children;
}