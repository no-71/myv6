#include "process/process.h"
#include "binary_code/binary_codes.h"
#include "cpus.h"
#include "driver/virtio.h"
#include "fs/defs.h"
#include "fs/file.h"
#include "fs/fs.h"
#include "fs/param.h"
#include "fs/stat.h"
#include "lock/spin_lock.h"
#include "process/proc_group.h"
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

int init_fs;

// proc lock acquired in sequence: parent -> child -> childchild
struct process proc_set[STATIC_PROC_NUM];

char pid_set[STATIC_PROC_NUM];
struct spin_lock pid_lock;

void process_init(void)
{
    init_spin_lock(&pid_lock);

    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        proc_set[i].pid = -1;
        proc_set[i].status = UNUSED;
        init_spin_lock(&proc_set[i].lock);
        proc_set[i].pgroup_id = -1;
    }
}

static struct process *get_init_process(void) { return &proc_set[0]; }

static int alloc_pid(void)
{
    int pid = -1;
    acquire_spin_lock(&pid_lock);
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        if (pid_set[i] == 0) {
            pid_set[i] = 1;
            pid = i;
            break;
        }
    }
    release_spin_lock(&pid_lock);

    return pid;
}

static void free_pid(int i)
{
    acquire_spin_lock(&pid_lock);
    if (pid_set[i] == 0) {
        PANIC_FN("try to free unused pid");
    }
    pid_set[i] = 0;
    release_spin_lock(&pid_lock);
}

static void user_proc_entry(void)
{
    release_spin_lock(&my_proc()->lock);

    if (init_fs == 0) {
        init_fs = 1;
        fsinit(ROOTDEV);
    }

    user_trap_ret();
}

static int init_user_pagetable(struct process *proc, page_table pgtable)
{
    void *trap_frame_page = kalloc();
    if (trap_frame_page == NULL) {
        return -1;
    }
    void *ustack = kalloc();
    if (ustack == NULL) {
        kfree(trap_frame_page);
        return -1;
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
        return -1;
    }

    proc->ustack = (uint64)ustack;
    proc->proc_trap_frame = trap_frame_page;

    return 0;
}

static int init_user_process(struct process *find_proc)
{
    page_table pgtable = get_pagetable();
    if (pgtable == NULL) {
        return -1;
    }
    find_proc->proc_pgtable = pgtable;

    int err = init_user_pagetable(find_proc, pgtable);
    if (err) {
        free_page_table(find_proc->proc_pgtable);
        return -1;
    }

    void *kstack = kalloc();
    if (kstack == NULL) {
        goto err_ret;
    }
    find_proc->kstack = (uint64)kstack;

    int pid = alloc_pid();
    if (pid == -1) {
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
    return -1;
}

static struct process *alloc_process(void)
{
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        struct process *proc = &proc_set[i];

        acquire_spin_lock(&proc->lock);
        if (proc->status != UNUSED) {
            release_spin_lock(&proc->lock);
            continue;
        }

        proc->status = USED;
        release_spin_lock(&proc->lock);

        int err = init_user_process(proc);
        if (err) {
            acquire_spin_lock(&proc->lock);
            proc->status = UNUSED;
            release_spin_lock(&proc->lock);
            return NULL;
        }
        return proc;
    }

    return NULL;
}

static int fake_elf_read(struct inode *ip, int user_dst, uint64 dst, uint off,
                         uint n)
{
    if (user_dst == UPTR) {
        panic("fake_elf_read: try to read into user addr");
    }
    memmove((char *)dst, ((char *)ip) + off, n);
    return n;
}

