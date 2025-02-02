#include <stdint.h>
#include "module.h"

static module *mod;

int _module_init(module *m) {
    mod = m;
    return 0;
}

void _module_exit() {
    return;
}

// #define NULL 0
// #define MODULE_NAME(x)
// #define MODULE_DESC(x)
// #define __init
// #define __exit

// typedef unsigned int size_t;
// typedef signed int ssize_t;
// typedef long long unsigned int loff_t;

// typedef struct _proc_dir_entry {

// } proc_dir_entry;

// typedef struct _module {
    
// } module;

// typedef struct _file_operations {
//     struct module *owner;
//     ssize_t (*read) (struct file*, char*, size_t, loff_t *);
//     ssize_t (*write) (struct file*, const char*, size_t, loff_t *);
// } file_operations;

// const static module *THIS_MODULE = (module*)0;
// static struct proc_dir_entry *proc_entry;

// proc_dir_entry *proc_create(const char *st, const struct file_operations *proc_fops) {
//     return 0;
// }

// static ssize_t custom_read(struct file *file, char *user_buffer, size_t count, loff_t* offset) {
//     kprintf("calling our very own custom read method.");
//     char greeting[] = "Hello world!\n";
//     int greeting_length = strlen(greeting);
//     if (*offset > 0) return 0;
//     copy_to_user(user_buffer, greeting, greeting_length);
//     *offset = greeting_length;
//     return greeting_length;
// }

// static file_operations fops = (file_operations) { 0 };

// static int __init custom_init(void) {
//     fops.owner  = THIS_MODULE;
//     fops.read   = custom_read;
//     proc_entry  = proc_create("hello_world", &fops);
//     return 0;
// }

// static struct proc_dir_entry* proc_entry;
// static ssize_t custom_read(struct file* file, char __user* user_buffer, size_t count, loff_t* offset)
// {
//  printk(KERN_INFO "calling our very own custom read method.");
//  char greeting[] = "Hello world!\n";
//  int greeting_length = strlen(greeting);
//  if (*offset > 0)
//   return 0;
//  copy_to_user(user_buffer, greeting, greeting_length);
//  *offset = greeting_length;
//  return greeting_length;
// }
// static struct file_operations fops =
// {
//  .owner = THIS_MODULE,
//  .read = custom_read
// };
// // Custom init and exit methods
// static int __init custom_init(void) {
//  proc_entry = proc_create("helloworlddriver", 0666, NULL, &fops);
//  printk(KERN_INFO "Hello world driver loaded.");
//  return 0;
// }
// static void __exit custom_exit(void) {
//  proc_remove(proc_entry);
//  printk(KERN_INFO "Goodbye my friend, I shall miss you dearly...");
// }
// module_init(custom_init);
// module_exit(custom_exit);