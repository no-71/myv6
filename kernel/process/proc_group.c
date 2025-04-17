#include "process/proc_group.h"
#include "config/basic_config.h"
#include "cpus.h"
#include "lock/spin_lock.h"
#include "process/cpu_message.h"
#include "process/process.h"
#include "scheduler/sleep.h"
#include "trap/intr_handler.h"
#include "trap/introff.h"
#include "util/kprint.h"
#include "util/list.h"

enum { WITH_CPU, JUST_PROC };
enum { DONT_WAIT_CPU, WAIT_FOR_CPU };

// proc group set, its size >= max cpu number usually
struct proc_group proc_group_set[MAX_PROC_GROUP_NUM];

// free cpus num, free cpu is cpu in dafault proc group
struct spin_lock free_cpus_lock;
int free_cpus;

static struct proc_group *get_default_pgroup(void)
{
    return &proc_group_set[DEFAULT_PGROUP_ID];
}

static void inc_free_cpus(void)
{
    acquire_spin_lock(&free_cpus_lock);
    if (free_cpus < 0) {
        PANIC_FN("free cpus < 0");
    }
    free_cpus++;
    release_spin_lock(&free_cpus_lock);
}

static int dec_free_cpus(void)
{
    acquire_spin_lock(&free_cpus_lock);
    if (free_cpus < 0) {
        PANIC_FN("free cpus < 0");
    }
    if (free_cpus == 0) {
        release_spin_lock(&free_cpus_lock);
        return -1;
    }
    free_cpus--;
    release_spin_lock(&free_cpus_lock);
    return 0;
}

// call with proc group lock
static void free_pgroup(struct proc_group *group)
{
    if (group->id == DEFAULT_PGROUP_ID) {
        PANIC_FN("default group been freed");
    }
    group->id = -1;
    group->exclusively_occupy = 0;
}

// call with my cpu and proc group lock
static void add_my_cpu_to_pgroup(struct proc_group *group, struct cpu *mycpu)
{
    if (mycpu != my_cpu()) {
        PANIC_FN("add other's cpu");
    }
    if (group->cpus[mycpu->cpu_id] != -1 || mycpu->pgroup_id != -1) {
        PANIC_FN("add cpu that has been added");
    }

    group->cpus[mycpu->cpu_id] = mycpu->cpu_id;
    mycpu->pgroup_id = group->id;
}

// call with my cpu and proc group lock
static void remove_my_cpu_from_pgroup(struct proc_group *group,
                                      struct cpu *mycpu)
{
    if (mycpu != my_cpu()) {
        PANIC_FN("add other's cpu");
    }
    if (group->cpus[mycpu->cpu_id] != mycpu->cpu_id ||
        mycpu->pgroup_id != group->id) {
        PANIC_FN("remove cpu that haven't been added");
    }

    group->cpus[mycpu->cpu_id] = -1;
    mycpu->pgroup_id = -1;
}

// call with my proc and proc group lock
static void add_my_proc_to_pgroup(struct proc_group *group,
                                  struct process *myproc)
{
    if (myproc != my_proc()) {
        PANIC_FN("add other's proc");
    }
    if (myproc->pgroup_id != -1) {
        PANIC_FN("add proc that has been added");
    }

    list_add(&myproc->pgroup_list, &group->procs_head);
    myproc->pgroup_id = group->id;
}

// call with my proc and proc group lock
static void remove_my_proc_from_pgroup(struct process *myproc)
{
    if (myproc != my_proc()) {
        PANIC_FN("add other's proc");
    }
    if (myproc->pgroup_id == -1) {
        PANIC_FN("remove proc that haven't been added");
    }

    list_del(&myproc->pgroup_list);
    myproc->pgroup_id = -1;
}

static struct proc_group *alloc_pgroup(struct cpu *guard_cpu)
{
    for (int i = 0; i < MAX_PROC_GROUP_NUM; i++) {
        struct proc_group *pgroup = &proc_group_set[i];

        acquire_spin_lock(&pgroup->lock);
        if (pgroup->id == -1) {
            pgroup->id = i;
            add_my_cpu_to_pgroup(pgroup, guard_cpu);
            release_spin_lock(&pgroup->lock);
            return pgroup;
        }
        release_spin_lock(&pgroup->lock);
    }

    return NULL;
}

void proc_group_init(void)
{
    for (int i = 0; i < MAX_PROC_GROUP_NUM; i++) {
        proc_group_set[i].id = -1;
        INIT_LIST_HEAD(&proc_group_set[i].procs_head);
        for (int j = 0; j < MAX_CPU_NUM; j++) {
            proc_group_set[i].cpus[j] = -1;
        }

        init_spin_lock(&proc_group_set[i].lock);
    }
    init_spin_lock(&free_cpus_lock);

    init_cpu_message();
}

