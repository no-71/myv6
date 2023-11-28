#include "scheduler/scheduler.h"
#include "cpus.h"
#include "lock/big_kernel_lock.h"
#include "lock/spin_lock.h"
#include "process/process.h"
#include "riscv/regs.h"
#include "scheduler/swtch.h"
#include "trap/introff.h"
#include "util/kprint.h"

void scheduler(void)
{
    int has_runable_proc = 0;

    while (1) {
        intron();

        has_runable_proc = 0;
        for (int i = 0; i < STATIC_PROC_NUM; i++) {
            struct process *proc = &proc_set[i];
            if (proc->status != RUNABLE) {
                continue;
            }

            has_runable_proc = 1;
            introff();
            push_introff();
            struct cpu *mycpu = my_cpu();

            mycpu->my_proc = proc;
            proc->status = RUNNING;
            swtch(&mycpu->scheduler_context, &proc->proc_context);
            mycpu->my_proc = NULL;
            pop_introff();
            intron();
        }

        if (has_runable_proc == 0) {
            release_spin_lock(&big_kernel_lock);
            for (volatile int i = 0; i < 10000; i++) {
            }
            acquire_spin_lock(&big_kernel_lock);
        }
    }
}
