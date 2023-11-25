#include "process/process.h"
#include "binary_code/binary_codes.h"
#include "cpus.h"
#include "process/process_loader.h"
#include "riscv/vm_system.h"
#include "scheduler/sleep.h"
#include "trap/intr_handler.h"
#include "trap/introff.h"
#include "trap/kernel_trap_jump.h"
#include "trap/trampoline.h"
#include "trap/user_trap_handler.h"
#include "util/kprint.h"
#include "util/string.h"
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
        proc_set[i].pid = -1;
        proc_set[i].status = UNUSED;
    }
}

static int alloc_pid(void)
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

static void free_pid(int i)
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

static int init_user_pagetable(struct process *proc, page_table pgtable)
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

static int init_user_process(struct process *find_proc)
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
    find_proc->xstatus = 0;
    find_proc->chain = NULL;
    find_proc->parent = NULL;

    // basic setup for trap frame
    find_proc->proc_trap_frame->kernel_trap_entry_ptr =
        (uint64)kernel_trap_entry;
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

static struct process *alloc_process(void)
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

    uint64 mem_end = load_process_elf(proc->proc_pgtable, init_code_binary,
                                      init_code_binary_size);
    if (mem_end == -1) {
        PANIC_FN("fail to setup init process, elf load error");
    }
    proc->status = RUNABLE;
    proc->mem_start = mem_end;
    proc->mem_brk = mem_end;
    proc->mem_end = mem_end;
}

static void free_user_memory(page_table pgtable, uint64 mem_end)
{
    unmap_n_pages_free(pgtable, PROC_VA_START,
                       (mem_end - PROC_VA_START) / PGSIZE);
}

static void free_process_user_memory(struct process *proc)
{
    free_user_memory(proc->proc_pgtable, proc->mem_end);
}

static void free_process(struct process *proc)
{
    push_introff();

    free_pid(proc->pid);
    free_process_user_memory(proc);
    kfree((void *)proc->ustack);
    kfree(proc->proc_trap_frame);
    free_page_table(proc->proc_pgtable);

    kfree((void *)proc->kstack);

    proc->pid = -1;
    proc->status = UNUSED;
    proc->killed = 0;
    proc->xstatus = 0;
    proc->chain = NULL;
    proc->parent = NULL;

    pop_introff();
}

uint64 fork(struct process *proc)
{
    struct process *fork_proc = alloc_process();
    if (fork_proc == NULL) {
        return -1;
    }

    // map and copy [va_start, mem_end]
    int err = merge_page_table_in_interval(fork_proc->proc_pgtable,
                                           proc->proc_pgtable, PROC_VA_START,
                                           proc->mem_end);
    if (err) {
        free_process(fork_proc);
        return -1;
    }
    // copy user stack
    void *fork_ustack = (void *)fork_proc->ustack;
    void *ustack = (void *)proc->ustack;
    memcpy(fork_ustack, ustack, PGSIZE);

    // copy user regs
    memcpy(fork_proc->proc_trap_frame, proc->proc_trap_frame,
           sizeof(uint64) * 31);
    fork_proc->proc_trap_frame->a0 = 0;
    fork_proc->proc_trap_frame->sepc = proc->proc_trap_frame->sepc;

    fork_proc->parent = proc;
    fork_proc->mem_start = proc->mem_start;
    fork_proc->mem_brk = proc->mem_brk;
    fork_proc->mem_end = proc->mem_end;
    push_introff();
    fork_proc->status = RUNABLE;
    pop_introff();

    return fork_proc->pid;
}

struct elf_in_kernel {
    char *name;
    char *binary;
    uint64 *size;
} all_elf_in_kernel[64] = {
    (struct elf_in_kernel){ "init_code", init_code_binary,
                            &init_code_binary_size },
    (struct elf_in_kernel){ "usertest", usertest_binary,
                            &usertest_binary_size },
    (struct elf_in_kernel){ "echo", echo_binary, &echo_binary_size }
};

struct elf_in_kernel *get_elf_in_kernel(char *name)
{
    for (int i = 0; all_elf_in_kernel[i].name; i++) {
        if (strcmp(name, all_elf_in_kernel[i].name) == 0) {
            return &all_elf_in_kernel[i];
        }
    }

    return NULL;
}

static void *copy_argv_to_page(int argc, char *argv[], char str_in_argv[])
{
    char *page = kalloc();
    if (page == NULL) {
        return NULL;
    }

    uint64 str_start = sizeof(uint64) * (argc + 1);
    memcpy(page + str_start, str_in_argv, 64 * argc);

    uint64 *argv0 = (uint64 *)page;
    for (int i = 0; i < argc; i++) {
        argv0[i] = str_start + 64 * i;
    }
    argv0[argc] = 0;

    return page;
}

int map_argv_to_page_table(page_table pgtable, uint64 uva, char *argv_page,
                           uint64 attribute)
{
    int err = map_page(pgtable, uva, (uint64)argv_page, attribute);
    if (err) {
        return 1;
    }

    uint64 *pchar_uva = (uint64 *)argv_page;
    for (int i = 0; pchar_uva[i]; i++) {
        pchar_uva[i] = uva + pchar_uva[i];
    }

    return 0;
}

