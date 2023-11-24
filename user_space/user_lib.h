#ifndef USER_LIB_H_
#define USER_LIB_H_

#include "include/config/basic_types.h"
#include "sys_calls.h"

char *strcpy(char *s, const char *t)
{
    char *os;

    os = s;
    while ((*s++ = *t++) != 0)
        ;
    return os;
}

int strcmp(const char *p, const char *q)
{
    while (*p && *p == *q)
        p++, q++;
    return (uchar)*p - (uchar)*q;
}

uint strlen(const char *s)
{
    int n;

    for (n = 0; s[n]; n++)
        ;
    return n;
}

void *memset(void *dst, int c, uint n)
{
    char *cdst = (char *)dst;
    int i;
    for (i = 0; i < n; i++) {
        cdst[i] = c;
    }
    return dst;
}

char *strchr(const char *s, char c)
{
    for (; *s; s++)
        if (*s == c)
            return (char *)s;
    return 0;
}

void *memmove(void *vdst, const void *vsrc, int n)
{
    char *dst;
    const char *src;

    dst = vdst;
    src = vsrc;
    if (src > dst) {
        while (n-- > 0)
            *dst++ = *src++;
    } else {
        dst += n;
        src += n;
        while (n-- > 0)
            *--dst = *--src;
    }
    return vdst;
}

int memcmp(const void *s1, const void *s2, uint n)
{
    const char *p1 = s1, *p2 = s2;
    while (n-- > 0) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

void *memcpy(void *dst, const void *src, uint n)
{
    return memmove(dst, src, n);
}

#include <stdarg.h>

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
