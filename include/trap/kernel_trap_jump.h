#ifndef KERNEL_TRAP_JUMP_H_
#define KERNEL_TRAP_JUMP_H_

#include "config/basic_types.h"

void kernel_trap_entry(void);
void kernel_trap_ret_end(uint64 origin_sp);

#endif
