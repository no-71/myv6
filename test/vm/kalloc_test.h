#ifndef KALLOC_TEST_H_
#define KALLOC_TEST_H_

#include "test/test_config.h"
#include "test/test_util.h"
#include "test/vm/rw_test.h"

static inline void kalloc_test(void)
{
#ifdef TEST_ENABLE
    TEST_START_MESSAGE;
    rw_mem_test();
    TEST_SUCCESS_MESSAGE;
#endif
}

#endif
