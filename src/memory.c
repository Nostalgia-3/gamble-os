#include <memory.h>

#define KERNEL_START    0x00200000
#define SKETCH_PAGE     0xC0400000

extern uint32_t get_kernel_size();
extern uint32_t read_cr3();
extern void     swap_page_dir(void* phys_addr);

extern uint32_t kernel_page_dir[1024];
extern uint32_t data_page_table[1024];
extern uint32_t boot_page_table1[1024];

void flush_tlb() {
    __asm__ volatile("push %eax\nmov %cr3, %eax\nmov %eax, %cr3\npop %eax\n");
}

static inline void invalidate_page(size_t addr) {
    // TODO: make this invalidate a single page using the invplg instruction
    flush_tlb();
}

void* virt_to_phys(void* addr) {
    size_t pdindex = (size_t)addr >> 22;
    size_t ptindex = (size_t)addr >> 12 & 0x03FF;
    
    size_t *pd = (size_t *)get_current_pagedir();
    if(!(pd[pdindex] & 1)) return NULL;

    data_page_table[0] = (pd[pdindex] & ~(0xFFF)) | 3;
    invalidate_page(SKETCH_PAGE);

    size_t* pt = (size_t*)SKETCH_PAGE;

    return (void*)((pt[ptindex] & ~(0xFFF)) + ((size_t)addr&0xFFF));
}

static uint32_t* cur_page_dir = NULL;

// The starting physical address of the memory pool
static size_t pool_start = 0;

// The length the memory pool
static size_t pool_len = 0;

// The length of the page status buffer in bytes
static size_t status_len = 0;

void mem_init(multiboot_info_t *mbd) {
    for(int i=0;i<mbd->mmap_length;i+=sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmmt =  (multiboot_memory_map_t*) (0xC0000000 + mbd->mmap_addr + i);
    
        if(mmmt->type != MULTIBOOT_MEMORY_AVAILABLE || mmmt->addr > (uint64_t)0xFFFFFFFF) {
            continue;
        }

        size_t addr = (size_t) mmmt->addr;
        size_t len  = (size_t) mmmt->len;
        size_t ksize = get_kernel_size();
    
        if(addr < 0x100000 || mmmt->len <= pool_len) continue;

        if(addr <= KERNEL_START) {
            // The section is exactly the same size (or smaller) than the kernel?
            if(len <= KERNEL_START + ksize) continue;

            // The kernel is within the block of memory
            // |----------------|--------|----------------|
            // | memory chunk 1 | kernel | memory chunk 2 |
            // |----------------|--------|----------------|
            // addr             kstart   kstart+klen      addr+len

            size_t chunk1_len = KERNEL_START - addr;
            size_t chunk2_len = (addr + len) - (KERNEL_START + ksize);

            if(chunk1_len > pool_len) {
                pool_start = addr;
                pool_len = chunk1_len;
            }

            if(chunk2_len > pool_len) {
                pool_start = KERNEL_START + ksize;
                pool_len = chunk2_len;
            }
        } else if(len > pool_len) {
            // printf_("new pool: %08X, %08X\n", pool_start, pool_len);
            pool_start = addr;
            pool_len = len;
        }
    }

    cur_page_dir = kernel_page_dir;
    
    // align pool to 4096
    pool_len -= 4096 - pool_start % 4096;
    pool_start += 4096 - pool_start % 4096;

    // calculate the length of the pool
    status_len = pool_len / 4096;
    status_len = (status_len + (8 - status_len % 8)) / 8;

    size_t status_pages = (status_len + (PAGE_SIZE - status_len % PAGE_SIZE)) / 4096;

    pool_start += status_pages * PAGE_SIZE;
    pool_len   -= status_pages * PAGE_SIZE;

    
    for(int i=0;i<status_pages;i++) {
        map_address((void*)0xC0501000 + i * PAGE_SIZE, (void*)(pool_start - status_pages * PAGE_SIZE + PAGE_SIZE * i), 0);
        
    }
    
    for(int i=0;i<254;i++) {
        kernel_page_dir[770 + i] = (uint32_t)consume_free_page() | 3;
    }
    
    void* fb = map_io(
        (void*)(uint32_t)mbd->framebuffer_addr,
        (mbd->framebuffer_pitch*mbd->framebuffer_height + PAGE_SIZE) / 4096,
        0
    );

    // Find a better place to setup the framebuffer
    set_fb(
        mbd->framebuffer_pitch, mbd->framebuffer_width, mbd->framebuffer_height,
        fb
    );

    for(size_t i=0;i<mbd->framebuffer_height * (mbd->framebuffer_pitch/sizeof(uint32_t));i++) {
        *((uint32_t*)fb + i) = 0x494d64;
    }

    if(!map_pages((void*)0xC0401000, 256, 0))
        kpanic("Failed to allocate pages for Chunk Status Buffer");

    flush_tlb();
}

