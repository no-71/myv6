#ifndef INTR_HANDLER_H_
#define INTR_HANDLER_H_

#include "config/basic_types.h"
void switch_to_scheduler(void);
void yield(uint64 status);
void intr_handler(uint64 scause);

#endif
