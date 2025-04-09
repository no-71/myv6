#ifndef CPUS_H_
#define CPUS_H_

#include "config/basic_config.h"
#include "config/basic_types.h"
#include "process/proc_group.h"
#include "process/process.h"
#include "riscv/regs.h"
#include "trap/introff.h"

struct cpu {
    int cpu_id;
    int pgroup_id;
    struct process *my_proc;
    struct context scheduler_context;

    uint64 origin_ie;
    int introff_n;
};

extern int panicked;

extern struct cpu cpus[MAX_CPU_NUM];

static inline uint64 cpu_id(void)
{
    if (intr_is_on()) {
        while (1) {
        }
    }
    return r_tp();
}

static inline uint64 cpu_id_unsafe(void) { return r_tp(); }

static inline struct cpu *my_cpu(void) { return &cpus[cpu_id()]; }

static inline struct cpu *my_cpu_unsafe(void) { return &cpus[cpu_id_unsafe()]; }

static inline struct process *my_proc(void)
{
    push_introff();
    struct process *mine = my_cpu()->my_proc;
    pop_introff();
    return mine;
}

static inline void init_cpus(void)
{
    for (int i = 0; i < MAX_CPU_NUM; i++) {
        cpus[i].cpu_id = -1;
        cpus[i].pgroup_id = -1;
    }
}

static inline void init_my_cpu(void)
{
    my_cpu()->cpu_id = cpu_id();
    my_cpu()->pgroup_id = -1;
}

#endif
