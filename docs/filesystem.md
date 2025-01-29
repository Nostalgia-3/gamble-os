Limitations:
 - Readonly
 - No directories
 - No file fragmentation

### Header

```c
struct {
    // The name of the volume
    char volume_name[16];
    // The size of the volume in 512-byte logical blocks
    uint32_t volume_size;
    // The number of file entries in the volume
    uint32_t file_entries;
};
```

### File entry

```c
struct {
    char filename[64];
    uint32_t start_block;
};
```