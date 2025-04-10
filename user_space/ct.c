#include "config/basic_types.h"
#include "ulib/user_all.h"

#define MILLION 1000000
#define BILLION (1000 * MILLION)

void kill_child(int *pids, int n)
{
    for (int i = 0; i < n; i++) {
        kill(pids[i]);
    }
}

int is_prime(uint64 n)
{
    if (n <= 1)
        return 0;
    if (n == 2)
        return 1;
    if (n % 2 == 0)
        return 0;
    for (uint64 i = 3; i * i <= n; i += 2) {
        if (n % i == 0)
            return 0;
    }
    return 1;
}

uint64 prime_test(const char *s, uint64 iterations, uint64 test_limit)
{
    uint64 total_primes = 0;
    for (int iter = 0; iter < iterations; iter++) {
        uint64 count = 0;
        for (uint64 n = 2; n <= test_limit; n++) {
            count += is_prime(n);
        }
        total_primes += count;
    }

    printf("%s: Total primes counted (sum over %d iterations): %l\n", s,
           iterations, total_primes);
    return total_primes;
}

uint64 run(const char *s, uint64 (*test)(const char *, uint64, uint64),
           uint64 iterations, uint64 test_limit)
{
    uint64 start = clock();
    test(s, iterations, test_limit);
    uint64 ticks = clock() - start;
    uint64 duration = ticks / CLOCKS_PER_SEC;
    printf("%s: Time: %l seconds, %l ticks\n", s, duration, ticks);

    return ticks;
}

int test_once(int child_proc_n, uint64 (*test)(const char *, uint64, uint64),
              uint64 iterations, uint64 test_limit)
{
    int pp_start[2], pp_end[2], pp_time[2];
    uint64 n;
    if (pipe(pp_start) < 0 || pipe(pp_end) < 0 || pipe(pp_time) < 0) {
        printf("pipe fail\n");
        exit(1);
    }

    int pids[child_proc_n];
    for (int i = 0; i < child_proc_n; i++) {
        int pid = pids[i] = fork();
        if (pid < 0) {
            kill_child(pids, i);
            printf("fork fail\n");
            exit(1);
        } else if (pid == 0) {
            if (i) {
                close(1);
                while (1) {
                    run("process racing test", test, iterations, test_limit);
                }
            } else {
                n = run("process racing test", test, iterations, test_limit);
                write(pp_time[1], &n, sizeof(uint64));
                exit(0);
            }
        }
    }

    read(pp_time[0], &n, sizeof(uint64));
    uint64 time1 = n;

    uint64 pid = fork();
    if (pid < 0) {
        printf("fork fail\n");
        exit(1);
    } else if (pid == 0) {
        int err = create_proc_group();
        if (err == -1) {
            printf("create proc group fail\n");
            exit(1);
        }
        err = proc_occupy_cpu();
        if (err) {
            printf("proc occupy cpu fail\n");
            exit(1);
        }
        n = run("cpu exclusively test", test, iterations, test_limit);
        write(pp_time[1], &n, sizeof(uint64));
        exit(0);
    }

    read(pp_time[0], &n, sizeof(uint64));
    uint64 time2 = n;
    printf("process race: %l ticks, cpu exclusively: %l ticks\n", time1, time2);

    kill_child(pids, child_proc_n);

    return 0;
}

const char whitespace[] = " \t\r\n\v";

const char *peek_emp(const char *s)
{
    while (*s != '\0' && strchr(whitespace, *s)) {
        s++;
    }
    return s;
}

uint64 parse_uint64(const char *s)
{
    s = peek_emp(s);
    uint64 n = 0;
    while (*s != '\0') {
        if (*s > '9' || *s < '0') {
            return n;
        }
        n = n * 10 + *s - '0';
        s++;
    }
    return n;
}

uint64 parse_int_arg(int argc, char *argv[], int i, const char *pattern)
{
    if (strcmp(argv[i], pattern) == 0) {
        if (i == argc - 1) {
            return -1;
        } else {
            return parse_uint64(argv[i + 1]);
        }
    }
    return -1;
}

int main(int argc, char *argv[])
{
    uint64 race_proc_n = 20, iterations = 5, test_limit = 10 * MILLION;
    for (int i = 1; i < argc; i++) {
        int n = -1;
        if ((n = parse_int_arg(argc, argv, i, "-p")) != -1) {
            race_proc_n = n;
        } else if ((n = parse_int_arg(argc, argv, i, "-i")) != -1) {
            iterations = n;
        } else if ((n = parse_int_arg(argc, argv, i, "-l")) != -1) {
            test_limit = n * 100;
        }
        i++;

        if (n == -1) {
            printf("Usage: ct [-p racing process number] [-i iterations times] "
                   "[-l test_limit], "
                   "final test_limit will multiple 100\n");
            exit(1);
        }
    }

    printf("start test, racing process number: %l, "
           "iterations: %l, test_limit: %l * 100\n",
           race_proc_n, iterations, test_limit / 100);
    test_once(race_proc_n, prime_test, iterations, test_limit);

    return 0;
}
