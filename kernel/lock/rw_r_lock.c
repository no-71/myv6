#include "lock/rw_r_lock.h"
#include "lock/spin_lock.h"
#include "scheduler/sleep.h"
#include "util/kprint.h"

void init_rw_r_lock(struct rw_r_lock *rw)
{
    init_spin_lock(&rw->lock);
    rw->read_count = 0;
    rw->write_active = 0;
    rw->write_wait_count = 0;
}

void acquire_rw_r_read_lock(struct rw_r_lock *rw)
{
    acquire_spin_lock(&rw->lock);

    // if there is any writer, wait for it
    while (rw->write_active || rw->write_wait_count > 0) {
        sleep(&rw->lock, &rw->write_wait_count);
    }

    rw->read_count++;

    release_spin_lock(&rw->lock);
}

void acquire_rw_r_write_lock(struct rw_r_lock *rw)
{
    acquire_spin_lock(&rw->lock);

    rw->write_wait_count++;
    // if there is any reader or writer, wait for it
    while (rw->read_count > 0 || rw->write_active) {
        sleep(&rw->lock, &rw->read_count);
    }

    rw->write_active = 1;
    rw->write_wait_count--;

    release_spin_lock(&rw->lock);
}

void release_rw_r_read_lock(struct rw_r_lock *rw)
{
    acquire_spin_lock(&rw->lock);

    if (rw->write_active || rw->read_count == 0) {
        panic("read_unlock: we should be reading");
    }

    rw->read_count--;
    // writer waiting, wake up them
    if (rw->read_count == 0 && rw->write_wait_count > 0) {
        wake_up(&rw->read_count);
    }

    release_spin_lock(&rw->lock);
}

void release_rw_r_write_lock(struct rw_r_lock *rw)
{
    acquire_spin_lock(&rw->lock);

    if (rw->write_active == 0 || rw->read_count > 0) {
        panic("write_unlock: we should be writting");
    }

    rw->write_active = 0;
    if (rw->write_wait_count > 0) {
        // other writer waiting, wake up writer
        wake_up(&rw->read_count);
    } else {
        // no writer, try wake up reader
        wake_up(&rw->write_wait_count);
    }

    release_spin_lock(&rw->lock);
}
