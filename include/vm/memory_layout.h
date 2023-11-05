#ifndef MEMORY_LAYOUT_H_
#define MEMORY_LAYOUT_H_

#include "driver/uart.h"
#include "riscv/clint.h"
#include "riscv/plic.h"

/**
 * physical memory layout:
 * 0x2000000  CLINT
 * 0xc000000  PLIC
 * 0x10000000 UART
 * 0x80000000 KERNEL
 *            etext
 *            kernel_end
 *            <free memory>
 *            MEMORY_END
 */

#define KERNEL_BASE 0x80000000L

#define MEMORY_END (KERNEL_BASE + 128 * (1L << 20))

extern char etext[];

extern char kernel_end[];

#endif
