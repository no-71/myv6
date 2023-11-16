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

typedef uint32 Elf32_Addr;
typedef uint16 Elf32_Half;
typedef uint32 Elf32_Off;
typedef uint32 Elf32_Sword;
typedef uint32 Elf32_Word;

struct Elf32_Ehdr {
    unsigned char e_ident[EI_NIDENT];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
};

struct Elf32_Phdr {
    Elf32_Word p_type;
    Elf32_Off p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
};

#endif
