#ifndef INTROFF_H_
#define INTROFF_H_

#include "config/basic_types.h"
#include "riscv/regs.h"

void push_introff(void);
void pop_introff(void);

static inline uint64 get_intr(void) { return r_sstatus() & XSTATUS_SIE; }
static inline uint64 intr_is_on(void) { return get_intr(); }
static inline void intron(void) { w_sstatus(r_sstatus() | XSTATUS_SIE); }
static inline void introff(void) { w_sstatus(r_sstatus() & (~XSTATUS_SIE)); }

static inline void enable_soft_intr(void) { w_sie(XIE_SSIE); }

static inline void enable_soft_n_external_intr(void)
{
    w_sie(XIE_SEIE | XIE_SSIE);
}

static inline uint64 pop_introff_return_val(uint64 val)
{
    pop_introff();
    return val;
}

#endif
