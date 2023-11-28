#ifndef INTROFF_H_
#define INTROFF_H_

#include "config/basic_types.h"
#include "riscv/regs.h"

void push_introff();
void pop_introff();

static inline uint64 get_intr() { return r_sstatus() & XSTATUS_SIE; }
static inline void intron() { w_sstatus(r_sstatus() | XSTATUS_SIE); }
static inline void introff() { w_sstatus(r_sstatus() & (~XSTATUS_SIE)); }

static inline uint64 pop_introff_return_val(uint64 val)
{
    pop_introff();
    return val;
}

#endif
