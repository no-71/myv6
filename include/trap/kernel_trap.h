#ifndef KERNEL_TRAP_H_
#define KERNEL_TRAP_H_

#include "config/basic_types.h"

void kernel_trap_init_hart(void);
void kernel_trap_handler(void);
void kernel_trap_ret(uint64 origin_sp, uint64 sepc);

#endif
