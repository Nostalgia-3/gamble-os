#include <memory.h>
#include <module.h>
#include <fs/vfs.h>
#include <printf.h>
#include <device.h>
#include <str.h>

enum tarnode_type {
    TAR_FILE,
    TAR_HARDLINK,
    TAR_SYMBOLIC,
    TAR_CHAR,
    TAR_BLOCK,
    TAR_DIR,
    TAR_FIFO
};

typedef struct _tarnode {
    unsigned char name[100];
    unsigned char mode[8];
    unsigned char owner_id[8];
    unsigned char group_id[8];
    unsigned char filesize[12];
    unsigned char last_modified[12];
    unsigned char checksum[8];
    unsigned char type;
    unsigned char link_name[100];
    unsigned char ustar[6];
    unsigned char version[2];
    unsigned char owner_username[32];
    unsigned char owner_groupname[32];
    unsigned char device_majorver[8];
    unsigned char device_minorver[8];
    unsigned char filename_prefix[155];
    unsigned char padding[12];
}__attribute__((packed)) tarnode;

uint32_t oct2bin(unsigned char *str, int size) {
    uint32_t n = 0;
    unsigned char *c = str;
    while (size-- > 0) {
        n *= 8;
        n += *c - '0';
        c++;
    }
    return n;
}

static fs_mount tar_fs;

inode* tar_mount(fs_mount* mount, inode* dev) {
    if(dev == NULL) {
        printf_("\x1b[33mWarning\x1b[0m: no specified inode for tar fs?\n");
        return NULL;
    }

    // printf_("mounting device \"%s\" as tar\n", dev->name);

    tarnode buf;
    off_t offset = 0;

    if(read(dev, &buf, 512, offset) < 0) {
        printf_("\x1b[33mWarning\x1b[0m: failed to read bytes for tarfs?\n");
        return NULL;
    }

    inode* root = (inode*)alloc_chunks(1, 0);
    generate_directory(NULL, root, "/");
    
    while(!strcmp((char*)buf.ustar, "ustar")) {
        if(read(dev, &buf, 512, offset) < 0) {
            printf_("\x1b[31mError\x1b[0m: failed to read bytes from device \"%s\" (offset=%u)\n", dev->name, offset);
            return NULL;
        }

        if(strlen((char*)buf.name) == 0) {
            // printf_("\x1b[33mWarning\x1b[0m: broken directory entry for tarfs? (offset=%u)\n", offset);
            continue;
        }

        uint8_t type = oct2bin(&buf.type, 1);
        uint32_t filesize = oct2bin(buf.filesize, 11);

        inode* parent = root;

        // This'll strip the initial "initrd/" bit of the message
        uint32_t start = 0;
        for(uint32_t i=0;i<100;i++) {
            if(buf.name[i] == '/') {
                if(start != 0) {
                    if(*(buf.name + start + (i - start) + 1) != 0) {
                        int len = 0;
                        while(*(buf.name + start + len) != '/') len++;
                        parent = node_at(parent, (const char*) buf.name + start, len);
                        start = i + 1;
                    } else if(type == TAR_DIR) {
                        if(*(buf.name + start + (i - start) + 1) != 0) {
                            int len = 0;
                            while(*(buf.name + start + len) != '/') len++;
                            parent = node_at(parent, (const char*) buf.name + start, len);
                            start = i + 1;
                        } else {
                            // printf_("%s\n", buf.name + start);
                            inode* in = (inode*)alloc_chunks(1, 0);
                            char* name = allocate_nfheap(strlen((char*)(buf.name + start)) + 1);
                            generate_directory(parent, in, name);
                            memcpy((void*)in->name, buf.name + start, strlen((char*)buf.name + start) - 1);

                            add_child(parent, in);
                        }
                    }
                } else {
                    start = i + 1;
                }
            }

            if(buf.name[i] == '\0') break;
        }

        if(type == TAR_FILE) {
            inode* in = (inode*)alloc_chunks(1, 0);
            file_resource* res = &in->resource.file;

            in->name = allocate_nfheap(strlen((char*)(buf.name + start)) + 1);
            in->type = INODE_FILE;

            res->fs = mount;
            res->filesize = filesize;
            res->_internal = (void*)(offset + 512);

            memcpy((void*)in->name, buf.name + start, strlen((char*)buf.name + start));

            add_child(parent, in);
        }

        offset += (((filesize + 511) / 512) + 1) * 512;
    }

    return root;
}

ssize_t tar_read(fs_mount* fs, inode* in, void* buf, uint32_t count, off_t *offset) {
    return read(fs->source, buf, count, *offset + (uint32_t)in->resource.file._internal);
}

int tarfs_start(module *mod) {
    tar_fs = (fs_mount) {
        .name = "tarfs",
        .mount = tar_mount,
        .read  = tar_read
    };

    if(register_fs_type(&tar_fs) < 0) {
        printf("Failed to register the initrd filesystem");
        return -1;
    }

    return 0;
}

int tarfs_end(module *mod) {
    return 0;
}

module get_tarfs_module() {
    return (module) {
        .name = "tarfs",
        .module_start = tarfs_start,
        .module_end = tarfs_end
    };
}