void* memset(void* ptr, char val, size_t count) {
    int d0, d1;
    __asm__ __volatile__(
        "rep\n\t"
        "stosb"
        : "=&c" (d0), "=&D" (d1)
        : "a" (val),"1" (ptr),"0" (count)
        : "memory"
    );

    return ptr;
}

void memcpy(void* dest, void* src, size_t amount) {
    for(size_t i=0;i<amount;i++) {
        *(uint8_t*)(dest + i) = *(uint8_t*)(src + i);
    }
}

int memcmp(void* a1, void* a2, int len) {
    if(a1 == NULL || a2 == NULL) return -1;

    while((uint8_t*)a1 == (uint8_t*)a2 && len) len--;
    if(len != 0) return 0;
    return 1;
}

void* consume_free_page() {
    uint32_t* addr = (uint32_t*)0xC0501000;

    for(size_t i=0;i<status_len;i++) {
        if(addr[i] != 0xFFFFFFFF) {
            for(size_t x=0;x<32;x++) {
                if(addr[i] & (1 << x)) continue;
                addr[i] |= (1 << x);
    
                return (void*)(pool_start + (PAGE_SIZE * x) + (i*32 * PAGE_SIZE));
            }
            return NULL;
        }
    }

    return NULL;
}

void free_page(void* addr) {
    if(
        (size_t)addr % 4096 != 0 ||
        (size_t)addr < pool_start ||
        (size_t)addr > pool_start + pool_len
    ) return;

    uint32_t* status_array = (uint32_t*)0xC0000000;
    size_t offset_from_start = (size_t)addr - pool_start;

    status_array[(offset_from_start / 4096) / 32] &= ~(1 << ((offset_from_start / 4096) % 32));
}

void* map_address(void* virt, void* physical, uint32_t flags) {
    size_t pd = (size_t)virt >> 22;
    size_t pt = ((size_t)virt >> 12) & 0x3FF;

    if(!(cur_page_dir[pd] & 1)) {
        cur_page_dir[pd] = (size_t)consume_free_page() | 3;
    }

    data_page_table[0] = (cur_page_dir[pd] & ~(0x20)) | 3;
    invalidate_page(SKETCH_PAGE);

    ((uint32_t*)SKETCH_PAGE)[pt] = (size_t) physical | 3;
    invalidate_page((size_t)virt);

    memset(virt, 0, PAGE_SIZE);

    return virt;
}

void* map_page(void* addr, uint32_t flags) {
    size_t pd = ((size_t)addr >> 22) & 0x3FF;
    size_t pt = ((size_t)addr >> 12) & 0x3FF;

    if(!(cur_page_dir[pd] & 1)) {
        cur_page_dir[pd] = (size_t)consume_free_page() | 3;
    }

    data_page_table[0] = cur_page_dir[pd];
    invalidate_page(SKETCH_PAGE);

    ((uint32_t*)SKETCH_PAGE)[pt] = (uint32_t) consume_free_page() | 3;
    invalidate_page((size_t)addr);

    memset(addr, 0, PAGE_SIZE);

    return addr;
}

