#include <module.h>
#include <str.h>
#include <fs/vfs.h>
#include <utils.h>

static device ramdisk;

struct {
    uint8_t*    start;
    size_t      len;
} ram;

ssize_t ramdisk_read(void *buf, size_t len, off_t *offset) {
    if(buf == NULL) return -1;
    if(len > ram.len || *offset > ram.len) {
        printf_("offset = %u, ram.len = %u\n", *offset, ram.len);
        return -1;
    }

    for(size_t i=0;i<len;i++) {
        if((i + *offset) > ram.len) return ram.len;
        ((uint8_t*)buf)[i] = (ram.start)[i + *offset];
    }

    return len;
}

int ramdisk_start(module* mod) {
    ramdisk.owner = mod;
    ramdisk.read  = ramdisk_read;

    if(mknod("/dev/", "ramdisk", INODE_DEV, &ramdisk) < 0) {
        kpanic("Failed to create ramdisk device!");
    }

    return 0;
}

module get_ramdisk_module(void *initram, size_t initsize) {
    ram.start = (uint8_t*)initram;
    ram.len = initsize;
    return (module) {
        .name = "ramdisk",
        .module_start = ramdisk_start
    };
}