int load_new_process_for_exec(struct process *proc, struct elf_in_kernel *elf,
                              void *argv_page)
{
    page_table new_pgtable = get_pagetable();
    if (new_pgtable == NULL) {
        return 1;
    }

    uint64 new_mem_end = load_process_elf(new_pgtable, elf->binary, *elf->size);
    if (new_mem_end == -1) {
        kfree(new_pgtable);
        return 1;
    }

    // map allocated memory
    int err = map_page(new_pgtable, TRAMPOLINE_BASE,
                       ROUND_DOWN_PGSIZE(user_trap_entry), PTE_R | PTE_X);
    err |= map_page(new_pgtable, TRAPFRAME_BASE, (uint64)proc->proc_trap_frame,
                    PTE_R | PTE_W);
    err |=
        map_page(new_pgtable, USTACK_BASE, proc->ustack, PTE_R | PTE_W | PTE_U);
    if (err) {
        free_user_memory(new_pgtable, new_mem_end);
        free_page_table(new_pgtable);
        return 1;
    }

    // map argv page
    if (argv_page != NULL) {
        err = map_argv_to_page_table(new_pgtable, new_mem_end, argv_page,
                                     PTE_R | PTE_W | PTE_U);
        if (err) {
            free_user_memory(new_pgtable, new_mem_end);
            free_page_table(new_pgtable);
            return 1;
        }
        new_mem_end += PGSIZE;
    }

    page_table old_pgtable = proc->proc_pgtable;
    free_user_memory(old_pgtable, proc->mem_end);
    free_page_table(old_pgtable);

    proc->proc_pgtable = new_pgtable;
    proc->mem_start = new_mem_end;
    proc->mem_brk = new_mem_end;
    proc->mem_end = new_mem_end;

    return 0;
}

uint64 exec(struct process *proc, char *file, int argc, char *argv[],
            char str_in_argv[])
{
    push_introff();

    // copy argv to memory
    void *argv_page = NULL;
    if (argc) {
        argv_page = copy_argv_to_page(argc, argv, str_in_argv);
        if (argv_page == NULL) {
            return pop_introff_return_val(-1);
        }
    }

    // load new proc
    struct elf_in_kernel *elf = get_elf_in_kernel(file);
    if (elf == NULL) {
        return pop_introff_return_val(-1);
    }

    int err = load_new_process_for_exec(proc, elf, argv_page);
    if (err) {
        return pop_introff_return_val(-1);
    }

    // set argc, argv and regs
    uint64 *proc_arg0 = &proc->proc_trap_frame->a0;
    proc_arg0[0] = argc;
    proc_arg0[1] = proc->mem_end - PGSIZE;
    proc->proc_trap_frame->sp = USTACK_BASE + PGSIZE;
    proc->proc_trap_frame->sepc = PROC_VA_START;

    pop_introff();
    return 0;
}

static void reparent_children(struct process *parent)
{
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        if (proc_set[i].parent == parent) {
            proc_set[i].parent = &proc_set[0];
        }
    }
}

static void wake_up_parent(struct process *parent)
{
    if (parent->status == SLEEP) {
        parent->status = RUNABLE;
        parent->chain = NULL;
    }
}

__attribute__((noreturn)) uint64 exit(struct process *proc, uint64 xstatus)
{
    push_introff();

    if (proc->pid == 0) {
        PANIC_FN("init proc exit");
    }

    proc->status = ZOMBIE;
    proc->xstatus = xstatus;
    proc->chain = NULL;

    reparent_children(proc);
    wake_up_parent(&proc_set[0]);

    wake_up_parent(proc->parent);

    // push_introff() called once, proc->status has been setted
    // swtch to scheduler
    switch_to_scheduler();

    // ZOMBIE wait for cleaning by parent, should never return
    kprintf("ZOMBIE proc(pid: %d) returned", (int)proc->pid);
    PANIC_FN("ZOMBIE proc returned");
}

static int handle_exit_child(struct process *proc, uint64 int_uva, pid_t *pid)
{
    *pid = -1;
    int no_child_err = 1;
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        struct process *other_proc = &proc_set[i];
        if (other_proc->parent != proc) {
            continue;
        }

        no_child_err = 0;
        if (other_proc->status != ZOMBIE) {
            continue;
        }

        *pid = other_proc->pid;
        if ((char *)int_uva != NULL) {
            copy_out(proc->proc_pgtable, int_uva, &other_proc->xstatus,
                     sizeof(int));
        }
        free_process(other_proc);
        return 0;
    }

    return no_child_err;
}

uint64 wait(struct process *proc, uint64 int_uva)
{
    push_introff();

    pid_t pid;
    int no_child_err = handle_exit_child(proc, int_uva, &pid);
    if (no_child_err) {
        return pop_introff_return_val(-1);
    }
    if (pid != -1) {
        return pop_introff_return_val(pid);
    }

    while (1) {
        sleep(proc);

        no_child_err = handle_exit_child(proc, int_uva, &pid);
        if (no_child_err) {
            PANIC_FN("wake up parent without any child");
        }
        if (pid == -1) {
            continue;
        }
        return pop_introff_return_val(pid);
    }
}

uint64 kill(pid_t pid)
{
    push_introff();

    struct process *target = NULL;
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        if (proc_set[i].pid == pid) {
            target = &proc_set[i];
            break;
        }
    }
    if (target == NULL) {
        return pop_introff_return_val(-1);
    }

    target->killed = 1;

    pop_introff();
    return 0;
}

uint64 count_proc_num(void)
{
    push_introff();
    int n = 0;
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        if (proc_set[i].status != UNUSED) {
            n++;
        }
    }
    pop_introff();
    return n;
}
