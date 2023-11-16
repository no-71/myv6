#include "cpus.h"
#include "config/basic_config.h"
#include "riscv/regs.h"

// debug
int panicked;

struct cpu cpus[MAX_CPU_NUM];
