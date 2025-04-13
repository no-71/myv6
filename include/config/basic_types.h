#ifndef BASIC_TYPES_H_
#define BASIC_TYPES_H_

#define NULL ((void *)0)

#define EOF (-1)

enum { false, true };

typedef _Bool bool;

typedef char int8;
typedef short int16;
typedef int int32;
typedef long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long uint64;

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

typedef uint64 size_t;
typedef int64 intptr_t;
typedef uint64 uintptr_t;
typedef int32 pid_t;

#endif
