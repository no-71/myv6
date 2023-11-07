#ifndef MEMORY_LAYOUT_H_
#define MEMORY_LAYOUT_H_

#include "driver/uart.h"
#include "riscv/clint.h"
#include "riscv/plic.h"

/**
 * physical memory layout:
 * 0x2000000  CLINT_BASE
 * 0xc000000  PLIC_BASE
 * 0x10000000 UART_BASE
 * 0x80000000 KERNEL_BASE
 *            etext (aligned to 0x1000)
 *            <kernel data>
 *            kernel_end
 *            <free memory>
 *            MEMORY_END
 */

#define KERNEL_BASE 0x80000000L
#define MEMORY_SIZE (128 * (1L << 20))
#define MEMORY_END (KERNEL_BASE + MEMORY_SIZE)
#define MAX_PA MEMORY_END

extern char etext[];

extern char kernel_end[];

#endif
