#include "vm/kalloc.h"
#include "config/basic_types.h"
#include "lock/spin_lock.h"
#include "riscv/vm_system.h"
#include "trap/introff.h"
#include "util/kprint.h"
#include "vm/memory_layout.h"
#include "vm/vm.h"

struct mem_list mem_container;

struct spin_lock mem_container_lock;

void make_garbage_value(void *m) { memset(m, 0x5, PGSIZE); }

void kalloc_init()
{
    init_spin_lock(&mem_container_lock);

    mem_container.head = NULL;

    char *end = (char *)MEMORY_END;
    if (MEMORY_END % PGSIZE) {
        PANIC_FN("MEMORY_END is not aligned to PGSIZE");
    }
    for (char *cur_page = (char *)ROUND_UP_PGSIZE(kernel_end); cur_page != end;
         cur_page += PGSIZE) {
        kfree(cur_page);
    }
}

void *kalloc()
{
    acquire_spin_lock(&mem_container_lock);
    if (mem_container.head == NULL) {
        release_spin_lock(&mem_container_lock);
        return NULL;
    }

    struct node *mem = mem_container.head;
    mem_container.head = mem->next;
    release_spin_lock(&mem_container_lock);

    make_garbage_value(mem);
    return (void *)mem;
}

void kfree(void *mem)
{
    if ((uint64)mem < (uint64)kernel_end || (uint64)mem >= MEMORY_END ||
        (uint64)mem % PGSIZE) {
        kprintf("try to free page never alloced %p\n", mem);
        PANIC_FN("free invalid page");
    }
    if (mem == NULL) {
        return;
    }

    acquire_spin_lock(&mem_container_lock);
    struct node *nd = (struct node *)mem;
    nd->next = mem_container.head;
    mem_container.head = (void *)nd;
    release_spin_lock(&mem_container_lock);
}
