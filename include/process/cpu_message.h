#ifndef CPU_MESSAGE_H_
#define CPU_MESSAGE_H_

#include "config/basic_config.h"
#include "config/basic_types.h"
#include "lock/spin_lock.h"

enum cpu_message_type { EMP_MESSAGE, NEED_CPU, NEED_CPU_FLEX };

struct cpu_message {
    enum cpu_message_type type;
    void *message;
};

#define CPU_MESSAGE_BOX_SIZE MAX_CPU_NUM

void init_cpu_message(void);

uint64 no_message_atomic(void);

struct cpu_message get_cpu_message(void);

void resend_cpu_message(struct cpu_message m);

void send_cpu_message(struct cpu_message m);

#endif
