#include "process/process_loader.h"
#include "process/elf.h"
#include "riscv/vm_system.h"
#include "vm/kalloc.h"
#include "vm/memory_layout.h"
#include "vm/vm.h"

const char EI_MAGIC[4] = { 0x7f, 'E', 'L', 'F' };

int check_elf_header(struct Elf64_Ehdr *header)
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

int check_segment(struct Elf64_Phdr *segment, Elf64_Word file_size)
{
    if (segment->p_filesz > segment->p_memsz) {
        return 1;
    }
    if (segment->p_vaddr % PGSIZE) {
        return 1;
    }
    if (segment->p_vaddr + segment->p_memsz > USTACK_BASE) {
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

uint64 get_attribute_from_p_flages(Elf64_Word p_flages)
{
    uint64 attribute = PTE_U;
    if ((p_flages & PF_X) || (p_flages & PTE_R)) {
        attribute |= PTE_R | PTE_X;
    }
    if (p_flages & PTE_W) {
        attribute |= PTE_R | PTE_W | PTE_X;
    }

    return attribute;
}

void *append_map_page(page_table pgtable, uint64 va_start, uint64 va,
                      uint64 attribute)
{
    void *page = kalloc();
    if (page == NULL) {
        goto err_ret;
        return NULL;
    }
    int err = map_page(pgtable, va, (uint64)page, attribute);
    if (err) {
        goto err_ret;
    }

    return page;

err_ret:
    unmap_n_pages_free(pgtable, va_start, (va - va_start) / PGSIZE);
    return NULL;
}

int map_segment_for_segment(page_table pgtable, struct Elf64_Phdr *segment,
                            char *elf_file)
{
    Elf64_Off file_off_start = segment->p_offset;
    Elf64_Off file_off = file_off_start;
    Elf64_Word file_size = segment->p_filesz;
    Elf64_Addr va_start = segment->p_vaddr;
    Elf64_Addr va = va_start;
    uint64 attribute = get_attribute_from_p_flages(segment->p_flags);

    Elf64_Off file_n_pages_end = file_off_start + ROUND_DOWN_PGSIZE(file_size);
    while (file_off < file_n_pages_end) {
        void *page = append_map_page(pgtable, va_start, va, attribute);
        if (page == NULL) {
            return 1;
        }

        memcpy(page, elf_file + file_off, PGSIZE);

        file_off += PGSIZE;
        va += PGSIZE;
    }

    Elf64_Off file_end = file_off_start + file_size;
    if (file_off < file_end) {
        void *page = append_map_page(pgtable, va_start, va, attribute);
        if (page == NULL) {
            return 1;
        }

        Elf64_Xword rest_size = file_end - file_off;
        memcpy(page, elf_file + file_off, rest_size);
        memset((char *)page + rest_size, 0, PGSIZE - rest_size);
        va += PGSIZE;
    }

    Elf64_Xword va_n_pages_end = va_start + ROUND_UP_PGSIZE(segment->p_memsz);
    while (va < va_n_pages_end) {
        void *page = append_map_page(pgtable, va_start, va, attribute);
        if (page == NULL) {
            return 1;
        }

        memset(page, 0, PGSIZE);
        va += PGSIZE;
    }

    return 0;
}

uint64 load_process_elf(page_table pgtable, char *elf_file,
                        Elf64_Word file_size)
{
    struct Elf64_Ehdr *elf_header = (struct Elf64_Ehdr *)elf_file;

    int err = check_elf_header(elf_header);
    if (err) {
        return 1;
    }

    char *prog_header = elf_file + elf_header->e_phoff;
    Elf64_Half ph_ent_sz = elf_header->e_phentsize;
    Elf64_Half ph_num = elf_header->e_phnum;
    uint64 mem_end = 0;

    for (int i = 0; i < ph_num; i++) {
        struct Elf64_Phdr *segment =
            (struct Elf64_Phdr *)(prog_header + ph_ent_sz * i);
        if (segment->p_type != PT_LOAD || segment->p_memsz == 0) {
            continue;
        }

        err = check_segment(segment, file_size);
        if (err) {
            return -1;
        }

        err = map_segment_for_segment(pgtable, segment, elf_file);
        if (err) {
            return -1;
        }

        mem_end = segment->p_vaddr + segment->p_memsz;
    }

    return ROUND_UP_PGSIZE(mem_end);
}
