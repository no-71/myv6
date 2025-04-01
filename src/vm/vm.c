#include "vm/vm.h"
#include "riscv/vm_system.h"
#include "util/arithmetic.h"
#include "util/kprint.h"
#include "vm/kalloc.h"
#include "vm/memory_layout.h"
#include <math.h>
#include <stddef.h>

enum { NO_ALLOC, ALLOC };
enum { NO_FREE, FREE };
enum { NO_PANIC, PANIC };
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

void *memmove(void *vdst, const void *vsrc, int n)
{
    char *dst;
    const char *src;

    dst = vdst;
    src = vsrc;
    if (src > dst) {
        while (n-- > 0)
            *dst++ = *src++;
    } else {
        dst += n;
        src += n;
        while (n-- > 0)
            *--dst = *--src;
    }
    return vdst;
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
        return -1;
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
            return -1;
        }

        va += PGSIZE;
        pa += PGSIZE;
    }

    return 0;
}

void unmap_page_flex(page_table pgtable, uint64 va, int free,
                     int panic_when_unmap)
{
    pte *target = walk(pgtable, va, NO_ALLOC);
    if (target == NULL || (*target & PTE_V) == 0) {
        if (panic_when_unmap == PANIC) {
            PANIC_FN("unmap unmaped page");
        } else {
            return;
        }
    }

    if (free == FREE) {
        kfree((void *)PTE_GET_PA(*target));
    }
    *target = 0;
}

void unmap_page(page_table pgtable, uint64 va)
{
    unmap_page_flex(pgtable, va, NO_FREE, PANIC);
}

void unmap_page_free(page_table pgtable, uint64 va)
{
    unmap_page_flex(pgtable, va, FREE, PANIC);
}

void unmap_n_pages_flex(page_table pgtable, uint64 va, int n, int free,
                        int panic_when_unmap)
{

    for (int i = 0; i < n; i++) {
        unmap_page_flex(pgtable, va, free, panic_when_unmap);
        va += PGSIZE;
    }
}

void unmap_n_pages(page_table pgtable, uint64 va, int n)
{
    unmap_n_pages_flex(pgtable, va, n, NO_FREE, PANIC);
}

void unmap_n_pages_free(page_table pgtable, uint64 va, int n)
{
    unmap_n_pages_flex(pgtable, va, n, FREE, PANIC);
}

void unmap_n_pages_free_hole(page_table pgtable, uint64 va, int n)
{
    unmap_n_pages_flex(pgtable, va, n, FREE, NO_PANIC);
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

page_table copy_page_table_aux(page_table pgtable, int level)
{
    pte *table = pgtable;
    pte *copy_table = get_pagetable();
    if (copy_table == NULL) {
        return (page_table)-1;
    }

    int has_sub_page = 0;
    for (int i = 0; i < 512; i++) {
        uint64 pte = table[i];
        if ((pte & PTE_V) == 0) {
            continue;
        }

        has_sub_page = 1;
        void *sub_table_or_page = (void *)PTE_GET_PA(pte);
        uint64 attribute = PTE_GET_ATTRIBUTE(pte);
        void *copy_page = NULL;
        if (level == 0) {
            void *page = kalloc();
            if (page == NULL) {
                free_page_table_aux(copy_table, 0);
                return (page_table)-1;
            }

            memcpy(page, sub_table_or_page, PGSIZE);
            copy_page = page;
        } else {
            page_table sub_table =
                copy_page_table_aux(sub_table_or_page, level - 1);
            if (sub_table == NULL) {
                continue;
            }
            if (sub_table == (page_table)-1) {
                free_page_table_aux(copy_table, level);
                return (page_table)-1;
            }
            copy_page = sub_table;
        }
        copy_table[i] = MAKE_PTE(copy_page, attribute);
    }

    if (has_sub_page) {
        return copy_table;
    } else {
        kfree(copy_table);
        return NULL;
    }
}

page_table copy_page_table(page_table pgtable)
{

    page_table ret = copy_page_table_aux(pgtable, MAX_LEVEL);
    return (ret == NULL || ret == (page_table)-1) ? NULL : ret;
}

int merge_page_table_in_interval(page_table mapped_table, page_table pgtable,
                                 uint64 start, uint64 end)
{
    uint64 cur_mem;
    for (cur_mem = start; cur_mem < end; cur_mem += PGSIZE) {
        pte *pte = walk(pgtable, cur_mem, NO_ALLOC);
        if (pte == NULL) {
            continue;
        }

        void *origin_page = (void *)PTE_GET_PA(*pte);
        uint64 attribute = PTE_GET_ATTRIBUTE(*pte);
        void *copy_page = kalloc();
        if (copy_page == NULL) {
            goto err_ret;
        }
        memcpy(copy_page, origin_page, PGSIZE);

        int err = map_page(mapped_table, cur_mem, (uint64)copy_page, attribute);
        if (err) {
            goto err_ret;
        }
    }

    return 0;

err_ret:
    unmap_n_pages_free_hole(mapped_table, start, (cur_mem - start) / PGSIZE);
    return -1;
}

int check_uva_attribute_valid(uint64 attribute)
{
    if ((attribute & PTE_U) == 0)
        return -1;
    if ((attribute & PTE_R) == 0 && (attribute & PTE_X) == 0)
        return -1;
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
            return -1;
        }
        uint64 attribute = PTE_GET_ATTRIBUTE(*pte);
        if (check_uva_attribute_valid(attribute)) {
            return -1;
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
            return -1;
        case COPY_FINISH:
            return 0;
        }

        kva += copy_size;
        uva += copy_size;
        size -= copy_size;
    }

    return 0;
}

int copy_in(page_table upgtable, uint64 uva, void *kva, uint64 size)
{
    return copy_in_or_out_may_str(upgtable, uva, kva, size, COPY_IN);
}

int copy_out(page_table upgtable, uint64 uva, void *kva, uint64 size)
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

int copyin(page_table pgtable, char *dst, uint64 src, uint64 size)
{
    return copy_in(pgtable, src, dst, size);
}

int copyout(page_table pgtable, uint64 dst, char *src, uint64 size)
{
    return copy_out(pgtable, dst, src, size);
}

int copyinstr(page_table pgtable, char *dst, uint64 src, uint64 size)
{
    return copy_in_str(pgtable, src, dst, size);
}

int copyoutstr(page_table upgtable, uint64 dst, char *src, uint64 size)
{
    return copy_out_str(upgtable, dst, src, size);
}

int copyout_uork(page_table pgtable, uint64 dst, char *src, uint64 size,
                 int user_dst)
{
    if (user_dst == UPTR) {
        return copyout(pgtable, dst, src, size);
    } else {
        memmove((char *)dst, src, size);
        return 0;
    }
}

int copyin_uork(page_table pgtable, char *dst, uint64 src, uint64 size,
                int user_src)
{
    if (user_src == UPTR) {
        return copyin(pgtable, dst, src, size);
    } else {
        memmove(dst, (char *)src, size);
        return 0;
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
