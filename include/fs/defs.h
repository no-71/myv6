#ifndef XV6_DEFS_H_
#define XV6_DEFS_H_

#include "config/basic_types.h"
#include "vm/vm.h"

struct buf;
struct file;
struct inode;
struct pipe;
struct sleeplock;
struct stat;
struct superblock;
#ifdef LAB_NET
struct mbuf;
struct sock;
#endif

// bio.c
void binit(void);
struct buf *bread(uint, uint);
void brelse(struct buf *);
void bwrite(struct buf *);
void bpin(struct buf *);
void bunpin(struct buf *);

// console.c
// void            consoleinit(void);
// void            consoleintr(int);
// void            consputc(int);

// exec.c
// int             exec(char*, char**);

// file.c
struct file *filealloc(void);
void fileclose(struct file *);
struct file *filedup(struct file *);
void fileinit(void);
int fileread(struct file *, uint64, int n);
int filestat(struct file *, uint64 addr);
int filewrite(struct file *, uint64, int n);

// fs.c
void fsinit(int);
int dirlink(struct inode *, char *, uint);
struct inode *dirlookup(struct inode *, char *, uint *);
struct inode *ialloc(uint, short);
struct inode *idup(struct inode *);
void iinit();
void ilock(struct inode *);
void iput(struct inode *);
void iunlock(struct inode *);
void iunlockput(struct inode *);
void iupdate(struct inode *);
int namecmp(const char *, const char *);
struct inode *namei(char *);
struct inode *nameiparent(char *, char *);
int readi(struct inode *, int, uint64, uint, uint);
void stati(struct inode *, struct stat *);
int writei(struct inode *, int, uint64, uint, uint);
void itrunc(struct inode *);

// ramdisk.c
void ramdiskinit(void);
void ramdiskintr(void);
void ramdiskrw(struct buf *);

// kalloc.c
// void*           kalloc(void);
// void            kfree(void *);
// void            kinit(void);

// log.c
void initlog(int, struct superblock *);
void log_write(struct buf *);
void begin_op(void);
void end_op(void);

// pipe.c
int pipealloc(struct file **, struct file **);
void pipeclose(struct pipe *, int);
int piperead(struct pipe *, uint64, int);
int pipewrite(struct pipe *, uint64, int);

// printf.c
// void            printf(char*, ...);
__attribute__((noreturn)) void panic(const char *s);
// void            printfinit(void);

// proc.c
#include "cpus.h"
#include "process/process.h"
#include "scheduler/sleep.h"

// int             cpuid(void);
// void            exit(int);
// int             fork(void);
// int             growproc(int);
// pagetable_t     proc_pagetable(struct proc *);
// void            proc_freepagetable(pagetable_t, uint64);
// int             kill(int);
// struct cpu*     mycpu(void);
static inline struct cpu *mycpu(void) { return my_cpu(); }
// struct cpu*     getmycpu(void);
// struct proc*    myproc();
static inline struct process *myproc() { return my_proc(); }
// void            procinit(void);
// void            scheduler(void) __attribute__((noreturn));
// void            sched(void);
// void            setproc(struct proc*);
// void            sleep(void*, struct spinlock*);
static inline void sleep_r(void *chain, struct spin_lock *lock)
{
    sleep(lock, chain);
}
// void            userinit(void);
// int             wait(uint64);
// void            wakeup(void*);
static inline void wakeup(void *chain) { wake_up(chain); }
// void            yield(void);
// int             either_copyout(int user_dst, uint64 dst, void *src, uint64
// len); int             either_copyin(void *dst, int user_src, uint64 src,
// uint64 len); void            procdump(void);

// swtch.S
// void            swtch(struct context*, struct context*);

// spinlock.c
#include "lock/spin_lock.h"
#include "trap/introff.h"
// void            acquire(struct spinlock*);
// int             holding(struct spinlock*);
// void            initlock(struct spinlock*, char*);
// void            release(struct spinlock*);
// void            push_off(void);
// void            pop_off(void);
static inline void acquire(struct spin_lock *lock) { acquire_spin_lock(lock); }
static inline void initlock(struct spin_lock *lock, char *s)
{
    init_spin_lock(lock);
}
static inline void release(struct spin_lock *lock) { release_spin_lock(lock); }
static inline void push_off(void) { push_introff(); }
static inline void pop_off(void) { pop_introff(); }

// sleeplock.c
void acquiresleep(struct sleeplock *);
void releasesleep(struct sleeplock *);
int holdingsleep(struct sleeplock *);
void initsleeplock(struct sleeplock *, char *);

