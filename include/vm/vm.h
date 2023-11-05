#ifndef VM_H_
#define VM_H_

#include "config/basic_types.h"
#include "riscv/vm_system.h"

#define ROUND_UP_PGSIZE(ADDR) ((((uint64)(ADDR)) + PGSIZE - 1) & (~PGSIZE_MASK))
#define ROUND_DOWN_PGSIZE(ADDR) (((uint64)(ADDR)) & (~PGSIZE_MASK))

typedef uint64 pte;
typedef pte *page_table;

void *memset(void *m, int val, int len);
int map_page(page_table pgtable, uint64 va, uint64 pa, uint64 attribute);
int map_n_pages(page_table pgtable, uint64 va, int n, uint64 pa,
                uint64 attribute);
void unmap_page(page_table pgtable, uint64 va);
void unmap_n_pages(page_table pgtable, uint64 va, int n);

#endif
