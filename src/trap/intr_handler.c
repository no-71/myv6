#include "trap/intr_handler.h"
#include "cpus.h"
#include "riscv/regs.h"
#include "riscv/trap_handle.h"
#include "scheduler/swtch.h"
#include "trap/introff.h"
#include "util/kprint.h"

void switch_to_scheduler(void)
{
    struct process *proc = my_proc();
    struct cpu *mycpu = my_cpu();
    if (r_sstatus() & XSTATUS_SIE) {
        PANIC_FN("sstatus(sie) != 0");
    }
    if (mycpu->introff_n != 1) {
        PANIC_FN("introff_n != 1");
    }

    uint64 origin_ie = mycpu->origin_ie;
    mycpu->origin_ie = 0;
    // kprintf("proc %d swtch\n", my_proc()->pid);
    swtch(&proc->proc_context, &mycpu->scheduler_context);
    // kprintf("proc %d come back\n", my_proc()->pid);
    my_cpu()->origin_ie = origin_ie;
}

void yield(uint64 status)
{
    push_introff();

    my_proc()->status = status;
    switch_to_scheduler();

    pop_introff();
}

void intr_handler(uint64 scause)
{
    if (scause == SCAUSE_SSI) {
        w_sip(r_sip() & (~XIP_SSIP));

        if (my_proc() == NULL) {
            return;
        }
        yield(RUNABLE);
    } else {
        kprintf("unexpect intrrupt:\n scause: %p\n stval: %p\n", scause,
                r_stval());
    }
}
