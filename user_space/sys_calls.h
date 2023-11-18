#ifndef SYS_CALL_H_
#define SYS_CALL_H_

int putc(char c);
int puts(const char *s);

#include <stdarg.h>

typedef unsigned long long uint64;

static char digits[] = "0123456789abcdef";

static void printint(int xx, int base, int sign)
{
    char buf[16];
    int i;
    unsigned x;

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
        putc(buf[i]);
    }
}

static void printptr(uint64 x)
{
    int i;
    putc('0');
    putc('x');
    for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4) {
        putc(digits[x >> (sizeof(uint64) * 8 - 4)]);
    }
}

// print to the console. only understands %d, %x, %p, %s.
int printf(const char *fmt, ...)
{
    va_list ap;
    int i, c;
    char *s;

    if (fmt == 0) {
        return 0;
    }

    va_start(ap, fmt);
    for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
        if (c != '%') {
            putc(c);
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
                putc(*s);
            break;
        case '%':
            putc('%');
            break;
        default:
            // print unknown % sequence to draw attention.
            putc('%');
            putc(c);
            break;
        }
    }

    return i;
}

#endif
