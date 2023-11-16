#ifndef PROCESS_LOADER_H_
#define PROCESS_LOADER_H_

#include "vm/vm.h"

int load_process_elf(page_table pgtable, char *elf_file);

#endif
