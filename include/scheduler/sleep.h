#ifndef SLEEP_H_
#define SLEEP_H_

#include "lock/spin_lock.h"

void sleep(struct spin_lock *lock, void *chain);
void wake_up(void *chain);

#endif
