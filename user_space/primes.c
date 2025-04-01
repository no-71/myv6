#include "ulib/user_all.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <sys/wait.h>
// #include <unistd.h>
#define MAX 35

int main(void)
{
    int used_pip1 = 1;
    int pip1[2], pip2[2];
    int *pip_left, *pip_right;
    pipe(pip1);
    pip_right = pip1;

    int no_child = 1;
    char base = 2;
    char cur = 3;
    printf("prime %d\n", base);

    if (fork() != 0) {
        // main
        close(pip_right[0]);
        while (++cur <= MAX) {
            if (cur % base != 0) {
                write(pip_right[1], &cur, 1);
            }
        }
        close(pip_right[1]);
        wait((int *)0);
        exit(0);
    }

    // fork
    base = cur;
    pip_left = pip_right;
    close(pip_left[1]);
    printf("prime %d\n", base);
    while (read(pip_left[0], &cur, 1)) {
        if (cur % base == 0) {
            continue;
        }
        // prime
        if (no_child) {
            pip_right = used_pip1 ? pip2 : pip1;
            pipe(pip_right);
            if (fork() != 0) {
                no_child = 0;
                close(pip_right[0]);
            } else {
                close(pip_left[0]);
                close(pip_right[1]);
                base = cur;
                pip_left = pip_right;
                used_pip1 = !used_pip1;
                printf("prime %d\n", base);
            }
            continue;
        }
        // to right
        write(pip_right[1], &cur, 1);
    }
    close(pip_left[0]);
    close(pip_right[1]);
    wait((int *)0);

    exit(0);
}
