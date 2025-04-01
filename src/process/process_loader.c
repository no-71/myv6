#include "process/process_loader.h"
#include "fs/defs.h"
#include "fs/file.h"
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
            return -1;
        }
    }
    if (header->e_ident[EI_CLASS] != ELFCLASS64) {
        return -1;
    }
    if (header->e_ident[EI_DATA] != ELFDATA2LSB) {
        return -1;
    }

    if (header->e_type != ET_EXEC) {
        return -1;
    }

    if (header->e_entry != PROC_VA_START) {
        return -1;
    }

    if (header->e_phoff == 0 || header->e_phentsize == 0 ||
        header->e_phnum == 0) {
        return -1;
    }

    return 0;
}

int check_segment(struct Elf64_Phdr *segment, Elf64_Word file_size)
{
    if (segment->p_filesz > segment->p_memsz) {
        return -1;
    }
    if (segment->p_vaddr % PGSIZE || segment->p_vaddr < PROC_VA_START) {
        return -1;
    }
    if (segment->p_vaddr + segment->p_memsz > USTACK_BASE) {
        return -1;
    }
    if (segment->p_offset + segment->p_filesz > file_size) {
        return -1;
    }
    if (segment->p_align > PGSIZE) {
        return -1;
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
        return NULL;
    }
    int err = map_page(pgtable, va, (uint64)page, attribute);
    if (err) {
        kfree(page);
        return NULL;
    }

    return page;
}

static int read_check(elf_read_fn_t elf_read, struct inode *ip, int user_dst,
                      uint64 dst, uint off, uint n)
{
    int sz = elf_read(ip, user_dst, dst, off, n);
    return sz != n;
}

int map_segment_for_segment(page_table pgtable, struct Elf64_Phdr *segment,
                            struct inode *elf_file, elf_read_fn_t elf_read)
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
            goto err_ret;
        }
        va += PGSIZE;

        int err = read_check(elf_read, elf_file, KPTR, (uint64)page, file_off,
                             PGSIZE);
        if (err) {
            goto err_ret;
        }
        file_off += PGSIZE;
    }

    Elf64_Off file_end = file_off_start + file_size;
    if (file_off < file_end) {
        void *page = append_map_page(pgtable, va_start, va, attribute);
        if (page == NULL) {
            goto err_ret;
        }
        va += PGSIZE;

        Elf64_Xword rest_size = file_end - file_off;
        int err = read_check(elf_read, elf_file, KPTR, (uint64)page, file_off,
                             rest_size);
        if (err) {
            goto err_ret;
        }
        memset((char *)page + rest_size, 0, PGSIZE - rest_size);
    }

    Elf64_Xword va_n_pages_end = va_start + ROUND_UP_PGSIZE(segment->p_memsz);
    while (va < va_n_pages_end) {
        void *page = append_map_page(pgtable, va_start, va, attribute);
        if (page == NULL) {
            goto err_ret;
        }
        va += PGSIZE;

        memset(page, 0, PGSIZE);
    }

    return 0;

err_ret:
    unmap_n_pages_free(pgtable, va_start, (va - va_start) / PGSIZE);
    return -1;
}

// call with inode lock
uint64 load_process_elf(page_table pgtable, void *elf_file,
                        Elf64_Word file_size, elf_read_fn_t elf_read)
{
    struct Elf64_Ehdr elf_header_mem;
    int err = read_check(elf_read, elf_file, KPTR, (uint64)&elf_header_mem, 0,
                         sizeof(struct Elf64_Ehdr));
    if (err) {
        return -1;
    }
    struct Elf64_Ehdr *elf_header = &elf_header_mem;

    err = check_elf_header(elf_header);
    if (err) {
        return -1;
    }

    Elf64_Off prog_header = elf_header->e_phoff;
    Elf64_Half ph_ent_sz = elf_header->e_phentsize;
    Elf64_Half ph_num = elf_header->e_phnum;
    uint64 mem_end = PROC_VA_START;

    for (int i = 0; i < ph_num; i++) {
        struct Elf64_Phdr segment_mem;
        err =
            read_check(elf_read, elf_file, KPTR, (uint64)&segment_mem,
                       prog_header + ph_ent_sz * i, sizeof(struct Elf64_Phdr));
        if (err) {
            goto err_ret;
        }
        struct Elf64_Phdr *segment = &segment_mem;
        if (segment->p_type != PT_LOAD || segment->p_memsz == 0) {
            continue;
        }

        err = check_segment(segment, file_size);
        if (err) {
            goto err_ret;
        }

        err = map_segment_for_segment(pgtable, segment, elf_file, elf_read);
        if (err) {
            goto err_ret;
        }

        uint64 new_end = segment->p_vaddr + segment->p_memsz;
        mem_end = mem_end > new_end ? mem_end : new_end;
    }

    return ROUND_UP_PGSIZE(mem_end);

err_ret:
    unmap_n_pages_free_hole(pgtable, PROC_VA_START, mem_end);
    return -1;
}
