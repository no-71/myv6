/*
 * this test file used code from xv6 usertests.c,
 * we add some tests for system calls from myv6.
 */

#include "include/fs/memlayout.h"
#include "include/fs/param.h"
#include "include/riscv/regs.h"
#include "include/riscv/vm_system.h"
#include "ulib/sys_calls.h"
#include "ulib/user_all.h"

//
// Tests myv6 system calls.  usertests without arguments runs them all
// and usertests <name> runs <name> test. The test runner creates for
// each test a process and based on the exit status of the process,
// the test runner reports "OK" or "FAILED".  Some tests result in
// kernel printing usertrap messages, which can be ignored if test
// prints "OK".
//

#define LEN(x, type) (sizeof(x) / sizeof(type))
#define BUFSZ ((MAXOPBLOCKS + 2) * BSIZE)

char buf[BUFSZ];
char name[3];

static int wait_children(char *s, int count)
{
    int xs = 0;
    for (int i = 0; i < count; i++) {
        int xstate = 1;
        int pid = wait(&xstate);
        if (pid == -1) {
            printf("%s: wait_children: failed in %d child\n", s, i);
            exit(1);
        }
        xs |= xstate;
    }
    return xs;
}

static void do_test_n_times(char *s, void f(char *s), int n)
{
    for (int i = 0; i < n; i++) {
        int pid = fork();
        if (pid < 0) {
            printf("%s: fork failed\n", s);
            exit(1);
        }
        if (pid == 0) {
            f(s);
            exit(0);
        }
        int xstatus = wait_children(s, 1);
        if (xstatus) {
            printf("fail when execute do_enter_pg %d\n", i);
            exit(1);
        }
        printf("%d..", i);
    }
    exit(0);
}

void exclusive_occupy(char *s)
{
    int err = proc_occupy_cpu();
    if (err == 0) {
        printf("proc occuy cpu in default group\n");
        exit(1);
    }

    err = create_proc_group();
    if (err == -1) {
        printf("create proc group fail\n");
        exit(1);
    }

    int pp_end[2], n;
    if (pipe(pp_end) < 0) {
        printf("pipe() failed in countfree()\n");
        exit(1);
    }
    int pid = fork();
    if (pid < 0) {
        printf("%s: fork failed\n", s);
        exit(1);
    }
    if (pid == 0) {
        // wait for parent
        read(pp_end[0], &n, 1);
        exit(0);
    }

    err = proc_occupy_cpu();
    if (err == 0) {
        printf("proc occuy cpu in proc group with 2 proc\n");
        exit(1);
    }
    write(pp_end[1], &n, 1);
    wait_children(s, 1);

    err = proc_occupy_cpu();
    if (err) {
        printf("proc occuy cpu fail\n");
        exit(1);
    }
    sleep(3);

    err = proc_release_cpu();
    if (err) {
        printf("proc release cpu fail\n");
        exit(1);
    }
    exit(0);
}

static void run_out_cpus(int free_cpus)
{
    int err;
    for (int i = 0; i < free_cpus; i++) {
        err = inc_proc_group_cpus();
        if (err) {
            printf("inc_proc_group_cpus() fail in %d\n", i);
            exit(1);
        }
        int cpus = proc_group_cpus();
        if (cpus != i + 2) {
            printf("wrong cpu number %d, should be %d in acquire\n", cpus,
                   i + 2);
            exit(1);
        }
    }

    err = inc_proc_group_cpus();
    if (err == 0) {
        printf("inc_proc_group_cpus() inc cpus when there is no free cpu\n");
        exit(1);
    }
    int cpus = proc_group_cpus();
    if (cpus != free_cpus + 1) {
        printf("wrong cpu number %d, should be %d in end\n", cpus,
               free_cpus + 1);
        exit(1);
    }
}

static void run_out_cpu_flex_leave_fast(int free_cpus)
{
    int err = create_proc_group();
    if (err == -1) {
        printf("create proc group fail\n");
        exit(1);
    }
    free_cpus--;

    for (int i = 0; i < free_cpus; i++) {
        int err = inc_proc_group_cpus_flex();
        if (err) {
            printf("inc_proc_group_cpus_flex() fail in %d\n", i);
            exit(1);
        }
    }
}

// what if we flex allocate all cpus then exit group?
// test whether cpu handle need_cpu_message from group that has been freed
// correctly
void leave_pg_fast(char *s)
{
    int free_cpus = proc_group_cpus() - 1;

    int pid = fork();
    if (pid < 0) {
        printf("%s: fork failed\n", s);
        exit(1);
    }
    if (pid == 0) {
        // fast ret, some cpus haven't add to the group when free group
        run_out_cpu_flex_leave_fast(free_cpus);
        exit(0);
    }

    wait_children(s, 1);
    sleep(5); // give some time for cpus to recieve message
    int cpus = proc_group_cpus();
    if (cpus != free_cpus + 1) {
        printf("wrong cpu number %d, should be %d when fast leave finished\n",
               cpus, free_cpus + 1);
        exit(1);
    }

    // run out all cpus to check free_cpus has right value
    int err = create_proc_group();
    if (err == -1) {
        printf("create proc group fail\n");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        printf("%s: fork failed\n", s);
        exit(1);
    }
    if (pid == 0) {
        run_out_cpus(free_cpus - 1);
        exit(0);
    }

    if (wait_children(s, 1)) {
        exit(1);
    }
    enter_proc_group(0);
    sleep(5); // give some time for cpus to leave group
    cpus = proc_group_cpus();
    if (cpus != free_cpus + 1) {
        printf("wrong cpu number %d, should be %d in end\n", cpus,
               free_cpus + 1);
        exit(1);
    }
    exit(0);
}

static void run_out_cpus_flex(int free_cpus)
{
    int err;
    for (int i = 0; i < free_cpus; i++) {
        err = inc_proc_group_cpus_flex();
        if (err) {
            printf("inc_proc_group_cpus_flex() fail in %d\n", i);
            exit(1);
        }
        int cpus = proc_group_cpus();
        if (cpus == i + 2) {
            // inc cpu immediately, may be wrong if occur too many times,
            // print to draw attention
            printf("inc_proc_group_cpus_flex() inc cpu immediately in %d\n", i);
        }
        for (int w = 0; w < 5; w++) {
            if (proc_group_cpus() != i + 2) {
                sleep(1);
            } else {
                break;
            }
        }
        cpus = proc_group_cpus();
        if (cpus != i + 2) {
            printf("wrong cpu number %d, should be %d in acquire\n", cpus,
                   i + 2);
            exit(1);
        }
    }

    err = inc_proc_group_cpus_flex();
    if (err == 0) {
        printf(
            "inc_proc_group_cpus_flex() inc cpus when there is no free cpu\n");
        exit(1);
    }
    int cpus = proc_group_cpus();
    if (cpus != free_cpus + 1) {
        printf("wrong cpu number %d, should be %d in end\n", cpus,
               free_cpus + 1);
        exit(1);
    }
}

static void run_out_cpus_flex_con(int free_cpus)
{
    int err;
    for (int i = 0; i < free_cpus; i++) {
        err = inc_proc_group_cpus_flex();
        if (err) {
            printf("inc_proc_group_cpus_flex() fail in %d\n", i);
            exit(1);
        }
    }
    for (int w = 0; w < 5; w++) {
        if (proc_group_cpus() != free_cpus + 1) {
            sleep(1);
        } else {
            break;
        }
    }
    int cpus = proc_group_cpus();
    if (cpus != free_cpus + 1) {
        printf("wrong cpu number %d, should be %d in acquire\n", cpus,
               free_cpus + 1);
        exit(1);
    }

    err = inc_proc_group_cpus_flex();
    if (err == 0) {
        printf(
            "inc_proc_group_cpus_flex() inc cpus when there is no free cpu\n");
        exit(1);
    }
    cpus = proc_group_cpus();
    if (cpus != free_cpus + 1) {
        printf("wrong cpu number %d, should be %d in end\n", cpus,
               free_cpus + 1);
        exit(1);
    }
}

