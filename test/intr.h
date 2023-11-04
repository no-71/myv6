#ifndef TEST_INTR_H_
#define TEST_INTR_H_

#include "riscv/regs.h"
#include "trap/introff.h"

static inline void tv() { asm volatile("sret"); }

static inline void intr_test()
{
    push_introff();
    while (1) {
    }
}

#endif
