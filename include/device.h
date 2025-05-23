#pragma once

#include <pci.h>
#include <usb.h>
#include <net/net.h>
#include <module.h>
#include <types.h>

typedef struct _device {
    module *owner;

    // Called when the device is opened
    void    (*open)();
    // Called when the device is closed
    void    (*close)();
    // Called when the device is written to
    ssize_t (*write)(const void* buf, size_t len, off_t* offset);
    // Called when the device is read to
    ssize_t (*read)(void* buf, size_t len, off_t* offset);
    // Called when the device is `ioctl()`-ed to
    int     (*ioctl)(int op, void* data);
} device;

#define DEVICE_UNUSED   0
#define DEVICE_PCI      1
#define DEVICE_USB      2
#define DEVICE_NIC      3

typedef struct _usb_device {
    module* owner;
} usb_device;

typedef struct _dev_entry {
    uint32_t type;

    union {
        pci_device pci;
        usb_device usb;
        nic_device nic;
    };
} dev_entry;

int find_pci_device(pci_requirements* req);

int register_device(dev_entry dev);
int device_init();