void setup_init_proc(void)
{
    struct process *proc = alloc_process();
    if (proc == NULL) {
        PANIC_FN("fail to setup init process, process alloc error");
    }

    uint64 mem_end = load_process_elf(proc->proc_pgtable, init_code_binary,
                                      init_code_binary_size, fake_elf_read);
    if (mem_end == -1) {
        PANIC_FN("fail to setup init process, elf load error");
    }
    proc->status = RUNABLE;
    proc->mem_start = mem_end;
    proc->mem_brk = mem_end;
    proc->mem_end = mem_end;
    proc->cwd = namei("/");

    setup_default_proc_group(proc);
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

// call with proc locked
// this function entirely free the process, make final free,
// and set the proc to unused
static void free_process(struct process *proc)
{
    // free user memory
    free_pid(proc->pid);
    free_process_user_memory(proc);
    kfree((void *)proc->ustack);
    kfree(proc->proc_trap_frame);
    free_page_table(proc->proc_pgtable);

    // free kernel memory
    kfree((void *)proc->kstack);

    // files and inode were freed when exit
    // don't need to free here

    // set other var to init status
    proc->pid = -1;
    proc->status = UNUSED;
    proc->killed = 0;
    proc->xstatus = 0;
    proc->chain = NULL;
    proc->parent = NULL;

    proc->pgroup_id = -1;
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
        acquire_spin_lock(&fork_proc->lock);
        free_process(fork_proc);
        release_spin_lock(&fork_proc->lock);
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

    // dup user opened file and cwd
    for (int i = 0; i < NOFILE; i++) {
        if (proc->ofile[i]) {
            fork_proc->ofile[i] = filedup(proc->ofile[i]);
        }
    }
    fork_proc->cwd = idup(proc->cwd);

    pid_t pid = fork_proc->pid;
    acquire_spin_lock(&fork_proc->lock);
    fork_proc->status = RUNABLE;
    release_spin_lock(&fork_proc->lock);

    // add to parent proc group
    err = forkproc_into_pgroup(proc->pgroup_id, fork_proc);
    if (err) {
        PANIC_FN("fail to add child to parent's proc group");
    }

    return pid;
}

struct elf_in_kernel {
    char *name;
    char *binary;
    uint64 *size;
} all_elf_in_kernel[64] = {
    (struct elf_in_kernel){ "init_code", init_code_binary,
                            &init_code_binary_size },
};

static void *alloc_page_copy_argv(int argc, char *argv[], char str_in_argv[])
{
    char *page = kalloc();
    if (page == NULL) {
        return NULL;
    }

    uint64 str_start = sizeof(uint64) * (argc + 1);
    memcpy(page + str_start, str_in_argv, ARGV_STR_LEN * argc);

    uint64 *argv0 = (uint64 *)page;
    for (int i = 0; i < argc; i++) {
        argv0[i] = str_start + ARGV_STR_LEN * i;
    }
    argv0[argc] = 0;

    return page;
}

static int map_argv_to_page_table(page_table pgtable, uint64 uva,
                                  char *argv_page, uint64 attribute)
{
    int err = map_page(pgtable, uva, (uint64)argv_page, attribute);
    if (err) {
        return -1;
    }

    uint64 *pchar_uva = (uint64 *)argv_page;
    for (int i = 0; pchar_uva[i]; i++) {
        pchar_uva[i] = uva + pchar_uva[i];
    }

    return 0;
}

static int load_new_process_for_exec(struct process *proc, struct inode *elf,
                                     void *argv_page)
{
    page_table new_pgtable = get_pagetable();
    if (new_pgtable == NULL) {
        return -1;
    }

    ilock(elf);
    if (elf->type != T_FILE) {
        iunlock(elf);
        kfree(new_pgtable);
        return -1;
    }
    uint64 new_mem_end = load_process_elf(new_pgtable, elf, elf->size, readi);
    iunlock(elf);
    if (new_mem_end == -1) {
        free_page_table(new_pgtable);
        return -1;
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
        return -1;
    }

    // map argv page
    if (argv_page != NULL) {
        err = map_argv_to_page_table(new_pgtable, new_mem_end, argv_page,
                                     PTE_R | PTE_W | PTE_U);
        if (err) {
            free_user_memory(new_pgtable, new_mem_end);
            free_page_table(new_pgtable);
            return -1;
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
    // load proc elf from fs
    struct inode *elf_inode = namei(file);
    if (elf_inode == NULL) {
        return -1;
    }

    // copy argv to memory
    void *argv_page = NULL;
    if (argc) {
        argv_page = alloc_page_copy_argv(argc, argv, str_in_argv);
        if (argv_page == NULL) {
            begin_op();
            iput(elf_inode);
            end_op();
            return -1;
        }
    }

    // load the elf to proc
    int err = load_new_process_for_exec(proc, elf_inode, argv_page);
    begin_op();
    iput(elf_inode);
    end_op();
    if (err) {
        if (argv_page != NULL) {
            kfree(argv_page);
        }
        return -1;
    }

    // set argc, argv and regs
    uint64 *proc_arg0 = &proc->proc_trap_frame->a0;
    proc_arg0[0] = argc;
    proc_arg0[1] = proc->mem_end - PGSIZE;
    proc->proc_trap_frame->sp = USTACK_BASE + PGSIZE;
    proc->proc_trap_frame->sepc = PROC_VA_START;

    return 0;
}

static void reparent_children(struct process *parent)
{
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        struct process *proc = &proc_set[i];
        if (proc->parent == parent) {
            acquire_spin_lock(&proc->lock);
            proc->parent = &proc_set[0];
            release_spin_lock(&proc->lock);
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
    if (proc != my_proc()) {
        PANIC_FN("exit on other proc");
    }
    if (proc->pid == 0) {
        PANIC_FN("init proc exit");
    }

    // free opened files and cwd
    for (int i = 0; i < NOFILE; i++) {
        if (proc->ofile[i]) {
            fileclose(proc->ofile[i]);
            proc->ofile[i] = NULL;
        }
    }
    begin_op();
    iput(proc->cwd);
    end_op();
    proc->cwd = NULL;

    // reparent
    acquire_spin_lock(&proc_set[0].lock);
    acquire_spin_lock(&proc->lock);

    reparent_children(proc);
    wake_up_parent(&proc_set[0]);
    struct process *parent = proc->parent;

    release_spin_lock(&proc_set[0].lock);
    release_spin_lock(&proc->lock);

    // we will exit proc group, close intr to avoid proc lost
    push_introff();

    exit_pgroup();

    // ZOMBIE current proc
    acquire_spin_lock(&parent->lock);
    acquire_spin_lock(&proc->lock);

    if (proc->parent == parent) {
        wake_up_parent(proc->parent);
        release_spin_lock(&parent->lock);
    } else {
        if (proc->parent != &proc_set[0]) {
            PANIC_FN("reparent proc to other proc(not proc 0)");
        }

        release_spin_lock(&parent->lock);
        release_spin_lock(&proc->lock);

        acquire_spin_lock(&proc_set[0].lock);
        acquire_spin_lock(&proc->lock);
        wake_up_parent(&proc_set[0]);
        release_spin_lock(&proc_set[0].lock);
    }

    proc->status = ZOMBIE;
    proc->xstatus = xstatus;
    proc->chain = NULL;
    pop_introff();

    // proc->status has been setted
    // swtch to scheduler
    switch_to_scheduler();

    // ZOMBIE wait for cleaning by parent, should never return
    kprintf("ZOMBIE proc(pid: %d) returned", (int)proc->pid);
    PANIC_FN("ZOMBIE proc returned");
}

// call with proc lock
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

        acquire_spin_lock(&other_proc->lock);
        if (other_proc->status != ZOMBIE) {
            release_spin_lock(&other_proc->lock);
            continue;
        }

        *pid = other_proc->pid;
        if ((char *)int_uva != NULL) {
            int err = copy_out(proc->proc_pgtable, int_uva,
                               &other_proc->xstatus, sizeof(int));
            if (err) {
                release_spin_lock(&other_proc->lock);
                return -1;
            }
        }
        free_process(other_proc);
        release_spin_lock(&other_proc->lock);
        return 0;
    }

    return no_child_err;
}

uint64 wait(struct process *proc, uint64 int_uva)
{
    acquire_spin_lock(&proc->lock);

    pid_t pid;
    int err = handle_exit_child(proc, int_uva, &pid);
    if (err) {
        release_spin_lock(&proc->lock);
        return -1;
    }
    if (pid != -1) {
        release_spin_lock(&proc->lock);
        return pid;
    }

    while (1) {
        sleep(&proc->lock, NULL);

        err = handle_exit_child(proc, int_uva, &pid);
        if (err) {
            PANIC_FN("wake up parent without any child");
        }
        if (pid == -1) {
            continue;
        }
        release_spin_lock(&proc->lock);
        return pid;
    }
}

uint64 kill(pid_t pid)
{
    struct process *target = NULL;
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        struct process *proc = &proc_set[i];

        acquire_spin_lock(&proc->lock);
        if (proc->status == USED || proc->pid != pid) {
            release_spin_lock(&proc->lock);
            continue;
        }
        target = proc;
        break;
    }
    if (target == NULL) {
        return -1;
    }

    target->killed = 1;
    release_spin_lock(&target->lock);

    return 0;
}

uint64 proc_sys_sleep(int sleep_ticks)
{
    acquire(&tick_lock);
    uint start_ticks = tick_count;
    while (tick_count - start_ticks < sleep_ticks) {
        if (myproc()->killed) {
            release(&tick_lock);
            return -1;
        }
        sleep(&tick_lock, &tick_count);
    }
    release(&tick_lock);
    return 0;
}

static void lock_rest_process(char *locked)
{
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        if (locked[i] == 0) {
            acquire_spin_lock(&proc_set[i].lock);
        }
    }
}

static void lock_all_process_aux(struct process *proc, int pi, char *locked)
{
    locked[pi] = 1;
    acquire_spin_lock(&proc->lock);
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        struct process *child = &proc_set[i];
        if (child->parent == proc) {
            lock_all_process_aux(child, i, locked);
        }
    }
}

static void lock_all_process(void)
{
    char locked[STATIC_PROC_NUM];
    memset(locked, 0, sizeof(locked));

    lock_all_process_aux(get_init_process(), 0, locked);
    lock_rest_process(locked);
}

static void release_all_process(void)
{
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        release_spin_lock(&proc_set[i].lock);
    }
}

uint64 count_proc_num(void)
{
    lock_all_process();
    int n = 0;
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        if (proc_set[i].status != UNUSED) {
            n++;
        }
    }
    release_all_process();

    return n;
}
