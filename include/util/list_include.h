#ifndef _LINUX_LIST_INCLUDE_H_
#define _LINUX_LIST_INCLUDE_H_

#include "config/basic_types.h"

// config
#define CONFIG_LIST_HARDENED
#define CONFIG_DEBUG_LIST

// we use more simple macro
#define __always_inline
#define IS_ENABLED(x) 0
#define likely(x) x

// some macro list.h need
/*
 * Use __READ_ONCE() instead of READ_ONCE() if you do not require any
 * atomicity. Note that this may result in tears!
 */
#define READ_ONCE(x) (*(const volatile typeof(x) *)&(x))
#define WRITE_ONCE(x, val)                                                     \
    do {                                                                       \
        *(volatile typeof(x) *)&(x) = (val);                                   \
    } while (0)

#define RISCV_FENCE_ASM(p, s) "\tfence " #p "," #s "\n"
#define RISCV_FENCE(p, s)                                                      \
    ({ __asm__ __volatile__(RISCV_FENCE_ASM(p, s) : : : "memory"); })

#define smp_store_release(p, v)                                                \
    do {                                                                       \
        RISCV_FENCE(rw, w);                                                    \
        WRITE_ONCE(*p, v);                                                     \
    } while (0)
#define smp_load_acquire(p)                                                    \
    ({                                                                         \
        typeof(*p) ___p1 = READ_ONCE(*p);                                      \
        RISCV_FENCE(r, rw);                                                    \
        ___p1;                                                                 \
    })

#define POISON_POINTER_DELTA 0

/*
 * These are non-NULL pointers that will result in page faults
 * under normal circumstances, used to verify that nobody uses
 * non-initialized list entries.
 */
#define LIST_POISON1 ((void *)0x100 + POISON_POINTER_DELTA)
#define LIST_POISON2 ((void *)0x122 + POISON_POINTER_DELTA)

/**
 * static_assert - check integer constant expression at build time
 *
 * static_assert() is a wrapper for the C11 _Static_assert, with a
 * little macro magic to make the message optional (defaulting to the
 * stringification of the tested expression).
 *
 * Contrary to BUILD_BUG_ON(), static_assert() can be used at global
 * scope, but requires the expression to be an integer constant
 * expression (i.e., it is not enough that __builtin_constant_p() is
 * true for expr).
 *
 * Also note that BUILD_BUG_ON() fails the build if the condition is
 * true, while static_assert() fails the build if the expression is
 * false.
 */
#define static_assert(expr, ...) __static_assert(expr, ##__VA_ARGS__, #expr)
#define __static_assert(expr, msg, ...) _Static_assert(expr, msg)

/* Are two types/vars the same type (ignoring qualifiers)? */
#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

#define offsetof(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 * WARNING: any const qualifier of @ptr is lost.
 */
#define container_of(ptr, type, member)                                        \
    ({                                                                         \
        void *__mptr = (void *)(ptr);                                          \
        static_assert(__same_type(*(ptr), ((type *)0)->member) ||              \
                          __same_type(*(ptr), void),                           \
                      "pointer type mismatch in container_of()");              \
        ((type *)(__mptr - offsetof(type, member)));                           \
    })

/**
 * container_of_const - cast a member of a structure out to the containing
 *			structure and preserve the const-ness of the pointer
 * @ptr:		the pointer to the member
 * @type:		the type of the container struct this is embedded in.
 * @member:		the name of the member within the struct.
 */
#define container_of_const(ptr, type, member)                                  \
    _Generic(ptr,							\
		const typeof(*(ptr)) *: ((const type *)container_of(ptr, type, member)),\
		default: ((type *)container_of(ptr, type, member))	\
	)

struct list_head {
    struct list_head *next, *prev;
};

struct hlist_head {
    struct hlist_node *first;
};

struct hlist_node {
    struct hlist_node *next, **pprev;
};

#endif
