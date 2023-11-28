#include "trap/user_trap_handler.h"
#include "config/basic_types.h"
#include "cpus.h"
#include "lock/spin_lock.h"
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

    uint64 scause = r_scause();
    if (scause == SCAUSE_ECALL_FROM_U) {
        intron();
        proc->proc_trap_frame->sepc += 4;
        handle_syscall(proc);
    } else if (scause & SCAUSE_INTERRPUT_MASK) {
        intr_handler(scause);
    } else {
        kprintf("unexpect exception from user:\n scause: %p\n stval: %p\n "
                "spec: %p\n",
                scause, r_stval(), r_sepc());
        proc->killed = 1;
    }

    if (proc->killed == 1) {
        exit(proc, -1);
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
    w_sie(XIE_SSIE);
    w_sepc(proc->proc_trap_frame->sepc);

    // prepare for next user trap
    w_stvec(GET_TRAMPOLINE_FN_VA(user_trap_entry));
    w_sscratch(TRAPFRAME_BASE);
    proc->proc_trap_frame->cpu_id = r_tp();

    uint64 utrap_ret_end_va = GET_TRAMPOLINE_FN_VA(user_trap_ret_end);
    user_trap_ret_end_t *ret_end_fn = (user_trap_ret_end_t *)utrap_ret_end_va;
    ret_end_fn(MAKE_SATP((uint64)proc->proc_pgtable));
}
