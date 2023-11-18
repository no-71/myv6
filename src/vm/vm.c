#include "vm/vm.h"
#include "riscv/vm_system.h"
#include "util/arithmetic.h"
#include "util/kprint.h"
#include "vm/kalloc.h"
#include "vm/memory_layout.h"
#include <math.h>
#include <stddef.h>

enum { ALLOC, NO_ALLOC };
enum { FREE, NO_FREE };
enum { COPY_IN, COPY_OUT, COPY_IN_STR, COPY_OUT_STR };
enum { COPY_SUCCESS, COPY_FAIL, COPY_FINISH };

void *memset(void *m, int val, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        ((char *)m)[i] = (char)val;
    }
    return m;
}

void *memcpy(void *dest, void *src, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        ((char *)dest)[i] = ((char *)src)[i];
    }
    return dest;
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
        return NULL;
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

void unmap_page_flex(page_table pgtable, uint64 va, int free)
{
    pte *target = walk(pgtable, va, NO_ALLOC);
    if (target == NULL || (*target & PTE_V) == 0) {
        PANIC_FN("unmap unmaped page");
    }

    if (free == FREE) {
        void *page = (void *)PTE_GET_PA(va);
        kfree(page);
    }
    *target = 0;
}

void unmap_page(page_table pgtable, uint64 va)
{
    unmap_page_flex(pgtable, va, NO_FREE);
}

void unmap_pages_free(page_table pgtable, uint64 va)
{
    unmap_page_flex(pgtable, va, FREE);
}

void unmap_n_pages_flex(page_table pgtable, uint64 va, int n, int free)
{

    for (int i = 0; i < n; i++) {
        unmap_page_flex(pgtable, va, free);
        va += PGSIZE;
    }
}

void unmap_n_pages(page_table pgtable, uint64 va, int n)
{
    unmap_n_pages_flex(pgtable, va, n, NO_FREE);
}

void unmap_n_pages_free(page_table pgtable, uint64 va, int n)
{
    unmap_n_pages_flex(pgtable, va, n, FREE);
}

void free_page_table_aux(page_table pgtable, int level)
{
    pte *table = pgtable;
    for (int i = 0; i < 512; i++) {
        uint64 pte = table[i];
        if ((pte & PTE_V) == 0) {
            continue;
        }

        page_table cur_table = (page_table)PTE_GET_PA(pte);
        if (level == 1) {
            kfree(cur_table);
        } else {
            free_page_table_aux(cur_table, level - 1);
        }
    }

    kfree(table);
}

void free_page_table(page_table pgtable)
{
    free_page_table_aux(pgtable, MAX_LEVEL);
}

int check_uva_attribute_valid(uint64 attribute)
{
    if ((attribute & PTE_U) == 0)
        return 1;
    if ((attribute & PTE_R) == 0 && (attribute & PTE_X) == 0)
        return 1;
    return 0;
}

int copy_str_in_page(char *mem_start, char *kva, uint64 copy_size, int meet_end,
                     int copy_way)
{
    for (int i = 0; i < copy_size; i++) {
        char copy_char;
        if (copy_way == COPY_IN_STR) {
            copy_char = *mem_start;
            *kva = copy_char;
        } else {
            copy_char = *kva;
            *mem_start = copy_char;
        }

        if (copy_char == '\0') {
            return COPY_FINISH;
        }
        mem_start++;
        kva++;
    }

    return meet_end ? COPY_FAIL : COPY_SUCCESS;
}

int handle_copy_for_page_with_way(char *mem_start, char *kva, uint64 copy_size,
                                  uint64 size, int copy_way)
{
    if (copy_way == COPY_IN) {
        memcpy(kva, mem_start, copy_size);
        return COPY_SUCCESS;
    }
    if (copy_way == COPY_OUT) {
        memcpy(mem_start, kva, copy_size);
        return COPY_SUCCESS;
    }

    int r = 0;
    if (copy_way == COPY_IN_STR) {
        r = copy_str_in_page(mem_start, kva, copy_size, size == copy_size,
                             COPY_IN_STR);
    } else {
        r = copy_str_in_page(mem_start, kva, copy_size, size == copy_size,
                             COPY_OUT_STR);
    }
    return r;
}

int copy_in_or_out_may_str(page_table upgtable, uint64 uva, char *kva,
                           uint64 size, int copy_way)
{
    while (size > 0) {
        pte *pte = walk(upgtable, uva, NO_ALLOC);
        if (pte == NULL) {
            return 1;
        }
        uint64 attribute = PTE_GET_ATTRIBUTE(*pte);
        if (check_uva_attribute_valid(attribute)) {
            return 1;
        }

        char *page = (char *)PTE_GET_PA(*pte);
        char *mem_start = page + (uva % PGSIZE);
        uint64 page_mem_size = PGSIZE - (uva % PGSIZE);
        uint64 copy_size = MIN(page_mem_size, size);
        int r = handle_copy_for_page_with_way(mem_start, kva, copy_size, size,
                                              copy_way);
        switch (r) {
        case COPY_SUCCESS:
            break;
        case COPY_FAIL:
            return 1;
        case COPY_FINISH:
            return 0;
        }

        kva += copy_size;
        uva += copy_size;
        size -= copy_size;
    }

    return 0;
}

int copy_in(page_table upgtable, uint64 uva, char *kva, uint64 size)
{
    return copy_in_or_out_may_str(upgtable, uva, kva, size, COPY_IN);
}

int copy_out(page_table upgtable, uint64 uva, char *kva, uint64 size)
{
    return copy_in_or_out_may_str(upgtable, uva, kva, size, COPY_OUT);
}

int copy_in_str(page_table upgtable, uint64 uva, char *kva, uint64 size)
{
    return copy_in_or_out_may_str(upgtable, uva, kva, size, COPY_IN_STR);
}

int copy_out_str(page_table upgtable, uint64 uva, char *kva, uint64 size)
{
    return copy_in_or_out_may_str(upgtable, uva, kva, size, COPY_OUT_STR);
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
