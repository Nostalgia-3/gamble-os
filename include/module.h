#ifndef MODULE_H
#define MODULE_H

typedef struct _module module;

#include <pci.h>
#include <usb.h>
#include <types.h>

#define MODULE_NONE 0
#define MODULE_PCI  1
#define MODULE_USB  2

typedef struct _module {
    const char *name;
    
    uint32_t _hooked_ints[8];
    
    int (*module_start)(module* mod);
    int (*module_int)(module* mod, uint32_t interrupt);
    int (*module_end)(module* mod);

    void *data;

    // Determines the loading priority of the module
    uint8_t priority;

    // Determines the type of requirements
    uint8_t type;

    union {
        pci_requirements pci;
        usb_requirements usb;
    } requirements;
} module;

int module_init();

// Load a module
int module_load(module* mod);

// Call all modules that have listened to the interrupt
int module_int(uint32_t);

// Close all modules
int module_end();

// This is a macro for the purposes of eventually
// allowing configuration for what modules will be loaded
// #define MODULE(mod) module_load((module*)(mod))

#endif