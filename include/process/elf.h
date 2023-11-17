#ifndef ELF_H_
#define ELF_H_

#include "config/basic_types.h"

#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_PAD 7
#define EI_NIDENT 16

// e_indent
#define ELFCLASS64 2
#define ELFDATA2LSB 1

// e_type
#define ET_EXEC 2

// p_type
#define PT_LOAD 1

// p_flags
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x3

/* Type for a 16-bit quantity.  */
typedef uint16 Elf32_Half;
typedef uint16 Elf64_Half;

/* Types for signed and unsigned 32-bit quantities.  */
typedef uint32 Elf32_Word;
typedef int32 Elf32_Sword;
typedef uint32 Elf64_Word;
typedef int32 Elf64_Sword;

/* Types for signed and unsigned 64-bit quantities.  */
typedef uint64 Elf32_Xword;
typedef int64 Elf32_Sxword;
typedef uint64 Elf64_Xword;
typedef int64 Elf64_Sxword;

/* Type of addresses.  */
typedef uint32 Elf32_Addr;
typedef uint64 Elf64_Addr;

/* Type of file offsets.  */
typedef uint32 Elf32_Off;
typedef uint64 Elf64_Off;

struct Elf64_Ehdr {
    unsigned char e_ident[EI_NIDENT];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off e_phoff;
    Elf64_Off e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
};

struct Elf64_Phdr {
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
};

#endif
