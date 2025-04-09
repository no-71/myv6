#ifndef SYS_CALLS_H_
#define SYS_CALLS_H_

#include "include/config/basic_types.h"
#include "include/fs/fcntl.h"
#include "include/fs/file.h"
#include "include/fs/fs.h"
#include "include/fs/stat.h"

// syscall
int kernelbreak(void);
int sleep(int sleep_ticks);
void *sbrk(intptr_t increment);
int fork(void);
int exec(char *file, char *argv[]);
int getpid(void);
__attribute__((noreturn)) int exit(int xstatus);
pid_t wait(int *xstatus);
int kill(pid_t pid);
int brk(void *addr);
uint64 time(uint64 *t);

// file syscall
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

/*
 * process group syscall
 *
 * process group is a group own 1 ~ n cpus and 0 ~ n process, identified by
 * process group id, id >= 0. group's cpus will run its process.
 */

/*
 * --- syscalls below are process group manage syscall
 */
// return value: current proc group id, always success.
int get_proc_group_id(void);

// return value: current proc group procs count.
int proc_group_procs_count(void);

/*
 * create a new group, bring current proc and a CPU in ORIGIN GOUP to this
 * group. it means that if group has 1 proc, it is fine to call it; but if your
 * group has proc >= 2 you shall keep your group's cpus >= 2.
 *
 * return value: new group id when success, -1 when fail.
 */
int create_proc_group(void);

/*
 * enter the pgroup_id group.
 *
 * return value: 0 when success, -1 when fail.
 */
int enter_proc_group(int pgroup_id);

/*
 * --- syscalls below are cpu manage in process group syscall
 */
// return cpu count in current group.
int proc_group_cpus(void);

/*
 * increment proc group's cpu.
 * it only work if your group is not default group, and the added cpu is from
 * default group. we only allocate cpus from default group, other group's cpus
 * won't be allocated.
 * fail if your group is default group, or there is no cpu to be allocated.
 *
 * return value: 0 when success, -1 when fail.
 */
int inc_proc_group_cpus(void);

/*
 * increment proc group's cpu.
 * same as inc_proc_group_cpus(), except that when it return the cpu number may
 * not increment, but cpu number will increment later. fail if your
 * group is default group, or there is no cpu to be allocated.
 *
 * return value: 0 when success, -1 when fail.
 */
int inc_proc_group_cpus_flex(void);

/*
 * decrement proc group's cpu.
 * the cpu been removed from group will be add to default group. fail if your
 * group only have 1 cpu, as any group has at least 1 cpu.
 *
 * return value: 0 when success, -1 when fail.
 */
int dec_proc_group_cpus(void);

/*
 * process occupy the cpu.
 * fail if your group have more than 1 cpu or 1 process.
 * when success, your proc will exclusively occupy the cpu, any external
 * intrupt(device intrupt) will not intrupt your process. your process will not
 * paticipanit shecduler too. all future above will work until you call
 * proc_release_cpu()
 *
 * return value: 0 when success, -1 when fail.
 */
int proc_occupy_cpu(void);

/*
 * process give up occuping the cpu.
 * give up before proc_occupy_cpu(), if your proc don't call proc_occupy_cpu(),
 * this call do nothing but fail.
 *
 * return value: 0 when success, -1 when fail.
 */
int proc_release_cpu(void);

// debug syscall
int count_proc_num(void);

#endif
