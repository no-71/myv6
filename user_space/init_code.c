#include "sys_calls.h"

void get_input(void)
{
    int pid = fork();
    if (pid == 0) {
        puts("receiving chars...\n");
        while (1) {
            char s[64];
            int i;
            for (i = 0; i < sizeof(s) - 1; i++) {
                s[i] = getc();
                if (s[i] == '\n') {
                    break;
                }
            }
            s[i] = '\0';
            puts("receive: ");
            puts(s);
            putc('\n');
        }
    }
}

void pull_up_proc(void)
{
    pid_t pid = fork();
    if (pid == -1) {
        puts("init_code fork fail\n");
    } else if (pid == 0) {
        char *argv[] = { "usertest", 0 };
        (void)argv;
        exec("usertest", argv);
        // exec fail
        puts("init_code exec after fork fail\n");
        exit(1);
    }
}

int main(void)
{
    puts("init_code start\n\n");

    // get_input();
    pull_up_proc();

    while (1) {
        pid_t pid = wait(NULL);
        if (pid == 1) {
            pull_up_proc();
        }
    }
}
