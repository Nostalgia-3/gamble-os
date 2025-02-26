#ifndef INODE_H
#define INODE_H

#include <gosh/gosh.h>

inode_t* get_free_inode(inode_t *inodes, u32 count);
inode_t* get_inode_from_path(inode_t *root, const char *pathname);

inode_t* setup_node(inode_t* inode, const char* name, dt_t type, void* resource);
inode_t* setup_directory(inode_t *inode, const char* name);
inode_t* setup_file(inode_t *inode, const char* name, fs_type_t *fs);

int     add_inode_child(inode_t *parent, inode_t *child);

#endif//INODE_H