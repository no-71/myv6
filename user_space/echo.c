#include "ulib/user_all.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
    }
    printf("\n");

    return 0;
}
