#include <printf.h>
#include <fs/vfs.h>

struct sregs {
    uint32_t edx, ecx, ebx, eax;
};

#define EREG "\x1b[93m%08X\x1b[0m"

void syscall_c(struct sregs d) {
    switch(d.eax) {
        case 0: // EXIT
            printf("exit");
        break;

        case 1: // WRITE
            write(d.ebx == 1 ? "/dev/tty" : "/dev/kbd", (void*)d.ecx, d.edx, 0);
        break;
    }
    // printf(
    //     "\x1b[92mSyscall:\x1b[0m\n"
    //     "  EAX="EREG" EBX="EREG" ECX="EREG" EDX="EREG"",
    //     d.eax, d.ebx, d.ecx, d.edx
    // );
    return;
}