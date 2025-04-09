#ifndef INTR_HANDLER_H_
#define INTR_HANDLER_H_

#include "config/basic_types.h"

extern struct spin_lock tick_lock;
extern uint64 tick_count;

void intr_init(void);

uint64 get_ticks(void);

void switch_to_scheduler(void);

void yield(uint64 status);

void intr_handler(uint64 scause);

#endif
