#include "trap/kernel_trap.h"
#include "riscv/regs.h"
#include "util/kprint.h"

void init_kernel_trap(void) { w_stvec((uint64)kernel_trap_handler); }

void kernel_trap_handler() { kprintf("meet kernel trap"); }
