#include "sys_calls.h"

char s[] = "dear";

int main(void)
{
    trap_into_k();
    for (int i = 0; i < 100; i++) {
        ((volatile char *)s)[0] = 'b';
        if (s[0] != 'b') {
            (void)*((volatile char *)0);
        }
    }
    trap_into_k();
    while (1) {
        ((volatile char *)s)[0] = 'b';
    }

    return 1;
}
