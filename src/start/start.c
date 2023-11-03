#include "config/basic_config.h"
#include "config/basic_types.h"
#include "riscv/clint.h"
#include "riscv/csrs.h"

extern void time_trap_vec(void);
extern void main(void);

char kstack_for_scheduler[4096];
uint64 mtime_setting[6];

void set_m_csrs(void);
void setup_time_trap(void);

void start(void)
{
    set_m_csrs();
    setup_time_trap();
    asm volatile("mret");
}

void set_m_csrs(void)
{
    uint64 ms = r_mstatus();
    // hard ward config
    ms &= ~XSTATUS_MPRV;
    ms &= ~XSTATUS_SUM;
    ms &= ~XSTATUS_MXR;
    ms &= ~XSTATUS_TVM;
    ms &= ~XSTATUS_TW;
    ms &= ~XSTATUS_TSR;
    // prepare for mret to s-mode
    ms |= XSTATUS_MPP_0;
    ms &= ~XSTATUS_MPP_1;
    w_mstatus(ms);

    // ret to main in s-mode
    w_mepc((uint64)main);

    // disable paging for s-mode
    w_satp(0x0);

    // disable interrput for s-mode
    uint64 ss = r_sstatus();
    ss &= ~XSTATUS_SIE;
    w_sstatus(ss);

    w_medeleg(0xffff);
    w_mideleg(0xffff);
}

void setup_time_trap(void)
{
    // prepare for mret to s-mode
    uint64 ms = r_mstatus();
    ms |= XSTATUS_MPIE;
    w_mstatus(ms);

    uint64 mie = r_mie();
    mie |= XIE_MTIE;
    w_mie(mie);

    uint64 hart_id = r_tp();
    // ask clint for starting time interrput
    uint64 next_trap_time = READ_REG(CLINT_MTIME) + TIME_TRAP_INTERVAL;
    WRITE_REG(CLINT_MTIMECMP(hart_id), next_trap_time);

    // information for timetrap handler to use
    // [0] = mtimecmp
    // [1] = mtime
    // [2] = interval of the time trap
    // [3..5] = save registers in time_trap_handler
    mtime_setting[0] = CLINT_MTIMECMP(hart_id);
    mtime_setting[1] = CLINT_MTIME;
    mtime_setting[2] = TIME_TRAP_INTERVAL;
    w_mscratch((uint64)mtime_setting);

    w_mtvec((uint64)time_trap_vec);
}
