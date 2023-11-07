#include "io/console/console.h"
#include "test/test_config.h"
#include "test/vm/kalloc_test.h"
#include "test/vm/kvm_test.h"
#include "util/kprint.h"
#include "vm/kalloc.h"
#include "vm/kvm.h"

void main(void)
{
    console_init();

    kprintf("hello, tinyos\n");
    kprintf("tinyos starts booting\n");
    print_test_enabled();
    kprintf("\n");

    kalloc_init();
    kalloc_test();

    kvm_init();
    kvm_init_hart();
    kvm_test();

    panic("panic test");

    while (1) {
    }
}
