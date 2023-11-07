#include "vm/vm.h"
#include "riscv/vm_system.h"
#include "util/kprint.h"
#include "vm/kalloc.h"
#include "vm/memory_layout.h"

enum { ALLOC, NO_ALLOC };

void *memset(void *m, int val, int len)
{
    for (int i = 0; i < len; i++) {
        ((char *)m)[i] = (char)val;
    }
    return m;
}

/**
 * search target pte in page_table
 * alloc != ALLOC: return NULL if search fail, return (pte *) if success
 * alloc == ALLOC: alloc pages if not find pages in the path. return NULL if
 * alloc fail, won't recycle page alloced(but all these pages are set 0, case no
 * effect to vm control), return (pte *) if success
 */
pte *walk(page_table pgtable, uint64 va, int alloc)
{
    if (va > MAX_VA) {
        PANIC_FN("try to walk with va that greater than MAX_VA");
    }

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

            void *pg = get_clear_page();
            if (pg == NULL) {
                return NULL;
            }
            *cur_pte = MAKE_PTE((uint64)pg, PTE_V);
        }

        cur_table = (pte *)PTE_GET_PA(*cur_pte);
        level--;
    }

    return cur_pte;
}

int map_page(page_table pgtable, uint64 va, uint64 pa, uint64 attribute)
{
    if (pa > MAX_PA) {
        PANIC_FN("try to map page to pa that greater than MAX_PA");
    }

    pte *target = walk(pgtable, va, ALLOC);
    if (target == NULL) {
        return 1;
    }
    if (*target & PTE_V) {
        PANIC_FN("try to map page that has been mapped");
    }

    *target = MAKE_PTE(pa, attribute | PTE_V);
    return 0;
}

int map_n_pages(page_table pgtable, uint64 va, int n, uint64 pa,
                uint64 attribute)
{
    uint64 va_start = va;
    for (int i = 0; i < n; i++) {
        int err = map_page(pgtable, va, pa, attribute);
        if (err) {
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
    pte *target = walk(pgtable, va, NO_ALLOC);
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

void vmprint_aux(int depth, page_table pgtable, int max_depth,
                 int max_level_count)
{
    int pcount = 0;
    for (int i = 0; i < 512; i++) {
        pte e = pgtable[i];
        if (e & PTE_V) {
            pcount++;
            for (int j = 0; j < depth; j++) {
                kprintf(".. ");
            }
            if (pcount > max_level_count) {
                kprintf("too much pte in this level, stop here\n");
                break;
            }
            kprintf("..%d: pte %p pa %p\n", i, e, PTE_GET_PA(e));
            if (depth < max_depth) {
                vmprint_aux(depth + 1, (page_table)PTE_GET_PA(e), max_depth,
                            max_level_count);
            }
        }
    }
}

void vmprint_accurate(page_table pgtable, int max_depth, int max_level_count)
{
    if (max_depth < 0 || max_depth > 2 || max_level_count < 0 ||
        max_level_count > 512) {
        PANIC_FN("incorrect max_depth or max_level_count");
    }
    kprintf("page table %p\n", pgtable);
    vmprint_aux(0, pgtable, max_depth, max_level_count);
}

void vmprint(page_table pgtable) { vmprint_accurate(pgtable, 2, 512); }
