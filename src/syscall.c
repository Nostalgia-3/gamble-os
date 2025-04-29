#include <printf.h>
#include <fs/vfs.h>
#include <process.h>
#include <utils.h>
#include <scheduler.h>

#define MAX_SYSCALLS 32

typedef int(*syscall_handler)(volatile struct scheduler_data* data);

int syscall_exit(volatile struct scheduler_data* data) {
    // TODO: actually delete processes when they're no longer running
    process* p = get_current_process();
    if(p == NULL) return -1;
    p->can_run = false;
    return 0;
};

int syscall_write(volatile struct scheduler_data* data) {
    if(data->ebx > 1) kpanic("What are you doing?");
    write(data->ebx == 1 ? "/dev/tty" : "/dev/kbd", (void*)data->ecx, data->edx, 0);
    return 0;
};

int syscall_read(volatile struct scheduler_data* data) {
    if(data->ebx > 1) kpanic("What are you doing?");
    printf_("read\n");
    read(data->ebx == 1 ? "/dev/tty" : "/dev/kbd", (void*)data->ecx, data->edx, 0);
    return 0;
};

int syscall_open(volatile struct scheduler_data* data) {
    printf("TODO");
    return -1;
};

int syscall_close(volatile struct scheduler_data* data) {
    printf("TODO");
    return -1;
};

int syscall_fork(volatile struct scheduler_data* data) {
    printf("TODO");
    return -1;
};

int syscall_exec(volatile struct scheduler_data* data) {
    return 0;
};

int syscall_getpid(volatile struct scheduler_data* data) {
    printf("TODO");
    return -1;
};

const syscall_handler handlers[MAX_SYSCALLS] = {
    /* 0 */ syscall_exit,
    /* 1 */ syscall_write,
    /* 2 */ syscall_read,
    /* 3 */ syscall_open,
    /* 4 */ syscall_close,
    /* 5 */ syscall_fork,
    /* 6 */ syscall_exec,
    /* 7 */ syscall_getpid
};

void syscall_c(volatile struct scheduler_data d) {
    if(d.eax > MAX_SYSCALLS) {
        kpanic("Unknown syscall #%u", d.eax);
    }

    syscall_handler handler = handlers[d.eax];

    if(handler == NULL) {
        kpanic("Unkown syscall #%u", d.eax);
    }

    if(handler(&d) < 0) {
        kpanic("An error occured while running syscall #%u", d.eax);
    }

    scheduler_tick(d);
    printf_("%08X\n", d.eip);
    return;
}