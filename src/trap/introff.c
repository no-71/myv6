#include "trap/introff.h"
#include "cpus.h"
#include "riscv/regs.h"
#include "util/kprint.h"

void push_introff()
{
    uint64 origin_ie = get_intr();
    introff();

    struct cpu *cur_cpu = my_cpu();
    if (cur_cpu->introff_n == 0) {
        cur_cpu->origin_ie = origin_ie;
    }
    cur_cpu->introff_n++;
}

void pop_introff()
{
    struct cpu *cur_cpu = my_cpu();

    if (get_intr()) {
        panic("pop_introff when status(sie) != 0");
    }
    if (cur_cpu->introff_n <= 0) {
        panic("pop_introff without push_introff before");
    }

    cur_cpu->introff_n--;
    if (cur_cpu->introff_n == 0 && cur_cpu->origin_ie) {
        intron();
    }
}
