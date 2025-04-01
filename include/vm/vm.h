#ifndef VM_H_
#define VM_H_

#include "config/basic_types.h"
#include "riscv/vm_system.h"
#include "vm/kalloc.h"

#define ROUND_UP_PGSIZE(ADDR) ((((uint64)(ADDR)) + PGSIZE - 1) & (~PGSIZE_MASK))
#define ROUND_DOWN_PGSIZE(ADDR) (((uint64)(ADDR)) & (~PGSIZE_MASK))

enum { KPTR, UPTR };

typedef uint64 pte;
typedef pte *page_table;

void *memset(void *m, int val, size_t len);
void *memcpy(void *dest, void *src, size_t len);
void *memmove(void *vdst, const void *vsrc, int n);

pte *walk(page_table pgtable, uint64 va, int alloc);

int map_page(page_table pgtable, uint64 va, uint64 pa, uint64 attribute);
int map_n_pages(page_table pgtable, uint64 va, int n, uint64 pa,
                uint64 attribute);

void unmap_page(page_table pgtable, uint64 va);
void unmap_page_free(page_table pgtable, uint64 va);
void unmap_n_pages(page_table pgtable, uint64 va, int n);
void unmap_n_pages_free(page_table pgtable, uint64 va, int n);
void unmap_n_pages_free_hole(page_table pgtable, uint64 va, int n);
page_table copy_page_table(page_table pgtable);
int merge_page_table_in_interval(page_table mapped_table, page_table pgtable,
                                 uint64 start, uint64 end);
void free_page_table(page_table pgtable);

// copy between kernel and user
int copy_in(page_table upgtable, uint64 uva, void *kva, uint64 size);
int copy_out(page_table upgtable, uint64 uva, void *kva, uint64 size);
int copy_in_str(page_table upgtable, uint64 uva, char *kva, uint64 size);
int copy_out_str(page_table upgtable, uint64 uva, char *kva, uint64 size);
int copyin(page_table pgtable, char *dst, uint64 src, uint64 size);
int copyout(page_table pgtable, uint64 dst, char *src, uint64 size);
int copyinstr(page_table pgtable, char *dst, uint64 src, uint64 size);
int copyoutstr(page_table upgtable, uint64 dst, char *src, uint64 size);
int copyin_uork(page_table pgtable, char *dst, uint64 src, uint64 size,
                int user_src);
int copyout_uork(page_table pgtable, uint64 dst, char *src, uint64 size,
                 int user_dst);

// db
void vmprint_accurate(page_table pgtable, int max_depth, int max_level_count);
void vmprint(page_table pgtable);

static inline void *get_clear_page()
{
    void *pg = kalloc();
    if (pg == NULL) {
        return NULL;
    }

    memset(pg, 0, PGSIZE);
    return pg;
}

static inline page_table get_pagetable(void)
{
    return (page_table)get_clear_page();
}

static inline int init_page_table(page_table *pgtable)
{
    void *pg = get_clear_page();
    if (pg == NULL) {
        return -1;
    }

    *pgtable = pg;
    return 0;
}

#endif
