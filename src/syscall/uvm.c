#include "syscall/uvm.h"
#include "riscv/vm_system.h"
#include "util/kprint.h"
#include "vm/kalloc.h"
#include "vm/memory_layout.h"
#include "vm/vm.h"

int increment_mem_end(struct process *proc, uint64 pages)
{
    uint64 pre_mem_end = proc->mem_end;
    uint64 cur;
    for (cur = 0; cur < pages * PGSIZE; cur += PGSIZE) {
        void *page = kalloc();
        if (page == NULL) {
            goto err_ret;
        }

        int err = map_page(proc->proc_pgtable, pre_mem_end + cur, (uint64)page,
                           PTE_R | PTE_W | PTE_X | PTE_U);
        if (err) {
            kfree(page);
            goto err_ret;
        }
    }

    return 0;

err_ret:
    unmap_n_pages_free(proc->proc_pgtable, pre_mem_end, cur / PGSIZE);
    return -1;
}

void decrease_mem_end(struct process *proc, uint64 pages)
{
    unmap_n_pages_free(proc->proc_pgtable, proc->mem_end - pages * PGSIZE,
                       pages);
}

uint64 brk(struct process *proc, uint64 new_brk)
{
    uint64 pre_mem_brk = proc->mem_brk;
    uint64 pre_mem_end = proc->mem_end;
    if (pre_mem_brk > pre_mem_end) {
        PANIC_FN("mem_brk > mem_end");
    }
    if (pre_mem_end % PGSIZE) {
        PANIC_FN("mem_end is not aligned to PGSIZE");
    }

    if (new_brk < proc->mem_start || new_brk >= PROC_VA_END) {
        return -1;
    }

    uint64 new_mem_end = ROUND_UP_PGSIZE(new_brk);
    if (new_brk > pre_mem_end) {
        int err = increment_mem_end(proc, (new_mem_end - pre_mem_end) / PGSIZE);
        if (err) {
            return -1;
        }
    } else if (new_brk < pre_mem_brk) {
        if (new_mem_end < pre_mem_end) {
            unmap_n_pages_free(proc->proc_pgtable, new_mem_end,
                               (pre_mem_end - new_mem_end) / PGSIZE);
        }
    }

    proc->mem_end = new_mem_end;
    proc->mem_brk = new_brk;
    return 0;
}

uint64 sbrk(struct process *proc, int64 increment)
{
    uint64 pre_mem_brk = proc->mem_brk;
    int err = brk(proc, proc->mem_brk + increment);
    if (err) {
        return -1;
    }

    return pre_mem_brk;
}
