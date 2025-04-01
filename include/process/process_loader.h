#ifndef PROCESS_LOADER_H_
#define PROCESS_LOADER_H_

#include "fs/file.h"
#include "process/elf.h"
#include "vm/vm.h"

typedef int (*elf_read_fn_t)(struct inode *ip, int user_dst, uint64 dst,
                             uint off, uint n);

uint64 load_process_elf(page_table pgtable, void *elf_file,
                        Elf64_Word file_size, elf_read_fn_t elf_read);

#endif
