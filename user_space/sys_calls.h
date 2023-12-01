#ifndef SYS_CALLS_H_
#define SYS_CALLS_H_

#include "include/config/basic_types.h"

int putc(char c);
int puts(const char *str);
void *sbrk(intptr_t increment);
int fork(void);
int exec(char *file, char *argv[]);
int getpid(void);
__attribute__((noreturn)) int exit(int v);
pid_t wait(int *xstatus);
int kill(pid_t pid);
int brk(void *addr);
char getc(void);

// db syscall
int count_proc_num(void);

#endif
