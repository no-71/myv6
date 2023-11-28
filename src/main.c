#include "io/console/console.h"
#include "lock/spin_lock.h"
#include "process/process.h"
#include "riscv/regs.h"
#include "scheduler/scheduler.h"
#include "trap/kernel_trap.h"
#include "util/kprint.h"
#include "vm/kalloc.h"
#include "vm/kvm.h"

volatile int kernel_init_finish = 0;

void main(void)
{
    if (r_tp() == 0) {
        console_init();

        kprintf_init();
        kprintf("tinyos starts booting\n");
        kprintf("cpu %d start\n", r_tp());

        kalloc_init();
        kvm_init();
        kvm_init_hart();

        init_process();
        setup_init_proc();

        init_kernel_trap_hart();

        __sync_synchronize();
        kernel_init_finish = 1;
    } else {
        while (kernel_init_finish == 0) {
        }
        __sync_synchronize();

        kprintf("cpu %d start\n", r_tp());

        kvm_init_hart();
        init_kernel_trap_hart();
    }

    // start scheduler, running process
    scheduler();
}
