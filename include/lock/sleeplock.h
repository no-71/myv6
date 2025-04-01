#ifndef SLEEPLOCK_H_
#define SLEEPLOCK_H_

#include "config/basic_types.h"
#include "lock/spin_lock.h"

// Long-term locks for processes
struct sleeplock {
    uint locked;        // Is the lock held?
    struct spin_lock lk; // spinlock protecting this sleep lock

    // For debugging:
    char *name; // Name of lock.
    int pid;    // Process holding lock
};

#endif
