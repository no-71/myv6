#include "trap/user_trap_handler.h"
#include "config/basic_types.h"
#include "cpus.h"
#include "lock/spin_lock.h"
#include "process/proc_group.h"
#include "process/process.h"
#include "riscv/regs.h"
#include "riscv/trap_handle.h"
#include "riscv/vm_system.h"
#include "syscall/syscall.h"
#include "trap/intr_handler.h"
#include "trap/introff.h"
#include "trap/trampoline.h"
#include "util/kprint.h"
#include "vm/memory_layout.h"
#include "vm/vm.h"

void user_trap_handler(void)
{
    struct process *proc = my_proc();
    int killed = 0;

    uint64 scause = r_scause();
    if (scause == SCAUSE_ECALL_FROM_U) {
        proc->proc_trap_frame->sepc += 4;
        intron();
        int err = handle_syscall(proc);
        if (err) {
            killed = 1;
        }
    } else if (scause & SCAUSE_INTERRPUT_MASK) {
        intr_handler(scause);
    } else {
        kprintf("unexpect exception from user:\n    scause: %p stval: %p\n    "
                "spec: %p pid: %d\n",
                scause, r_stval(), r_sepc(), proc->pid);
        killed = 1;
    }

    if (killed || proc->killed) {
        exit(proc, -1);
    }

    if (panicked) {
        while (1) {
        }
    }

    user_trap_ret();
}

void user_trap_ret(void)
{
    introff();

    struct process *proc = my_proc();

    // prepare to return to user
    uint64 sstatus = r_sstatus();
    sstatus |= XSTATUS_SPIE;
    sstatus &= (~XSTATUS_SPP);
    w_sstatus(sstatus);
    if (is_exclusive_occupy(proc)) {
        enable_soft_intr(); // exclusive occupy proc, ignore device intr
    } else {
        enable_soft_n_external_intr(); // common proc, prepare intr
    }
    w_sepc(proc->proc_trap_frame->sepc);

    // prepare for next user trap
    w_stvec(GET_TRAMPOLINE_FN_VA(user_trap_entry));
    w_sscratch(TRAPFRAME_BASE);
    proc->proc_trap_frame->cpu_id = cpu_id();

    uint64 utrap_ret_end_va = GET_TRAMPOLINE_FN_VA(user_trap_ret_end);
    user_trap_ret_end_t *ret_end_fn = (user_trap_ret_end_t *)utrap_ret_end_va;
    ret_end_fn(MAKE_SATP((uint64)proc->proc_pgtable));
}
