#include "sys_calls.h"

char s[] = "dear";

int main(void)
{
    trap_into_k();
    while (1) {
        s[0] = 'b';
    }

    return 1;
}
