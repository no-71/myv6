#ifndef MEMORY_LAYOUT_H_
#define MEMORY_LAYOUT_H_

#include "driver/uart.h"
#include "riscv/clint.h"
#include "riscv/plic.h"
#include "riscv/vm_system.h"

/**
 * kernel memory layout:
 * 0x2000000            CLINT_BASE
 * 0xc000000            PLIC_BASE
 * 0x10000000           UART_BASE
 * 0x80000000           KERNEL_BASE
 *                      etext (aligned to 0x1000)
 *                      <kernel data>
 *                      kernel_end
 *                      <free memory>
 *                      MEMORY_END
 *
 *                      <hole>
 *
 * VA_END - PGSIZE      TRAMPOLINE_BASE
 */
#define KERNEL_BASE 0x80000000L
#define MEMORY_SIZE (128 * (1L << 20))
#define MEMORY_END (KERNEL_BASE + MEMORY_SIZE)
#define TRAMPOLINE_BASE (VA_END - PGSIZE)

/**
 * user memory layout
 * 0x1000                   PROC_VA_START
 *                          <elf things>
 * VA_END - PGSIZE*4        PROTECT_PAGE
 * VA_END - PGSIZE*3        USTACK_BASE
 * VA_END - PGSIZE*2        TRAP_FRAME_BASE
 * VA_END - PGSIZE          TRAMPOLINE_BASE
 */
#define PROC_VA_START 0x1000
#define PROC_VA_END (VA_END - PGSIZE * 4)
#define USTACK_BASE (VA_END - PGSIZE * 3)
#define TRAPFRAME_BASE (VA_END - PGSIZE * 2)
//      TRAMPOLINE_BASE

#define MAX_PA (MEMORY_END - 1)
#define PA_END MEMORY_END

#define GET_TRAMPOLINE_FN_VA(FN) (TRAMPOLINE_BASE + ((uint64)FN) % PGSIZE)

extern char etext[];

extern char kernel_end[];

#endif
