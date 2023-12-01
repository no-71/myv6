#include "io/console/console.h"
#include "driver/uart.h"
#include "lock/spin_lock.h"
#include "scheduler/sleep.h"
#include "trap/introff.h"

uint64 input_lines = 0;
struct spin_lock input_lines_lock;

void console_init()
{
    init_spin_lock(&input_lines_lock);
    uartinit();
}

void console_kputc(char c)
{
    push_introff();
    uart_polling_putc(c);
    pop_introff();
}

void console_kwrite(const char *s, size_t len)
{
    push_introff();
    for (int i = 0; i < len; i++) {
        uart_polling_putc(s[i]);
    }
    pop_introff();
}

void console_kwrite_str(const char *s)
{
    push_introff();
    while (*s != '\0') {
        uart_polling_putc(*s++);
    }
    pop_introff();
}

void console_putc(char c) { uart_putc(c); }

void console_write(const char *s, size_t len) { uart_write(s, len); }

char console_getc(void)
{
    acquire_spin_lock(&input_lines_lock);
    while (input_lines == 0) {
        sleep(&input_lines_lock, &input_lines);
    }

    int c = uart_just_getc();
    if (c == -1) {
        release_spin_lock(&input_lines_lock);
        c = uart_getc();
        acquire_spin_lock(&input_lines_lock);
    }
    if (c == '\n') {
        input_lines--;
    }

    release_spin_lock(&input_lines_lock);
    return c;
}

int console_intr(char c)
{
    switch (c) {
    case CTRL('H'):
        console_kputc('\b');
        console_kputc(' ');
        console_kputc('\b');
        return BACKSPACE;

    case '\r':
    case '\n':
        console_kputc('\n');
        acquire_spin_lock(&input_lines_lock);
        input_lines++;
        release_spin_lock(&input_lines_lock);
        wake_up(&input_lines);
        return GET_LINE;

    default:
        console_kputc(c);
        return GET_CH;
    }
}
