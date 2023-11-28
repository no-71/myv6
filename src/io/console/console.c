#include "io/console/console.h"
#include "driver/uart.h"
#include "lock/spin_lock.h"
#include "trap/introff.h"

struct spin_lock console_lock;

void console_init()
{
    init_spin_lock(&console_lock);
    uartinit();
}

void console_kputc(char c)
{
    acquire_spin_lock(&console_lock);
    uart_polling_write(c);
    release_spin_lock(&console_lock);
}

void console_kwrite(const char *s, int len)
{
    acquire_spin_lock(&console_lock);
    while (s < s + len) {
        uart_polling_write(*s++);
    }
    release_spin_lock(&console_lock);
}
