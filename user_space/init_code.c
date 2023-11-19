#include "sys_calls.h"

char s[64] = "dear";

void test_mem_rw(void)
{
    char stack[64];
    for (int i = 0; i < 64; i++) {
        ((volatile char *)s)[i] = 'b';
        ((volatile char *)stack)[i] = 'c';
        if (s[i] != 'b' || stack[i] != 'c') {
            (void)*((volatile char *)0);
        }
    }
}

int main(void)
{
    puts("user space say hello\n");
    puts("repeat hello...\n");

    char *err_str = (char *)0x100;
    int err = puts(err_str);
    if (err == 0) {
        puts("ERR: puts test fail\n");
    }

    printf("hello form printf\n");
    printf("s: %p\n", s);

    while (1) {
        ((volatile char *)s)[0] = 'b';
    }
    return 1;
}
