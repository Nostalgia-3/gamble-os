#include <stdint.h>

inline void _boot_memset(void *dest, uint8_t v, uint32_t amount) __attribute__((section(".entry.text")));
extern void boot_setup_paging() __attribute__((section(".entry.text")));

extern uint32_t kernel_page_dir[1024];
extern uint32_t kernel_page_tables[64][1024];

inline void _boot_memset(void *dest, uint8_t v, uint32_t amount) {
    for(uint32_t i=0;i<amount;i++) *((uint8_t*)dest) = v;
}

#define PAGE_PRESENT    (1<<0)
#define PAGE_READWRITE  (1<<1)
#define PAGE_SIZE_4MIB  (1<<7)

#define PAGE_SIZE       (4194304)

extern void boot_setup_paging() {
    _boot_memset(&kernel_page_dir, 0, sizeof(kernel_page_dir));
    _boot_memset(&kernel_page_tables, 0, sizeof(kernel_page_tables));

    uint32_t addr = 0;
    for(int i=0;i<64;i++) {
        for(int x=0;x<1024;x++) {
            kernel_page_tables[i][x] = addr | PAGE_PRESENT | PAGE_READWRITE;
            addr += 4096;
        }
        kernel_page_dir[i] = (uint32_t)kernel_page_tables[i] | PAGE_PRESENT | PAGE_READWRITE;
    }

    // for(int i=0;i<1024;i++) {
    //     kernel_page_dir[i] = (uint32_t) (i*PAGE_SIZE & 0xFFC00000) | PAGE_PRESENT | PAGE_READWRITE | PAGE_SIZE_4MIB;
    // }

    *(uint8_t*)0xB8004 = 'P';
}