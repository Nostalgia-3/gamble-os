#ifndef GOSH_FS_H
#define GOSH_FS_H

#include <types.h>
#include <gosh/common.h>

typedef struct _DriveDeviceData {
    u16 sector_size;
    /* The number of sectors */
    u32 sectors;

    /* Read a sector and write it to addr (sizeof(addr) = sector_size) */
    void (*read_sector)(Device *dev, u32 sector, u8 *addr);
    /* Write a sector, writing data (sizeof(data) = sector_size) */
    void (*write_sector)(Device *dev, u32 sector, u8* data);
} DriveDeviceData;

// Read a sector from a drive drive, and write it to data
bool    read_sector(Device *drive, u32 sector, u8* data);
bool    write_sector(Device *drive, u32 sector, u8* data);

typedef i32 fd_t;
typedef i64 off_t;

// scan a drive for any filesystems, adding them to the vfs at path if there are
// valid partitions found. Returns zero if failed
bool    mount_drive(Device *drive, char* path);

// Open a file at a specified path, returning the file descriptor, or an error
// if it failed to open
fd_t    open(char* path);

// Close a file based on the file descriptor
void    close(fd_t fd);

// Read count bytes from fd at offset to buf
int     pread(fd_t fd, void *buf, size_t count, off_t offset);
// Write count bytes to fd at offset from buf
int     pwrite(fd_t fd, void *buf, size_t count, off_t offset);

#endif//GOSH_FS_H