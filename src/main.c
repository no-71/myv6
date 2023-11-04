#include "driver/uart.h"
#include "riscv/regs.h"
#include "trap/introff.h"
#include "util/kprint.h"

void main(void)
{
    uartinit();

    kprintf("hello, tinyos\n");
    kprintf("tinyos starts booting\n");
    panic("panic test");

    while (1) {
    }
}
