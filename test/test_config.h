#ifndef TEST_CONFIG_H_
#define TEST_CONFIG_H_

#include "util/kprint.h"

#define TEST_ENABLE

static inline void print_test_enabled(void)
{
#ifdef TEST_ENABLE
    kprintf(">>> test enabled\n");
#endif
}

#endif
