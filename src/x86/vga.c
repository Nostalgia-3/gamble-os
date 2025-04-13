#include <module.h>

int vga_start(module *mod) {
    return 0;
}

module get_vga_module() {
    return (module) {
        .name = "vga",
        .module_start = vga_start
    };
}