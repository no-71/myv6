#include "riscv/csrs.h"

void tv(void)
{
    w_sip(0x0);
    asm volatile("sret");
}

void main(void)
{
    w_sstatus(r_sstatus() | XSTATUS_SIE);
    w_sie(r_sie() | XIE_SSIE);
    w_stvec((int64)tv);
    while (1) {
    }
}
