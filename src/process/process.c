#include "process/process.h"
#include "cpus.h"
#include "riscv/vm_system.h"
#include "trap/introff.h"
#include "trap/trampoline.h"
#include "trap/user_trap_handler.h"
#include "util/kprint.h"
#include "vm/kalloc.h"
#include "vm/memory_layout.h"
#include "vm/vm.h"

#define PID_ALLOC_ERR -1

struct process proc_set[STATIC_PROC_NUM];

char pid_set[STATIC_PROC_NUM];

void init_process(void)
{
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        proc_set[i].status = UNUSED;
    }
}

int alloc_pid(void)
{
    push_introff();
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        if (pid_set[i] == 0) {
            pid_set[i] = 1;
            return i;
        }
    }
    pop_introff();

    return PID_ALLOC_ERR;
}

void free_pid(int i)
{
    push_introff();
    if (pid_set[i] == 0) {
        PANIC_FN("try to free unused pid");
    }
    pid_set[i] = 0;
    pop_introff();
}

void user_proc_entry(void)
{
    pop_introff();

    user_trap_ret();
}

int init_user_pagetable(struct process *proc, page_table pgtable)
{
    void *trap_frame_page = kalloc();
    void *ustack = kalloc();
    if (trap_frame_page == NULL || ustack == NULL) {
        return 1;
    }

    int err = map_page(pgtable, TRAMPOLINE_BASE,
                       ROUND_DOWN_PGSIZE(user_trap_entry), PTE_R | PTE_X);
    err |= map_page(pgtable, TRAPFRAME_BASE, (uint64)trap_frame_page,
                    PTE_R | PTE_W);
    err |= map_page(pgtable, USTACK_BASE, (uint64)ustack, PTE_R | PTE_W);
    if (err) {
        kfree(trap_frame_page);
        kfree(ustack);
        return 1;
    }

    proc->ustack = (uint64)ustack;
    proc->proc_pgtable = pgtable;
    proc->proc_trap_frame = trap_frame_page;

    return 0;
}

int init_user_process(struct process *find_proc)
{
    page_table pgtable = get_pagetable();
    if (pgtable == NULL) {
        return 1;
    }
    find_proc->proc_pgtable = pgtable;

    int err = init_user_pagetable(find_proc, pgtable);
    if (err) {
        free_page_table(find_proc->proc_pgtable);
        return 1;
    }

    void *kstack = kalloc();
    if (kstack == NULL) {
        goto err_ret;
    }
    find_proc->kstack = (uint64)kstack;

    int pid = alloc_pid();
    if (pid == PID_ALLOC_ERR) {
        kfree(kstack);
        goto err_ret;
    }

    find_proc->pid = pid;
    find_proc->status = USED;
    find_proc->killed = 0;
    find_proc->parent = NULL;

    // fake process context
    find_proc->proc_context.ra = (uint64)user_proc_entry;
    find_proc->proc_context.sp = find_proc->kstack + PGSIZE;

    // fake return to user sapce
    find_proc->proc_trap_frame->sepc = PROC_VA_START;
    find_proc->proc_trap_frame->sp = USTACK_BASE + PGSIZE;

    return 0;

err_ret:
    free_page_table(find_proc->proc_pgtable);
    kfree((void *)find_proc->ustack);
    kfree(find_proc->proc_trap_frame);
    return 1;
}

struct process *alloc_process(void)
{
    push_introff();
    struct process *find_proc = NULL;
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        if (proc_set[i].status != UNUSED) {
            continue;
        }

        int err = init_user_process(&proc_set[i]);
        find_proc = err ? NULL : &proc_set[i];
        break;
    }

    pop_introff();
    return find_proc;
}

void setup_init_proc(void)
{
    struct process *proc = alloc_process();
    if (proc == NULL) {
        PANIC_FN("setup init proc fail");
    }

    // set up user space memory

    // set up user space stack(sp)
}
