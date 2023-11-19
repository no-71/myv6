#include "scheduler/scheduler.h"
#include "cpus.h"
#include "process/process.h"
#include "riscv/regs.h"
#include "scheduler/swtch.h"
#include "trap/introff.h"
#include "util/kprint.h"

void scheduler(void)
{
    while (1) {
        if (r_sstatus() & XSTATUS_SIE) {
            PANIC_FN("SIE = 1 in scheduler");
        }

        for (int i = 0; i < STATIC_PROC_NUM; i++) {
            struct process *proc = &proc_set[i];
            if (proc->status != RUNABLE) {
                continue;
            }

            introff();
            push_introff();
            struct cpu *mycpu = my_cpu();
            mycpu->my_proc = proc;

            proc->status = RUNNING;
            swtch(&mycpu->scheduler_context, &proc->proc_context);

            mycpu->my_proc = NULL;
            pop_introff();
        }
    }
}
