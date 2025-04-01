#ifndef STRING_H_
#define STRING_H_

#include "config/basic_types.h"

int strncmp(const char *p, const char *q, uint n);
char *strncpy(char *s, const char *t, int n);
char *safestrcpy(char *s, const char *t, int n);
int strlen(const char *s);

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

#endif