void setup_default_proc_group(struct process *proc)
{
    struct proc_group *group = alloc_pgroup(my_cpu());
    if (group == NULL || group->id != DEFAULT_PGROUP_ID) {
        PANIC_FN("init group fail");
    }

    list_add(&proc->pgroup_list, &group->procs_head);
    proc->pgroup_id = DEFAULT_PGROUP_ID;
}

void proc_group_init_hart(void)
{
    struct proc_group *pgroup = &proc_group_set[DEFAULT_PGROUP_ID];
    struct cpu *mycpu = my_cpu();

    acquire_spin_lock(&pgroup->lock);
    add_my_cpu_to_pgroup(pgroup, mycpu);
    release_spin_lock(&pgroup->lock);
    inc_free_cpus();
}

// call with pgroup lock
static int pgroup_cpu_count_unsafe(struct proc_group *group)
{
    int n = 0;
    for (int i = 0; i < MAX_CPU_NUM; i++) {
        if (group->cpus[i] != -1) {
            n++;
        }
    }

    return n;
}

// call with pgroup lock and intr off, leave group
static int cpu_leave_pgroup(void)
{
    if (intr_is_on()) {
        PANIC_FN("call with intr on");
    }

    struct cpu *mycpu = my_cpu_unsafe();
    struct proc_group *old_group = &proc_group_set[mycpu->pgroup_id];
    int origin_id = mycpu->pgroup_id;

    remove_my_cpu_from_pgroup(old_group, mycpu);

    // there are still some cpus
    if (pgroup_cpu_count_unsafe(old_group) != 0) {
        // if we are not default group, ok to leave
        if (origin_id != DEFAULT_PGROUP_ID) {
            return 0;
        }
        // if we are default group, if there are free cpus, ok to leave
        if (dec_free_cpus() == 0) {
            return 0;
        }
        add_my_cpu_to_pgroup(old_group, mycpu);
        return -1;
    }
    // there is no cpu in the group, also no proc, we can free it.
    // free cpus' number don't change
    if (list_empty(&old_group->procs_head)) {
        free_pgroup(old_group);
        return 0;
    }
    // last cpu in none-empty group, can't leave
    add_my_cpu_to_pgroup(old_group, mycpu);
    return -1;
}

// call with intr off, as we will remove proc from proc group set
static int proc_leave_pgroup_may_with_cpu(int leave_way)
{
    if (intr_is_on()) {
        PANIC_FN("call with intr on");
    }

    struct process *proc = my_proc();
    struct proc_group *old_group = &proc_group_set[proc->pgroup_id];

    acquire_spin_lock(&old_group->lock);
    remove_my_proc_from_pgroup(proc);
    if (leave_way != WITH_CPU) {
        release_spin_lock(&old_group->lock);
        return 0;
    }

    int err = cpu_leave_pgroup();
    if (err) {
        add_my_proc_to_pgroup(old_group, proc);
        release_spin_lock(&old_group->lock);
        return -1;
    }

    release_spin_lock(&old_group->lock);
    return 0;
}

static void proc_leave_pgroup_without_cpu(void)
{
    int err = proc_leave_pgroup_may_with_cpu(JUST_PROC);
    if (err) {
        PANIC_FN("proc fail to leave proc group without cpu");
    }
}

static int proc_leave_pgroup_with_cpu(void)
{
    return proc_leave_pgroup_may_with_cpu(WITH_CPU);
}

int get_pgroup_id(void) { return my_proc()->pgroup_id; }

static int pgroup_procs_count_unsafe(struct proc_group *pgroup)
{
    return list_count_nodes(&pgroup->procs_head);
}

int pgroup_procs_count(void)
{
    struct proc_group *pgroup = get_proc_group(my_proc()->pgroup_id);
    acquire_spin_lock(&pgroup->lock);
    int n = pgroup_procs_count_unsafe(pgroup);
    release_spin_lock(&pgroup->lock);
    return n;
}

// create a group, bring current proc and cpu to the group
int create_pgroup(void)
{
    // after leave group, proc won't belong to any group, cpu can't leave this
    // proc, which will cause the proc lost. we close intr to make sure that we
    // don't leave this proc
    push_introff();

    // remove proc and cpu from old group
    int err = proc_leave_pgroup_with_cpu();
    if (err) {
        pop_introff();
        return -1;
    }

    struct process *proc = my_proc();
    struct cpu *mycpu = my_cpu();
    struct proc_group *new_group = alloc_pgroup(mycpu);
    if (new_group == NULL) {
        PANIC_FN("run out of proc group");
    }

    // add proc to new group, bring my cpu to the group
    acquire_spin_lock(&new_group->lock);
    add_my_proc_to_pgroup(new_group, proc);
    release_spin_lock(&new_group->lock);

    // finish, pop introff
    pop_introff();
    return proc->pgroup_id;
}

