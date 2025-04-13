#include <module.h>
#include <printf.h>
#include <interrupt.h>

void pit_tick(int timer) {
    
}

int pit_start(module *mod) {
    // Configure the pit

    if(hook_interrupt(mod, 0x20) < 0) {
        printf("Failed to hook PIT interrupt\n");
        return -1;
    }

    return 0;
}

int pit_int(module *mod, uint32_t interrupt) {
    return 0;
}

module get_pit_module() {
    return (module) {
        .name = "pit",
        .module_start = pit_start,
        .module_int = pit_int
    };
}

// #include <printf.h>

// // This is called from the assembly interrupt
// void pit_tick() {
//     printf("Hello");
// }