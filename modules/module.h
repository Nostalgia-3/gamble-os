#ifndef MODULE_H
#define MODULE_H

#define MODULE_NAME(x)
#define MODULE_DESC(x)

typedef struct _module {

} module;

typedef struct _pipe_entry {
    
} pipe_entry;

pipe_entry *pipe_create(const char *st, const struct file_operations *proc_fops);

#endif//MODULE_H