#ifndef TRAMPOLINE_H_
#define TRAMPOLINE_H_

#include "config/basic_types.h"
#include "vm/vm.h"

extern void user_trap_entry(void);

typedef void user_trap_ret_end_t(uint64 pgtable);

extern user_trap_ret_end_t user_trap_ret_end;

#endif
