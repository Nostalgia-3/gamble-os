#include <scheduler.h>
#include <x86/pic.h>
#include <process.h>
#include <printf.h>
#include <utils.h>
#include <memory.h>

// static process* cur_process     = NULL;
static process* queue[MAX_PROCESSES];
static size_t   cur_process     = 0;
static bool     is_pqueue_on    = false;
static bool     just_started    = false;

void pqueue_init() {
    for(int i=0;i<MAX_PROCESSES;i++) {
        queue[i] = NULL;
    }
}

void pqueue_start() {
    is_pqueue_on = true;
    just_started = true;
    pic_enable_irq(0);
}

bool should_change_pqueue() {
    return is_pqueue_on;
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

#define EREG "\x1b[93m%08X\x1b[0m"

struct scheduler_data scheduler_next_process(volatile struct scheduler_data d) {
    // unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    // unsigned int eip, cs, eflags, useresp, ss;

    // printf(
    //     "\x1b[91mRegister Start\x1b[0m:\n"
    //     "  EAX="EREG" EBX="EREG" ECX="EREG" EDX="EREG"\n"
    //     "  ESI="EREG" EDI="EREG" EBP="EREG" ESP="EREG"\n"
    //     "  EIP="EREG" EFL="EREG"  CS="EREG"  SS="EREG"\n"
    //     "  USP="EREG"",
    //     d.eax, d.ebx, d.ecx, d.edx,
    //     d.esi, d.edi, d.ebp, d.esp,
    //     d.eip, d.eflags, d.cs, d.ss,
    //     d.useresp
    // );

    if(just_started == false) {
        process* previous = queue[cur_process];

        if(queue[cur_process] != NULL) {
            previous->edi    = d.edi;
            previous->esi    = d.esi;
            previous->ebp    = d.ebp;
            previous->esp    = d.esp;
            previous->ebx    = d.ebx;
            previous->edx    = d.edx;
            previous->ecx    = d.ecx;
            previous->eax    = d.eax;
            previous->eip    = d.eip;
            previous->cs     = d.cs;
            previous->eflags = d.eflags;
            previous->ss     = d.ss;
        }

        size_t last_process = cur_process;
        cur_process++;
        while(queue[cur_process] == NULL || !queue[cur_process]->can_run) {
            if(cur_process++ > MAX_PROCESSES) cur_process = 0;
            if(cur_process == last_process) {
                printf("No processes open? Shutting down.");
                // TODO: figure out how to shut down; ACPI?
                while(1);
            }
        }
    } else {
        just_started = false;
        if(queue[cur_process] == NULL) {
            // kpanic("No first process?");
            return d;
        }
    }

    process* current = queue[cur_process];

    d.edi    = current->edi;
    d.esi    = current->esi;
    d.ebp    = current->ebp;
    d.esp    = current->esp;
    d.ebx    = current->ebx;
    d.edx    = current->edx;
    d.ecx    = current->ecx;
    d.eax    = current->eax;
    d.eip    = current->eip;
    d.cs     = current->cs;
    d.eflags = current->eflags;
    d.ss     = current->ss;

    swap_pagedir(current->page_dir);

    return d;
}