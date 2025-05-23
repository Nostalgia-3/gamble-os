#include <process.h>
#include <memory.h>
#include <fs/fs.h>
#include <fs/vfs.h>

#include <printf.h>
#include <utils.h>

#include <x86/pic.h>

#include <scheduler.h>

#include <str.h>

process* create_process(inode* file) {
    if(file->type != INODE_FILE)
        kpanic("Inode passed is not a file, but instead type %u", file->type);
    
    process* p = (process*) alloc_chunks(1, 0);
    file_resource* f = &file->resource.file;

    p->page_dir     = (size_t**) alloc_chunks(PAGE_SIZE / CHUNK_SIZE, ALIGN_PAGE);
    p->open_fds     = (inode**) alloc_chunks((MAX_OPEN_FDS * sizeof(inode*)) / CHUNK_SIZE, 0);
    p->text_size    = (f->filesize + PAGE_SIZE - (f->filesize % PAGE_SIZE))/PAGE_SIZE;
    p->stack_size   = STACK_SIZE;
    p->data_size    = 0;
    p->eip          = 0x1000;
    p->esp          = 0xC0000000;
    p->cs           = 0x00000008;
    p->eflags       = 0x10200;
    p->can_run      = true;
    p->data_start   = p->text_size*PAGE_SIZE + 0x1000 + PAGE_SIZE*2;
    p->source       = file;
    p->cwd          = (char*)alloc_chunks(PAGE_SIZE / CHUNK_SIZE, 0);

    memcpy(p->cwd, "/", 2);

    file->open_count++;

    // Copy the kernel directory pages to the processes page directory
    size_t* prev_pagedir = (size_t*)get_current_pagedir();
    for(int i=0;i<256;i++) {
        p->page_dir[768 + i] = (size_t*)prev_pagedir[768 + i];
    }
    
    swap_pagedir(p->page_dir);

    map_pages((void*)0x1000, p->text_size, 0);
    map_pages((void*)0xC0000000 - p->stack_size*PAGE_SIZE - PAGE_SIZE, p->stack_size, 0);

    size_t amount_read = (size_t)read(file, (void*)0x1000, f->filesize, 0);

    if(f->filesize != amount_read) {
        kpanic("Returned value of \x1b[91mfs->read\x1b[0m is not equal to the filesize! (expected = %d, got = %d)", f->filesize, amount_read);
    }

    // Go back to the previous page directory
    swap_pagedir(prev_pagedir);

    int id = 0;
    open_inode(p, node_at(get_root(), "/dev/kbd", sizeof("/dev/kbd")), &id);
    id = 1;
    open_inode(p, node_at(get_root(), "/dev/tty", sizeof("/dev/tty")), &id);

    return p;
}

