#include <x86/gdt.h>
#include <printf.h>
#include <memory.h>

extern gdt_entry_bits_t gdt_start[6];
static tss_entry_t tss_entry;

struct {
    uint16_t size;
    uint32_t offset;
} __attribute__((packed)) gdtr;

void gdt_init() {    
    write_tss(&gdt_start[5]);
}

size_t get_sp() {
    size_t sp;
    asm("mov %%esp, %0" : "=r" (sp));
    return sp;
}

void write_tss(gdt_entry_bits_t *g) {
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = sizeof(tss_entry);

    // Add a TSS descriptor to the GDT.
	g->limit_low = limit;
	g->base_low = base;
	g->accessed = 1;        // With a system entry (`code_data_segment` = 0), 1 indicates TSS and 0 indicates LDT
	g->read_write = 0;      // For a TSS, indicates busy (1) or not busy (0).
	g->conforming = 0;      // always 0 for TSS
	g->code = 1;            // For a TSS, 1 indicates 32-bit (1) or 16-bit (0).
	g->code_data_segment=0; // indicates TSS/LDT (see also `accessed`)
	g->DPL = 0;             // ring 0, see the comments below
	g->present = 1;
	g->limit_high = (limit & (0xf << 16)) >> 16; // isolate top nibble
	g->available = 0;       // 0 for a TSS
	g->long_mode = 0;
	g->big = 0;             // should leave zero according to manuals.
	g->gran = 0;            // limit is in bytes, not pages
	g->base_high = (base & (0xff << 24)) >> 24; //isolate top byte

    memset(&tss_entry, 0, sizeof(tss_entry));

    tss_entry.ss0 = 16;     // offset in the gdt for the data segment
    tss_entry.esp0=get_sp();// stack address for kernel
}