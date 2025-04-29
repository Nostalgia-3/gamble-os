#include <printf.h>
#include <fs/vfs.h>
#include <process.h>
#include <utils.h>
#include <scheduler.h>

#define MAX_SYSCALLS 32

typedef int(*syscall_handler)(volatile struct scheduler_data* data, process* p);

static inode* tty = NULL;
static inode* kbd = NULL;

int syscall_exit(volatile struct scheduler_data* data, process* p) {
    // TODO: actually delete processes when they're no longer running
    if(p == NULL) return -1;
    p->can_run = false;
    return 0;
};

int syscall_write(volatile struct scheduler_data* data, process* p) {
    if(tty == NULL || kbd == NULL) {
        tty = node_at(get_root(), "/dev/tty", sizeof("/dev/tty"));
        kbd = node_at(get_root(), "/dev/kbd", sizeof("/dev/kbd"));
    };
    if(data->ebx > 1) kpanic("What are you doing?");
    return write(data->ebx == 1 ? tty : kbd, (void*)data->ecx, data->edx, 0);
};

int syscall_read(volatile struct scheduler_data* data, process* p) {
    if(tty == NULL || kbd == NULL) {
        tty = node_at(get_root(), "/dev/tty", sizeof("/dev/tty"));
        kbd = node_at(get_root(), "/dev/kbd", sizeof("/dev/kbd"));
    };
    if(data->ebx > 1) kpanic("What are you doing?");
    return read(data->ebx == 1 ? tty : kbd, (void*)data->ecx, data->edx, 0);
};

int syscall_open(volatile struct scheduler_data* data, process* p) {
    printf("TODO");
    return -1;
};

int syscall_close(volatile struct scheduler_data* data, process* p) {
    printf("TODO");
    return -1;
};

int syscall_fork(volatile struct scheduler_data* data, process* p) {
    printf("TODO");
    return -1;
};

int syscall_exec(volatile struct scheduler_data* data, process* p) {
    return 0;
};

int syscall_getpid(volatile struct scheduler_data* data, process* p) {
    printf("TODO");
    return -1;
};

int syscall_regdump(volatile struct scheduler_data* data, process* p) {
    printf("TODO");
    return 0;
};

const syscall_handler handlers[MAX_SYSCALLS] = {
    /* 0 */ syscall_exit,
    /* 1 */ syscall_write,
    /* 2 */ syscall_read,
    /* 3 */ syscall_open,
    /* 4 */ syscall_close,
    /* 5 */ syscall_fork,
    /* 6 */ syscall_exec,
    /* 7 */ syscall_getpid,
    /* 8 */ syscall_regdump
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
        kpanic("An error occured while running syscall #%u", d.eax);
    }
    
    d.eax = ret;

    scheduler_tick(d);
    return;
}