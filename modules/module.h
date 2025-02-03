#ifndef MODULE_H
#define MODULE_H

#include <stdint.h>

typedef uint32_t        size_t;
typedef int32_t         ssize_t;
typedef uint64_t        loff_t;

#define MODULE_NAME(x)
#define MODULE_DESC(x)

typedef struct _module {

} module;

typedef struct _pipe_config {
    ssize_t (*write)(const char *buffer, size_t count, loff_t *pos);
    ssize_t (*read)(char *buffer, size_t count, loff_t *pos);
} pipe_config;

typedef struct _pipe_entry {
    pipe_config* config;
} pipe_entry;

// Pipes are the primary way userspace programs interact with kernel-level
// modules.
pipe_entry *pipe_create(const char *st, pipe_config* config);

#endif//MODULE_H