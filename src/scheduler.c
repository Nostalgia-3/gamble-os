#include <scheduler.h>
#include <x86/pic.h>
#include <process.h>
#include <printf.h>
#include <utils.h>
#include <memory.h>

// // static process* cur_process     = NULL;
// static process* queue[MAX_PROCESSES];
// static size_t   cur_process     = 0;
// static bool     is_pqueue_on    = false;
// static bool     just_started    = false;

extern process* queue[MAX_PROCESSES];
extern uint32_t cur_process;

void pqueue_init() {
    for(int i=0;i<MAX_PROCESSES;i++) {
        queue[i] = NULL;
    }
}

void pqueue_start() {
    cur_process = 0;
    pic_enable_irq(0);
}

process* get_current_process() {
    return queue[cur_process];
}

// Add a process to the process queue (round-robin)
void add_to_process_queue(process* p) {
    for(int i=0;i<MAX_PROCESSES;i++) {
        if(queue[i] != NULL) continue;
        queue[i] = p;
        p->pid = i;
        return;
    }

    kpanic("Failed to add process to queue");
}

// #define EREG "\x1b[93m%08X\x1b[0m"

// void dump_regs(volatile struct scheduler_data d) {
//     printf(
//         "\x1b[91mRegister Dump\x1b[0m:\n"
//         "  EAX="EREG" EBX="EREG" ECX="EREG" EDX="EREG"\n"
//         "  ESI="EREG" EDI="EREG" EBP="EREG" ESP="EREG"\n"
//         "  EIP="EREG" EFL="EREG"  CS="EREG"  SS="EREG"\n"
//         "  USP="EREG"",
//         d.eax, d.ebx, d.ecx, d.edx,
//         d.esi, d.edi, d.ebp, d.esp,
//         d.eip, d.eflags, d.cs, d.ss,
//         d.useresp
//     );
// }