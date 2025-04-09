#include "process/cpu_message.h"
#include "lock/spin_lock.h"
#include "scheduler/sleep.h"
#include "util/kprint.h"

const struct cpu_message EMPTY = { EMP_MESSAGE, NULL };

struct {
    struct cpu_message box[CPU_MESSAGE_BOX_SIZE];
    uint64 box_beg, box_end;
    __attribute__((aligned(8))) uint64 size;
    struct spin_lock lock;
} um_box;

void init_cpu_message(void) { init_spin_lock(&um_box.lock); }

static int box_empty(void) { return um_box.size == 0; }

static int box_full(void) { return um_box.size == CPU_MESSAGE_BOX_SIZE; }

uint64 no_message_atomic(void)
{
    return __atomic_load_n(&um_box.size, __ATOMIC_RELAXED) == 0;
}

struct cpu_message get_cpu_message(void)
{
    acquire_spin_lock(&um_box.lock);
    if (box_empty()) {
        release_spin_lock(&um_box.lock);
        return EMPTY;
    }

    struct cpu_message m = um_box.box[um_box.box_beg % CPU_MESSAGE_BOX_SIZE];
    um_box.box_beg++;
    __atomic_fetch_sub(&um_box.size, 1, __ATOMIC_RELAXED);
    release_spin_lock(&um_box.lock);
    if (m.type != NEED_CPU && m.type != NEED_CPU_FLEX) {
        PANIC_FN("get unknow message");
    }
    return m;
}

void resend_cpu_message(struct cpu_message m)
{
    if (m.type != EMP_MESSAGE) {
        send_cpu_message(m);
    }
}

void send_cpu_message(struct cpu_message m)
{
    acquire_spin_lock(&um_box.lock);
    if (box_full()) {
        PANIC_FN("cpu message box full");
    }

    um_box.box[um_box.box_end % CPU_MESSAGE_BOX_SIZE] = m;
    um_box.box_end++;
    __atomic_add_fetch(&um_box.size, 1, __ATOMIC_RELAXED);
    release_spin_lock(&um_box.lock);
}
