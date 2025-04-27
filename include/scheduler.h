#pragma once

#include <types.h>
#include <process.h>

struct scheduler_data {
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int eip, cs, eflags, useresp, ss;
};

void pqueue_init();
void pqueue_start();
bool should_change_pqueue();

process* get_current_process();

void add_to_process_queue(process* p);
struct scheduler_data scheduler_next_process(struct scheduler_data p);