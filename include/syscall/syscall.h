#ifndef SYSCALL_H_
#define SYSCALL_H_

#include "process/process.h"

#define SYSCALL_PUTC 0
#define SYSCALL_PRINT 1
#define SYSCALL_SBRK 2
#define SYSCALL_FORK 3
#define SYSCALL_EXEC 4
#define SYSCALL_GETPID 5
#define SYSCALL_EXIT 6
#define SYSCALL_WAIT 7
#define SYSCALL_KILL 8
#define SYSCALL_BRK 9
#define SYSCALL_MAX_ID 9

#define SYSCALL_NUM (SYSCALL_MAX_ID + 1)

#define DB_SYSCALL_COUNT_PROC_NUM 1000

void handle_syscall(struct process *proc);

#endif
