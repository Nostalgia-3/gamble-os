#ifndef DRIVER_H
#define DRIVER_H

#include <types.h>
#include <gosh/common.h>

#define DRIVER_SUCCESS  0
#define DRIVER_FAILED   1

typedef struct _pci_flags_t {
    u16 vendor;
    u16 device;
    u8 class;
    u8 subclass;
    u8 interface;

    // The bus the PCI device is on; set by the module loader
    u8 r_bus;
    // The slot the PCI device is in; set by the module loader
    u8 r_slot;
} pci_flags_t;

typedef struct _module_t module_t;

typedef struct _module_t {
    // Bitfield of active interrupts
    u8 r_active_ints[32];
    // The id of the module
    size_t id;
    // The name of the module
    const char *name;
    // A value owned by the module
    void *data;

    // PCI Flags (disable by setting .vendor = 0)
    pci_flags_t pci_flags;

    // Called when the module is opened
    int (*module_start)(module_t *mod);
    // Called when the module recieves an interrupt
    int (*module_int)(module_t *mod, u8 irq);
    // Called when the module is closed
    int (*module_end)(module_t *mod);
} module_t;

int _setup_module_manager();

// Open a module into the system, returning 0 if successful.
int open_module(module_t* module);

// Unload a module from the system
void close_module(module_t *module);

// Connect an interrupt to a driver
int k_register_int(module_t *mod, u8 int_id);
int k_unregister_int(module_t *mod, u8 int_id);

#endif//DRIVER_H