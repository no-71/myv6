#ifndef UVM_H_
#define UVM_H_

#include "config/basic_types.h"
#include "process/process.h"

uint64 brk(struct process *proc, uint64 new_brk);
uint64 sbrk(struct process *proc, int64 increment);

#endif
