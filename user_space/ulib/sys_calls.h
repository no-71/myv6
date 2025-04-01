#ifndef SYS_CALLS_H_
#define SYS_CALLS_H_

#include "include/config/basic_types.h"
#include "include/fs/fcntl.h"
#include "include/fs/file.h"
#include "include/fs/fs.h"
#include "include/fs/stat.h"

int intokernel(void);
// int puts(const char *str);
void *sbrk(intptr_t increment);
int fork(void);
int exec(char *file, char *argv[]);
int getpid(void);
__attribute__((noreturn)) int exit(int v);
pid_t wait(int *xstatus);
int kill(pid_t pid);
int brk(void *addr);
// char getc(void);

int dup(int);
int read(int, void *, int);
int write(int, const void *, int);
int close(int);
int fstat(int fd, struct stat *);
int link(const char *, const char *);
int unlink(const char *);
int open(const char *, int);
int mkdir(const char *);
int mknod(const char *, short, short);
int chdir(const char *);
int pipe(int *);

// db syscall
int count_proc_num(void);

#endif
