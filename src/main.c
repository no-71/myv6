#include "io/console/console.h"
#include "process/process.h"
#include "riscv/regs.h"
#include "scheduler/scheduler.h"
#include "test/test_config.h"
#include "test/vm/kalloc_test.h"
#include "test/vm/kvm_test.h"
#include "trap/kernel_trap.h"
#include "trap/kernel_trap_jump.h"
#include "trap/user_trap_handler.h"
#include "util/kprint.h"
#include "vm/kalloc.h"
#include "vm/kvm.h"

void main(void)
{
    console_init();

    kprintf("hello, tinyos\n");
    kprintf("tinyos starts booting\n");
    kprintf("\n");

    kalloc_init();
    kvm_init();
    kvm_init_hart();

    init_process();
    setup_init_proc();

    init_kernel_trap_hart();

    // start scheduler, running process
    scheduler();
}
