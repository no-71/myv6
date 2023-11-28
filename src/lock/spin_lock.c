#include "lock/spin_lock.h"
#include "riscv/regs.h"
#include "riscv/vm_system.h"
#include "trap/introff.h"
#include "util/kprint.h"

void check_not_hold(struct spin_lock *lock)
{
    if (lock->cpu_id == r_tp()) {
        PANIC_FN("acquire holding lock");
    }
}

void check_hold(struct spin_lock *lock)
{
    if (lock->locked != 1) {
        PANIC_FN("release spin_lock never acquired");
    }
    if (lock->cpu_id != r_tp()) {
        PANIC_FN("release spin_lock not hold by this cpu");
    }
}

void acquire_spin_lock(struct spin_lock *lock)
{
    check_not_hold(lock);

    push_introff();
    while (__sync_lock_test_and_set(&lock->locked, 1)) {
    }
    __sync_synchronize();

    // lock acquire
    lock->cpu_id = r_tp();
}

void release_spin_lock(struct spin_lock *lock)
{
    check_hold(lock);
    lock->cpu_id = -1;

    __sync_synchronize();
    __sync_lock_release(&lock->locked);
    // lock release
    pop_introff();
}
