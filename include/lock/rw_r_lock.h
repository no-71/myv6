#ifndef RW_R_LOCK_H_
#define RW_R_LOCK_H_

#include "lock/spin_lock.h"

/*
 * write & read lock, read first
 *
 * write_active == 0:
 *   read_count >=0, write_waiting >= 0
 *   reader read, writer wait, later reader wait
 * write_active == 1:
 *   read_count == 0, write_waiting >= 0
 *   writer write, later reader wait
 */
struct rw_r_lock {
    int read_count;        // active reader
    int write_active;      // is there writer writting?
    int write_wait_count;  // counts of reader that's waiting
    struct spin_lock lock; // guard all above
};

#endif
