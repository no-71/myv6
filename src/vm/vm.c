#include "vm/vm.h"
#include "riscv/vm_system.h"
#include "util/kprint.h"
#include "vm/kalloc.h"

enum { ALLOC, NO_ALLIC };

void *memset(void *m, int val, int len)
{
    for (int i = 0; i < len; i++) {
        ((char *)m)[i] = (char)val;
    }
    return m;
}

pte *walk(page_table pgtable, uint64 va, int alloc)
{
    int level = MAX_LEVEL;
    pte *cur_table = pgtable;
    pte *cur_pte;
    while (1) {
        cur_pte = cur_table + VPN_LEVEL_N(va, level);

        if (level == 0) {
            break;
        }

        if ((*cur_pte & PTE_V) == 0) {
            if (alloc != ALLOC) {
                return NULL;
            }

            void *pg = kalloc();
            if (pg == NULL) {
                return NULL;
            }
            memset(pg, 0, PGSIZE);
            *cur_pte = MAKE_PTE((uint64)pg, PTE_V);
        }

        cur_table = (pte *)PTE_GET_PA(*cur_pte);
        level--;
    }

    return cur_pte;
}

int map_page(page_table pgtable, uint64 va, uint64 pa, uint64 attribute)
{
    pte *target = walk(pgtable, va, ALLOC);
    if (target == NULL) {
        return 1;
    }

    *target = MAKE_PTE(pa, attribute);
    return 0;
}

int map_n_pages(page_table pgtable, uint64 va, int n, uint64 pa,
                uint64 attribute)
{
    uint64 va_start = va;
    for (int i = 0; i < n; i++) {
        int res = map_page(pgtable, va, pa, attribute);
        if (res != 0) {
            unmap_n_pages(pgtable, va_start, i);
            return 1;
        }

        va += PGSIZE;
        pa += PGSIZE;
    }

    return 0;
}

void unmap_page(page_table pgtable, uint64 va)
{
    pte *target = walk(pgtable, va, ALLOC);
    if (target == NULL || (*target & PTE_V) == 0) {
        PANIC_FN("unmap unmaped page");
    }

    *target = 0;
}

void unmap_n_pages(page_table pgtable, uint64 va, int n)
{
    for (int i = 0; i < n; i++) {
        unmap_page(pgtable, va);
        va += PGSIZE;
    }
}
