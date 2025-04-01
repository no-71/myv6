#ifndef SYSCALL_H_
#define SYSCALL_H_

#include "process/process.h"

#define SYSCALL_INTOK 0
#define SYSCALL_1EMP 1
#define SYSCALL_SBRK 2
#define SYSCALL_FORK 3
#define SYSCALL_EXEC 4
#define SYSCALL_GETPID 5
#define SYSCALL_EXIT 6
#define SYSCALL_WAIT 7
#define SYSCALL_KILL 8
#define SYSCALL_BRK 9
#define SYSCALL_10EMP 10
#define SYSCALL_DUP 11
#define SYSCALL_READ 12
#define SYSCALL_WRITE 13
#define SYSCALL_CLOSE 14
#define SYSCALL_FSTAT 15
#define SYSCALL_LINK 16
#define SYSCALL_UNLINK 17
#define SYSCALL_OPEN 18
#define SYSCALL_MKDIR 19
#define SYSCALL_MKNOD 20
#define SYSCALL_CHDIR 21
#define SYSCALL_PIPE 22
#define SYSCALL_MAX_ID 22

#define SYSCALL_NUM (SYSCALL_MAX_ID + 1)

#define DB_SYSCALL_COUNT_PROC_NUM 1000

int handle_syscall(struct process *proc);
uint64 get_arg_n(struct trap_frame *tf, int i);

#endif
