#include "vm/kalloc.h"
#include "config/basic_types.h"
#include "riscv/vm_system.h"
#include "vm/memory_layout.h"
#include "vm/vm.h"

struct mem_list mem_container;

void make_garbage_value(char *m);

void kalloc_init()
{
    mem_container.head = NULL;

    char *end = (char *)MEMORY_END;
    for (char *cur_page = (char *)ROUND_UP_PGSIZE(kernel_end); cur_page != end;
         cur_page += PGSIZE) {
        make_garbage_value(cur_page);
        kfree(cur_page);
    }
}

void *kalloc()
{
    if (mem_container.head == NULL) {
        return NULL;
    }

    struct node *mem = mem_container.head;
    mem_container.head = mem->next;
    return (void *)mem;
}

void kfree(void *mem)
{
    if (mem == NULL) {
        return;
    }

    struct node *nd = (struct node *)mem;
    nd->next = mem_container.head;
    mem_container.head = nd;
}

void make_garbage_value(char *m) { memset(m, 0x5, PGSIZE); }
