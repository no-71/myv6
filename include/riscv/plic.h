#ifndef PLIC_H_
#define PLIC_H_

#include "config/basic_types.h"
#include "cpus.h"
#include "riscv/regs.h"

#define PLIC_BASE 0x0c000000L
#define PLIC_SIZE 0x4000000
#define PLIC_END (PLIC_BASE + PLIC_SIZE)

#define PLIC_I_SOURCE_ENABLE(IS) (PLIC_BASE + IS * 4)
#define PLIC_S_I_ENABLE(HART_ID) (PLIC_BASE + 0x2080 + (HART_ID)*0x100)
#define PLIC_S_PRIORITY(HART_ID) (PLIC_BASE + 0x201000 + (HART_ID)*0x2000)
#define PLIC_S_CLAIM(HART_ID) (PLIC_BASE + 0x201004 + (HART_ID)*0x2000)

void plic_init(void);
void plic_init_hart(void);

static inline uint32 plic_claim(void)
{
    uint32 irq = GET_REG(uint32, PLIC_S_CLAIM(cpu_id()));
    return irq;
}

static inline void plic_complete(int irq)
{
    GET_REG(uint32, PLIC_S_CLAIM(cpu_id())) = irq;
}

#endif
