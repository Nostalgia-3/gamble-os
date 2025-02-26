#include <inode.h>
#include <gosh/gosh.h>
#include <memory.h>
#include <str.h>

inode_t* get_free_inode(inode_t *inodes, u32 count) {
    for(int i=0;i<count;i++) {
        if(inodes[i].in_use == false) {
            inodes[i].in_use = true;
            return &inodes[i];
        }
    }

    return NULL;
}

inode_t *get_inode_from_path(inode_t *root, const char *pathname) {
    if(strlen((char*)pathname) == 0) {
        return NULL;
    }

    inode_t *inode = root;

    if(pathname[0] == '/') {
        inode = root;
    } else {
        // TODO: working directory?
        inode = root;
    }

    char cpath[128] = {0};
    memset(cpath, 0, sizeof(cpath));
    int cpath_ind = 0;

    for(int i=0;i<strlen((char*)pathname);i++) {
        switch(pathname[i]) {
            case '/':
                if(i != 0) {
                    if(inode == NULL || inode->type != DT_DIR) {
                        kprintf("inode is either null or not a directory\n");
                        return NULL;
                    }

                    directory_t *dir = ((directory_t*)inode->resource);
                    if(dir->children_count == 0) {
                        kprintf("Directory \"%s\" has no children slots!\n", inode->name);
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
                                break;
                            }
                        }
                    }

                    if(!found) {
                        kprintf("Failed to find inode with name \"%s\"\n", cpath);
                        return NULL;
                    }
                }
            break;

            default:
                cpath[cpath_ind++] = pathname[i];
            break;
        }
    }

    if(strlen(cpath)) {
        if(inode == NULL || inode->type != DT_DIR) {
            kprintf("inode is either null or not a directory\n");
            return NULL;
        }

        directory_t *dir = ((directory_t*)inode->resource);
        if(dir->children_count == 0) {
            return NULL;
        }

        for(int x=0;x<dir->children_count;x++) {
            if(dir->children[x] != NULL) {
                if(strcmp((char*)dir->children[x]->name, cpath) == 0) {
                    return dir->children[x];
                }
            }
        }

        return NULL;
    }

    return inode;
}

inode_t* setup_directory(inode_t *inode, const char* name) {
    directory_t *d = k_malloc(sizeof(directory_t));
    if(d == NULL) return NULL;
    d->children = k_malloc(sizeof(inode_t*)*256);
    memset(d->children, 0, sizeof(inode_t*)*256);
    d->children_count = 256;
    
    inode->in_use = true;
    inode->name = name;
    inode->resource = d;
    inode->type = DT_DIR;

    return inode;
}

inode_t* setup_file(inode_t *inode, const char* name, fs_type_t *fs) {
    file_t *d = k_malloc(sizeof(file_t));
    if(d == NULL) return NULL;
    d->fs = fs;
    
    inode->in_use = true;
    inode->name = name;
    inode->resource = d;
    inode->type = DT_FILE;

    return inode;
}

inode_t* setup_node(inode_t* inode, const char* name, dt_t type, void* resource) {
    inode->in_use = true;
    inode->name = name;
    inode->type = type;
    inode->resource = resource;
    return inode;
}

int add_inode_child(inode_t *parent, inode_t *child) {
    if(parent == NULL || parent->in_use == false) return -1;
    if(parent->type != DT_DIR) return -1;
    if(child == NULL || child->in_use == false) return -1;

    directory_t* dir = (directory_t*)parent->resource;

    for(int i=0;i<dir->children_count;i++) {
        if(dir->children[i] == NULL) {
            child->parent = parent;
            dir->children[i] = child;
            return 0;
        }
    }

    return -1;
}