// add current proc to group
/*
 * you can consider require two proc group lock(acquire lock with smaller group
 * id first), then it won't cause CPU lost when proc leave the group and readd
 */
int enter_pgroup(int pgroup_id)
{
    if (pgroup_id >= MAX_PROC_GROUP_NUM || pgroup_id < 0) {
        return -1;
    }

    struct process *proc = my_proc();
    struct proc_group *new_group = &proc_group_set[pgroup_id];
    struct proc_group *old_group = &proc_group_set[proc->pgroup_id];
    if (new_group == old_group) {
        return 0;
    }

    // after leave group, proc won't belong to any group, cpu can't leave this
    // proc, which will cause the proc lost. we close intr to make sure that we
    // don't leave this proc
    push_introff();

    proc_leave_pgroup_without_cpu();

    acquire_spin_lock(&new_group->lock);
    if (new_group->id == -1) {
        release_spin_lock(&new_group->lock);

        // current cpu still in old_group, and proc group won't be free if there
        // is any cpu in this group. we can add proc to old group back safely.
        acquire_spin_lock(&old_group->lock);
        add_my_proc_to_pgroup(old_group, proc);
        release_spin_lock(&old_group->lock);
        pop_introff();
        return -1;
    }

    add_my_proc_to_pgroup(new_group, proc);
    release_spin_lock(&new_group->lock);
    pop_introff();

    // current cpu maybe other group's, yield proc
    // if current cpu is current group, it's ok to yield too
    yield(RUNABLE);
    return 0;
}

// call in scheduler with pgroup lock. we free proc group here.
// if proc group is empty, leave group, into default group.
// return 0 if leave success
int cpu_leave_pgroup_if_empty(void)
{
    struct cpu *mycpu = my_cpu_unsafe();
    struct proc_group *pgroup = &proc_group_set[mycpu->pgroup_id];

    // not empty, don't leave
    if (list_empty(&pgroup->procs_head) == 0) {
        return -1;
    }
    remove_my_cpu_from_pgroup(pgroup, mycpu);
    // there is no cpu in group, free group
    if (pgroup_cpu_count_unsafe(pgroup) == 0) {
        free_pgroup(pgroup);
    }
    release_spin_lock(&pgroup->lock);

    struct proc_group *default_group = get_default_pgroup();
    acquire_spin_lock(&default_group->lock);
    add_my_cpu_to_pgroup(default_group, mycpu);
    release_spin_lock(&default_group->lock);

    // new cpus in default group
    inc_free_cpus();

    acquire_spin_lock(&pgroup->lock);
    return 0;
}

// call in scheduler with pgroup lock.
// move the proc to the tail in proc group's proc list
void proc_move_to_tail(struct proc_group *pgroup, struct process *proc)
{
    list_del(&proc->pgroup_list);
    list_add_tail(&proc->pgroup_list, &pgroup->procs_head);
}

// call by fork() to add child, only in this circumstance, we won't lost proc
int forkproc_into_pgroup(int pgroup_id, struct process *proc)
{
    if (pgroup_id >= MAX_CPU_NUM || pgroup_id < 0) {
        return -1;
    }

    struct proc_group *new_group = &proc_group_set[pgroup_id];

    acquire_spin_lock(&new_group->lock);
    if (new_group->id == -1 || new_group->exclusively_occupy) {
        release_spin_lock(&new_group->lock);
        return -1;
    }

    list_add(&proc->pgroup_list, &new_group->procs_head);
    proc->pgroup_id = pgroup_id;
    release_spin_lock(&new_group->lock);

    return 0;
}

// call by exit() with intr off, current proc exit pgroup
void exit_pgroup(void) { proc_leave_pgroup_without_cpu(); }

int pgroup_cpu_count()
{
    struct proc_group *group = &proc_group_set[my_proc()->pgroup_id];
    acquire_spin_lock(&group->lock);
    int n = pgroup_cpu_count_unsafe(group);
    release_spin_lock(&group->lock);
    return n;
}

int inc_pgroup_cpus(void)
{
    struct process *proc = my_proc();
    struct proc_group *pgroup = get_proc_group(proc->pgroup_id);
    struct need_cpu_message m;
    m.sleep_chain = &m;
    m.pgroup_id = proc->pgroup_id;
    if (proc->pgroup_id == DEFAULT_PGROUP_ID || pgroup->exclusively_occupy) {
        return -1;
    }

    int err = dec_free_cpus();
    if (err) {
        return -1;
    }
    acquire_spin_lock(&pgroup->lock);
    send_cpu_message((struct cpu_message){ NEED_CPU, &m });
    sleep(&pgroup->lock, m.sleep_chain);
    release_spin_lock(&pgroup->lock);
    return 0;
}

