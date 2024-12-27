#include "types.h"
#include "memory.h"
#include "math.h"

// each bit in each byte corresponds to one
// block (size defined by BLOCK_SIZE)
static u32 blocks;

// BEHOLD: The first fit, mostly inefficient malloc function!!
void *malloc(size_t size) {
    u8 blocks_req = ceil((f32)size/(f32)BLOCK_SIZE);
    u32 free_secs = 0;
    u32 start_at = 0xFF;
    for(u32 x=0;x<32;x++) {
        u8 used = (blocks & 2^x) >> x;
        if(!used) {
            if(start_at == 0xFF) {
                start_at = x;
            }
            blocks |= 2^x; // set the bit
            free_secs++;
            if(free_secs >= blocks_req) {
                return (void*)(MEM_BASE + free_secs*BLOCK_SIZE);
            }
        } else {
            if(start_at != 0xFF) break;
        }
    }
    
    return 0;
}

void free(void *mem) {

}