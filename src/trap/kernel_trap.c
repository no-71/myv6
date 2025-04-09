#include "trap/kernel_trap.h"
#include "cpus.h"
#include "riscv/regs.h"
#include "riscv/trap_handle.h"
#include "trap/intr_handler.h"
#include "trap/kernel_trap_jump.h"
#include "util/kprint.h"

void kernel_trap_init_hart(void)
{
    if (cpu_id() == 0) {
        intr_init();
    }

    w_stvec((uint64)kernel_trap_entry);
    w_sie(XIE_SSIE | XIE_SEIE);
}

void kernel_trap_handler(void)
{
    uint64 origin_sp = r_sscratch();
    uint64 sepc = r_sepc();

    if ((r_sstatus() & XSTATUS_SPP) == 0) {
        PANIC_FN("trap not from kernel");
    }

    uint64 scause = r_scause();
    if (scause & SCAUSE_INTERRPUT_MASK) {
        // kprintf("kernel trap time intr\n");
        intr_handler(scause);
    } else {
        kprintf("unexpect exception from kernel:\n scause: %p\n stval: %p\n "
                "spec: %p\n",
                r_scause(), r_stval(), r_sepc());
        panic("kernel trap\n");
    }

    kernel_trap_ret(origin_sp, sepc);
}

void kernel_trap_ret(uint64 origin_sp, uint64 sepc)
{
    uint64 sstatus = r_sstatus();
    sstatus |= XSTATUS_SPP;
    sstatus |= XSTATUS_SPIE;
    w_sstatus(sstatus);
    w_sepc(sepc);

    kernel_trap_ret_end(origin_sp);
}
