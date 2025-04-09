#include "syscall/syscall.h"
#include "cpus.h"
#include "fs/defs.h"
#include "io/console/console.h"
#include "process/proc_group.h"
#include "process/process.h"
#include "syscall/uvm.h"
#include "trap/intr_handler.h"
#include "trap/introff.h"
#include "util/kprint.h"
#include "util/string.h"
#include "vm/vm.h"

typedef uint64 (*syscall_fn_t)(struct process *);

uint64 get_arg_n(struct trap_frame *tf, int i) { return *(&tf->a0 + i); }

uint64 kernelbreak() { return -1; }

uint64 syscall_kernelbreak(struct process *proc) { return kernelbreak(); }

uint64 syscall_sleep(struct process *proc)
{
    return proc_sys_sleep(get_arg_n(proc->proc_trap_frame, 0));
}

uint64 syscall_sbrk(struct process *proc)
{
    return sbrk(proc, get_arg_n(proc->proc_trap_frame, 0));
}

uint64 syscall_fork(struct process *proc) { return fork(proc); }

static int copy_in_argv(struct process *proc, int *argc, uint64 argv_uva,
                        char *argv[], char str_in_argv[][64])
{
    *argc = 0;
    for (int i = 0; i < 7; i++) {
        int err = copy_in(proc->proc_pgtable, argv_uva + sizeof(char *) * i,
                          &argv[i], sizeof(char *));
        if (err) {
            return -1;
        }
        if (argv[i] == 0) {
            *argc = i;
            break;
        }
    }

    if (argv[6] != 0) {
        return -1;
    }

    for (int i = 0; i < *argc; i++) {
        int err = copy_in_str(proc->proc_pgtable, (uint64)argv[i],
                              str_in_argv[i], 64);
        if (err) {
            return -1;
        }
        argv[i] = str_in_argv[i];
    }

    return 0;
}

uint64 syscall_exec(struct process *proc)
{
    char file[64];
    char str_in_argv[7][64];
    char *argv[7];
    memset(file, 0, sizeof(file));
    memset(str_in_argv, 0, sizeof(str_in_argv));
    memset(argv, 0, sizeof(argv));

    uint64 file_pchar_uva = get_arg_n(proc->proc_trap_frame, 0);
    uint64 argv_uva = get_arg_n(proc->proc_trap_frame, 1);

    int err =
        copy_in_str(proc->proc_pgtable, file_pchar_uva, file, sizeof(file));
    if (err) {
        return -1;
    }

    if ((char *)argv_uva == NULL) {
        err = exec(proc, file, 0, NULL, NULL);
    } else {
        int argc;
        int err = copy_in_argv(proc, &argc, argv_uva, argv, str_in_argv);
        if (err) {
            return -1;
        }

        err = exec(proc, file, argc, argv, (char *)str_in_argv);
    }

    return err ? -1 : proc->proc_trap_frame->a0;
}

uint64 syscall_getpid(struct process *proc) { return proc->pid; }

uint64 syscall_exit(struct process *proc)
{
    // never return
    exit(proc, get_arg_n(proc->proc_trap_frame, 0));
}

uint64 syscall_wait(struct process *proc)
{
    return wait(proc, get_arg_n(proc->proc_trap_frame, 0));
}

uint64 syscall_kill(struct process *proc)
{
    return kill(get_arg_n(proc->proc_trap_frame, 0));
}

uint64 syscall_brk(struct process *proc)
{
    return brk(proc, get_arg_n(proc->proc_trap_frame, 0));
}

uint64 syscall_time(struct process *proc)
{
    uint64 uva = get_arg_n(proc->proc_trap_frame, 0);
    uint64 ticks = get_ticks();
    if ((void *)uva != NULL) {
        copyout(proc->proc_pgtable, uva, (char *)&ticks, sizeof(uint64));
    }
    return ticks;
}

uint64 syscall_get_pg_id(struct process *proc) { return get_pgroup_id(); }

uint64 syscall_create_pg(struct process *proc) { return create_pgroup(); }

uint64 syscall_enter_pg(struct process *proc)
{
    return enter_pgroup(get_arg_n(proc->proc_trap_frame, 0));
}

uint64 syscall_get_pg_cpus(struct process *proc) { return pgroup_cpu_count(); }

uint64 syscall_inc_pg_cpus(struct process *proc) { return inc_pgroup_cpus(); }

uint64 syscall_dec_pg_cpus(struct process *proc) { return dec_pgroup_cpus(); }

