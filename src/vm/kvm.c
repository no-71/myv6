#include "vm/kvm.h"
#include "config/basic_types.h"
#include "driver/uart.h"
#include "riscv/clint.h"
#include "riscv/plic.h"
#include "riscv/regs.h"
#include "riscv/vm_system.h"
#include "util/kprint.h"
#include "vm/memory_layout.h"
#include "vm/vm.h"

page_table kernel_page_table;

void kvm_init(void)
{
    int err = init_page_table(&kernel_page_table);
    if (err) {
        PANIC_FN("fail to init kernel_page_table");
    }
    if ((uint64)etext % PGSIZE) {
        PANIC_FN("etext is not aligned to PGSIZE");
    }

    map_n_pages(kernel_page_table, CLINT_BASE,
                ROUND_UP_PGSIZE(CLINT_SIZE) / PGSIZE, CLINT_BASE,
                PTE_R | PTE_W);

    map_n_pages(kernel_page_table, PLIC_BASE,
                ROUND_UP_PGSIZE(PLIC_SIZE) / PGSIZE, PLIC_BASE, PTE_R | PTE_W);

    map_n_pages(kernel_page_table, UART_BASE,
                ROUND_UP_PGSIZE(UART_SIZE) / PGSIZE, UART_BASE, PTE_R | PTE_W);

    map_n_pages(kernel_page_table, KERNEL_BASE,
                ((uint64)etext - KERNEL_BASE) / PGSIZE, KERNEL_BASE,
                PTE_R | PTE_X);

    map_n_pages(kernel_page_table, (uint64)etext,
                ((uint64)MEMORY_END - (uint64)etext) / PGSIZE, (uint64)etext,
                PTE_R | PTE_W);
}

void kvm_init_hart(void)
{
    w_satp(MAKE_SATP((uint64)kernel_page_table));
    sfence_vma_all();
}
