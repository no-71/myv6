#include "riscv/plic.h"
#include "config/basic_types.h"
#include "cpus.h"
#include "driver/uart.h"
#include "riscv/regs.h"

void plic_init(void) { GET_REG(uint32, PLIC_I_SOURCE_ENABLE(UART_IRQ)) = 1; }

void plic_init_hart(void)
{
    GET_REG(uint32, PLIC_S_I_ENABLE(cpu_id())) = 1 << UART_IRQ;
    GET_REG(uint32, PLIC_S_PRIORITY(cpu_id())) = 0;
}
