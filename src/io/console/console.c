#include "io/console/console.h"
#include "driver/uart.h"
#include "trap/introff.h"

void console_kputc(char c)
{
    push_introff();
    uart_polling_write(c);
    pop_introff();
}

void console_kwrite(const char *s, int len)
{
    push_introff();
    while (s < s + len) {
        uart_polling_write(*s++);
    }
    pop_introff();
}
