#include "trap/intr_handler.h"
#include "cpus.h"
#include "riscv/regs.h"
#include "riscv/trap_handle.h"
#include "scheduler/swtch.h"
#include "trap/introff.h"
#include "util/kprint.h"

void yield(void)
{
    push_introff();

    struct process *proc = my_proc();
    struct cpu *mycpu = my_cpu();
    if (mycpu->introff_n != 1) {
        PANIC_FN("introff_n != 1");
    }
    if (mycpu->origin_ie != 0) {
        PANIC_FN("origin_ie != 0");
    }

    proc->status = RUNABLE;
    kprintf("proc %d swtch\n", my_proc()->pid);
    swtch(&proc->proc_context, &mycpu->scheduler_context);
    proc->proc_trap_frame->cpu_id = r_tp();
    kprintf("proc %d come back\n", my_proc()->pid);

    pop_introff();
}

int intr_handler(uint64 scause)
{
    if (scause == SCAUSE_SSI) {
        w_sip(r_sip() & (~XIP_SSIP));

        if (my_proc() == NULL) {
            return 0;
        }
        yield();
    } else {
        return 1;
    }

    return 0;
}
