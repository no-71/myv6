#ifndef CPUS_H_
#define CPUS_H_

#include "config/basic_config.h"
#include "config/basic_types.h"
#include "riscv/regs.h"

struct cpu {
    uint64 origin_ie;
    int introff_n;
};

extern int panicked;

extern struct cpu cpus[NCPUS];

static inline int64 cpu_id() { return r_tp(); }

static inline struct cpu *my_cpu() { return &cpus[cpu_id()]; }

#endif
