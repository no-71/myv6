#include "trap/kernel_trap.h"
#include "riscv/regs.h"
#include "trap/kernel_trap_jump.h"
#include "util/kprint.h"

void init_kernel_trap_hart(void) { w_stvec((uint64)kernel_trap_entry); }

void kernel_trap_handler(void)
{
    kprintf("unexpect exception from kernel:\n scause: %p\n stval: %p\n "
            "spec: %p\n",
            r_scause(), r_stval(), r_sepc());
    panic("kernel trap\n");
}
