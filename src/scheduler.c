#include <scheduler.h>
#include <x86/pic.h>
#include <process.h>
#include <printf.h>
#include <utils.h>
#include <memory.h>

extern process* queue[MAX_PROCESSES];
extern uint32_t cur_process;

static bool pqueue_started = false;

void pqueue_start() {
    cur_process = 0;
    pqueue_started = true;
    pic_enable_irq(0);
}

process* get_current_process() {
    if(!pqueue_started) return NULL;
    return queue[cur_process];
}

process* get_next_process() {
    for(int i=cur_process;i<MAX_PROCESSES;i++) {
        if(queue[cur_process + i] != NULL) {
            return queue[cur_process + i];
        }
    }

    for(int i=0;i<cur_process;i++) {
        if(queue[i] != NULL) {
            return queue[cur_process + i];
        }
    }

    return NULL;
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