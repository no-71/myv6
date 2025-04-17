#ifndef PROC_GROUP_H_
#define PROC_GROUP_H_

#include "config/basic_config.h"
#include "lock/spin_lock.h"
#include "process/process.h"
#include "util/list.h"

#define MAX_PROC_GROUP_NUM MAX_CPU_NUM
#define DEFAULT_PGROUP_ID 0

/* invariants:
 * id == -1: empty group, all members inited.
 *
 * id != -1:
 *   procs_head not empty:
 *     there are procs and cpus in the group.
 *   procs_head empty:
 *     1-n cpus in the group, only last cpus in the group can free it.
 *
 * in every time we can only acess my own proc and cpu's proc id.
 *
 * lock acquire sequence:
 * group lock -> a proc lock in group
 */
struct proc_group {
    int id;
    struct list_head procs_head;
    int cpus[MAX_CPU_NUM];
    int exclusively_occupy;

    /* protect all above and process.pgroup_head, and all the process.pgroup_id
     * and cpu.pgroup_id */
    struct spin_lock lock;
};

struct need_cpu_message {
    void *sleep_chain;
    int pgroup_id;
};

// get proc group
static inline struct proc_group *get_proc_group(int id)
{
    extern struct proc_group proc_group_set[MAX_PROC_GROUP_NUM];
    return &proc_group_set[id];
}

// init process group
void proc_group_init(void);
void setup_default_proc_group(struct process *proc);
void proc_group_init_hart(void);

// process group syscall to operate proc group
int get_pgroup_id(void);
int pgroup_procs_count(void);
int create_pgroup(void);
int enter_pgroup(int pgroup_id);
// for process manager to use
int cpu_leave_pgroup_if_empty(void);
void proc_move_to_tail(struct proc_group *pgroup, struct process *proc);
int forkproc_into_pgroup(int pgroup_id, struct process *proc);
void exit_pgroup(void);

// process group syscall to operate cpus in group
int pgroup_cpu_count(void);
int inc_pgroup_cpus(void);
int inc_pgroup_cpus_flex(void);
int dec_pgroup_cpus(void);
// for scheduler to use
void handle_cpu_acquire(void);

// procrss group syscall to make proc occupy
int proc_occupy_cpu(void);
int proc_release_cpu(void);
// for others to know currnet condition
int is_exclusive_occupy(struct process *proc);

#endif
