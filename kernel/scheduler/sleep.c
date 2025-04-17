#include "scheduler/sleep.h"
#include "cpus.h"
#include "lock/spin_lock.h"
#include "process/process.h"
#include "trap/intr_handler.h"
#include "trap/introff.h"
#include "util/kprint.h"

void sleep(struct spin_lock *lock, void *chain)
{
    struct process *proc = my_proc();
    if (proc->chain != NULL) {
        PANIC_FN("sleep when sleeping");
    }

    if (lock != &proc->lock) {
        acquire_spin_lock(&proc->lock);
        release_spin_lock(lock);
    }
    proc->status = SLEEP;
    proc->chain = chain;

    switch_to_scheduler();

    if (lock != &proc->lock) {
        release_spin_lock(&proc->lock);
        acquire_spin_lock(lock);
    }
}

void wake_up(void *chain)
{
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        struct process *proc = &proc_set[i];
        acquire_spin_lock(&proc->lock);
        if (proc->status == SLEEP && proc->chain == chain) {
            proc->status = RUNABLE;
            proc->chain = NULL;
        }
        release_spin_lock(&proc->lock);
    }
}
