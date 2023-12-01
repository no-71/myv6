#ifndef PROCESS_H_
#define PROCESS_H_

#include "config/basic_types.h"
#include "lock/spin_lock.h"
#include "vm/vm.h"

#define STATIC_PROC_NUM 32

struct context {
    uint64 ra;
    uint64 sp;
    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
};

struct trap_frame {
    uint64 ra;
    uint64 sp;
    uint64 gp;
    uint64 tp;
    uint64 t0;
    uint64 t1;
    uint64 t2;
    uint64 s0;
    uint64 s1;
    uint64 a0;
    uint64 a1;
    uint64 a2;
    uint64 a3;
    uint64 a4;
    uint64 a5;
    uint64 a6;
    uint64 a7;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
    uint64 t3;
    uint64 t4;
    uint64 t5;
    uint64 t6;

    // return to user space
    uint64 sepc;

    // current cpu
    uint64 cpu_id;

    // kernel setup
    uint64 kernel_trap_entry_ptr;
    uint64 user_trap_hadnler_ptr;
    uint64 kstack;
    uint64 satp;
};

enum { UNUSED, USED, RUNABLE, RUNNING, SLEEP, ZOMBIE };

struct process {
    pid_t pid;
    int status;
    int killed;
    int xstatus;
    void *chain;
    struct process *parent;
    struct spin_lock lock;

    // private, don't need lock
    uint64 ustack;
    uint64 mem_start;
    uint64 mem_brk;
    uint64 mem_end;
    page_table proc_pgtable;

    uint64 kstack;
    struct trap_frame *proc_trap_frame;
    struct context proc_context;
};

void process_init(void);
void setup_init_proc(void);
uint64 fork(struct process *proc);
__attribute__((noreturn)) uint64 exit(struct process *proc, uint64 xstatus);
uint64 exec(struct process *proc, char *file, int argc, char *argv[],
            char str_in_argv[]);
uint64 wait(struct process *proc, uint64 int_uva);
uint64 kill(pid_t pid);
uint64 count_proc_num(void);

extern struct process proc_set[STATIC_PROC_NUM];

#endif
