#include <module.h>
#include <memory.h>

#define MAX_MODULES 128
#define MODULE_CHUNKS 32

static module **slots;

int module_init() {
    slots = (module**)alloc_chunks(MODULE_CHUNKS, 0);
    if(slots == NULL) return -1;
    memset(slots, 0, sizeof(module*) * MAX_MODULES);
    return 0;
}

int module_load(module* mod) {
    for(int i=0;i<MAX_MODULES;i++) {
        if(slots[i] == NULL) {
            slots[i] = mod;
            if(slots[i]->module_start != NULL) {
                slots[i]->module_start(slots[i]);
            }
            return 0;
        }
    }

    return 0;
}

int module_int(uint32_t inter) {
    for(int i=0;i<MAX_MODULES;i++) {
        if(
            slots[i] != NULL &&
            slots[i]->module_int &&
            slots[i]->_hooked_ints[inter/32] & (1 << (inter % 32))
        ) {
            slots[i]->module_int(slots[i], inter);
        }
    }
    return 0;
}

int module_end() {
    return 0;
}