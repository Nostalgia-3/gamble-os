#include <inode.h>
#include <drivers/fs/initrd.h>
#include <gosh/gosh.h>
#include <memory.h>
#include <str.h>

static device_t initrd_dev;
static fs_type_t initrd_fs;
static void *initrd_address;
static inode_t *initrd_root;
static u32 tsize;

typedef struct _initrd_header {
    u32 magic;
    u32 filecount;
    u16 version;
    u16 padding;
} initrd_header;

typedef struct _initrd_file_header {
    u32 name_size;
    u32 content_size;
} initrd_file_header;

typedef struct _initrd_int {
    u32 inode_count;
    inode_t *inodes;
} initrd_int;

ssize_t initrd_write(const void *buf, size_t count, off_t *offset) {
    return 0;
}

ssize_t initrd_read(void *buf, size_t count, off_t *offset) {
    return 0;
}

inode_t *initrd_mount(fs_type_t *fs_type, const char *dev_name) {
    fs_type->internal = k_malloc(sizeof(initrd_int));
    if(fs_type->internal == NULL) return 0;

    initrd_int *inte = (initrd_int*)fs_type->internal;
    
    inte->inode_count = 256;
    inte->inodes = k_malloc(sizeof(inode_t)*inte->inode_count);
    
    if(inte->inodes == NULL) return 0;
    memset(inte->inodes, 0, sizeof(inode_t)*inte->inode_count);
}

void initrd_unmount(fs_type_t *fs_type, const char *dev_name) {

}

int register_fs(fs_type_t *d) { return 0; }

int initrd_start(module_t *mod) {
    initrd_dev = (device_t) {
        .owner  = mod->id,
        .read   = initrd_read,
        .write  = initrd_write
    };

    initrd_address = mod->data;

    if(register_device("initrd", &initrd_dev) < 0) {
        kprintf("Failed creating /dev/initrd\n");
        return DRIVER_FAILED;
    }

    initrd_fs = (fs_type_t) {
        .name = "initrd",
        .mount = initrd_mount,
        .unmount = initrd_unmount
    };

    if(register_fs(&initrd_fs) < 0) {
        kprintf("Failed registering initrd filesystem\n");
        return DRIVER_FAILED;
    }

    initrd_header *header = initrd_address;
    if(header->magic != 0x534F6147) {
        kprintf("initrd magic isn't correct! (expected=0x534F6147, got=0x%08X)\n", header->magic);
        return DRIVER_FAILED;
    } else if(header->version != 0x01) {
        kprintf("initrd version isn't correct! (expected=1, got=%u)\n", header->version);
        return DRIVER_FAILED;
    }

    kprintf("filecount: %u\n", header->filecount);

    u32 offset = sizeof(initrd_header) + header->padding;

    for(int i=0;i<header->filecount;i++) {
        initrd_file_header *h = initrd_address+offset;
        offset += sizeof(initrd_file_header);
        kprintf("name = ");
        write(DBGOUT, initrd_address+offset, h->name_size);
        kprintf(", size = %u\n", h->content_size);
        offset += h->name_size + h->content_size;
    }

    return DRIVER_SUCCESS;
}

module_t get_initrd_module(void *initrd, u32 size) {
    tsize = size;
    
    return (module_t) {
        .name = "initrd",
        .module_start = initrd_start,
        .data = initrd
    };
}