#include "scheduler/scheduler.h"
#include "config/basic_config.h"
#include "cpus.h"
#include "lock/spin_lock.h"
#include "process/proc_group.h"
#include "process/process.h"
#include "riscv/regs.h"
#include "scheduler/swtch.h"
#include "trap/introff.h"
#include "util/kprint.h"
#include "util/list.h"

// uint64 handle_proc[MAX_CPU_NUM];
//
// void handle_proc_count(void)
// {
//     handle_proc[cpu_id()]++;
//     if (handle_proc[cpu_id()] % 1000 == 0) {
//         kprintf("cpu %d has handled %d proc\n", cpu_id(),
//                 (int)handle_proc[cpu_id()]);
//     }
// }

static struct process *get_runnable_proc_with_lock(struct proc_group *pgroup)
{
    struct process *proc;
    list_for_each_entry(proc, &pgroup->procs_head, pgroup_list)
    {
        acquire_spin_lock(&proc->lock);
        if (proc->status == RUNABLE) {
            return proc;
        }
        release_spin_lock(&proc->lock);
    }
    return NULL;
}

static int run_process(void)
{
    struct cpu *mycpu = my_cpu_unsafe();
    if (mycpu->pgroup_id == DEFAULT_PGROUP_ID) {
        handle_cpu_acquire(); // mycpu->pgroup_id may change here
    }

    struct proc_group *pgroup = get_proc_group(mycpu->pgroup_id);
    acquire_spin_lock(&pgroup->lock);
    if (pgroup->id == -1) {
        PANIC_FN("cpu try to run in empty proc group");
    }
    if (mycpu->pgroup_id != DEFAULT_PGROUP_ID &&
        cpu_leave_pgroup_if_empty() == 0) {
        release_spin_lock(&pgroup->lock);
        return 0;
    }

    struct process *proc = get_runnable_proc_with_lock(pgroup);
    if (proc == NULL) {
        release_spin_lock(&pgroup->lock);
        return -1;
    }
    proc_move_to_tail(pgroup, proc);
    release_spin_lock(&pgroup->lock);

    // we already got proc lock
    mycpu->origin_ie = 0;
    mycpu->my_proc = proc;
    proc->status = RUNNING;
    swtch(&mycpu->scheduler_context, &proc->proc_context);
    mycpu->my_proc = NULL;
    release_spin_lock(&proc->lock);
    return 0;
}

void scheduler(void)
{
    intron();
    while (1) {
        int no_runable_proc = run_process();
        intron();

        if (no_runable_proc) {
            // intr has already enable
            asm volatile("wfi");
        }
    }
}
