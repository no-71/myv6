#ifndef STRING_H_
#define STRING_H_

#include "config/basic_types.h"

static inline char *strcpy(char *s, const char *t)
{
    char *os;

    os = s;
    while ((*s++ = *t++) != 0) {
    }
    return os;
}

static inline int strcmp(const char *p, const char *q)
{
    while (*p && *p == *q) {
        p++, q++;
    }
    return (uchar)*p - (uchar)*q;
}

static inline uint strlen(const char *s)
{
    int n;

    for (n = 0; s[n]; n++) {
    }
    return n;
}

#endif
