#pragma once

#include <types.h>

/**
 * This is the elf header for 32-bit elf files;
 * since this is a 32-bit kernel I don't really care
 * about not being able to parse 64-bit elf files.
 */
typedef struct {
    /**
     * 0x7F followed by ELF (0x45, 0x4C, 0x46) in ASCII
     */
    uint8_t e_magic[4];
    /**
     * This byte is set to either 1 or 2 to signify 32- or 64-bit format,
     * respectively.
     */
    uint8_t e_class;
    /**
     * This byte is set to either 1 or 2 to signify little or big endianness,
     * respectively. This affects interpretation of multi-byte fields starting
     * with offset 0x10 (e_type)
     */
    uint8_t e_data;
    /**
     * Set to 1 for the original and current version of ELF.
     */
    uint8_t ei_version;
    /**
     * Identifies the target operating system ABI.
     */
    uint8_t e_osabi;
    /**
     * Further specifies the ABI version. Its interpretation depends on the
     * target ABI. Linux kernel (after at least 2.6) has no definition of it, so
     * it is ignored dfor statically linked executables. In that case, offset
     * and size of e_pad are 8.
     */
    uint8_t e_abiversion;
    /**
     * Unused.
     */
    uint8_t e_pad;
    /**
     * Identifies object file type.
     */
    uint16_t e_type;
    /**
     * Specifies target instruction set architecture.
     */
    uint16_t e_machine;
    /**
     * Set to 1 for the original version of ELF.
     */
    uint32_t e_version;
    /**
     * This is the memory address of the entry point from where the process
     * starts executing. This field is either 32 or 64 bits, but elf files will
     * fail to load if it's 64 bit before we ever get this point.
     */
    uint32_t e_entry;
    /**
     * Points to the start of the program header table. It is usually the file
     * header immediately following this one, making the offset 0x34 or 0x40
     * for 32- and 64-bit ELF executables, respectively
     */
    uint32_t e_phoff;
    /**
     * Points to the start of the section header table.
     */
    uint32_t e_shoff;
    /**
     * Interpretation of this field depends on the target architecture.
     */
    uint32_t e_flags;
    /**
     * Contains the size of this header, normally 64 bytes for 64-bit and 52
     * bytes for 32-bit format
     */
    uint16_t e_ehsize;
    /**
     * Contains the size of a program header table entry. As explained below
     * this will typically be 0x20 (32 bit) or 0x38 (56 bit).
     */
    uint16_t e_phentsize;
    /**
     * Contains the number of entries in the program header table.
     */
    uint16_t e_phnum;
    /**
     * Contains the size of a section header table entry. As explained below,
     * this will typically be 0x28 (32 bit) or 0x40 (64 bit).
     */
    uint16_t e_shentsize;
    /**
     * Contains the number of entries in the section header table.
     */
    uint16_t e_shnum;
    /**
     * Contains index of the section header table entry that contains the
     * section names.
     */
    uint16_t e_shstrndx;
} elf_header;