#include "trap/introff.h"
#include "cpus.h"
#include "riscv/regs.h"
#include "util/kprint.h"

void push_introff()
{
    struct cpu *cur_cpu = my_cpu();

    if (cur_cpu->introff_n != 0) {
        cur_cpu->introff_n++;
        return;
    }

    uint64 ss = r_sstatus();
    cur_cpu->origin_ie = ss & XSTATUS_SIE;
    w_sstatus(ss & (~XSTATUS_SIE));
    cur_cpu->introff_n++;
}

void pop_introff()
{
    struct cpu *cur_cpu = my_cpu();

    if (cur_cpu->introff_n <= 0) {
        panic("pop_introff without push_introff before");
    }

    cur_cpu->introff_n--;
    if (cur_cpu->introff_n == 0 && cur_cpu->origin_ie) {
        w_sstatus(r_sstatus() | XSTATUS_SIE);
    }
}
