#include "include/fn.h"

#define MCRO 1

__attribute__((aligned(16))) char kstack[4096];
char s[] = "my dear consort, eternal";

void main(void)
{
    int a = MCRO;
    int c = f(a);
    s[0] = 'n' + c;
    while (1) {
    }
}
