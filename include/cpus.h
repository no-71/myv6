#ifndef CPUS_H_
#define CPUS_H_

#include "config/basic_config.h"
#include "config/basic_types.h"
#include "process/process.h"
#include "riscv/regs.h"

struct cpu {
    struct process *my_proc;
    struct context scheduler_context;

    uint64 origin_ie;
    int introff_n;
};

extern int panicked;

extern struct cpu cpus[MAX_CPU_NUM];

static inline int64 cpu_id(void) { return r_tp(); }

static inline struct cpu *my_cpu(void) { return &cpus[cpu_id()]; }

static inline struct process *my_proc(void) { return my_cpu()->my_proc; }

#endif
