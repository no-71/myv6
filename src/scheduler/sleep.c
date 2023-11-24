#include "scheduler/sleep.h"
#include "cpus.h"
#include "process/process.h"
#include "trap/intr_handler.h"
#include "trap/introff.h"
#include "util/kprint.h"

// call with push_introff() called
void sleep(void *chain)
{
    struct process *proc = my_proc();
    if (proc->chain != NULL) {
        PANIC_FN("sleep when sleeping");
    }

    proc->status = SLEEP;
    proc->chain = chain;
    switch_to_scheduler();
    proc->chain = NULL;
}

void wake_up(void *chain)
{
    push_introff();
    for (int i = 0; i < STATIC_PROC_NUM; i++) {
        struct process *proc = &proc_set[i];
        if (proc->status == SLEEP && proc->chain == chain) {
            proc->status = RUNNING;
            proc->chain = NULL;
        }
    }
    pop_introff();
}
