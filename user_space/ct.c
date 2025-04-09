#include "config/basic_types.h"
#include "ulib/user_all.h"

#define MILLION 1000000
#define BILLION (1000 * MILLION)
#define TEST_LIMIT (5 * MILLION) // test range
#define ITERATIONS 5             // repeat times

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

int main(void)
{
    uint64 start = clock();
    uint64 total_primes = 0;

    for (int iter = 0; iter < ITERATIONS; iter++) {
        uint64 count = 0;
        for (uint64 n = 2; n <= TEST_LIMIT; n++) {
            count += is_prime(n);
        }
        total_primes += count;
    }

    uint64 duration = (clock() - start) / CLOCKS_PER_SEC;
    printf("Total primes counted (sum over %d iterations): %l\n", ITERATIONS,
           total_primes);
    printf("Time: %l seconds\n", duration);

    return 0;
}
