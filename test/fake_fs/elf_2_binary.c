#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define LEN 4096

unsigned char bytes[LEN];

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("wrong number of param, need 1 param\n");
        exit(1);
    }

    int size = 0;
    printf("#include \"config/basic_types.h\"\n");
    printf("char %s[] = { ", argv[1]);
    while (1) {
        int n = read(0, bytes, LEN);
        for (int i = 0; i < n; i++) {
            if ((i % 16) == 0) {
                printf("\n");
            }
            printf("0x%x, ", bytes[i]);
        }

        size += n;
        if (n < LEN) {
            break;
        }
    }
    printf("0x0 };\n");
    printf("uint64 %s_size = %d;", argv[1], size);

    return 0;
}