void do_inc_cpus_flex(char *s, void (*runout)(int))
{
    int free_cpus = proc_group_cpus() - 1;

    int err = create_proc_group();
    if (err == -1) {
        printf("create proc group fail\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        printf("%s: fork failed\n", s);
        exit(1);
    }
    if (pid == 0) {
        runout(free_cpus - 1);
        exit(0);
    }

    if (wait_children(s, 1)) {
        exit(1);
    }
    enter_proc_group(0);
    sleep(5); // give some time for cpus to leave group
    int cpus = proc_group_cpus();
    if (cpus != free_cpus + 1) {
        printf("wrong cpu number %d, should be %d in end\n", cpus,
               free_cpus + 1);
        exit(1);
    }
    exit(0);
}

void inc_cpus_flex(char *s) { do_inc_cpus_flex(s, run_out_cpus_flex); }

void inc_cpus_flex_con(char *s) { do_inc_cpus_flex(s, run_out_cpus_flex_con); }

static void do_inc_cpus(char *s)
{
    int free_cpus = proc_group_cpus() - 1;

    int err = create_proc_group();
    if (err == -1) {
        printf("create proc group fail\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        printf("%s: fork failed\n", s);
        exit(1);
    }
    if (pid == 0) {
        run_out_cpus(free_cpus - 1);
        exit(0);
    }

    if (wait_children(s, 1)) {
        exit(1);
    }
    enter_proc_group(0);
    sleep(5); // give some time for cpus to leave group
    int cpus = proc_group_cpus();
    if (cpus != free_cpus + 1) {
        printf("wrong cpu number %d, should be %d in end\n", cpus,
               free_cpus + 1);
        exit(1);
    }
    exit(0);
}

// run out all free cpus, then quit the group and check wheter cpus leave the
// empty group. test multiple times to ensure every cpu in right condition.
void inc_cpus(char *s) { do_test_n_times(s, do_inc_cpus, 5); }

// run out free cpus then dec them all.
// check wheter they come back to default group.
void dec_cpus(char *s)
{
    int free_cpus = proc_group_cpus() - 1;
    int group_free_cpus = free_cpus - 1;
    int pp_start[2], pp_dec_fin[2], n;
    if (pipe(pp_start)) {
        printf("pipe() failed\n");
        exit(1);
    }
    if (pipe(pp_dec_fin)) {
        printf("pipe() failed\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        printf("%s: fork failed\n", s);
    }
    if (pid == 0) {
        // check fin dec all
        read(pp_dec_fin[0], &n, 1);
        int cpus = proc_group_cpus();
        if (cpus != group_free_cpus + 1) {
            printf("default group don't get all cpus, get %d cpus\n", cpus);
            exit(1);
        }
        exit(0);
    }

    int pgroup_id = create_proc_group();
    if (pgroup_id == -1) {
        printf("create proc group fail\n");
        exit(1);
    } else if (pgroup_id != 1) {
        printf("proc group's id != 1\n");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        printf("%s: fork failed\n", s);
    }
    if (pid == 0) {
        // run out cpus
        run_out_cpus(group_free_cpus);
        write(pp_start[1], &n, 1);
        exit(0);
    }

    read(pp_start[0], &n, 1);
    for (int i = 0; i < group_free_cpus; i++) {
        int err = dec_proc_group_cpus();
        if (err) {
            printf("dec proc cpus fail in %d\n", i);
            exit(1);
        }
    }
    int err = dec_proc_group_cpus();
    if (err == 0) {
        printf("dec proc cpus success when there is only 1 cpu\n");
        exit(1);
    }
    write(pp_dec_fin[1], &n, 1);

    int xs = wait_children(s, 2);
    close(pp_start[0]);
    close(pp_start[1]);
    close(pp_dec_fin[0]);
    close(pp_dec_fin[1]);
    exit(xs);
}

// dose create_proc_group() refuse to leave group when there are still some proc
// but only 1 cpu?
void leave_pg(char *s)
{
    int err = create_proc_group();
    if (err == -1) {
        printf("create proc group fail\n");
        exit(1);
    }

    int pp_end[2];
    if (pipe(pp_end) < 0) {
        printf("pipe() failed\n");
        exit(1);
    }
    int pid = fork();
    if (pid < 0) {
        printf("%s: fork1 failed\n", s);
        exit(1);
    }
    if (pid == 0) {
        read(pp_end[0], &err, 1);
        exit(0);
    }

    for (int i = 0; i < 20; i++) {
        err = create_proc_group();
        if (err != -1) {
            printf("create_proc_group claim to create a group\n");
            exit(1);
        }
    }
    write(pp_end[1], &err, 1);
    wait_children(s, 1);
    close(pp_end[0]);
    close(pp_end[1]);
    for (int i = 0; i < 20; i++) {
        err = create_proc_group();
        if (err == -1) {
            printf("create proc group fail in %d\n", i);
            exit(1);
        }
    }
    if (err == -1) {
        printf("create proc group fail\n");
        exit(1);
    }
}

// dose create_proc_group() clean group when the last cpu leave?
void create_pg(char *s)
{
    int cpus = proc_group_cpus();
    int child_count = cpus - 2;
    printf("we got %d cpus, start 1 main process, %d child process\n", cpus,
           child_count);

    int pid = 1;
    for (int i = 0; i < child_count; i++) {
        pid = fork();
        if (pid < 0) {
            printf("%s: fork1 failed\n", s);
            exit(1);
        } else if (pid == 0) {
            break;
        }
    }

    for (int i = 0; i < 1000; i++) {
        int err = create_proc_group();
        if (err == -1) {
            printf("create %d proc group fail cpus\n", i);
            exit(1);
        }
    }

    if (pid == 0) {
        exit(0);
    } else {
        exit(wait_children(s, child_count));
    }
}

// dose enter_proc_group() refuse to enter a ridiculous proc group?
void enter_wrong_pg(char *s)
{
    int id[10] = { 100,     101,     102, 103,  1 << 10,
                   1 << 11, 1 << 12, -1,  -100, -(1 << 10) };
    for (int i = 0; i < LEN(id, int); i++) {
        int err = enter_proc_group(id[i]);
        if (err == 0) {
            printf("enter_proc_group() success to enter %d proc group\n", i);
            exit(1);
        }
    }
    exit(0);
}

static void do_enter_pg(char *s)
{
    int pp_start[2], pp_end[2], n;
    if (pipe(pp_start) < 0) {
        printf("pipe() failed\n");
        exit(1);
    }
    if (pipe(pp_end) < 0) {
        printf("pipe() failed\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        printf("%s: fork failed\n", s);
        exit(1);
    }
    if (pid == 0) {
        int pgid = create_proc_group();
        if (pgid < 0) {
            write(pp_start[1], &n, 1);
            printf("create proc group fail\n");
            exit(1);
        } else if (pgid != 1) {
            write(pp_start[1], &n, 1);
            printf("proc group's id != 1\n");
            exit(1);
        }
        write(pp_start[1], &n, 1);
        read(pp_end[0], &n, 1);
        exit(0);
    }

    pid = fork();
    if (pid < 0) {
        printf("%s: fork2 failed\n", s);
        exit(1);
    }
    if (pid == 0) {
        read(pp_start[0], &n, 1);
        const int child_count = 10;
        for (int i = 0; i < child_count; i++) {
            int pid1 = fork();
            if (pid1 < 0) {
                printf("%s: fork1 failed\n", s);
                exit(1);
            }
            if (pid1 == 0) {
                int err = enter_proc_group(1);
                if (err) {
                    printf("X%d", i);
                    exit(1);
                }
                sleep(3); // stay for a while
                exit(0);
            }
        }
        int err = enter_proc_group(1);
        if (err) {
            write(pp_end[1], &n, 1);
            printf("enter proc group fail\n");
            exit(1);
        }
        write(pp_end[1], &n, 1);
        sleep(5); // wait first proc group proc zombie
        int xstate = wait_children(s, child_count);
        exit(xstate);
    }

    int r = wait_children(s, 2);
    close(pp_start[0]);
    close(pp_start[1]);
    close(pp_end[0]);
    close(pp_end[1]);
    exit(r);
}

// dose enter_proc_group() clean up the group when the last cpu leave group?
// test multiple times to ensure every group been freed correctly.
void enter_pg(char *s) { do_test_n_times(s, do_enter_pg, 9); }

// what if you pass ridiculous pointers to system calls
// that read user memory with copyin?
void copyin(char *s)
{
    uint64 addrs[] = { 0x80000000LL, 0xffffffffffffffff };

    for (int ai = 0; ai < 2; ai++) {
        uint64 addr = addrs[ai];

        int fd = open("copyin1", O_CREATE | O_WRONLY);
        if (fd < 0) {
            printf("open(copyin1) failed\n");
            exit(1);
        }
        int n = write(fd, (void *)addr, 8192);
        if (n >= 0) {
            printf("write(fd, %p, 8192) returned %d, not -1\n", addr, n);
            exit(1);
        }
        close(fd);
        unlink("copyin1");

        n = write(1, (char *)addr, 8192);
        if (n > 0) {
            printf("write(1, %p, 8192) returned %d, not -1 or 0\n", addr, n);
            exit(1);
        }

        int fds[2];
        if (pipe(fds) < 0) {
            printf("pipe() failed\n");
            exit(1);
        }
        n = write(fds[1], (char *)addr, 8192);
        if (n > 0) {
            printf("write(pipe, %p, 8192) returned %d, not -1 or 0\n", addr, n);
            exit(1);
        }
        close(fds[0]);
        close(fds[1]);
    }
}

// what if you pass ridiculous pointers to system calls
// that write user memory with copyout?
void copyout(char *s)
{
    uint64 addrs[] = { 0x80000000LL, 0xffffffffffffffff };

    for (int ai = 0; ai < 2; ai++) {
        uint64 addr = addrs[ai];

        int fd = open("README", 0);
        if (fd < 0) {
            printf("open(README) failed\n");
            exit(1);
        }
        int n = read(fd, (void *)addr, 8192);
        if (n > 0) {
            printf("read(fd, %p, 8192) returned %d, not -1 or 0\n", addr, n);
            exit(1);
        }
        close(fd);

        int fds[2];
        if (pipe(fds) < 0) {
            printf("pipe() failed\n");
            exit(1);
        }
        n = write(fds[1], "x", 1);
        if (n != 1) {
            printf("pipe write failed\n");
            exit(1);
        }
        n = read(fds[0], (void *)addr, 8192);
        if (n > 0) {
            printf("read(pipe, %p, 8192) returned %d, not -1 or 0\n", addr, n);
            exit(1);
        }
        close(fds[0]);
        close(fds[1]);
    }
}

// what if you pass ridiculous string pointers to system calls?
void copyinstr1(char *s)
{
    uint64 addrs[] = { 0x80000000LL, 0xffffffffffffffff };

    for (int ai = 0; ai < 2; ai++) {
        uint64 addr = addrs[ai];

        int fd = open((char *)addr, O_CREATE | O_WRONLY);
        if (fd >= 0) {
            printf("open(%p) returned %d, not -1\n", addr, fd);
            exit(1);
        }
    }
}

// what if a string system call argument is exactly the size
// of the kernel buffer it is copied into, so that the null
// would fall just beyond the end of the kernel buffer?
void copyinstr2(char *s)
{
    char b[MAXPATH + 1];

    for (int i = 0; i < MAXPATH; i++)
        b[i] = 'x';
    b[MAXPATH] = '\0';

    int ret = unlink(b);
    if (ret != -1) {
        printf("unlink(%s) returned %d, not -1\n", b, ret);
        exit(1);
    }

    int fd = open(b, O_CREATE | O_WRONLY);
    if (fd != -1) {
        printf("open(%s) returned %d, not -1\n", b, fd);
        exit(1);
    }

    ret = link(b, b);
    if (ret != -1) {
        printf("link(%s, %s) returned %d, not -1\n", b, b, ret);
        exit(1);
    }

    char *args[] = { "xx", 0 };
    ret = exec(b, args);
    if (ret != -1) {
        printf("exec(%s) returned %d, not -1\n", b, fd);
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        printf("fork failed\n");
        exit(1);
    }
    if (pid == 0) {
        static char big[PGSIZE + 1];
        for (int i = 0; i < PGSIZE; i++)
            big[i] = 'x';
        big[PGSIZE] = '\0';
        char *args2[] = { big, big, big, 0 };
        ret = exec("echo", args2);
        if (ret != -1) {
            printf("exec(echo, BIG) returned %d, not -1\n", fd);
            exit(1);
        }
        exit(747); // OK
    }

    int st = 0;
    wait(&st);
    if (st != 747) {
        printf("exec(echo, BIG) succeeded, should have failed\n");
        exit(1);
    }
}

// what if a string argument crosses over the end of last user page?
void copyinstr3(char *s)
{
    sbrk(8192);
    uint64 top = (uint64)sbrk(0);
    if ((top % PGSIZE) != 0) {
        sbrk(PGSIZE - (top % PGSIZE));
    }
    top = (uint64)sbrk(0);
    if (top % PGSIZE) {
        printf("oops\n");
        exit(1);
    }

    char *b = (char *)(top - 1);
    *b = 'x';

    int ret = unlink(b);
    if (ret != -1) {
        printf("unlink(%s) returned %d, not -1\n", b, ret);
        exit(1);
    }

    int fd = open(b, O_CREATE | O_WRONLY);
    if (fd != -1) {
        printf("open(%s) returned %d, not -1\n", b, fd);
        exit(1);
    }

    ret = link(b, b);
    if (ret != -1) {
        printf("link(%s, %s) returned %d, not -1\n", b, b, ret);
        exit(1);
    }

    char *args[] = { "xx", 0 };
    ret = exec(b, args);
    if (ret != -1) {
        printf("exec(%s) returned %d, not -1\n", b, fd);
        exit(1);
    }
}

// test O_TRUNC.
void truncate1(char *s)
{
    char buf[32];

    unlink("truncfile");
    int fd1 = open("truncfile", O_CREATE | O_WRONLY | O_TRUNC);
    write(fd1, "abcd", 4);
    close(fd1);

    int fd2 = open("truncfile", O_RDONLY);
    int n = read(fd2, buf, sizeof(buf));
    if (n != 4) {
        printf("%s: read %d bytes, wanted 4\n", s, n);
        exit(1);
    }

    fd1 = open("truncfile", O_WRONLY | O_TRUNC);

    int fd3 = open("truncfile", O_RDONLY);
    n = read(fd3, buf, sizeof(buf));
    if (n != 0) {
        printf("aaa fd3=%d\n", fd3);
        printf("%s: read %d bytes, wanted 0\n", s, n);
        exit(1);
    }

    n = read(fd2, buf, sizeof(buf));
    if (n != 0) {
        printf("bbb fd2=%d\n", fd2);
        printf("%s: read %d bytes, wanted 0\n", s, n);
        exit(1);
    }

    write(fd1, "abcdef", 6);

    n = read(fd3, buf, sizeof(buf));
    if (n != 6) {
        printf("%s: read %d bytes, wanted 6\n", s, n);
        exit(1);
    }

    n = read(fd2, buf, sizeof(buf));
    if (n != 2) {
        printf("%s: read %d bytes, wanted 2\n", s, n);
        exit(1);
    }

    unlink("truncfile");

    close(fd1);
    close(fd2);
    close(fd3);
}

// write to an open FD whose file has just been truncated.
// this causes a write at an offset beyond the end of the file.
// such writes fail on xv6 (unlike POSIX) but at least
// they don't crash.
void truncate2(char *s)
{
    unlink("truncfile");

    int fd1 = open("truncfile", O_CREATE | O_TRUNC | O_WRONLY);
    write(fd1, "abcd", 4);

    int fd2 = open("truncfile", O_TRUNC | O_WRONLY);

    int n = write(fd1, "x", 1);
    if (n != -1) {
        printf("%s: write returned %d, expected -1\n", s, n);
        exit(1);
    }

    unlink("truncfile");
    close(fd1);
    close(fd2);
}

void truncate3(char *s)
{
    int pid, xstatus;

    close(open("truncfile", O_CREATE | O_TRUNC | O_WRONLY));

    pid = fork();
    if (pid < 0) {
        printf("%s: fork failed\n", s);
        exit(1);
    }

    if (pid == 0) {
        for (int i = 0; i < 100; i++) {
            char buf[32];
            int fd = open("truncfile", O_WRONLY);
            if (fd < 0) {
                printf("%s: open failed\n", s);
                exit(1);
            }
            int n = write(fd, "1234567890", 10);
            if (n != 10) {
                printf("%s: write got %d, expected 10\n", s, n);
                exit(1);
            }
            close(fd);
            fd = open("truncfile", O_RDONLY);
            read(fd, buf, sizeof(buf));
            close(fd);
        }
        exit(0);
    }

    for (int i = 0; i < 150; i++) {
        int fd = open("truncfile", O_CREATE | O_WRONLY | O_TRUNC);
        if (fd < 0) {
            printf("%s: open failed\n", s);
            exit(1);
        }
        int n = write(fd, "xxx", 3);
        if (n != 3) {
            printf("%s: write got %d, expected 3\n", s, n);
            exit(1);
        }
        close(fd);
    }

    wait(&xstatus);
    unlink("truncfile");
    exit(xstatus);
}

// does chdir() call iput(p->cwd) in a transaction?
void iputtest(char *s)
{
    if (mkdir("iputdir") < 0) {
        printf("%s: mkdir failed\n", s);
        exit(1);
    }
    if (chdir("iputdir") < 0) {
        printf("%s: chdir iputdir failed\n", s);
        exit(1);
    }
    if (unlink("../iputdir") < 0) {
        printf("%s: unlink ../iputdir failed\n", s);
        exit(1);
    }
    if (chdir("/") < 0) {
        printf("%s: chdir / failed\n", s);
        exit(1);
    }
}

// does exit() call iput(p->cwd) in a transaction?
void exitiputtest(char *s)
{
    int pid, xstatus;

    pid = fork();
    if (pid < 0) {
        printf("%s: fork failed\n", s);
        exit(1);
    }
    if (pid == 0) {
        if (mkdir("iputdir") < 0) {
            printf("%s: mkdir failed\n", s);
            exit(1);
        }
        if (chdir("iputdir") < 0) {
            printf("%s: child chdir failed\n", s);
            exit(1);
        }
        if (unlink("../iputdir") < 0) {
            printf("%s: unlink ../iputdir failed\n", s);
            exit(1);
        }
        exit(0);
    }
    wait(&xstatus);
    exit(xstatus);
}

// does the error path in open() for attempt to write a
// directory call iput() in a transaction?
// needs a hacked kernel that pauses just after the namei()
// call in sys_open():
//    if((ip = namei(path)) == 0)
//      return -1;
//    {
//      int i;
//      for(i = 0; i < 10000; i++)
//        yield();
//    }
void openiputtest(char *s)
{
    int pid, xstatus;

    if (mkdir("oidir") < 0) {
        printf("%s: mkdir oidir failed\n", s);
        exit(1);
    }
    pid = fork();
    if (pid < 0) {
        printf("%s: fork failed\n", s);
        exit(1);
    }
    if (pid == 0) {
        int fd = open("oidir", O_RDWR);
        if (fd >= 0) {
            printf("%s: open directory for write succeeded\n", s);
            exit(1);
        }
        exit(0);
    }
    sleep(1);
    if (unlink("oidir") != 0) {
        printf("%s: unlink failed\n", s);
        exit(1);
    }
    wait(&xstatus);
    exit(xstatus);
}

// simple file system tests

void opentest(char *s)
{
    int fd;

    fd = open("echo", 0);
    if (fd < 0) {
        printf("%s: open echo failed!\n", s);
        exit(1);
    }
    close(fd);
    fd = open("doesnotexist", 0);
    if (fd >= 0) {
        printf("%s: open doesnotexist succeeded!\n", s);
        exit(1);
    }
}

void writetest(char *s)
{
    int fd;
    int i;
    enum { N = 100, SZ = 10 };

    fd = open("small", O_CREATE | O_RDWR);
    if (fd < 0) {
        printf("%s: error: creat small failed!\n", s);
        exit(1);
    }
    for (i = 0; i < N; i++) {
        if (write(fd, "aaaaaaaaaa", SZ) != SZ) {
            printf("%s: error: write aa %d new file failed\n", i);
            exit(1);
        }
        if (write(fd, "bbbbbbbbbb", SZ) != SZ) {
            printf("%s: error: write bb %d new file failed\n", i);
            exit(1);
        }
    }
    close(fd);
    fd = open("small", O_RDONLY);
    if (fd < 0) {
        printf("%s: error: open small failed!\n", s);
        exit(1);
    }
    i = read(fd, buf, N * SZ * 2);
    if (i != N * SZ * 2) {
        printf("%s: read failed\n", s);
        exit(1);
    }
    close(fd);

    if (unlink("small") < 0) {
        printf("%s: unlink small failed\n", s);
        exit(1);
    }
}

void writebig(char *s)
{
    int i, fd, n;

    fd = open("big", O_CREATE | O_RDWR);
    if (fd < 0) {
        printf("%s: error: creat big failed!\n", s);
        exit(1);
    }

    for (i = 0; i < MAXFILE; i++) {
        ((int *)buf)[0] = i;
        if (write(fd, buf, BSIZE) != BSIZE) {
            printf("%s: error: write big file failed\n", i);
            exit(1);
        }
    }

    close(fd);

    fd = open("big", O_RDONLY);
    if (fd < 0) {
        printf("%s: error: open big failed!\n", s);
        exit(1);
    }

    n = 0;
    for (;;) {
        i = read(fd, buf, BSIZE);
        if (i == 0) {
            if (n == MAXFILE - 1) {
                printf("%s: read only %d blocks from big", n);
                exit(1);
            }
            break;
        } else if (i != BSIZE) {
            printf("%s: read failed %d\n", i);
            exit(1);
        }
        if (((int *)buf)[0] != n) {
            printf("%s: read content of block %d is %d\n", n, ((int *)buf)[0]);
            exit(1);
        }
        n++;
    }
    close(fd);
    if (unlink("big") < 0) {
        printf("%s: unlink big failed\n", s);
        exit(1);
    }
}

// many creates, followed by unlink test
void createtest(char *s)
{
    int i, fd;
    enum { N = 52 };

    name[0] = 'a';
    name[2] = '\0';
    for (i = 0; i < N; i++) {
        name[1] = '0' + i;
        fd = open(name, O_CREATE | O_RDWR);
        close(fd);
    }
    name[0] = 'a';
    name[2] = '\0';
    for (i = 0; i < N; i++) {
        name[1] = '0' + i;
        unlink(name);
    }
}

void dirtest(char *s)
{
    printf("mkdir test\n");

    if (mkdir("dir0") < 0) {
        printf("%s: mkdir failed\n", s);
        exit(1);
    }

    if (chdir("dir0") < 0) {
        printf("%s: chdir dir0 failed\n", s);
        exit(1);
    }

    if (chdir("..") < 0) {
        printf("%s: chdir .. failed\n", s);
        exit(1);
    }

    if (unlink("dir0") < 0) {
        printf("%s: unlink dir0 failed\n", s);
        exit(1);
    }
    printf("%s: mkdir test ok\n");
}

void exectest(char *s)
{
    int fd, xstatus, pid;
    char *echoargv[] = { "echo", "OK", 0 };
    char buf[3];

    unlink("echo-ok");
    pid = fork();
    if (pid < 0) {
        printf("%s: fork failed\n", s);
        exit(1);
    }
    if (pid == 0) {
        close(1);
        fd = open("echo-ok", O_CREATE | O_WRONLY);
        if (fd < 0) {
            printf("%s: create failed\n", s);
            exit(1);
        }
        if (fd != 1) {
            printf("%s: wrong fd\n", s);
            exit(1);
        }
        if (exec("echo", echoargv) < 0) {
            printf("%s: exec echo failed\n", s);
            exit(1);
        }
        // won't get to here
    }
    if (wait(&xstatus) != pid) {
        printf("%s: wait failed!\n", s);
    }
    if (xstatus != 0)
        exit(xstatus);

    fd = open("echo-ok", O_RDONLY);
    if (fd < 0) {
        printf("%s: open failed\n", s);
        exit(1);
    }
    if (read(fd, buf, 2) != 2) {
        printf("%s: read failed\n", s);
        exit(1);
    }
    unlink("echo-ok");
    if (buf[0] == 'O' && buf[1] == 'K')
        exit(0);
    else {
        printf("%s: wrong output\n", s);
        exit(1);
    }
}

// simple fork and pipe read/write

void pipe1(char *s)
{
    int fds[2], pid, xstatus;
    int seq, i, n, cc, total;
    enum { N = 5, SZ = 1033 };

    if (pipe(fds) != 0) {
        printf("%s: pipe() failed\n", s);
        exit(1);
    }
    pid = fork();
    seq = 0;
    if (pid == 0) {
        close(fds[0]);
        for (n = 0; n < N; n++) {
            for (i = 0; i < SZ; i++)
                buf[i] = seq++;
            if (write(fds[1], buf, SZ) != SZ) {
                printf("%s: pipe1 oops 1\n", s);
                exit(1);
            }
        }
        exit(0);
    } else if (pid > 0) {
        close(fds[1]);
        total = 0;
        cc = 1;
        while ((n = read(fds[0], buf, cc)) > 0) {
            for (i = 0; i < n; i++) {
                if ((buf[i] & 0xff) != (seq++ & 0xff)) {
                    printf("%s: pipe1 oops 2\n", s);
                    return;
                }
            }
            total += n;
            cc = cc * 2;
            if (cc > sizeof(buf))
                cc = sizeof(buf);
        }
        if (total != N * SZ) {
            printf("%s: pipe1 oops 3 total %d\n", total);
            exit(1);
        }
        close(fds[0]);
        wait(&xstatus);
        exit(xstatus);
    } else {
        printf("%s: fork() failed\n", s);
        exit(1);
    }
}

// meant to be run w/ at most two CPUs
void preempt(char *s)
{
    int pid1, pid2, pid3;
    int pfds[2];

    pid1 = fork();
    if (pid1 < 0) {
        printf("%s: fork failed");
        exit(1);
    }
    if (pid1 == 0)
        for (;;)
            ;

    pid2 = fork();
    if (pid2 < 0) {
        printf("%s: fork failed\n", s);
        exit(1);
    }
    if (pid2 == 0)
        for (;;)
            ;

    pipe(pfds);
    pid3 = fork();
    if (pid3 < 0) {
        printf("%s: fork failed\n", s);
        exit(1);
    }
    if (pid3 == 0) {
        close(pfds[0]);
        if (write(pfds[1], "x", 1) != 1)
            printf("%s: preempt write error");
        close(pfds[1]);
        for (;;)
            ;
    }

    close(pfds[1]);
    if (read(pfds[0], buf, sizeof(buf)) != 1) {
        printf("%s: preempt read error");
        return;
    }
    close(pfds[0]);
    printf("kill... ");
    kill(pid1);
    kill(pid2);
    kill(pid3);
    printf("wait... ");
    wait(0);
    wait(0);
    wait(0);
}

// try to find any races between exit and wait
void exitwait(char *s)
{
    int i, pid;

    for (i = 0; i < 100; i++) {
        pid = fork();
        if (pid < 0) {
            printf("%s: fork failed\n", s);
            exit(1);
        }
        if (pid) {
            int xstate;
            if (wait(&xstate) != pid) {
                printf("%s: wait wrong pid\n", s);
                exit(1);
            }
            if (i != xstate) {
                printf("%s: wait wrong exit status\n", s);
                exit(1);
            }
        } else {
            exit(i);
        }
    }
}

// try to find races in the reparenting
// code that handles a parent exiting
// when it still has live children.
void reparent(char *s)
{
    int master_pid = getpid();
    for (int i = 0; i < 200; i++) {
        int pid = fork();
        if (pid < 0) {
            printf("%s: fork failed\n", s);
            exit(1);
        }
        if (pid) {
            if (wait(0) != pid) {
                printf("%s: wait wrong pid\n", s);
                exit(1);
            }
        } else {
            int pid2 = fork();
            if (pid2 < 0) {
                kill(master_pid);
                exit(1);
            }
            exit(0);
        }
    }
    exit(0);
}

// what if two children exit() at the same time?
void twochildren(char *s)
{
    for (int i = 0; i < 1000; i++) {
        int pid1 = fork();
        if (pid1 < 0) {
            printf("%s: fork failed\n", s);
            exit(1);
        }
        if (pid1 == 0) {
            exit(0);
        } else {
            int pid2 = fork();
            if (pid2 < 0) {
                printf("%s: fork failed\n", s);
                exit(1);
            }
            if (pid2 == 0) {
                exit(0);
            } else {
                wait(0);
                wait(0);
            }
        }
    }
}

// concurrent forks to try to expose locking bugs.
void forkfork(char *s)
{
    enum { N = 2 };

    for (int i = 0; i < N; i++) {
        int pid = fork();
        if (pid < 0) {
            printf("%s: fork failed", s);
            exit(1);
        }
        if (pid == 0) {
            for (int j = 0; j < 200; j++) {
                int pid1 = fork();
                if (pid1 < 0) {
                    exit(1);
                }
                if (pid1 == 0) {
                    exit(0);
                }
                wait(0);
            }
            exit(0);
        }
    }

    int xstatus;
    for (int i = 0; i < N; i++) {
        wait(&xstatus);
        if (xstatus != 0) {
            printf("%s: fork in child failed", s);
            exit(1);
        }
    }
}

void forkforkfork(char *s)
{
    unlink("stopforking");

    int pid = fork();
    if (pid < 0) {
        printf("%s: fork failed", s);
        exit(1);
    }
    if (pid == 0) {
        while (1) {
            int fd = open("stopforking", 0);
            if (fd >= 0) {
                exit(0);
            }
            if (fork() < 0) {
                close(open("stopforking", O_CREATE | O_RDWR));
            }
        }

        exit(0);
    }

    sleep(20); // two seconds
    close(open("stopforking", O_CREATE | O_RDWR));
    wait(0);
    sleep(10); // one second
}

// regression test. does reparent() violate the parent-then-child
// locking order when giving away a child to init, so that exit()
// deadlocks against init's wait()? also used to trigger a "panic:
// release" due to exit() releasing a different p->parent->lock than
// it acquired.
void reparent2(char *s)
{
    for (int i = 0; i < 800; i++) {
        int pid1 = fork();
        if (pid1 < 0) {
            printf("fork failed\n");
            exit(1);
        }
        if (pid1 == 0) {
            fork();
            fork();
            exit(0);
        }
        wait(0);
    }

    exit(0);
}

// allocate all mem, free it, and allocate again
void mem(char *s)
{
    void *m1, *m2;
    int pid;

    if ((pid = fork()) == 0) {
        m1 = 0;
        while ((m2 = malloc(10001)) != 0) {
            *(char **)m2 = m1;
            m1 = m2;
        }
        while (m1) {
            m2 = *(char **)m1;
            free(m1);
            m1 = m2;
        }
        m1 = malloc(1024 * 20);
        if (m1 == 0) {
            printf("couldn't allocate mem?!!\n", s);
            exit(1);
        }
        free(m1);
        exit(0);
    } else {
        int xstatus;
        wait(&xstatus);
        if (xstatus == -1) {
            // probably page fault, so might be lazy lab,
            // so OK.
            exit(0);
        }
        exit(xstatus);
    }
}

// More file system tests

// two processes write to the same file descriptor
// is the offset shared? does inode locking work?
void sharedfd(char *s)
{
    int fd, pid, i, n, nc, np;
    enum { N = 1000, SZ = 10 };
    char buf[SZ];

    unlink("sharedfd");
    fd = open("sharedfd", O_CREATE | O_RDWR);
    if (fd < 0) {
        printf("%s: cannot open sharedfd for writing", s);
        exit(1);
    }
    pid = fork();
    memset(buf, pid == 0 ? 'c' : 'p', sizeof(buf));
    for (i = 0; i < N; i++) {
        if (write(fd, buf, sizeof(buf)) != sizeof(buf)) {
            printf("%s: write sharedfd failed\n", s);
            exit(1);
        }
    }
    if (pid == 0) {
        exit(0);
    } else {
        int xstatus;
        wait(&xstatus);
        if (xstatus != 0)
            exit(xstatus);
    }

    close(fd);
    fd = open("sharedfd", 0);
    if (fd < 0) {
        printf("%s: cannot open sharedfd for reading\n", s);
        exit(1);
    }
    nc = np = 0;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        for (i = 0; i < sizeof(buf); i++) {
            if (buf[i] == 'c')
                nc++;
            if (buf[i] == 'p')
                np++;
        }
    }
    close(fd);
    unlink("sharedfd");
    if (nc == N * SZ && np == N * SZ) {
        exit(0);
    } else {
        printf("%s: nc/np test fails\n", s);
        exit(1);
    }
}

// four processes write different files at the same
// time, to test block allocation.
void fourfiles(char *s)
{
    int fd, pid, i, j, n, total, pi;
    char *names[] = { "f0", "f1", "f2", "f3" };
    char *fname;
    enum { N = 12, NCHILD = 4, SZ = 500 };

    for (pi = 0; pi < NCHILD; pi++) {
        fname = names[pi];
        unlink(fname);

        pid = fork();
        if (pid < 0) {
            printf("fork failed\n", s);
            exit(1);
        }

        if (pid == 0) {
            fd = open(fname, O_CREATE | O_RDWR);
            if (fd < 0) {
                printf("create failed\n", s);
                exit(1);
            }

            memset(buf, '0' + pi, SZ);
            for (i = 0; i < N; i++) {
                if ((n = write(fd, buf, SZ)) != SZ) {
                    printf("write failed %d\n", n);
                    exit(1);
                }
            }
            exit(0);
        }
    }

    int xstatus;
    for (pi = 0; pi < NCHILD; pi++) {
        wait(&xstatus);
        if (xstatus != 0)
            exit(xstatus);
    }

    for (i = 0; i < NCHILD; i++) {
        fname = names[i];
        fd = open(fname, 0);
        total = 0;
        while ((n = read(fd, buf, sizeof(buf))) > 0) {
            for (j = 0; j < n; j++) {
                if (buf[j] != '0' + i) {
                    printf("wrong char\n", s);
                    exit(1);
                }
            }
            total += n;
        }
        close(fd);
        if (total != N * SZ) {
            printf("wrong length %d\n", total);
            exit(1);
        }
        unlink(fname);
    }
}

// four processes create and delete different files in same directory
void createdelete(char *s)
{
    enum { N = 20, NCHILD = 4 };
    int pid, i, fd, pi;
    char name[32];

    for (pi = 0; pi < NCHILD; pi++) {
        pid = fork();
        if (pid < 0) {
            printf("fork failed\n", s);
            exit(1);
        }

        if (pid == 0) {
            name[0] = 'p' + pi;
            name[2] = '\0';
            for (i = 0; i < N; i++) {
                name[1] = '0' + i;
                fd = open(name, O_CREATE | O_RDWR);
                if (fd < 0) {
                    printf("%s: create failed\n", s);
                    exit(1);
                }
                close(fd);
                if (i > 0 && (i % 2) == 0) {
                    name[1] = '0' + (i / 2);
                    if (unlink(name) < 0) {
                        printf("%s: unlink failed\n", s);
                        exit(1);
                    }
                }
            }
            exit(0);
        }
    }

    int xstatus;
    for (pi = 0; pi < NCHILD; pi++) {
        wait(&xstatus);
        if (xstatus != 0)
            exit(1);
    }

    name[0] = name[1] = name[2] = 0;
    for (i = 0; i < N; i++) {
        for (pi = 0; pi < NCHILD; pi++) {
            name[0] = 'p' + pi;
            name[1] = '0' + i;
            fd = open(name, 0);
            if ((i == 0 || i >= N / 2) && fd < 0) {
                printf("%s: oops createdelete %s didn't exist\n", s, name);
                exit(1);
            } else if ((i >= 1 && i < N / 2) && fd >= 0) {
                printf("%s: oops createdelete %s did exist\n", s, name);
                exit(1);
            }
            if (fd >= 0)
                close(fd);
        }
    }

    for (i = 0; i < N; i++) {
        for (pi = 0; pi < NCHILD; pi++) {
            name[0] = 'p' + i;
            name[1] = '0' + i;
            unlink(name);
        }
    }
}

// can I unlink a file and still read it?
void unlinkread(char *s)
{
    enum { SZ = 5 };
    int fd, fd1;

    fd = open("unlinkread", O_CREATE | O_RDWR);
    if (fd < 0) {
        printf("%s: create unlinkread failed\n", s);
        exit(1);
    }
    write(fd, "hello", SZ);
    close(fd);

    fd = open("unlinkread", O_RDWR);
    if (fd < 0) {
        printf("%s: open unlinkread failed\n", s);
        exit(1);
    }
    if (unlink("unlinkread") != 0) {
        printf("%s: unlink unlinkread failed\n", s);
        exit(1);
    }

    fd1 = open("unlinkread", O_CREATE | O_RDWR);
    write(fd1, "yyy", 3);
    close(fd1);

    if (read(fd, buf, sizeof(buf)) != SZ) {
        printf("%s: unlinkread read failed", s);
        exit(1);
    }
    if (buf[0] != 'h') {
        printf("%s: unlinkread wrong data\n", s);
        exit(1);
    }
    if (write(fd, buf, 10) != 10) {
        printf("%s: unlinkread write failed\n", s);
        exit(1);
    }
    close(fd);
    unlink("unlinkread");
}

void linktest(char *s)
{
    enum { SZ = 5 };
    int fd;

    unlink("lf1");
    unlink("lf2");

    fd = open("lf1", O_CREATE | O_RDWR);
    if (fd < 0) {
        printf("%s: create lf1 failed\n", s);
        exit(1);
    }
    if (write(fd, "hello", SZ) != SZ) {
        printf("%s: write lf1 failed\n", s);
        exit(1);
    }
    close(fd);

    if (link("lf1", "lf2") < 0) {
        printf("%s: link lf1 lf2 failed\n", s);
        exit(1);
    }
    unlink("lf1");

    if (open("lf1", 0) >= 0) {
        printf("%s: unlinked lf1 but it is still there!\n", s);
        exit(1);
    }

    fd = open("lf2", 0);
    if (fd < 0) {
        printf("%s: open lf2 failed\n", s);
        exit(1);
    }
    if (read(fd, buf, sizeof(buf)) != SZ) {
        printf("%s: read lf2 failed\n", s);
        exit(1);
    }
    close(fd);

    if (link("lf2", "lf2") >= 0) {
        printf("%s: link lf2 lf2 succeeded! oops\n", s);
        exit(1);
    }

    unlink("lf2");
    if (link("lf2", "lf1") >= 0) {
        printf("%s: link non-existant succeeded! oops\n", s);
        exit(1);
    }

    if (link(".", "lf1") >= 0) {
        printf("%s: link . lf1 succeeded! oops\n", s);
        exit(1);
    }
}

// test concurrent create/link/unlink of the same file
void concreate(char *s)
{
    enum { N = 40 };
    char file[3];
    int i, pid, n, fd;
    char fa[N];
    struct {
        ushort inum;
        char name[DIRSIZ];
    } de;

    file[0] = 'C';
    file[2] = '\0';
    for (i = 0; i < N; i++) {
        file[1] = '0' + i;
        unlink(file);
        pid = fork();
        if (pid && (i % 3) == 1) {
            link("C0", file);
        } else if (pid == 0 && (i % 5) == 1) {
            link("C0", file);
        } else {
            fd = open(file, O_CREATE | O_RDWR);
            if (fd < 0) {
                printf("concreate create %s failed\n", file);
                exit(1);
            }
            close(fd);
        }
        if (pid == 0) {
            exit(0);
        } else {
            int xstatus;
            wait(&xstatus);
            if (xstatus != 0)
                exit(1);
        }
    }

    memset(fa, 0, sizeof(fa));
    fd = open(".", 0);
    n = 0;
    while (read(fd, &de, sizeof(de)) > 0) {
        if (de.inum == 0)
            continue;
        if (de.name[0] == 'C' && de.name[2] == '\0') {
            i = de.name[1] - '0';
            if (i < 0 || i >= sizeof(fa)) {
                printf("%s: concreate weird file %s\n", s, de.name);
                exit(1);
            }
            if (fa[i]) {
                printf("%s: concreate duplicate file %s\n", s, de.name);
                exit(1);
            }
            fa[i] = 1;
            n++;
        }
    }
    close(fd);

    if (n != N) {
        printf("%s: concreate not enough files in directory listing\n", s);
        exit(1);
    }

    for (i = 0; i < N; i++) {
        file[1] = '0' + i;
        pid = fork();
        if (pid < 0) {
            printf("%s: fork failed\n", s);
            exit(1);
        }
        if (((i % 3) == 0 && pid == 0) || ((i % 3) == 1 && pid != 0)) {
            close(open(file, 0));
            close(open(file, 0));
            close(open(file, 0));
            close(open(file, 0));
            close(open(file, 0));
            close(open(file, 0));
        } else {
            unlink(file);
            unlink(file);
            unlink(file);
            unlink(file);
            unlink(file);
            unlink(file);
        }
        if (pid == 0)
            exit(0);
        else
            wait(0);
    }
}

// another concurrent link/unlink/create test,
// to look for deadlocks.
void linkunlink(char *s)
{
    int pid, i;

    unlink("x");
    pid = fork();
    if (pid < 0) {
        printf("%s: fork failed\n", s);
        exit(1);
    }

    unsigned int x = (pid ? 1 : 97);
    for (i = 0; i < 100; i++) {
        x = x * 1103515245 + 12345;
        if ((x % 3) == 0) {
            close(open("x", O_RDWR | O_CREATE));
        } else if ((x % 3) == 1) {
            link("cat", "x");
        } else {
            unlink("x");
        }
    }

    if (pid)
        wait(0);
    else
        exit(0);
}

// directory that uses indirect blocks
void bigdir(char *s)
{
    enum { N = 500 };
    int i, fd;
    char name[10];

    unlink("bd");

    fd = open("bd", O_CREATE);
    if (fd < 0) {
        printf("%s: bigdir create failed\n", s);
        exit(1);
    }
    close(fd);

    for (i = 0; i < N; i++) {
        name[0] = 'x';
        name[1] = '0' + (i / 64);
        name[2] = '0' + (i % 64);
        name[3] = '\0';
        if (link("bd", name) != 0) {
            printf("%s: bigdir link(bd, %s) failed\n", s, name);
            exit(1);
        }
    }

    unlink("bd");
    for (i = 0; i < N; i++) {
        name[0] = 'x';
        name[1] = '0' + (i / 64);
        name[2] = '0' + (i % 64);
        name[3] = '\0';
        if (unlink(name) != 0) {
            printf("%s: bigdir unlink failed", s);
            exit(1);
        }
    }
}

void subdir(char *s)
{
    int fd, cc;

    unlink("ff");
    if (mkdir("dd") != 0) {
        printf("%s: mkdir dd failed\n", s);
        exit(1);
    }

    fd = open("dd/ff", O_CREATE | O_RDWR);
    if (fd < 0) {
        printf("%s: create dd/ff failed\n", s);
        exit(1);
    }
    write(fd, "ff", 2);
    close(fd);

    if (unlink("dd") >= 0) {
        printf("%s: unlink dd (non-empty dir) succeeded!\n", s);
        exit(1);
    }

    if (mkdir("/dd/dd") != 0) {
        printf("subdir mkdir dd/dd failed\n", s);
        exit(1);
    }

    fd = open("dd/dd/ff", O_CREATE | O_RDWR);
    if (fd < 0) {
        printf("%s: create dd/dd/ff failed\n", s);
        exit(1);
    }
    write(fd, "FF", 2);
    close(fd);

    fd = open("dd/dd/../ff", 0);
    if (fd < 0) {
        printf("%s: open dd/dd/../ff failed\n", s);
        exit(1);
    }
    cc = read(fd, buf, sizeof(buf));
    if (cc != 2 || buf[0] != 'f') {
        printf("%s: dd/dd/../ff wrong content\n", s);
        exit(1);
    }
    close(fd);

    if (link("dd/dd/ff", "dd/dd/ffff") != 0) {
        printf("link dd/dd/ff dd/dd/ffff failed\n", s);
        exit(1);
    }

    if (unlink("dd/dd/ff") != 0) {
        printf("%s: unlink dd/dd/ff failed\n", s);
        exit(1);
    }
    if (open("dd/dd/ff", O_RDONLY) >= 0) {
        printf("%s: open (unlinked) dd/dd/ff succeeded\n", s);
        exit(1);
    }

    if (chdir("dd") != 0) {
        printf("%s: chdir dd failed\n", s);
        exit(1);
    }
    if (chdir("dd/../../dd") != 0) {
        printf("%s: chdir dd/../../dd failed\n", s);
        exit(1);
    }
    if (chdir("dd/../../../dd") != 0) {
        printf("chdir dd/../../dd failed\n", s);
        exit(1);
    }
    if (chdir("./..") != 0) {
        printf("%s: chdir ./.. failed\n", s);
        exit(1);
    }

    fd = open("dd/dd/ffff", 0);
    if (fd < 0) {
        printf("%s: open dd/dd/ffff failed\n", s);
        exit(1);
    }
    if (read(fd, buf, sizeof(buf)) != 2) {
        printf("%s: read dd/dd/ffff wrong len\n", s);
        exit(1);
    }
    close(fd);

    if (open("dd/dd/ff", O_RDONLY) >= 0) {
        printf("%s: open (unlinked) dd/dd/ff succeeded!\n", s);
        exit(1);
    }

    if (open("dd/ff/ff", O_CREATE | O_RDWR) >= 0) {
        printf("%s: create dd/ff/ff succeeded!\n", s);
        exit(1);
    }
    if (open("dd/xx/ff", O_CREATE | O_RDWR) >= 0) {
        printf("%s: create dd/xx/ff succeeded!\n", s);
        exit(1);
    }
    if (open("dd", O_CREATE) >= 0) {
        printf("%s: create dd succeeded!\n", s);
        exit(1);
    }
    if (open("dd", O_RDWR) >= 0) {
        printf("%s: open dd rdwr succeeded!\n", s);
        exit(1);
    }
    if (open("dd", O_WRONLY) >= 0) {
        printf("%s: open dd wronly succeeded!\n", s);
        exit(1);
    }
    if (link("dd/ff/ff", "dd/dd/xx") == 0) {
        printf("%s: link dd/ff/ff dd/dd/xx succeeded!\n", s);
        exit(1);
    }
    if (link("dd/xx/ff", "dd/dd/xx") == 0) {
        printf("%s: link dd/xx/ff dd/dd/xx succeeded!\n", s);
        exit(1);
    }
    if (link("dd/ff", "dd/dd/ffff") == 0) {
        printf("%s: link dd/ff dd/dd/ffff succeeded!\n", s);
        exit(1);
    }
    if (mkdir("dd/ff/ff") == 0) {
        printf("%s: mkdir dd/ff/ff succeeded!\n", s);
        exit(1);
    }
    if (mkdir("dd/xx/ff") == 0) {
        printf("%s: mkdir dd/xx/ff succeeded!\n", s);
        exit(1);
    }
    if (mkdir("dd/dd/ffff") == 0) {
        printf("%s: mkdir dd/dd/ffff succeeded!\n", s);
        exit(1);
    }
    if (unlink("dd/xx/ff") == 0) {
        printf("%s: unlink dd/xx/ff succeeded!\n", s);
        exit(1);
    }
    if (unlink("dd/ff/ff") == 0) {
        printf("%s: unlink dd/ff/ff succeeded!\n", s);
        exit(1);
    }
    if (chdir("dd/ff") == 0) {
        printf("%s: chdir dd/ff succeeded!\n", s);
        exit(1);
    }
    if (chdir("dd/xx") == 0) {
        printf("%s: chdir dd/xx succeeded!\n", s);
        exit(1);
    }

    if (unlink("dd/dd/ffff") != 0) {
        printf("%s: unlink dd/dd/ff failed\n", s);
        exit(1);
    }
    if (unlink("dd/ff") != 0) {
        printf("%s: unlink dd/ff failed\n", s);
        exit(1);
    }
    if (unlink("dd") == 0) {
        printf("%s: unlink non-empty dd succeeded!\n", s);
        exit(1);
    }
    if (unlink("dd/dd") < 0) {
        printf("%s: unlink dd/dd failed\n", s);
        exit(1);
    }
    if (unlink("dd") < 0) {
        printf("%s: unlink dd failed\n", s);
        exit(1);
    }
}

// test writes that are larger than the log.
void bigwrite(char *s)
{
    int fd, sz;

    unlink("bigwrite");
    for (sz = 499; sz < (MAXOPBLOCKS + 2) * BSIZE; sz += 471) {
        fd = open("bigwrite", O_CREATE | O_RDWR);
        if (fd < 0) {
            printf("%s: cannot create bigwrite\n", s);
            exit(1);
        }
        int i;
        for (i = 0; i < 2; i++) {
            int cc = write(fd, buf, sz);
            if (cc != sz) {
                printf("%s: write(%d) ret %d\n", s, sz, cc);
                exit(1);
            }
        }
        close(fd);
        unlink("bigwrite");
    }
}

void bigfile(char *s)
{
    enum { N = 20, SZ = 600 };
    int fd, i, total, cc;

    unlink("bigfile.dat");
    fd = open("bigfile.dat", O_CREATE | O_RDWR);
    if (fd < 0) {
        printf("%s: cannot create bigfile", s);
        exit(1);
    }
    for (i = 0; i < N; i++) {
        memset(buf, i, SZ);
        if (write(fd, buf, SZ) != SZ) {
            printf("%s: write bigfile failed\n", s);
            exit(1);
        }
    }
    close(fd);

    fd = open("bigfile.dat", 0);
    if (fd < 0) {
        printf("%s: cannot open bigfile\n", s);
        exit(1);
    }
    total = 0;
    for (i = 0;; i++) {
        cc = read(fd, buf, SZ / 2);
        if (cc < 0) {
            printf("%s: read bigfile failed\n", s);
            exit(1);
        }
        if (cc == 0)
            break;
        if (cc != SZ / 2) {
            printf("%s: short read bigfile\n", s);
            exit(1);
        }
        if (buf[0] != i / 2 || buf[SZ / 2 - 1] != i / 2) {
            printf("%s: read bigfile wrong data\n", s);
            exit(1);
        }
        total += cc;
    }
    close(fd);
    if (total != N * SZ) {
        printf("%s: read bigfile wrong total\n", s);
        exit(1);
    }
    unlink("bigfile.dat");
}

void fourteen(char *s)
{
    int fd;

    // DIRSIZ is 14.

    if (mkdir("12345678901234") != 0) {
        printf("%s: mkdir 12345678901234 failed\n", s);
        exit(1);
    }
    if (mkdir("12345678901234/123456789012345") != 0) {
        printf("%s: mkdir 12345678901234/123456789012345 failed\n", s);
        exit(1);
    }
    fd = open("123456789012345/123456789012345/123456789012345", O_CREATE);
    if (fd < 0) {
        printf("%s: create 123456789012345/123456789012345/123456789012345 "
               "failed\n",
               s);
        exit(1);
    }
    close(fd);
    fd = open("12345678901234/12345678901234/12345678901234", 0);
    if (fd < 0) {
        printf("%s: open 12345678901234/12345678901234/12345678901234 failed\n",
               s);
        exit(1);
    }
    close(fd);

    if (mkdir("12345678901234/12345678901234") == 0) {
        printf("%s: mkdir 12345678901234/12345678901234 succeeded!\n", s);
        exit(1);
    }
    if (mkdir("123456789012345/12345678901234") == 0) {
        printf("%s: mkdir 12345678901234/123456789012345 succeeded!\n", s);
        exit(1);
    }

    // clean up
    unlink("123456789012345/12345678901234");
    unlink("12345678901234/12345678901234");
    unlink("12345678901234/12345678901234/12345678901234");
    unlink("123456789012345/123456789012345/123456789012345");
    unlink("12345678901234/123456789012345");
    unlink("12345678901234");
}

void rmdot(char *s)
{
    if (mkdir("dots") != 0) {
        printf("%s: mkdir dots failed\n", s);
        exit(1);
    }
    if (chdir("dots") != 0) {
        printf("%s: chdir dots failed\n", s);
        exit(1);
    }
    if (unlink(".") == 0) {
        printf("%s: rm . worked!\n", s);
        exit(1);
    }
    if (unlink("..") == 0) {
        printf("%s: rm .. worked!\n", s);
        exit(1);
    }
    if (chdir("/") != 0) {
        printf("%s: chdir / failed\n", s);
        exit(1);
    }
    if (unlink("dots/.") == 0) {
        printf("%s: unlink dots/. worked!\n", s);
        exit(1);
    }
    if (unlink("dots/..") == 0) {
        printf("%s: unlink dots/.. worked!\n", s);
        exit(1);
    }
    if (unlink("dots") != 0) {
        printf("%s: unlink dots failed!\n", s);
        exit(1);
    }
}

void dirfile(char *s)
{
    int fd;

    fd = open("dirfile", O_CREATE);
    if (fd < 0) {
        printf("%s: create dirfile failed\n", s);
        exit(1);
    }
    close(fd);
    if (chdir("dirfile") == 0) {
        printf("%s: chdir dirfile succeeded!\n", s);
        exit(1);
    }
    fd = open("dirfile/xx", 0);
    if (fd >= 0) {
        printf("%s: create dirfile/xx succeeded!\n", s);
        exit(1);
    }
    fd = open("dirfile/xx", O_CREATE);
    if (fd >= 0) {
        printf("%s: create dirfile/xx succeeded!\n", s);
        exit(1);
    }
    if (mkdir("dirfile/xx") == 0) {
        printf("%s: mkdir dirfile/xx succeeded!\n", s);
        exit(1);
    }
    if (unlink("dirfile/xx") == 0) {
        printf("%s: unlink dirfile/xx succeeded!\n", s);
        exit(1);
    }
    if (link("README", "dirfile/xx") == 0) {
        printf("%s: link to dirfile/xx succeeded!\n", s);
        exit(1);
    }
    if (unlink("dirfile") != 0) {
        printf("%s: unlink dirfile failed!\n", s);
        exit(1);
    }

    fd = open(".", O_RDWR);
    if (fd >= 0) {
        printf("%s: open . for writing succeeded!\n", s);
        exit(1);
    }
    fd = open(".", 0);
    if (write(fd, "x", 1) > 0) {
        printf("%s: write . succeeded!\n", s);
        exit(1);
    }
    close(fd);
}

// test that iput() is called at the end of _namei().
// also tests empty file names.
void iref(char *s)
{
    int i, fd;

    for (i = 0; i < NINODE + 1; i++) {
        if (mkdir("irefd") != 0) {
            printf("%s: mkdir irefd failed\n", s);
            exit(1);
        }
        if (chdir("irefd") != 0) {
            printf("%s: chdir irefd failed\n", s);
            exit(1);
        }

        mkdir("");
        link("README", "");
        fd = open("", O_CREATE);
        if (fd >= 0)
            close(fd);
        fd = open("xx", O_CREATE);
        if (fd >= 0)
            close(fd);
        unlink("xx");
    }

    // clean up
    for (i = 0; i < NINODE + 1; i++) {
        chdir("..");
        unlink("irefd");
    }

    chdir("/");
}

// test that fork fails gracefully
// the forktest binary also does this, but it runs out of proc entries first.
// inside the bigger usertests binary, we run out of memory first.
void forktest(char *s)
{
    enum { N = 1000 };
    int n, pid;

    for (n = 0; n < N; n++) {
        pid = fork();
        if (pid < 0)
            break;
        if (pid == 0)
            exit(0);
    }

    if (n == 0) {
        printf("%s: no fork at all!\n", s);
        exit(1);
    }

    if (n == N) {
        printf("%s: fork claimed to work 1000 times!\n", s);
        exit(1);
    }

    for (; n > 0; n--) {
        if (wait(0) < 0) {
            printf("%s: wait stopped early\n", s);
            exit(1);
        }
    }

    if (wait(0) != -1) {
        printf("%s: wait got too many\n", s);
        exit(1);
    }
}

void sbrkbasic(char *s)
{
    enum { TOOMUCH = 1024 * 1024 * 1024 };
    int i, pid, xstatus;
    char *c, *a, *b;

    // does sbrk() return the expected failure value?
    pid = fork();
    if (pid < 0) {
        printf("fork failed in sbrkbasic\n");
        exit(1);
    }
    if (pid == 0) {
        a = sbrk(TOOMUCH);
        if (a == (char *)0xffffffffffffffffL) {
            // it's OK if this fails.
            exit(0);
        }

        for (b = a; b < a + TOOMUCH; b += 4096) {
            *b = 99;
        }

        // we should not get here! either sbrk(TOOMUCH)
        // should have failed, or (with lazy allocation)
        // a pagefault should have killed this process.
        exit(1);
    }

    wait(&xstatus);
    if (xstatus == 1) {
        printf("%s: too much memory allocated!\n", s);
        exit(1);
    }

    // can one sbrk() less than a page?
    a = sbrk(0);
    for (i = 0; i < 5000; i++) {
        b = sbrk(1);
        if (b != a) {
            printf("%s: sbrk test failed %d %x %x\n", i, a, b);
            exit(1);
        }
        *b = 1;
        a = b + 1;
    }
    pid = fork();
    if (pid < 0) {
        printf("%s: sbrk test fork failed\n", s);
        exit(1);
    }
    c = sbrk(1);
    c = sbrk(1);
    if (c != a + 1) {
        printf("%s: sbrk test failed post-fork\n", s);
        exit(1);
    }
    if (pid == 0)
        exit(0);
    wait(&xstatus);
    exit(xstatus);
}

void sbrkmuch(char *s)
{
    enum { BIG = 100 * 1024 * 1024 };
    char *c, *oldbrk, *a, *lastaddr, *p;
    uint64 amt;

    oldbrk = sbrk(0);

    // can one grow address space to something big?
    a = sbrk(0);
    amt = BIG - (uint64)a;
    p = sbrk(amt);
    if (p != a) {
        printf("%s: sbrk test failed to grow big address space; enough phys "
               "mem?\n",
               s);
        exit(1);
    }

    // touch each page to make sure it exists.
    char *eee = sbrk(0);
    for (char *pp = a; pp < eee; pp += 4096)
        *pp = 1;

    lastaddr = (char *)(BIG - 1);
    *lastaddr = 99;

    // can one de-allocate?
    a = sbrk(0);
    c = sbrk(-PGSIZE);
    if (c == (char *)0xffffffffffffffffL) {
        printf("%s: sbrk could not deallocate\n", s);
        exit(1);
    }
    c = sbrk(0);
    if (c != a - PGSIZE) {
        printf("%s: sbrk deallocation produced wrong address, a %x c %x\n", a,
               c);
        exit(1);
    }

    // can one re-allocate that page?
    a = sbrk(0);
    c = sbrk(PGSIZE);
    if (c != a || sbrk(0) != a + PGSIZE) {
        printf("%s: sbrk re-allocation failed, a %x c %x\n", a, c);
        exit(1);
    }
    if (*lastaddr == 99) {
        // should be zero
        printf("%s: sbrk de-allocation didn't really deallocate\n", s);
        exit(1);
    }

    a = sbrk(0);
    c = sbrk(-((char *)sbrk(0) - oldbrk));
    if (c != a) {
        printf("%s: sbrk downsize failed, a %x c %x\n", a, c);
        exit(1);
    }
}

// can we read the kernel's memory?
void kernmem(char *s)
{
    char *a;
    int pid;

    for (a = (char *)(KERNBASE); a < (char *)(KERNBASE + 2000000); a += 50000) {
        pid = fork();
        if (pid < 0) {
            printf("%s: fork failed\n", s);
            exit(1);
        }
        if (pid == 0) {
            printf("%s: oops could read %x = %x\n", a, *a);
            exit(1);
        }
        int xstatus;
        wait(&xstatus);
        if (xstatus != -1) // did kernel kill child?
            exit(1);
    }
}

// if we run the system out of memory, does it clean up the last
// failed allocation?
void sbrkfail(char *s)
{
    enum { BIG = 100 * 1024 * 1024 };
    int i, xstatus;
    int fds[2];
    char scratch;
    char *c, *a;
    int pids[10];
    int pid;

    if (pipe(fds) != 0) {
        printf("%s: pipe() failed\n", s);
        exit(1);
    }
    for (i = 0; i < sizeof(pids) / sizeof(pids[0]); i++) {
        if ((pids[i] = fork()) == 0) {
            // allocate a lot of memory
            sbrk(BIG - (uint64)sbrk(0));
            write(fds[1], "x", 1);
            // sit around until killed
            for (;;)
                sleep(1000);
        }
        if (pids[i] != -1)
            read(fds[0], &scratch, 1);
    }

    // if those failed allocations freed up the pages they did allocate,
    // we'll be able to allocate here
    c = sbrk(PGSIZE);
    for (i = 0; i < sizeof(pids) / sizeof(pids[0]); i++) {
        if (pids[i] == -1)
            continue;
        kill(pids[i]);
        wait(0);
    }
    if (c == (char *)0xffffffffffffffffL) {
        printf("%s: failed sbrk leaked memory\n", s);
        exit(1);
    }

    // test running fork with the above allocated page
    pid = fork();
    if (pid < 0) {
        printf("%s: fork failed\n", s);
        exit(1);
    }
    if (pid == 0) {
        // allocate a lot of memory.
        // this should produce a page fault,
        // and thus not complete.
        a = sbrk(0);
        sbrk(10 * BIG);
        int n = 0;
        for (i = 0; i < 10 * BIG; i += PGSIZE) {
            n += *(a + i);
        }
        // print n so the compiler doesn't optimize away
        // the for loop.
        printf("%s: allocate a lot of memory succeeded %d\n", n);
        exit(1);
    }
    wait(&xstatus);
    if (xstatus != -1 && xstatus != 2)
        exit(1);
}

// test reads/writes from/to allocated memory
void sbrkarg(char *s)
{
    char *a;
    int fd, n;

    a = sbrk(PGSIZE);
    fd = open("sbrk", O_CREATE | O_WRONLY);
    unlink("sbrk");
    if (fd < 0) {
        printf("%s: open sbrk failed\n", s);
        exit(1);
    }
    if ((n = write(fd, a, PGSIZE)) < 0) {
        printf("%s: write sbrk failed\n", s);
        exit(1);
    }
    close(fd);

    // test writes to allocated memory
    a = sbrk(PGSIZE);
    if (pipe((int *)a) != 0) {
        printf("%s: pipe() failed\n", s);
        exit(1);
    }
}

void validatetest(char *s)
{
    int hi;
    uint64 p;

    hi = 1100 * 1024;
    for (p = 0; p <= (uint)hi; p += PGSIZE) {
        // try to crash the kernel by passing in a bad string pointer
        if (link("nosuchfile", (char *)p) != -1) {
            printf("%s: link should not succeed\n", s);
            exit(1);
        }
    }
}

// does unintialized data start out zero?
char uninit[10000];
void bsstest(char *s)
{
    int i;

    for (i = 0; i < sizeof(uninit); i++) {
        if (uninit[i] != '\0') {
            printf("%s: bss test failed\n", s);
            exit(1);
        }
    }
}

// does exec return an error if the arguments
// are larger than a page? or does it write
// below the stack and wreck the instructions/data?
void bigargtest(char *s)
{
    int pid, fd, xstatus;

    unlink("bigarg-ok");
    pid = fork();
    if (pid == 0) {
        static char *args[MAXARG];
        int i;
        for (i = 0; i < MAXARG - 1; i++)
            args[i] = "bigargs test: failed\n                                  "
                      "                                                        "
                      "                                                        "
                      "                                                     ";
        args[MAXARG - 1] = 0;
        exec("echo", args);
        fd = open("bigarg-ok", O_CREATE);
        close(fd);
        exit(0);
    } else if (pid < 0) {
        printf("%s: bigargtest: fork failed\n", s);
        exit(1);
    }

    wait(&xstatus);
    if (xstatus != 0)
        exit(xstatus);
    fd = open("bigarg-ok", 0);
    if (fd < 0) {
        printf("%s: bigarg test failed!\n", s);
        exit(1);
    }
    close(fd);
}

// what happens when the file system runs out of blocks?
// answer: balloc panics, so this test is not useful.
void fsfull()
{
    int nfiles;
    int fsblocks = 0;
    (void)fsblocks;

    printf("fsfull test\n");

    for (nfiles = 0;; nfiles++) {
        char name[64];
        name[0] = 'f';
        name[1] = '0' + nfiles / 1000;
        name[2] = '0' + (nfiles % 1000) / 100;
        name[3] = '0' + (nfiles % 100) / 10;
        name[4] = '0' + (nfiles % 10);
        name[5] = '\0';
        printf("%s: writing %s\n", name);
        int fd = open(name, O_CREATE | O_RDWR);
        if (fd < 0) {
            printf("%s: open %s failed\n", name);
            break;
        }
        int total = 0;
        while (1) {
            int cc = write(fd, buf, BSIZE);
            if (cc < BSIZE)
                break;
            total += cc;
            fsblocks++;
        }
        printf("%s: wrote %d bytes\n", total);
        close(fd);
        if (total == 0)
            break;
    }

    while (nfiles >= 0) {
        char name[64];
        name[0] = 'f';
        name[1] = '0' + nfiles / 1000;
        name[2] = '0' + (nfiles % 1000) / 100;
        name[3] = '0' + (nfiles % 100) / 10;
        name[4] = '0' + (nfiles % 10);
        name[5] = '\0';
        unlink(name);
        nfiles--;
    }

    printf("fsfull test finished\n");
}

void argptest(char *s)
{
    int fd;
    fd = open("init_code", O_RDONLY);
    if (fd < 0) {
        printf("%s: open failed\n", s);
        exit(1);
    }
    read(fd, sbrk(0) - 1, -1);
    close(fd);
}
//
// unsigned long randstate = 1;
// unsigned int rand()
// {
//     randstate = randstate * 1664525 + 1013904223;
//     return randstate;
// }

// check that there's an invalid page beneath
// the user stack, to catch stack overflow.
void stacktest(char *s)
{
    int pid;
    int xstatus;

    pid = fork();
    if (pid == 0) {
        char *sp = (char *)r_sp();
        sp -= PGSIZE;
        // the *sp should cause a trap.
        printf("%s: stacktest: read below stack %p\n", *sp);
        exit(1);
    } else if (pid < 0) {
        printf("%s: fork failed\n", s);
        exit(1);
    }
    wait(&xstatus);
    if (xstatus == -1) // kernel killed child?
        exit(0);
    else
        exit(xstatus);
}

// regression test. copyin(), copyout(), and copyinstr() used to cast
// the virtual page address to uint, which (with certain wild system
// call arguments) resulted in a kernel page faults.
void pgbug(char *s)
{
    char *argv[1];
    argv[0] = 0;
    exec((char *)0xeaeb0b5b00002f5e, argv);

    pipe((int *)0xeaeb0b5b00002f5e);

    exit(0);
}

// regression test. does the kernel panic if a process sbrk()s its
// size to be less than a page, or zero, or reduces the break by an
// amount too small to cause a page to be freed?
void sbrkbugs(char *s)
{
    int pid = fork();
    if (pid < 0) {
        printf("fork failed\n");
        exit(1);
    }
    if (pid == 0) {
        int sz = (uint64)sbrk(0);
        // free all user memory; there used to be a bug that
        // would not adjust p->sz correctly in this case,
        // causing exit() to panic.
        sbrk(-sz);
        // user page fault here.
        exit(0);
    }
    wait(0);

    pid = fork();
    if (pid < 0) {
        printf("fork failed\n");
        exit(1);
    }
    if (pid == 0) {
        int sz = (uint64)sbrk(0);
        // set the break to somewhere in the very first
        // page; there used to be a bug that would incorrectly
        // free the first page.
        sbrk(-(sz - 3500));
        exit(0);
    }
    wait(0);

    pid = fork();
    if (pid < 0) {
        printf("fork failed\n");
        exit(1);
    }
    if (pid == 0) {
        // set the break in the middle of a page.
        sbrk((10 * 4096 + 2048) - (uint64)sbrk(0));

        // reduce the break a bit, but not enough to
        // cause a page to be freed. this used to cause
        // a panic.
        sbrk(-10);

        exit(0);
    }
    wait(0);

    exit(0);
}

// regression test. does write() with an invalid buffer pointer cause
// a block to be allocated for a file that is then not freed when the
// file is deleted? if the kernel has this bug, it will panic: balloc:
// out of blocks. assumed_free may need to be raised to be more than
// the number of free blocks. this test takes a long time.
void badwrite(char *s)
{
    int assumed_free = 600;

    unlink("junk");
    for (int i = 0; i < assumed_free; i++) {
        int fd = open("junk", O_CREATE | O_WRONLY);
        if (fd < 0) {
            printf("open junk failed\n");
            exit(1);
        }
        write(fd, (char *)0xffffffffffL, 1);
        close(fd);
        unlink("junk");
    }

    int fd = open("junk", O_CREATE | O_WRONLY);
    if (fd < 0) {
        printf("open junk failed\n");
        exit(1);
    }
    if (write(fd, "x", 1) != 1) {
        printf("write failed\n");
        exit(1);
    }
    close(fd);
    unlink("junk");

    exit(0);
}

// regression test. test whether exec() leaks memory if one of the
// arguments is invalid. the test passes if the kernel doesn't panic.
void badarg(char *s)
{
    for (int i = 0; i < 50000; i++) {
        char *argv[2];
        argv[0] = (char *)0xffffffff;
        argv[1] = 0;
        exec("echo", argv);
    }

    exit(0);
}

// test the exec() code that cleans up if it runs out
// of memory. it's really a test that such a condition
// doesn't cause a panic.
void execout(char *s)
{
    for (int avail = 0; avail < 15; avail++) {
        int pid = fork();
        if (pid < 0) {
            printf("fork failed\n");
            exit(1);
        } else if (pid == 0) {
            // allocate all of memory.
            while (1) {
                uint64 a = (uint64)sbrk(4096);
                if (a == 0xffffffffffffffffLL)
                    break;
                *(char *)(a + 4096 - 1) = 1;
            }

            // free a few pages, in order to let exec() make some
            // progress.
            for (int i = 0; i < avail; i++)
                sbrk(-4096);

            // close(1);
            char *args[] = { "echo", "x", 0 };
            exec("echo", args);
            exit(0);
        } else {
            wait((int *)0);
        }
    }

    exit(0);
}

//
// use sbrk() to count how many free physical memory pages there are.
// touches the pages to force allocation.
// because out of memory with lazy allocation results in the process
// taking a fault and being killed, fork and report back.
//
int countfree()
{
    int fds[2];

    if (pipe(fds) < 0) {
        printf("pipe() failed in countfree()\n");
        exit(1);
    }

    int pid = fork();

    if (pid < 0) {
        printf("fork failed in countfree()\n");
        exit(1);
    }

    if (pid == 0) {
        close(fds[0]);

        while (1) {
            uint64 a = (uint64)sbrk(4096);
            if (a == 0xffffffffffffffff) {
                break;
            }

            // modify the memory to make sure it's really allocated.
            *(char *)(a + 4096 - 1) = 1;

            // report back one more page.
            if (write(fds[1], "x", 1) != 1) {
                printf("write() failed in countfree()\n");
                exit(1);
            }
        }

        exit(0);
    }

    close(fds[1]);

    int n = 0;
    while (1) {
        char c;
        int cc = read(fds[0], &c, 1);
        if (cc < 0) {
            printf("read() failed in countfree()\n");
            exit(1);
        }
        if (cc == 0)
            break;
        n += 1;
    }

    close(fds[0]);
    wait((int *)0);

    return n;
}

// run each test in its own process. run returns 1 if child's exit()
// indicates success.
int run(void f(char *), char *s)
{
    int pid;
    int xstatus;

    printf("test %s: ", s);
    if ((pid = fork()) < 0) {
        printf("runtest: fork error\n");
        exit(1);
    }
    if (pid == 0) {
        f(s);
        exit(0);
    } else {
        wait(&xstatus);
        if (xstatus != 0)
            printf("FAILED\n");
        else
            printf("OK\n");
        return xstatus == 0;
    }
}

struct test {
    void (*f)(char *);
    char *s;
} tests[] = {
    { exclusive_occupy, "exclusive_occupy" }, 
    { leave_pg_fast, "leave_pg_fast" }, 
    { dec_cpus, "dec_cpus" }, 
    { inc_cpus_flex, "inc_cpus_flex" }, 
    { inc_cpus_flex_con, "inc_cpus_flex_con" }, 
    { inc_cpus, "inc_cpus" }, 
    { leave_pg, "leave_pg" }, 
    { create_pg, "create_pg" }, 
    { enter_wrong_pg, "enter_wrong_pg" }, 
    { enter_pg, "enter_pg" }, 
    { execout, "execout" },
    { copyin, "copyin" },
    { copyout, "copyout" },
    { copyinstr1, "copyinstr1" },
    { copyinstr2, "copyinstr2" },
    { copyinstr3, "copyinstr3" },
    { truncate1, "truncate1" },
    { truncate2, "truncate2" },
    { truncate3, "truncate3" },
    { reparent2, "reparent2" },
    { pgbug, "pgbug" },
    { sbrkbugs, "sbrkbugs" },
    { badwrite, "badwrite" },
    { badarg, "badarg" },
    { reparent, "reparent" },
    { twochildren, "twochildren" },
    { forkfork, "forkfork" },
    { forkforkfork, "forkforkfork" }, 
    { argptest, "argptest" },
    { createdelete, "createdelete" },
    { linkunlink, "linkunlink" },
    { linktest, "linktest" },
    { unlinkread, "unlinkread" },
    { concreate, "concreate" },
    { subdir, "subdir" },
    { fourfiles, "fourfiles" },
    { sharedfd, "sharedfd" },
    { exectest, "exectest" },
    { bigargtest, "bigargtest" },
    { bigwrite, "bigwrite" },
    { bsstest, "bsstest" },
    { sbrkbasic, "sbrkbasic" },
    { sbrkmuch, "sbrkmuch" },
    { kernmem, "kernmem" },
    { sbrkfail, "sbrkfail" }, 
    { sbrkarg, "sbrkarg" },
    { validatetest, "validatetest" },
    { stacktest, "stacktest" },
    { opentest, "opentest" },
    { writetest, "writetest" },
    { writebig, "writebig" },
    { createtest, "createtest" },
    { openiputtest, "openiput" },
    { exitiputtest, "exitiput" },
    { iputtest, "iput" },
    { mem, "mem" },
    { pipe1, "pipe1" },
    { preempt, "preempt" },
    { exitwait, "exitwait" },
    { rmdot, "rmdot" },
    { fourteen, "fourteen" },
    { bigfile, "bigfile" },
    { dirfile, "dirfile" },
    { iref, "iref" },
    { forktest, "forktest" },
    { bigdir, "bigdir" }, // slow
    { 0, 0 },
}, temp_tests[]={
    { create_pg, "create_pg" }, 
    { 0, 0 },
};

struct test *get_tests(void) { return tests; }

int do_test(char *justone)
{
    printf("usertests starting\n");
    int free0 = countfree();
    int free1 = 0;
    int fail = 0;
    for (struct test *t = get_tests(); t->s != 0; t++) {
        if ((justone == 0) || strcmp(t->s, justone) == 0) {
            if (!run(t->f, t->s))
                fail = 1;
        }
    }

    if (fail) {
        printf("SOME TESTS FAILED\n");
        return 1;
    } else if ((free1 = countfree()) < free0) {
        printf("FAILED -- lost some free pages %d (out of %d)\n", free1, free0);
        return 1;
    } else {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
}

int main(int argc, char *argv[])
{
    int continuous = 0;
    int times = 0;
    char *justone = 0;

    if (argc == 2 && strcmp(argv[1], "-c") == 0) {
        continuous = 1;
    } else if (argc == 3 && strcmp(argv[1], "-C") == 0) {
        continuous = 2;
        char c = argv[2][0];
        if (c >= '1' && c <= '9') {
            times = c - '0';
        } else {
            printf("Usage: usertests -C [0-9]");
            exit(0);
        }
    } else if (argc == 2 && argv[1][0] != '-') {
        justone = argv[1];
    } else if (argc > 1) {
        printf("Usage: usertests [-c] [testname]\n    usertests -C [0-9]");
        exit(1);
    }

    int pgroup_id = get_proc_group_id();
    if (pgroup_id != 0) {
        printf("please run usertests in default process group\n");
        exit(1);
    }
    int cpus = proc_group_cpus();
    if (cpus < 2) {
        printf("usertests need at least 2 cpus to run correctly, all test "
               "about process group will fail\n");
    }

    int retval = 0;
    if (continuous == 1) {
        printf("continuous usertests starting:\n");
        uint64 n = 0;
        while (1) {
            int err = do_test(justone);
            if (err) {
                exit(1);
            }
            n++;
            printf("ALL TESTS PASSED: %d USERTESTS SO FAR\n", n);
        }
    } else if (continuous == 2) {
        for (int i = 0; i < times; i++) {
            printf("usertests starting %d/%d:\n", i + 1, times);
            int err = do_test(justone);
            if (err) {
                exit(1);
            }
        }
        printf("ALL TESTS PASSED: %d USERTESTS TOTAL\n", times);
    } else {
        retval = do_test(justone);
    }

    return retval;
}
