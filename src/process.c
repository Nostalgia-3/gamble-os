#include <process.h>
#include <memory.h>
#include <fs/fs.h>
#include <fs/vfs.h>

#include <printf.h>

#include <utils.h>

extern uint32_t read_cr3();

process* create_process(inode* file) {
    if(file->type != INODE_FILE)
        kpanic("Inode passed is not a file, but instead type %u", file->type);
    
    process* p = kmalloc(sizeof(process), MALLOC_FILL_ZERO);
    p->page_dir = kmalloc(4096, MALLOC_ALIGNED_4096);

    inode_file* f = file->resource;
    p->text_size = f->filesize/4096 + 1;
    p->stack_size = STACK_SIZE;

    // I'm sure that this is incredibly inefficient (and tbh could be replaced
    // with PAE; but what about poor i386 -> i586 machines!!!) but I can't think
    // of any better way to do this atm.
    // Copy the kernel directory pages to the processes page directory
    size_t* real_pagedir = (size_t*)get_current_pagedir();
    for(int i=0;i<256;i++) {
        p->page_dir[768 + i] = (size_t*)real_pagedir[768 + i];
    }

    // I *know* that this is inefficient
    swap_pagedir(p->page_dir);
    // At this point, whatever we do the memory map will be process specific

    for(int i=0;i<p->text_size;i++) {
        alloc_page((void*)0x1000+(PAGE_SIZE*i), PAGE_SET_ZERO | PAGE_READONLY);
    }

    for(int i=0;i<p->stack_size;i++) {
        alloc_page((void*)0xC0000000 - i*PAGE_SIZE - PAGE_SIZE, PAGE_SET_ZERO);
    }

    off_t _d = 0;

    if(f->filesize != f->fs->read(f->fs, file, (void*)0x1000, f->filesize, &_d)) {
        kpanic("Returned value of \x1b[91mfs->read\x1b[0m is not equal to the filesize!");
    }

    p->eip = 0x1000;
    p->esp = 0xC0000000;

    // Go back to the real page directory
    swap_pagedir(real_pagedir);

    return p;
}

// Start a process
void start_process(process* p) {
    swap_pagedir(p->page_dir);
    void(*yabba)() = (void(*)())0x1000;
    yabba();
}