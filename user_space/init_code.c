#include "sys_calls.h"

const char s[] = "dear";

int main(void)
{
    trap_into_k();
    while (1) {
    }

    return 1;
}
