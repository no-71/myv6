#include "cpus.h"
#include "riscv/regs.h"

// debug
int panicked;

struct cpu cpus[NCPUS];
