#include "process/process_loader.h"
#include "process/elf.h"
#include "riscv/vm_system.h"
#include "vm/kalloc.h"
#include "vm/memory_layout.h"
#include "vm/vm.h"

const char EI_MAGIC[4] = { 0x7f, 'E', 'L', 'F' };

int check_elf_header(struct Elf32_Ehdr *header)
{
    for (int i = 0; i < 4; i++) {
        if (header->e_ident[i] != EI_MAGIC[i]) {
            return 1;
        }
    }
    if (header->e_ident[EI_CLASS] != ELFCLASS64) {
        return 1;
    }
    if (header->e_ident[EI_DATA] != ELFDATA2LSB) {
        return 1;
    }

    if (header->e_type != ET_EXEC) {
        return 1;
    }

    if (header->e_entry != PROC_VA_START) {
        return 1;
    }

    if (header->e_phoff == 0 || header->e_phentsize == 0 ||
        header->e_phnum == 0) {
        return 1;
    }

    return 0;
}

int check_segment(struct Elf32_Phdr *segment, Elf32_Word file_size)
{
    if (segment->p_filesz > segment->p_memsz) {
        return 1;
    }
    if (segment->p_vaddr % PGSIZE) {
        return 1;
    }
    if (segment->p_offset + segment->p_filesz > file_size) {
        return 1;
    }
    if (segment->p_align > PGSIZE) {
        return 1;
    }

    return 0;
}

uint64 get_attribute_from_p_flages(Elf32_Word p_flages)
{
    uint64 attribute = 0;
    if ((p_flages & PF_X) || (p_flages & PTE_R)) {
        attribute |= PTE_R | PTE_X;
    }
    if (p_flages & PTE_W) {
        attribute |= PTE_R | PTE_W | PTE_X;
    }

    return attribute;
}

int map_page_for_segment(page_table pgtable, struct Elf32_Phdr *segment,
                         char *elf_file)
{
    Elf32_Word file_size = segment->p_filesz;
    Elf32_Off file_off = segment->p_offset;
    Elf32_Addr va_start = segment->p_vaddr;
    Elf32_Addr va = va_start;
    uint64 attribute = get_attribute_from_p_flages(segment->p_flags);

    Elf32_Off file_n_pages_end = ROUND_DOWN_PGSIZE(file_size);
    void *page = NULL;
    while (1) {
        page = kalloc();
        if (page == NULL) {
            unmap_n_pages_free(pgtable, va_start, (va - va_start) / PGSIZE);
            return 1;
        }
        map_page(pgtable, va, (uint64)page, attribute);

        if (file_off >= file_n_pages_end) {
            break;
        }

        memcpy(page, elf_file + file_off, PGSIZE);

        file_off += PGSIZE;
        va += PGSIZE;
    }

    if (file_off < file_size) {
        size_t rest_size = file_size - file_off;
        memcpy(page, elf_file + file_off, rest_size);
        memset((char *)page + rest_size, 0, PGSIZE - rest_size);
        va += PGSIZE;
    }

    size_t va_n_pages_end = ROUND_UP_PGSIZE(segment->p_memsz);
    while (va - va_start < va_n_pages_end) {
        page = kalloc();
        if (page == NULL) {
            unmap_n_pages_free(pgtable, va_start, (va - va_start) / PGSIZE);
            return 1;
        }
        map_page(pgtable, va, (uint64)page, attribute);
        memset(page, 0, PGSIZE);
    }

    return 0;
}

int load_process_elf(page_table pgtable, char *elf_file, Elf32_Word file_size)
{
    struct Elf32_Ehdr *elf_header = (struct Elf32_Ehdr *)elf_file;

    int err = check_elf_header(elf_header);
    if (err) {
        return 1;
    }

    Elf32_Off ph_off = elf_header->e_phoff;
    Elf32_Half ph_ent_sz = elf_header->e_phentsize;
    Elf32_Half ph_num = elf_header->e_phnum;
    char *prog_header = elf_file + ph_off;

    for (int i = 0; i < ph_num; i++) {
        struct Elf32_Phdr *segment =
            (struct Elf32_Phdr *)(prog_header + ph_ent_sz * ph_num);
        if (segment->p_type != PT_LOAD || segment->p_memsz == 0) {
            continue;
        }

        err = check_segment(segment, file_size);
        if (err) {
            return 1;
        }

        err = map_page_for_segment(pgtable, segment, elf_file);
        if (err) {
            return 1;
        }
    }

    return 0;
}
