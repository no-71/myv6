#include "ulib/sys_calls.h"
#include "ulib/user_lib.h"

// void get_input(void)
// {
//     int pid = fork();
//     if (pid == 0) {
//         printf("receiving chars...\n");
//         while (1) {
//             char s[64];
//             int i;
//             for (i = 0; i < sizeof(s) - 1; i++) {
//                 s[i] = getc();
//                 if (s[i] == '\n') {
//                     break;
//                 }
//             }
//             s[i] = '\0';
//             puts("receive: ");
//             puts(s);
//             putc('\n');
//         }
//     }
// }

void pull_up_one(char *argv[])
{
    pid_t pid = fork();
    if (pid == -1) {
        printf("init_code fork fail: %s\n", argv[0]);
    } else if (pid == 0) {
        exec(argv[0], argv);
        // exec fail
        printf("init_code execute after fork exec: %s\n", argv[0]);
        exit(1);
    }
}

void pull_up_proc(void)
{
    char *argv1[] = { "echo", "\nsay hello", " before start\n", 0 };
    pull_up_one(argv1);
    wait(NULL);

    char *argv0[] = { "sh", 0 };
    pull_up_one(argv0);
}

void main(void)
{
    if (open("console", O_RDWR) < 0) {
        mknod("console", CONSOLE, 0);
        // mknod("statistics", STATS, 0);
        open("console", O_RDWR);
    }
    dup(0); // stdout
    dup(0); // stderr

    printf("init_code start\n\n");
    pull_up_proc();
    while (1) {
        pid_t pid = wait(NULL);
        if (pid == 1) {
            printf("no proc alive\n");
        }
    }
}
