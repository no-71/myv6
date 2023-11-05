#include "io/console/console.h"
#include "test/test_config.h"
#include "test/vm/rw_test.h"
#include "util/kprint.h"
#include "vm/kalloc.h"

void main(void)
{
    console_init();

    kprintf("hello, tinyos\n");
    kprintf("tinyos starts booting\n");
    print_test_enabled();
    kprintf("\n");

    kalloc_init();
    rw_mem_test();

    panic("panic test");

    while (1) {
    }
}
