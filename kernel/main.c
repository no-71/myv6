/**
 * 2023/12/02 01:10, I think we should just pause here for once.
 * 2025/03/30 23:08, Restart, for graduation project.
 * 2025/04/13 16:41, Graduation project finished. Wonder when I will come back again.
 */

#include "cpus.h"
#include "process/proc_group.h"
#include "util/list.h"

#include "driver/virtio.h"
#include "fs/defs.h"
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
        init_cpus(); // cpu, claim my cpu exist
        init_my_cpu();

        console_init(); // uart, char io
        kprintf_init();
        kprintf("myv6 starts booting\n");
        kprintf("hart %d start\n", cpu_id());

        kalloc_init(); // mem alloc and kernel page table
        kvm_init();
        kvm_init_hart();

        binit(); // file system
        iinit();
        fileinit();
        virtio_disk_init();

        process_init(); // process and proc group
        proc_group_init();
        setup_init_proc(); // also do proc group init hart here

        plic_init(); // io intr and kernel trap
        plic_init_hart();
        kernel_trap_init_hart();

        __sync_synchronize();
        kernel_init_finish = 1;
    } else {
        while (kernel_init_finish == 0) {
        }
        __sync_synchronize();

        kprintf("hart %d start\n", cpu_id());

        init_my_cpu();
        proc_group_init_hart();
        kvm_init_hart();
        plic_init_hart();
        kernel_trap_init_hart();
    }

    // start scheduler, running process
    scheduler();
}
