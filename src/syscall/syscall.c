#include "syscall/syscall.h"
#include "cpus.h"
#include "io/console/console.h"
#include "process/process.h"
#include "util/kprint.h"
#include "vm/vm.h"

typedef uint64 (*syscall_fn_t)(struct process *);

uint64 get_arg_n(struct trap_frame *tf, int i) { return *(&tf->a0 + i); }

uint64 syscall_putc(struct process *proc)
{
    console_kputc(my_proc()->proc_trap_frame->a0);
    return 0;
}

uint64 syscall_puts(struct process *proc)
{

    char str[64];
    uint64 uva_str = get_arg_n(proc->proc_trap_frame, 0);

    int err = copy_in_str(proc->proc_pgtable, uva_str, str, sizeof(str));
    if (err) {
        return 1;
    }

    kprintf(str);

    return 0;
}

syscall_fn_t syscall_table[SYSCALL_NUM] = {
    [SYSCALL_PUTC] = syscall_putc, [SYSCALL_PRINT] = syscall_puts
};

void handle_syscall()
{
    struct process *proc = my_proc();
    struct trap_frame *tf = proc->proc_trap_frame;

    uint64 syscall_id = get_arg_n(tf, 7);
    if (syscall_id > SYSCALL_MAX_ID) {
        tf->a0 = 1;
        return;
    }

    int ret_val = syscall_table[syscall_id](proc);
    tf->a0 = ret_val;
}
