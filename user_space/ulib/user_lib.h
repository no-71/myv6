#ifndef USER_LIB_H_
#define USER_LIB_H_

#include "include/config/basic_config.h"
#include "include/config/basic_types.h"
#include "sys_calls.h"

#include <stdarg.h>

#define RAND_MAX 32767
#define CLOCKS_PER_SEC (10000000 / TIME_TRAP_INTERVAL)

int stat(const char *, struct stat *);
char *strcpy(char *, const char *);
void *memmove(void *, const void *, int);
char *strchr(const char *, char c);
int strcmp(const char *, const char *);
void fprintf(int, const char *, ...);
void printf(const char *, ...);
char *gets(char *, int max);
uint strlen(const char *);
void *memset(void *, int, uint);
void *malloc(uint);
void free(void *);
int atoi(const char *);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);
int statistics(void *, int);
void srand(unsigned int seed);
uint rand(void);

static inline uint64 clock(void) { return time(NULL); }

#endif
