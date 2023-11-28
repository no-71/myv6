#ifndef SPIN_LOCK_H_
#define SPIN_LOCK_H_

#include "config/basic_types.h"

struct spin_lock {
    volatile uint locked;

    // debug
    uint64 cpu_id;
};

void acquire_spin_lock(struct spin_lock *lock);
void release_spin_lock(struct spin_lock *lock);

static inline void init_spin_lock(struct spin_lock *lock)
{
    lock->locked = 0;
    lock->cpu_id = -1;
}

#endif
