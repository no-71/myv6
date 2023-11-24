#ifndef PROCESS_LOADER_H_
#define PROCESS_LOADER_H_

#include "process/elf.h"
#include "vm/vm.h"

uint64 load_process_elf(page_table pgtable, char *elf_file,
                        Elf32_Word file_size);

#endif
