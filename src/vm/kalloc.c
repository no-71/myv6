#include "vm/kalloc.h"
#include "config/basic_types.h"
#include "riscv/vm_system.h"
#include "trap/introff.h"
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
        kfree(cur_page);
    }
}

void *kalloc()
{
    push_introff();
    if (mem_container.head == NULL) {
        return NULL;
    }

    struct node *mem = mem_container.head;
    mem_container.head = mem->next;
    pop_introff();

    make_garbage_value((char *)mem);
    return (void *)mem;
}

void kfree(void *mem)
{
    if (mem == NULL) {
        return;
    }

    push_introff();
    struct node *nd = (struct node *)mem;
    nd->next = mem_container.head;
    mem_container.head = nd;
    pop_introff();
}

void make_garbage_value(char *m) { memset(m, 0x5, PGSIZE); }
