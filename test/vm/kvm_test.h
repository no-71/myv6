#ifndef KVM_TEST_H_
#define KVM_TEST_H_

#include "test/test_config.h"
#include "test/test_util.h"
#include "test/vm/rw_test.h"
#include "vm/vm.h"

extern page_table kernel_page_table;

static inline void kvm_test(void)
{
#ifdef TEST_ENABLE
    TEST_START_MESSAGE;
    // vmprint_accurate(kernel_page_table, 1, 512);
    rw_mem_test();
    TEST_SUCCESS_MESSAGE;
#endif
}

#endif