void* map_pages(void* addr, size_t count, uint32_t flags) {
    if((size_t)addr % 4096 != 0) return NULL;

    for(size_t i=0;i<count;i++) {
        if(map_page(addr + i * PAGE_SIZE, flags) == NULL) return NULL;
    }

    return addr;
}

void free_pages(void* addr, size_t count) {
    for(size_t i=0;i<count;i++) {
        free_page(virt_to_phys(addr + i * PAGE_SIZE));
    }
}

void swap_pagedir(void* pagedir) {
    swap_page_dir(virt_to_phys(pagedir));
    cur_page_dir = pagedir;
}

void* get_current_pagedir() {
    return cur_page_dir;
}

size_t chunks_allocated = 0;

chunk* alloc_chunks(size_t count, int flags) {
    uint32_t* chunk_status = (uint32_t*)0xC0401000;

    size_t start    = 0xC0501000;
    size_t addr     = start;

    size_t sec      = 0;

    for(size_t i=0;i<0x8000;i++) {
        if(chunk_status[i] == ~0) {
            addr += 32 * CHUNK_SIZE;
            continue;
        }

        for(size_t x=0;x<32;x++) {
            if(chunk_status[i] & (1 << x) || (flags & ALIGN_PAGE && addr % 4096 != 0)) {
                addr += CHUNK_SIZE * (sec + 1);
                continue;
            }

            sec++;

            if(sec == count) break;
        }

        if(sec == count) break;
    }

    if(sec != count) return NULL;

    size_t chunk_offset = (addr - start) / CHUNK_SIZE;
    size_t chunk_group = chunk_offset / 32;
    size_t chunk_bit = chunk_offset % 32;

    for(size_t i=0;i<count;i++) {
        chunk_status[chunk_group] |= (1 << chunk_bit);

        chunk_bit++;
        if(chunk_bit > 31) {
            chunk_group++;
            chunk_bit = 0;
        }
    }

    while((chunks_allocated * CHUNK_SIZE) + 0xC0501000 < start) {
        if(map_pages((void*)(chunks_allocated*128 + 0xC0501000), 1, 0) == NULL) return NULL;
        chunks_allocated += PAGE_SIZE / CHUNK_SIZE;
    }

    for(size_t i=0;i<(count*CHUNK_SIZE)/sizeof(uint32_t);i++) {
        *(uint32_t*)(addr + i * sizeof(uint32_t)) = 0;
    }

    return (chunk*)addr;
}

int free_chunks(chunk* addr, size_t count) {
    return 0;
}

static void* nfheap_addr = NULL;
static size_t nfheap_off = 0;
void* allocate_nfheap(size_t count) {
    if(nfheap_addr == NULL) {
        nfheap_addr = alloc_chunks(NFHEAP_SIZE/CHUNK_SIZE, 0);
    }

    if((nfheap_off + count) > NFHEAP_SIZE) {
        kpanic("NF-Heap has overflowed!");
    }

    void* addr = nfheap_addr + nfheap_off;
    nfheap_off += count;

    return addr;
}

static void* ioaddr = (void*)0xE0001000;

void* map_io(void* addr, size_t pagecount, uint32_t flags) {
    void* r = ioaddr;

    for(int i=0;i<pagecount;i++) {
        map_address((void*)ioaddr, (void*)((size_t)addr + i*PAGE_SIZE), 0);
        ioaddr += PAGE_SIZE;
    }

    return r;
}

int copy_page(uint32_t** pdir, void* virt_source, void* virt_dest) {
    size_t source = (size_t)virt_source;

    size_t pd = ((size_t)source >> 22) & 0x3FF;
    size_t pt = ((size_t)source >> 12) & 0x3FF;

    data_page_table[0] = (uint32_t)pdir[pd][pt] | 3;
    invalidate_page(SKETCH_PAGE);

    uint32_t* src = (uint32_t*)SKETCH_PAGE;

    for(int i=0;i<PAGE_SIZE/sizeof(uint32_t);i++) {
        ((uint32_t*)virt_dest)[i] = src[i];
    }

    return 0;
}