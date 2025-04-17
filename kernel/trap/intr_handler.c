#include "trap/intr_handler.h"
#include "cpus.h"
#include "driver/uart.h"
#include "driver/virtio.h"
#include "fs/defs.h"
#include "lock/spin_lock.h"
#include "riscv/plic.h"
#include "riscv/regs.h"
#include "riscv/trap_handle.h"
#include "scheduler/swtch.h"
#include "trap/introff.h"
#include "util/kprint.h"

struct spin_lock tick_lock;
__attribute__((aligned(8))) uint64 tick_count;

void intr_init(void) { init_spin_lock(&tick_lock); }

uint64 get_ticks(void)
{
    return __atomic_load_n(&tick_count, __ATOMIC_RELAXED);
}

void switch_to_scheduler(void)
{
    struct process *proc = my_proc();
    struct cpu *mycpu = my_cpu();
    if (r_sstatus() & XSTATUS_SIE) {
        PANIC_FN("sstatus(sie) != 0");
    }
    if (mycpu->introff_n != 1) {
        PANIC_FN("introff_n != 1");
    }

    uint64 origin_ie = mycpu->origin_ie;
    mycpu->origin_ie = 0;
    swtch(&proc->proc_context, &mycpu->scheduler_context);
    my_cpu()->origin_ie = origin_ie;
}

void yield(uint64 status)
{
    push_introff();
    struct process *proc = my_proc();
    pop_introff();

    acquire_spin_lock(&proc->lock);

    proc->status = status;
    switch_to_scheduler();

    release_spin_lock(&proc->lock);
}

void intr_handler(uint64 scause)
{
    if (scause == SCAUSE_SEI) {
        uint32 irq = plic_claim();
        if (irq == UART_IRQ) {
            uart_intr();
        } else if (irq == VIRTIO0_IRQ) {
            // kprintf("\n into vitio \n");
            virtio_disk_intr();
        } else if (irq) {
            kprintf("unexpect irq: %d\n", (int)irq);
        }

        if (irq) {
            plic_complete(irq);
        }
    } else if (scause == SCAUSE_SSI) {
        w_sip(r_sip() & (~XIP_SSIP));

        if (cpu_id() == 0) {
            acquire_spin_lock(&tick_lock);
            tick_count++;
            wakeup(&tick_count);
            release_spin_lock(&tick_lock);
        }

        if (my_proc() == NULL || is_exclusive_occupy(my_proc())) {
            return;
        }
        yield(RUNABLE);
    } else {
        kprintf("unexpect intrrupt:\n scause: %p\n stval: %p\n", scause,
                r_stval());
    }
}