int inc_pgroup_cpus_flex(void)
{
    struct process *proc = my_proc();
    struct proc_group *pgroup = get_proc_group(proc->pgroup_id);
    if (proc->pgroup_id == DEFAULT_PGROUP_ID || pgroup->exclusively_occupy) {
        return -1;
    }

    int err = dec_free_cpus();
    if (err) {
        return -1;
    }
    send_cpu_message((struct cpu_message){ NEED_CPU_FLEX, pgroup });
    return 0;
}

int dec_pgroup_cpus(void)
{
    push_introff();
    struct cpu *mycpu = my_cpu();
    struct proc_group *pgroup = &proc_group_set[mycpu->pgroup_id];
    if (mycpu->pgroup_id == DEFAULT_PGROUP_ID) {
        pop_introff();
        return -1;
    }

    acquire_spin_lock(&pgroup->lock);
    int err = cpu_leave_pgroup();
    release_spin_lock(&pgroup->lock);
    if (err) {
        pop_introff();
        return -1;
    }

    struct proc_group *default_group = get_default_pgroup();
    acquire_spin_lock(&default_group->lock);
    add_my_cpu_to_pgroup(default_group, mycpu);
    release_spin_lock(&default_group->lock);

    // new cpu in default group
    inc_free_cpus();
    pop_introff();

    // current cpu maybe other group's, yield proc
    // if current cpu is current group, it's ok to yield too
    yield(RUNABLE);
    return 0;
}

// call by scheduler if cpu belongs to dafault group, handle cpu acquire
void handle_cpu_acquire(void)
{
    struct cpu *mycpu = my_cpu_unsafe();
    if (mycpu->pgroup_id != DEFAULT_PGROUP_ID) {
        PANIC_FN("other proc group cpus want to handle cpu acquire");
    }
    if (no_message_atomic()) {
        return;
    }

    struct cpu_message m = get_cpu_message();
    struct need_cpu_message *mm = m.message;
    struct proc_group *old_group = get_default_pgroup();
    struct proc_group *new_group;

    if (m.type == NEED_CPU) {
        new_group = get_proc_group(mm->pgroup_id);
    } else if (m.type == NEED_CPU_FLEX) {
        new_group = m.message;
    } else {
        resend_cpu_message(m);
        return;
    }

    acquire_spin_lock(&old_group->lock);
    if (pgroup_cpu_count_unsafe(old_group) == 1) {
        PANIC_FN("too many cpus been allocate");
    }
    remove_my_cpu_from_pgroup(old_group, mycpu);
    release_spin_lock(&old_group->lock);

    acquire_spin_lock(&new_group->lock);
    if (new_group->id == -1) {
        if (m.type == NEED_CPU) {
            PANIC_FN("empty group acquire cpu");
        } else if (m.type == NEED_CPU_FLEX) {
            release_spin_lock(&new_group->lock);

            acquire_spin_lock(&old_group->lock);
            add_my_cpu_to_pgroup(old_group, mycpu);
            inc_free_cpus();
            release_spin_lock(&old_group->lock);
            return;
        }
    }
    add_my_cpu_to_pgroup(new_group, mycpu);
    if (m.type == NEED_CPU) {
        wake_up(mm->sleep_chain);
    }
    release_spin_lock(&new_group->lock);
}

int proc_occupy_cpu(void)
{
    struct process *proc = my_proc();
    struct proc_group *pgroup = get_proc_group(proc->pgroup_id);
    if (proc->pgroup_id == DEFAULT_PGROUP_ID) {
        return -1;
    }

    acquire_spin_lock(&pgroup->lock);
    if (pgroup_procs_count_unsafe(pgroup) != 1 ||
        pgroup_cpu_count_unsafe(pgroup) != 1) {
        release_spin_lock(&pgroup->lock);
        return -1;
    }
    pgroup->exclusively_occupy = 1;
    enable_soft_intr();
    release_spin_lock(&pgroup->lock);
    return 0;
}

int proc_release_cpu(void)
{
    struct process *proc = my_proc();
    struct proc_group *pgroup = get_proc_group(proc->pgroup_id);

    acquire_spin_lock(&pgroup->lock);
    if (pgroup_procs_count_unsafe(pgroup) != 1 ||
        pgroup_cpu_count_unsafe(pgroup) != 1) {
        PANIC_FN("process exclusively occupy cpu fail, 1 more procs");
    }
    if (pgroup->exclusively_occupy == 0) {
        release_spin_lock(&pgroup->lock);
        return -1;
    }
    pgroup->exclusively_occupy = 0;
    enable_soft_n_external_intr();
    release_spin_lock(&pgroup->lock);
    return 0;
}

int is_exclusive_occupy(struct process *proc)
{
    // we don't acquire lock as we will use it in stable environment
    return get_proc_group(proc->pgroup_id)->exclusively_occupy;
}
