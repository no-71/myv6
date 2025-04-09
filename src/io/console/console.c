#include "io/console/console.h"
#include "cpus.h"
#include "driver/uart.h"
#include "fs/file.h"
#include "lock/spin_lock.h"
#include "scheduler/sleep.h"
#include "trap/introff.h"
#include "util/kprint.h"
#include "vm/vm.h"

uint64 input_lines = 0;
struct spin_lock input_lines_lock;

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
    if (c == '\n' || c == CTRL('D')) {
        input_lines--;
    }

    release_spin_lock(&input_lines_lock);
    return c;
}

static int console_write_dev(int userdst, uint64 ustr, int len)
{
    for (int i = 0; i < len; i++) {
        char c;
        int err = copyin_uork(my_proc()->proc_pgtable, &c, ustr, 1, userdst);
        if (err) {
            return i;
        }
        console_putc(c);
        ustr++;
    }

    return len;
}

static int console_read_dev(int userdst, uint64 ustr, int len)
{
    for (int i = 0; i < len; i++) {
        char c = console_getc();
        if (c == CTRL('D')) {
            return -1;
        }
        int err = copyout_uork(my_proc()->proc_pgtable, ustr, &c, 1, userdst);
        if (err) {
            return i;
        }
        ustr++;
    }

    return len;
}

int console_intr(char c)
{
    switch (c) {
    case CTRL('H'):
        if (uart_input_not_empty()) {
            console_kputc('\b');
            console_kputc(' ');
            console_kputc('\b');
        }
        return BACKSPACE;

    case '\r':
    case '\n':
    case CTRL('D'):
        console_kputc('\n');
        acquire_spin_lock(&input_lines_lock);
        input_lines++;
        release_spin_lock(&input_lines_lock);
        wake_up(&input_lines);
        return c == CTRL('D') ? GET_EOF : GET_LINE;

    default:
        console_kputc(c);
        return GET_CH;
    }
}

void console_init()
{
    init_spin_lock(&input_lines_lock);
    uartinit();

    devsw[CONSOLE].write = console_write_dev;
    devsw[CONSOLE].read = console_read_dev;
}