// string.c
// int             memcmp(const void*, const void*, uint);
// void*           memmove(void*, const void*, uint);
// void*           memset(void*, int, uint);
// char*           safestrcpy(char*, const char*, int);
// int             strlen(const char*);
// int             strncmp(const char*, const char*, uint);
// char*           strncpy(char*, const char*, int);

// syscall.c
// int             argint(int, int*);
// int             argstr(int, char*, int);
// int             argaddr(int, uint64 *);
// int             fetchstr(uint64, char*, int);
// int             fetchaddr(uint64, uint64*);
// void            syscall();

// trap.c
// extern uint     ticks;
// void            trapinit(void);
// void            trapinithart(void);
// extern struct spinlock tickslock;
// void            usertrapret(void);

// uart.c
// void            uartinit(void);
// void            uartintr(void);
// void            uartputc(int);
// void            uartputc_sync(int);
// int             uartgetc(void);

// vm.c
// void            kvminit(void);
// void            kvminithart(void);
// uint64          kvmpa(uint64);
// void            kvmmap(uint64, uint64, uint64, int);
// int             mappages(pagetable_t, uint64, uint64, uint64, int);
// pagetable_t     uvmcreate(void);
// void            uvminit(pagetable_t, uchar *, uint);
// uint64          uvmalloc(pagetable_t, uint64, uint64);
// uint64          uvmdealloc(pagetable_t, uint64, uint64);
#ifdef SOL_COW
#else
// int             uvmcopy(pagetable_t, pagetable_t, uint64);
#endif
// void            uvmfree(pagetable_t, uint64);
// void            uvmunmap(pagetable_t, uint64, uint64, int);
// void            uvmclear(pagetable_t, uint64);
// uint64          walkaddr(pagetable_t, uint64);
// int             copyout(pagetable_t, uint64, char *, uint64);
// int             copyin(pagetable_t, char *, uint64, uint64);
// int             copyinstr(pagetable_t, char *, uint64, uint64);

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
static inline int either_copyout(int user_dst, uint64 dst, void *src,
                                 uint64 len)
{
    return copyout_uork(my_proc()->proc_pgtable, dst, src, len, user_dst);
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
static inline int either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
    return copyin_uork(my_proc()->proc_pgtable, dst, src, len, user_src);
}

// plic.c
// void            plicinit(void);
// void            plicinithart(void);
// int             plic_claim(void);
// void            plic_complete(int);

// virtio_disk.c
// void virtio_disk_init(void);
// void virtio_disk_rw(struct buf *, int);
// void virtio_disk_intr(void);

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x) / sizeof((x)[0]))

// sysfile.c
extern uint64 sys_chdir(void);
extern uint64 sys_close(void);
extern uint64 sys_dup(void);
// extern uint64 sys_exec(void);
// extern uint64 sys_exit(void);
// extern uint64 sys_fork(void);
extern uint64 sys_fstat(void);
// extern uint64 sys_getpid(void);
// extern uint64 sys_kill(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_mknod(void);
extern uint64 sys_open(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
// extern uint64 sys_sbrk(void);
// extern uint64 sys_sleep(void);
extern uint64 sys_unlink(void);
// extern uint64 sys_wait(void);
extern uint64 sys_write(void);
extern uint64 sys_uptime(void);

#define GEN_SYSCALL_FN(NAME)                                                   \
    static inline uint64 syscall_##NAME(struct process *proc)                  \
    {                                                                          \
        return sys_##NAME();                                                   \
    }

GEN_SYSCALL_FN(chdir)
GEN_SYSCALL_FN(close)
GEN_SYSCALL_FN(dup)
GEN_SYSCALL_FN(fstat)
GEN_SYSCALL_FN(link)
GEN_SYSCALL_FN(mkdir)
GEN_SYSCALL_FN(mknod)
GEN_SYSCALL_FN(open)
GEN_SYSCALL_FN(pipe)
GEN_SYSCALL_FN(read)
GEN_SYSCALL_FN(unlink)
GEN_SYSCALL_FN(write)
GEN_SYSCALL_FN(uptime)

// stats.c
// void            statsinit(void);
// void            statsinc(void);

// sprintf.c
// int             snprintf(char*, int, char*, ...);

#ifdef LAB_NET
// pci.c
void pci_init();

// e1000.c
void e1000_init(uint32 *);
void e1000_intr(void);
int e1000_transmit(struct mbuf *);

// net.c
void net_rx(struct mbuf *);
void net_tx_udp(struct mbuf *, uint32, uint16, uint16);

// sysnet.c
void sockinit(void);
int sockalloc(struct file **, uint32, uint16, uint16);
void sockclose(struct sock *);
int sockread(struct sock *, uint64, int);
int sockwrite(struct sock *, uint64, int);
void sockrecvudp(struct mbuf *, uint32, uint16, uint16);
#endif

#endif
