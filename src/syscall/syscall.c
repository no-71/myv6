#include "syscall/syscall.h"
#include "cpus.h"
#include "fs/defs.h"
#include "io/console/console.h"
#include "process/process.h"
#include "syscall/uvm.h"
#include "trap/introff.h"
#include "util/kprint.h"
#include "util/string.h"
#include "vm/vm.h"

typedef uint64 (*syscall_fn_t)(struct process *);

uint64 get_arg_n(struct trap_frame *tf, int i) { return *(&tf->a0 + i); }

// uint64 syscall_putc(struct process *proc)
// {
//     console_putc(proc->proc_trap_frame->a0);
//     return 0;
// }

// uint64 syscall_puts(struct process *proc)
// {
//     char str[64];
//     memset(str, 0, 64);
//     uint64 uva_str = get_arg_n(proc->proc_trap_frame, 0);
//
//     int err = copy_in_str(proc->proc_pgtable, uva_str, str, sizeof(str));
//     if (err) {
//         return -1;
//     }
//
//     console_write(str, strlen(str));
//
//     return 0;
// }

uint64 syscall_intokernel(struct process *proc) { return -1; }

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

// uint64 syscall_getc(struct process *proc) { return console_getc(); }

#define SYSTABLE_ELEM(NAMEC, NAMEL) [SYSCALL_##NAMEC] = syscall_##NAMEL

syscall_fn_t syscall_table[SYSCALL_NUM] = {
    SYSTABLE_ELEM(INTOK, intokernel), SYSTABLE_ELEM(1EMP, intokernel),
    SYSTABLE_ELEM(SBRK, sbrk),        SYSTABLE_ELEM(FORK, fork),
    SYSTABLE_ELEM(EXEC, exec),        SYSTABLE_ELEM(GETPID, getpid),
    SYSTABLE_ELEM(EXIT, exit),        SYSTABLE_ELEM(WAIT, wait),
    SYSTABLE_ELEM(KILL, kill),        SYSTABLE_ELEM(BRK, brk),
    SYSTABLE_ELEM(10EMP, intokernel), SYSTABLE_ELEM(CHDIR, chdir),
    SYSTABLE_ELEM(CLOSE, close),      SYSTABLE_ELEM(DUP, dup),
    SYSTABLE_ELEM(FSTAT, fstat),      SYSTABLE_ELEM(LINK, link),
    SYSTABLE_ELEM(MKDIR, mkdir),      SYSTABLE_ELEM(MKNOD, mknod),
    SYSTABLE_ELEM(OPEN, open),        SYSTABLE_ELEM(PIPE, pipe),
    SYSTABLE_ELEM(READ, read),        SYSTABLE_ELEM(UNLINK, unlink),
    SYSTABLE_ELEM(WRITE, write),
};

int handle_db_syscall(struct process *proc, uint64 syscall_id)
{
    uint64 ret_val = 0;
    if (syscall_id == DB_SYSCALL_COUNT_PROC_NUM) {
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
    if (syscall_id > SYSCALL_MAX_ID) {
        return handle_db_syscall(proc, syscall_id);
    }

    uint64 ret_val = syscall_table[syscall_id](proc);
    tf->a0 = ret_val;
    return 0;
}
