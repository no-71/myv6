/**
 * 2023/12/02 1:10, I think we should just pause here for once.
 */

#include "io/console/console.h"
#include "process/process.h"
#include "riscv/plic.h"
#include "riscv/regs.h"
#include "scheduler/scheduler.h"
#include "trap/kernel_trap.h"
#include "util/kprint.h"
#include "vm/kalloc.h"
#include "vm/kvm.h"

volatile int kernel_init_finish = 0;

void main(void)
{
    if (cpu_id() == 0) {
        console_init();

        kprintf_init();
        kprintf("tinyos starts booting\n");
        kprintf("hart %d start\n", cpu_id());

        kalloc_init();
        kvm_init();
        kvm_init_hart();

        process_init();
        setup_init_proc();

        plic_init();
        plic_init_hart();

        kernel_trap_init_hart();

        __sync_synchronize();
        kernel_init_finish = 1;
    } else {
        while (kernel_init_finish == 0) {
        }
        __sync_synchronize();

        kprintf("hart %d start\n", cpu_id());

        kvm_init_hart();
        plic_init_hart();
        kernel_trap_init_hart();
    }

    // start scheduler, running process
    scheduler();
}