uint64 syscall_pg_procs_count(struct process *proc)
{
    return pgroup_procs_count();
}

uint64 syscall_proc_occupy_cpu(struct process *proc)
{
    return proc_occupy_cpu();
}

uint64 syscall_proc_release_cpu(struct process *proc)
{
    return proc_release_cpu();
}

uint64 syscall_inc_pg_cpus_flex(struct process *proc)
{
    return inc_proc_group_cpus_flex();
}

// uint64 syscall_getc(struct process *proc) { return console_getc(); }

#define SYSTABLE_ELEM(NAMEC, NAMEL) [SYSCALL_##NAMEC] = syscall_##NAMEL

syscall_fn_t syscall_table[SYSCALL_NUM] = {
    SYSTABLE_ELEM(KBREAK, kernelbreak), SYSTABLE_ELEM(SLEEP, sleep),
    SYSTABLE_ELEM(SBRK, sbrk),          SYSTABLE_ELEM(FORK, fork),
    SYSTABLE_ELEM(EXEC, exec),          SYSTABLE_ELEM(GETPID, getpid),
    SYSTABLE_ELEM(EXIT, exit),          SYSTABLE_ELEM(WAIT, wait),
    SYSTABLE_ELEM(KILL, kill),          SYSTABLE_ELEM(BRK, brk),
    SYSTABLE_ELEM(TIME, time),          SYSTABLE_ELEM(CHDIR, chdir),
    SYSTABLE_ELEM(CLOSE, close),        SYSTABLE_ELEM(DUP, dup),
    SYSTABLE_ELEM(FSTAT, fstat),        SYSTABLE_ELEM(LINK, link),
    SYSTABLE_ELEM(MKDIR, mkdir),        SYSTABLE_ELEM(MKNOD, mknod),
    SYSTABLE_ELEM(OPEN, open),          SYSTABLE_ELEM(PIPE, pipe),
    SYSTABLE_ELEM(READ, read),          SYSTABLE_ELEM(UNLINK, unlink),
    SYSTABLE_ELEM(WRITE, write),
};

#define SYSTABLE_PG_ELEM(NAMEC, NAMEL)                                         \
    [SYSCALL_##NAMEC - SYSCALL_PG_START_ID] = syscall_##NAMEL

syscall_fn_t syscall_pg_table[SYSCALL_PG_NUM] = {
    SYSTABLE_PG_ELEM(GET_PG_ID, get_pg_id),
    SYSTABLE_PG_ELEM(CREATE_PG, create_pg),
    SYSTABLE_PG_ELEM(ENTER_PG, enter_pg),
    SYSTABLE_PG_ELEM(GET_PG_CPUS, get_pg_cpus),
    SYSTABLE_PG_ELEM(INC_PG_CPUS, inc_pg_cpus),
    SYSTABLE_PG_ELEM(DEC_PG_CPUS, dec_pg_cpus),
    SYSTABLE_PG_ELEM(PG_PROCS_COUNT, pg_procs_count),
    SYSTABLE_PG_ELEM(PROC_OCCUPY_CPU, proc_occupy_cpu),
    SYSTABLE_PG_ELEM(PROC_RELEASE_CPU, proc_release_cpu),
    SYSTABLE_PG_ELEM(INC_PG_CPUS_FLEX, inc_pg_cpus_flex),
};

int handle_db_syscall(struct process *proc, uint64 syscall_id)
{
    uint64 ret_val = 0;
    if (syscall_id == SYSCALL_DB_COUNT_PROC_NUM) {
        ret_val = count_proc_num();
    } else {
        kprintf("unexpect syscall id: %d\n", syscall_id);
        return -1;
    }

    proc->proc_trap_frame->a0 = ret_val;
    return 0;
}

int handle_syscall(struct process *proc)
{
    struct trap_frame *tf = proc->proc_trap_frame;
    uint64 syscall_id = get_arg_n(tf, 7);
    syscall_fn_t *syscalls;

    if (syscall_id >= SYSCALL_PG_START_ID && syscall_id <= SYSCALL_PG_MAX_ID) {
        syscall_id -= SYSCALL_PG_START_ID;
        syscalls = syscall_pg_table;
    } else if (syscall_id <= SYSCALL_MAX_ID) {
        syscalls = syscall_table;
    } else {
        return handle_db_syscall(proc, syscall_id);
    }

    uint64 ret_val = syscalls[syscall_id](proc);
    tf->a0 = ret_val;
    return 0;
}
