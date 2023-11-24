#include "user_all.h"

int main(void)
{
    puts("init_code start\n\n");

    pid_t pid = fork();
    if (pid == -1) {
        puts("init_code fork fail\n");
    } else if (pid == 0) {
        char *argv[] = { "usertest", 0 };
        (void)argv;
        exec("usertest", argv);
        // exec fail
        puts("init_code exec after fork fail\n");
        return 1;
    }

    while (1) {
        wait(NULL);
    }
}
