#include "process/process.h"
#include "binary_code/init_code_binary.h"
#include "binary_code/simple_user_binary.h"
#include "cpus.h"
#include "process/process_loader.h"
#include "riscv/vm_system.h"
#include "trap/introff.h"
#include "trap/trampoline.h"
#include "trap/user_trap_handler.h"
#include "util/kprint.h"
#include "vm/kalloc.h"
#include "vm/kvm.h"
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
    int pid = PID_ALLOC_ERR;
    push_introff();
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        if (pid_set[i] == 0) {
            pid_set[i] = 1;
            pid = i;
            break;
        }
    }
    pop_introff();

    return pid;
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
    my_proc()->proc_trap_frame->cpu_id = r_tp();
    pop_introff();

    user_trap_ret();
}

int init_user_pagetable(struct process *proc, page_table pgtable)
{
    void *trap_frame_page = kalloc();
    if (trap_frame_page == NULL) {
        return 1;
    }
    void *ustack = kalloc();
    if (ustack == NULL) {
        kfree(trap_frame_page);
        return 1;
    }

    int err = map_page(pgtable, TRAMPOLINE_BASE,
                       ROUND_DOWN_PGSIZE(user_trap_entry), PTE_R | PTE_X);
    err |= map_page(pgtable, TRAPFRAME_BASE, (uint64)trap_frame_page,
                    PTE_R | PTE_W);
    err |=
        map_page(pgtable, USTACK_BASE, (uint64)ustack, PTE_R | PTE_W | PTE_U);
    if (err) {
        kfree(trap_frame_page);
        kfree(ustack);
        return 1;
    }

    proc->ustack = (uint64)ustack;
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

    // basic setup for trap frame
    find_proc->proc_trap_frame->kernel_trap_handler_ptr = 0;
    find_proc->proc_trap_frame->kstack = find_proc->kstack;
    find_proc->proc_trap_frame->satp = MAKE_SATP((uint64)kernel_page_table);
    find_proc->proc_trap_frame->user_trap_hadnler_ptr =
        (uint64)user_trap_handler;
    // fake return to user sapce
    find_proc->proc_trap_frame->sepc = PROC_VA_START;
    find_proc->proc_trap_frame->sp = USTACK_BASE + PGSIZE;

    // fake process context
    find_proc->proc_context.ra = (uint64)user_proc_entry;
    find_proc->proc_context.sp = find_proc->kstack + PGSIZE;

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
        PANIC_FN("fail to setup init process, process alloc error");
    }

    int err = load_process_elf(proc->proc_pgtable, init_code_binary,
                               init_code_binary_size);
    if (err) {
        PANIC_FN("fail to setup init process, elf load error");
    }
    proc->status = RUNABLE;

    struct process *proc2 = alloc_process();
    if (proc2 == NULL) {
        PANIC_FN("fail to setup init process, process alloc error");
    }

    err = load_process_elf(proc2->proc_pgtable, simple_user_binary,
                           simple_user_binary_size);
    if (err) {
        PANIC_FN("fail to setup init process, elf load error");
    }
    proc2->status = RUNABLE;
}
