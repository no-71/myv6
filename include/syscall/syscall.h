#ifndef SYSCALL_H_
#define SYSCALL_H_

#define SYSCALL_PUTC 0
#define SYSCALL_PRINT 1
#define SYSCALL_MAX_ID 1

#define SYSCALL_NUM (SYSCALL_MAX_ID + 1)

void handle_syscall();

#endif
