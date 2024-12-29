#include "types.h"
#include "memory.h"
#include "math.h"

#include "str.h"
#include "vga.h"

typedef struct _Allocation {
    u16 start;
    u16 length;
} Allocation;

static u8 blocks[MAX_BLOCKS/8];

static Allocation allocs[MAX_ALLOCS];

void set_block_used(u32 block, bool used) {
    if(used) blocks[block/8] |= (1 << (block % 8));
    else blocks[block/8] &= ~(1 << (block % 8));
}

bool is_block_used(u32 block) {
    if(blocks[block/8] & (1 << (block % 8))) return TRUE;
    return FALSE;
}

void push_alloc(u16 start, u16 length) {
    for(int i=0;i<MAX_ALLOCS;i++) {
        if(allocs[i].length == 0) {
            allocs[i].start = start;
            allocs[i].length = length;
        }
    }
}

// BEHOLD: The first fit, mostly inefficient malloc function!!
void *k_malloc(size_t size) {
    u32 blocks_req = ceil((f32)size/(f32)BLOCK_SIZE);

    u16 p   = 0;
    u16 st  = 0;

    for(u32 i=0;i<MAX_BLOCKS;i++) {
        if(is_block_used(i)) {
            st = i+1;
            p = 0;
        } else {
            p++;
        }

        if(p == blocks_req) {
            for(int x=0;x<p;x++) {
                set_block_used(st+x, TRUE);
            }

            return (void*)(MEM_BASE+BLOCK_SIZE*st);
        }
    }
    
    return NULL;
}

void k_free(void *mem) {
    if(mem == NULL) return;

    u32 start = ((size_t)mem - MEM_BASE)/BLOCK_SIZE;

    for(int i=0;i<MAX_ALLOCS;i++) {
        if(allocs[i].start == start && allocs[i].length != 0) {
            for(int x=0;x<allocs[i].length;x++) {
                set_block_used(x+allocs[i].start, FALSE);
            }
            allocs[i].start = 0;
            allocs[i].length = 0;
        }
    }
}

int k_get_used() {
    int used = 0;
    for(int i=0;i<sizeof(blocks)*8;i++) {
        if(is_block_used(i)) used+=BLOCK_SIZE;
    }
    return used;
}