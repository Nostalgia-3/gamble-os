#pragma once

#include <types.h>
#include <process.h>

struct __attribute__((packed)) scheduler_data {
    /* 0x00 */ uint32_t edi;
    /* 0x04 */ uint32_t esi;
    /* 0x08 */ uint32_t ebp;
    /* 0x0C */ uint32_t esp;
    /* 0x10 */ uint32_t ebx;
    /* 0x14 */ uint32_t edx;
    /* 0x18 */ uint32_t ecx;
    /* 0x1C */ uint32_t eax;
    /* 0x20 */ uint32_t eip;
    /* 0x24 */ uint32_t cs;
    /* 0x28 */ uint32_t eflags;
    /* 0x2C */ uint32_t useresp;
    /* 0x30 */ uint32_t ss;
};

void pqueue_start();

process* get_current_process();

void add_to_process_queue(process* p);

void scheduler_tick(volatile struct scheduler_data p);

// struct scheduler_data scheduler_next_process(struct scheduler_data p);