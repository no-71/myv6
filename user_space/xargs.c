#include "ulib/user_all.h"
// #include <fcntl.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/stat.h>
// #include <sys/types.h>
// #include <sys/wait.h>
// #include <unistd.h>
// #define MAXARG 32

int main(int argc, char *argv[])
{
    char buf[512];
    char *base_av[MAXARG];
    for (int i = 1; argv[i] != 0; ++i) {
        base_av[i - 1] = argv[i];
    }
    base_av[argc - 1] = 0;
    int base_ac = argc - 1;

    while (gets(buf, 512)) {
        /* different behave in xv6(compare to c lib), make it equal, simple
         * implement */
        for (char *cur = buf; *cur != '\0'; ++cur) {
            if (*cur == '\n') {
                *cur = '\0';
            }
        }
        if (*buf == '\0') {
            break;
        }

        char *cur = buf;
        while (*cur == ' ') {
            ++cur;
        }
        char *arg = cur;
        int new_ac = base_ac;
        // printf("line: %s\n", buf);
        while (*cur != '\0') {
            if (*cur != ' ') {
                cur++;
                continue;
            }

            if (new_ac == MAXARG) {
                printf("too many args\n");
                exit(0);
            }
            char *p = cur;
            *p = '\0';
            base_av[new_ac] = arg;
            new_ac++;
            // printf("new arg: %s\n", arg);

            cur++;
            while (*cur == ' ') {
                cur++;
            }
            arg = cur;
        }
        // for (int i = 0; i < new_ac; i++) {
        //     printf("%s ", base_av[i]);
        // }
        // printf("\n");
        // printf("new_ac: %d\n", new_ac);
        if (arg != cur) {
            base_av[new_ac] = arg;
            new_ac++;
            // printf("add: %d\n", new_ac);
        }

        base_av[new_ac] = 0;
        // for (int i = 0; i < new_ac; i++) {
        //     printf("%s ", base_av[i]);
        // }
        // printf("to fork\n");
        if (fork() == 0) {
            exec(base_av[0], base_av);
            exit(0);
        }
        wait((int *)0);
    }

    exit(0);
}
