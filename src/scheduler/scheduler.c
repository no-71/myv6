#include "scheduler/scheduler.h"
#include "config/basic_config.h"
#include "cpus.h"
#include "lock/spin_lock.h"
#include "process/process.h"
#include "riscv/regs.h"
#include "scheduler/swtch.h"
#include "trap/introff.h"
#include "util/kprint.h"

uint64 handle_proc[MAX_CPU_NUM];

void handle_proc_count(void)
{
    handle_proc[cpu_id()]++;
    if (handle_proc[cpu_id()] % 1000 == 0) {
        kprintf("cpu %d has handled %d proc\n", cpu_id(),
                (int)handle_proc[cpu_id()]);
    }
}

void scheduler(void)
{
    int has_runable_proc = 0;

    while (1) {
        intron();
        has_runable_proc = 0;
        for (int i = 0; i < STATIC_PROC_NUM; i++) {
            struct process *proc = &proc_set[i];

            acquire_spin_lock(&proc->lock);
            if (proc->status != RUNABLE) {
                release_spin_lock(&proc->lock);
                continue;
            }

            has_runable_proc = 1;
            // handle_proc_count();

            struct cpu *mycpu = my_cpu();
            mycpu->origin_ie = 0;
            mycpu->my_proc = proc;
            proc->status = RUNNING;
            swtch(&mycpu->scheduler_context, &proc->proc_context);
            mycpu->my_proc = NULL;
            release_spin_lock(&proc->lock);
            intron();
        }

        if (has_runable_proc == 0) {
            // asm volatile("wfi");
        }
    }
}