process* clone_process(process* parent) {
    if(parent == NULL)
        kpanic("tried to clone a NULL process");
    
    process* p = (process*) alloc_chunks(1, 0);

    p->page_dir     = (size_t**) alloc_chunks(PAGE_SIZE / CHUNK_SIZE, ALIGN_PAGE);
    p->open_fds     = (inode**) alloc_chunks((MAX_OPEN_FDS * sizeof(inode*)) / CHUNK_SIZE, 0);
    p->text_size    = parent->text_size; // (f->filesize + PAGE_SIZE - (f->filesize % PAGE_SIZE))/PAGE_SIZE
    p->stack_size   = parent->stack_size;
    p->data_size    = parent->data_size;

    p->edi = parent->edi;
    p->esi = parent->esi;
    p->ebp = parent->ebp;
    p->esp = parent->esp;
    p->ebx = parent->ebx;
    p->edx = parent->edx;
    p->ecx = parent->ecx;
    p->eax = parent->eax;
    p->eip = parent->eip;
    p->cs = parent->cs;
    p->eflags = parent->eflags;
    p->ss = parent->ss;

    p->can_run      = true;
    p->data_start   = p->text_size*PAGE_SIZE + 0x1000 + PAGE_SIZE*2;
    p->source       = parent->source;
    p->cwd          = (char*)alloc_chunks(PAGE_SIZE / CHUNK_SIZE, 0);

    memcpy(p->cwd, parent->cwd, strlen(parent->cwd));

    p->source->open_count++;

    // Copy the kernel directory pages to the processes page directory
    size_t* prev_pagedir = (size_t*)get_current_pagedir();
    for(int i=0;i<256;i++) {
        p->page_dir[768 + i] = (size_t*)prev_pagedir[768 + i];
    }
    
    swap_pagedir(p->page_dir);

    map_pages((void*)0x1000, p->text_size, 0);
    map_pages((void*)0xC0000000 - p->stack_size*PAGE_SIZE - PAGE_SIZE, p->stack_size, 0);

    for(int i=0;i<parent->text_size;i++) {
        copy_page(
            parent->page_dir,
            (void*)0x1000 + i * PAGE_SIZE,
            (void*)0x1000 + i * PAGE_SIZE
        );
    }

    for(int i=0;i<parent->stack_size;i++) {
        copy_page(
            parent->page_dir,
            (void*)0xC0000000 - p->stack_size*PAGE_SIZE - PAGE_SIZE + i*PAGE_SIZE,
            (void*)0xC0000000 - p->stack_size*PAGE_SIZE - PAGE_SIZE + i*PAGE_SIZE
        );
    }

    for(int i=0;i<parent->data_size;i++) {
        copy_page(
            parent->page_dir,
            (void*)p->data_start + i*PAGE_SIZE,
            (void*)p->data_start + i*PAGE_SIZE
        );
    }

    swap_pagedir(prev_pagedir);

    int id = 0;
    open_inode(p, node_at(get_root(), "/dev/kbd", sizeof("/dev/kbd")), &id);
    id = 1;
    open_inode(p, node_at(get_root(), "/dev/tty", sizeof("/dev/tty")), &id);

    return p;
}

int delete_process(process* p) {
    if(p == NULL) return 0;
    
    if(get_current_process() == p) {
        kpanic("Tried to delete current process? (pid = %u, addr = 0x%08x)", p->pid, p);
    }

    size_t* prev_pagedir = (size_t*)get_current_pagedir();

    swap_pagedir(p->page_dir);

    free_pages((void*)0x1000, p->text_size);
    free_pages((void*)0xC0000000 - p->stack_size*PAGE_SIZE - PAGE_SIZE, p->stack_size);

    swap_pagedir(prev_pagedir);

    for(int i=0;i<MAX_OPEN_FDS;i++) {
        if(p->open_fds[i] != NULL) {
            close_inode(p, i);
        }
    }

    p->source->open_count--;

    free_chunks((chunk*)p->cwd, PAGE_SIZE / CHUNK_SIZE);
    free_chunks((chunk*)p->page_dir, PAGE_SIZE/CHUNK_SIZE);
    free_chunks((chunk*)p->open_fds, (MAX_OPEN_FDS * sizeof(inode*)) / CHUNK_SIZE);

    free_chunks((chunk*)p, 1);

    return 0;
}

int open_inode(process* process, inode* in, int* id) {
    if(process == NULL || id == NULL || in == NULL || *id > MAX_OPEN_FDS) return -1;

    if(process->open_fds[*id] == NULL) {
        process->open_fds[*id] = in;
        return 0;
    } else {
        for(int i=0;i<MAX_OPEN_FDS;i++) {
            if(process->open_fds[i] == NULL) {
                process->open_fds[i] = in;
                in->open_count++;
                *id = i;
                return i;
            }
        }
    }

    return -1;
}

int close_inode(process* process, int id) {
    if(process == NULL) return -1;
    if(id > MAX_OPEN_FDS) return -1;

    if(process->open_fds[id] != NULL) {
        process->open_fds[id]->open_count--;
        process->open_fds[id] = NULL;
        return 0;
    }

    return -1;
}