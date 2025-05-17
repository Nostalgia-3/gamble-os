#include <module.h>
#include <str.h>
#include <fs/vfs.h>
#include <utils.h>

static device framebuffer;

struct {
    uint8_t*    start;
    size_t      len;
} fb;

ssize_t fb_read(void *buf, size_t len, off_t *offset) {
    if(buf == NULL) return -1;
    if(len > fb.len || *offset > fb.len) {
        printf_("offset = %u, fb.len = %u\n", *offset, fb.len);
        return -1;
    }

    for(size_t i=0;i<len;i++) {
        if((i + *offset) > fb.len) return fb.len;
        ((uint8_t*)buf)[i] = (fb.start)[i + *offset];
    }

    return len;
}

ssize_t fb_write(const void* buf, size_t len, off_t *offset) {
    off_t x = 0;
    
    if(buf == NULL) return -1;
    if(offset == NULL) offset = &x;
    if(len > fb.len) len = fb.len;

    memcpy((void*)fb.start + *offset, (void*)buf, len);

    return 0;
}

int fb_start(module* mod) {
    framebuffer.owner = mod;
    framebuffer.read  = fb_read;
    framebuffer.write = fb_write;

    if(mknod("/dev/", "fb0", INODE_DEV, &framebuffer) < 0) {
        kpanic("Failed to create framebuffer device!");
    }

    return 0;
}

module get_framebuffer_module(void *fbstart, size_t fbsize) {
    fb.start = (uint8_t*)fbstart;
    fb.len = fbsize;
    return (module) {
        .name = "framebuffer",
        .module_start = fb_start
    };
}