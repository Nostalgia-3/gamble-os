#include <printf.h>
#include <fs/vfs.h>
#include <process.h>
#include <utils.h>
#include <scheduler.h>

#define MAX_SYSCALLS 32

typedef int(*syscall_handler)(volatile struct scheduler_data* data, process* p);

int syscall_exit(volatile struct scheduler_data* data, process* p) {
    // TODO: actually delete processes when they're no longer running
    if(p == NULL) return -1;
    p->can_run = false;
    return 0;
};

int syscall_write(volatile struct scheduler_data* data, process* p) {
    if(data->ebx > MAX_OPEN_FDS) {
        kpanic("tried to access a fd that is larger than the max open fd (%u > %u=max)", data->ebx, MAX_OPEN_FDS);
    }

    if(p->open_fds[data->ebx] == NULL) {
        kpanic("tried to access a fd that doesn't have a resource (fd = %u)", data->ebx);
    }

    return write(p->open_fds[data->ebx], (void*)data->ecx, data->edx, 0);
};

int syscall_read(volatile struct scheduler_data* data, process* p) {
    if(data->ebx > MAX_OPEN_FDS) {
        kpanic("tried to access a fd that is larger than the max open fd (%u > %u=max)", data->ebx, MAX_OPEN_FDS);
    }

    if(p->open_fds[data->ebx] == NULL) {
        kpanic("tried to access a fd that doesn't have a resource (fd = %u)", data->ebx);
    }

    return read(p->open_fds[data->ebx], (void*)data->ecx, data->edx, 0);
};

#include <str.h>

int syscall_open(volatile struct scheduler_data* data, process* p) {
    const char* path    = (const char*)data->ebx;
    if(path == NULL) return -1;
    int pathlen         = strlen((char*)path);
    int id              = 0;

    if(pathlen == 0) return -1;
    
    inode* in = node_at(get_root(), path, pathlen);

    if(in == NULL) return -1;

    return open_inode(p, in, &id);
};

int syscall_close(volatile struct scheduler_data* data, process* p) {
    return close_inode(p, data->ebx);
};

int syscall_fork(volatile struct scheduler_data* data, process* p) {
    printf("TODO\n");
    return -1;
};

int syscall_exec(volatile struct scheduler_data* data, process* p) {
    return 0;
};

int syscall_getpid(volatile struct scheduler_data* data, process* p) {
    return p->pid;
};

int syscall_getdents(volatile struct scheduler_data* data, process* p) {
    uint32_t fd = data->ebx;
    dirent* dents = (dirent*)data->ecx;
    size_t size = data->edx;

    if(fd > MAX_OPEN_FDS) return -1;
    if(p->open_fds[fd] == NULL) return -1;
    if(dents == NULL) return -1;
    if(size == 0) return -1;

    inode* node = p->open_fds[fd];
    uint32_t offset = 0;

check_inode:
    if(node->type != INODE_DIR) return -1;

    if(node->resource.dir.is_mounted) {
        node = node->resource.dir.mount;
        goto check_inode;
    }

    for(int i=0;i<node->resource.dir.children_count;i++) {
        if(node->resource.dir.children[i] == NULL) continue;

        inode* child = node->resource.dir.children[i];

        int namelen = strlen((char*)child->name) + 1;

        if((namelen + sizeof(dirent)) + offset > size) return -1;

        dents->len = namelen + sizeof(dirent);
        dents->type = child->type;

        if(child->type == INODE_FILE) {
            dents->size = child->resource.file.filesize;
        } else if(child->type == INODE_DIR) {
            dents->size = node->resource.dir.children_count * sizeof(inode*);
        } else {
            dents->size = 0;
        }

        memcpy(dents->name, (void*)child->name, namelen - 1);

        offset += namelen + sizeof(dirent);
        dents = (void*)((size_t)offset + (size_t)dents);
    }

    return offset;
}

int syscall_stat(volatile struct scheduler_data* data, process* p) {
    uint32_t fd   = (uint32_t)data->ebx;
    stat* statbuf = (stat*)data->ecx;

    if(fd > MAX_OPEN_FDS || p->open_fds[fd] == NULL) return -1;

    inode* node = p->open_fds[fd];

    statbuf->type = node->type;

    if(node->type == INODE_FILE) {
        statbuf->size   = node->resource.file.filesize;
        statbuf->ctime  = node->resource.file.creation;
        statbuf->mtime  = node->resource.file.last_modified;
    } else if(node->type == INODE_DIR) {
        statbuf->size   = node->resource.dir.children_count * sizeof(inode*);
        statbuf->ctime  = 0;
        statbuf->mtime  = 0;
    } else {
        statbuf->size   = 0;
        statbuf->ctime  = 0;
        statbuf->mtime  = 0;
    }

    return 0;
}

int syscall_brk(volatile struct scheduler_data* data, process* p) {
    size_t size = (size_t)data->ebx;

    if(size == 0) return p->data_size;

    size_t old_page_count = (p->data_size + (PAGE_SIZE - 1))/PAGE_SIZE;
    size_t new_page_count = (size + (PAGE_SIZE - 1))/PAGE_SIZE;

    p->data_size = size;

    if(new_page_count > old_page_count) {
        if(map_pages((void*)p->data_start + old_page_count * PAGE_SIZE, new_page_count - old_page_count, 0) == NULL)
            return -1;
    } else if(new_page_count < old_page_count) {
        free_pages((void*)p->data_start + new_page_count * PAGE_SIZE, old_page_count - new_page_count);
    }

    return p->data_start;
}

int syscall_ioctl(volatile struct scheduler_data* data, process* p) {
    uint32_t fd = (uint32_t)data->ebx;

    if(fd > MAX_OPEN_FDS || p->open_fds[fd] == NULL) return -1;

    inode* node = p->open_fds[fd];

    if(node->type != INODE_DEV) return -1;

    if(node->resource.dev == NULL) {
        kpanic("Device with name \"%s\" has no associated device resource?", node->name);
    }

    if(node->resource.dev->ioctl == NULL) return -1;

    return node->resource.dev->ioctl(data->ecx, (void*)data->edx);
}

int syscall_pwd(volatile struct scheduler_data* data, process* p) {
    // void* buf   = (void*)data->ebx;
    size_t len  = (size_t)data->ecx;

    if(len < strlen(p->cwd) + 1) {
        printf_("buffer too small for working directory!\n");
        return -1;
    }

    return 0;
}

const syscall_handler handlers[MAX_SYSCALLS] = {
    /*  0 */ syscall_exit,
    /*  1 */ syscall_write,
    /*  2 */ syscall_read,
    /*  3 */ syscall_open,
    /*  4 */ syscall_close,
    /*  5 */ syscall_fork,
    /*  6 */ syscall_exec,
    /*  7 */ syscall_getpid,
    /*  8 */ syscall_getdents,
    /*  9 */ syscall_stat,
    /* 10 */ syscall_brk,
    /* 11 */ syscall_ioctl,
    /* 12 */ syscall_pwd
};

void syscall_c(volatile struct scheduler_data d) {
    if(d.eax > MAX_SYSCALLS) {
        kpanic("Unknown syscall #%u", d.eax);
    }

    syscall_handler handler = handlers[d.eax];

    if(handler == NULL) {
        kpanic("Unkown syscall #%u", d.eax);
    }

    int ret = handler(&d, get_current_process());
    
    if((ret) < 0) {
        // printf_("\x1b[33mWarning\x1b[0m: An error occured while running syscall #%u\n", d.eax);
    }
    
    d.eax = ret;
    scheduler_tick(d);

    return;
}