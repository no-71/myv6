#include "riscv/regs.h"
#include "riscv/vm_system.h"
#include "sys_calls.h"
#include "user_all.h"

#define KERNEL_BASE 0x80000000L

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
        int xstatus = 0;
        wait(&xstatus);
        if (xstatus == -1) {
            // probably page fault, so might be lazy lab,
            // so OK.
            exit(0);
        }
        exit(xstatus);
    }
}

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

// test that fork fails gracefully
// the forktest binary also does this, but it runs out of proc entries first.
// inside the bigger usertests binary, we run out of memory first.
void forktest(char *s)
{
    enum { N = 1000 };
    int n, pid;

    for (n = 0; n < N; n++) {
        pid = fork();
        if (pid < 0) {
            break;
        }
        if (pid == 0) {
            exit(0);
        }
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
        // printf("pa 1\n");
        if (b != a) {
            // printf("pa 2\n");
            printf("%s: sbrk test failed %d %x %x\n", s, i, a, b);
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
    for (char *pp = a; pp < eee; pp += 4096) {
        *pp = 1;
    }

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

    // origin 2000000, dec it to make it less noisy
    for (a = (char *)(KERNEL_BASE); a < (char *)(KERNEL_BASE + 200000);
         a += 50000) {
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
        if (xstatus != -1) { // did kernel kill child?
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
                if (a == 0xffffffffffffffffLL) {
                    break;
                }
                *(char *)(a + 4096 - 1) = 1;
            }

            // free a few pages, in order to let exec() make some
            // progress.
            for (int i = 0; i < avail; i++) {
                sbrk(-4096);
            }

            // close(1);
            char *args[] = { "echo", "x", 0 };
            exec("echo", args);
            exit(0);
        } else {
            wait((int *)0);
        }
    }

    putc('\n');
    exit(0);
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
        if (xstatus != 0) {
            printf("FAILED\n");
        } else {
            printf("OK\n");
        }
        return xstatus == 0;
    }
}

int countfree(void) { return 0; }

int main(int argc, char *argv[])
{
    int continuous = 0;
    char *justone = 0;

    if (argc == 2 && strcmp(argv[1], "-c") == 0) {
        continuous = 1;
    } else if (argc == 2 && strcmp(argv[1], "-C") == 0) {
        continuous = 2;
    } else if (argc == 2 && argv[1][0] != '-') {
        justone = argv[1];
    } else if (argc > 1) {
        printf("Usage: usertests [-c] [testname]\n");
        exit(1);
    }

    struct test {
        void (*f)(char *);
        char *s;
    } tests[] = {
        { exitwait, "exitwait" },
        { reparent, "reparent" },
        { twochildren, "twochildren" },
        { forkfork, "forkfork" },
        { reparent2, "reparent2" },
        { mem, "mem" },
        { forktest, "forktest" },
        { sbrkbasic, "sbrkbasic" },
        { sbrkmuch, "sbrkmuch" },
        { kernmem, "kernmem" },
        { bsstest, "bsstest" },
        { stacktest, "stacktest" },
        { sbrkbugs, "sbrkbugs" },
        { badarg, "badarg" },
        { execout, "execout" },
        { 0, 0 },
        { 0, 0 },
    };

    if (continuous) {
        printf("continuous usertests starting\n");
        while (1) {
            int fail = 0;
            int free0 = countfree();
            for (struct test *t = tests; t->s != 0; t++) {
                if (!run(t->f, t->s)) {
                    fail = 1;
                    break;
                }
            }
            if (fail) {
                printf("SOME TESTS FAILED\n");
                if (continuous != 2) {
                    exit(1);
                }
            }
            int free1 = countfree();
            if (free1 < free0) {
                printf("FAILED -- lost %d free pages\n", free0 - free1);
                if (continuous != 2) {
                    exit(1);
                }
            }
        }
    }

    printf("usertests starting\n");
    int free0 = countfree();
    int free1 = 0;
    int fail = 0;
    int proc_num_begin = count_proc_num();
    int proc_num_end = 0;
    for (struct test *t = tests; t->s != 0; t++) {
        if ((justone == 0) || strcmp(t->s, justone) == 0) {
            if (!run(t->f, t->s)) {
                fail = 1;
            }
        }
    }

    if (fail) {
        printf("SOME TESTS FAILED\n");
        exit(1);
    } else if ((free1 = countfree()) < free0) {
        printf("FAILED -- lost some free pages %d (out of %d)\n", free1, free0);
        exit(1);
    } else if ((proc_num_end = count_proc_num()) != proc_num_begin) {
        printf("FAILED -- proc not cleaned completely (begin: %d, end: %d)\n",
               proc_num_begin, proc_num_end);
        exit(1);
    } else {
        printf("proc_num: OK, begin proc num: %d, end proc num %d\n",
               proc_num_begin, proc_num_end);
        printf("ALL TESTS PASSED\n");
        exit(0);
    }
}
