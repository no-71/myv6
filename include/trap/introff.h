#ifndef INTROFF_H_
#define INTROFF_H_

#include "riscv/regs.h"

void push_introff();
void pop_introff();

static inline void intron() { w_sstatus(r_sstatus() | XSTATUS_SIE); }
static inline void introff() { w_sstatus(r_sstatus() & (~XSTATUS_SIE)); }

#endif
