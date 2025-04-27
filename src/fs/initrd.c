#include <memory.h>
#include <module.h>
#include <fs/vfs.h>
#include <printf.h>
#include <device.h>
#include <str.h>

static fs_mount initrd_fs;
static device ram;
static void* iram;
static size_t isize;

typedef struct _initrd_header {
    uint32_t magic;
    uint32_t filecount;
    uint16_t version;
    uint16_t padding;
} initrd_header;

typedef struct _initrd_file_header {
    uint32_t name_size;
    uint32_t content_size;
} initrd_file_header;

inode* initrd_mount(fs_mount* mount, inode* dev) {
    inode* d = node_at(get_root(), "/dev/ramdisk", strlen("/dev/ramdisk"));
    
    if(d == NULL) {
        printf("Failed to find /dev/ramdisk!");
        return NULL;
    }

    initrd_header header;

    if(read("/dev/ramdisk", &header, sizeof(initrd_header), 0) < 0) {
        printf("Failed to get ramdisk header");
        return NULL;
    }

    if(header.magic != 0x534F6147) {
        printf("initrd magic isn't correct! (expected=0x534F6147, got=0x%08X)", header.magic);
        return NULL;
    } else if(header.version != 0x01) {
        printf("initrd version isn't correct! (expected=1, got=%u)", header.version);
        return NULL;
    }

    inode* root = kmalloc(sizeof(inode), 0);
    memset(root, 0, sizeof(inode));

    generate_directory(NULL, root, "/", header.filecount + 1);

    inode_dir* dir = root->resource;

    uint32_t offset = sizeof(initrd_header) + header.padding;

    for(int i=0;i<header.filecount;i++) {
        initrd_file_header h;
        
        read("/dev/ramdisk", &h, sizeof(initrd_file_header), offset);
        offset += sizeof(initrd_file_header);

        inode* node = kmalloc(sizeof(inode), 0);
        char* name = kmalloc(h.name_size+1, 0);
        inode_file* file = kmalloc(sizeof(inode_file), 0);

        memset(name, 0, h.name_size);
        memset(file, 0, sizeof(inode_file));

        if(read("/dev/ramdisk", name, h.name_size, offset) < 0) {
            printf("Failed to read the name for file #%u", i);
            return NULL;
        }

        file->fs = mount;
        file->_internal = (void*)i;
        file->filesize = h.content_size;

        dir->children[i] = node;
        dir->children[i]->name = name;
        dir->children[i]->parent = root;
        dir->children[i]->resource = file;
        dir->children[i]->type = INODE_FILE;
        dir->children[i]->used = true;

        if(add_child(root, node) < 0) {
            printf("Failed to add \"%s\" to /initrd!", name);
        }

        offset += h.name_size + h.content_size;
    }

    return root;
}

ssize_t ramdisk_read(void* buf, size_t len, off_t *offset) {
    if(*offset > isize) return -1;

    uint8_t* b = buf;

    for(int i=0;i<len;i++) {
        if(*offset + i > isize) return i;
        b[i] = ((uint8_t*)iram)[*offset + i];
    }

    return len;
}

ssize_t initrd_read(fs_mount* fs, inode* in, void* buf, uint32_t count, off_t *offset) {
    // The only inodes created by initrd are files, atm
    if(in->type != INODE_FILE) return -1;

    inode_file* file = in->resource;
    uint32_t f = (uint32_t)file->_internal;

    initrd_header header;

    if(read("/dev/ramdisk", &header, sizeof(initrd_header), 0) < 0) {
        printf("Failed to get ramdisk header!\n");
        return -1;
    }

    if(header.magic != 0x534F6147) {
        printf("initrd magic isn't correct! (expected=0x534F6147, got=0x%08X)", header.magic);
        return -1;
    } else if(header.version != 0x01) {
        printf("initrd version isn't correct! (expected=1, got=%u)", header.version);
        return -1;
    }

    uint32_t off = sizeof(initrd_header) + header.padding;

    for(int i=0;i<header.filecount;i++) {
        initrd_file_header h;
        
        read("/dev/ramdisk", &h, sizeof(initrd_file_header), off);
        off += sizeof(initrd_file_header) + h.name_size;
        
        if(i == f) {
            if(count > h.content_size) count = h.content_size;
            return read("/dev/ramdisk", buf, count, off);
        }
        
        off += h.content_size;
    }

    return 0;
}

int initrd_start(module *mod) {
    initrd_fs = (fs_mount) {
        .name = "initrd",
        .mount = initrd_mount,
        .read  = initrd_read
    };

    ram = (device) {
        .owner = mod,
        .read = ramdisk_read
    };

    if(register_fs_type(&initrd_fs) < 0) {
        printf("Failed to register the initrd filesystem");
        return -1;
    }

    if(mknod("/dev/ramdisk", INODE_DEV, &ram) < 0) {
        printf("Failed to create ramdisk");
        return -1;
    }

    return 0;
}

int initrd_end(module *mod) {
    return 0;
}

module get_initrd_module(void *initram, size_t initsize) {
    iram = initram;
    isize = initsize;
    return (module) {
        .name = "initrd",
        .module_start = initrd_start,
        .module_end = initrd_end
    };
}