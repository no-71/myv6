#include "util/kprint.h"
#include "config/basic_types.h"
#include "cpus.h"
#include "io/console/console.h"

#include <stdarg.h>

static char digits[] = "0123456789abcdef";

static void printint(int xx, int base, int sign)
{
    char buf[16];
    int i;
    uint x;

    if (sign && (sign = xx < 0)) {
        x = -xx;
    } else {
        x = xx;
    }

    i = 0;
    do {
        buf[i++] = digits[x % base];
    } while ((x /= base) != 0);

    if (sign) {
        buf[i++] = '-';
    }

    while (--i >= 0) {
        console_kputc(buf[i]);
    }
}

static void printptr(uint64 x)
{
    int i;
    console_kputc('0');
    console_kputc('x');
    for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4) {
        console_kputc(digits[x >> (sizeof(uint64) * 8 - 4)]);
    }
}

// print to the console. only understands %d, %x, %p, %s.
void kprintf(const char *fmt, ...)
{
    va_list ap;
    int i, c;
    char *s;

    if (fmt == 0) {
        panic("try to kprint(null)");
    }

    va_start(ap, fmt);
    for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
        if (c != '%') {
            console_kputc(c);
            continue;
        }

        c = fmt[++i] & 0xff;
        if (c == 0) {
            break;
        }
        switch (c) {
        case 'd':
            printint(va_arg(ap, int), 10, 1);
            break;
        case 'x':
            printint(va_arg(ap, int), 16, 1);
            break;
        case 'p':
            printptr(va_arg(ap, uint64));
            break;
        case 's':
            if ((s = va_arg(ap, char *)) == 0)
                s = "(null)";
            for (; *s; s++)
                console_kputc(*s);
            break;
        case '%':
            console_kputc('%');
            break;
        default:
            // print unknown % sequence to draw attention.
            console_kputc('%');
            console_kputc(c);
            break;
        }
    }
}

__attribute__((noreturn)) void panic_2str(const char *s1, const char *s2)
{
    panicked = 1;
    kprintf("panic: %s%s\n", s1, s2);
    while (1) {
    }
}

__attribute__((noreturn)) void panic(const char *s) { panic_2str(s, ""); }
